/* roadmap_dbread.c - a module to read a roadmap database.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   #include "roadmap_dbread.h"
 *
 *   roadmap_db_model *roadmap_db_register
 *          (char *section, roadmap_db_handler *handler);
 *
 *   int  roadmap_db_open (const char *path,
 *                         const char *name, roadmap_db_model *model);
 *
 *   void roadmap_db_activate (const char *path, const char *name);
 *
 *   roadmap_db *roadmap_db_get_subsection (roadmap_db *parent, char *path);
 *
 *   roadmap_db *roadmap_db_get_first (roadmap_db *parent);
 *   char       *roadmap_db_get_name  (roadmap_db *section);
 *   unsigned    roadmap_db_get_size  (roadmap_db *section);
 *   int         roadmap_db_get_count (roadmap_db *section);
 *   char       *roadmap_db_get_data  (roadmap_db *section);
 *   roadmap_db *roadmap_db_get_next  (roadmap_db *section);
 *
 *   void roadmap_db_close (const char *path, const char *name);
 *   void roadmap_db_end   (void);
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "roadmap_file.h"

#include "roadmap.h"
#include "roadmap_dbread.h"


typedef struct roadmap_db_database_s {

   char *name;
   char *path;

   RoadMapFileContext file;
   char *base;
   int   size;
   roadmap_db root;

   struct roadmap_db_database_s *next;
   struct roadmap_db_database_s *previous;

   roadmap_db_model *model;

} roadmap_db_database;

static roadmap_db_database *RoadmapDatabaseFirst  = NULL;



static struct roadmap_db_section *roadmap_db_locate
                 (roadmap_db_database *database, int offset) {

   return (struct roadmap_db_section *) (database->base + offset);
}


static roadmap_db_database *roadmap_db_find (const char *path,
                                             const char *name) {

   roadmap_db_database *database;

   for (database = RoadmapDatabaseFirst;
        database != NULL;
        database = database->next) {

      if ((strcmp (name, database->name) == 0) &&
          (strcmp (path, database->path) == 0)) {

         return database;
      }
   }

   return NULL;
}


static void roadmap_db_make_tree
               (roadmap_db_database *database, roadmap_db *parent) {

   int child_offset;
   roadmap_db *child = NULL;

   if (parent->head->first <= 0) {
      if (parent->head->first < 0) {
         roadmap_log (ROADMAP_FATAL,
                      "section %s: head.first invalid", parent->head->name);
      }
      parent->first = NULL;
      parent->last = NULL;
      return;
   }

   parent->first = NULL;

   for (child_offset = parent->head->first;
        child_offset != 0;
        child_offset = child->head->next) {

      if ((child_offset < 0) || (child_offset >= database->size)) {
         roadmap_log (ROADMAP_FATAL,
                      "illegal offset %d in database %s",
                      child_offset,
                      database->name);
      }

      child_offset = (child_offset + 3) & (~3);

      child = (roadmap_db *) malloc (sizeof(roadmap_db));

      child->head = roadmap_db_locate (database, child_offset);
      child->parent = parent;
      child->next = NULL;
      child->level = parent->level + 1;

      if (parent->first == NULL) {
         parent->first = child;
      } else {
         parent->last->next = child;
      }
      parent->last = child;

      roadmap_db_make_tree (database, child);

      if (child->head->next > 0) {
         int aligned = (child->head->next + 3) & (~3);
         child->next = (roadmap_db *) (database->base + aligned);
      } else {
         child->next = NULL;
      }
   }
}


static void roadmap_db_free_subtree (roadmap_db *parent) {

   roadmap_db *child;
   roadmap_db *next;

   for (child = parent->first; child != NULL; child = next) {

      roadmap_db_free_subtree (child);
      next = child->next;
      free (child);
   }
}


static int roadmap_db_call_map_one
              (roadmap_db_model *model, roadmap_db *this, char *section) {

   roadmap_db_model *registered;

   this->handler_context = NULL;

   for (registered = model;
        registered != NULL;
        registered = registered->next) {

      if (strcasecmp (registered->section, section) == 0) {

         this->handler_context = (* (registered->handler->map)) (this);

         if (this->handler_context == NULL) {
            return 0;
         }
         break;
      }
   }

   return 1;
}


static int roadmap_db_call_map (roadmap_db_database *database) {

   roadmap_db *child;

   for (child = database->root.first; child != NULL; child = child->next) {

      if (! roadmap_db_call_map_one
                (database->model, child, child->head->name)) {

          for (child = child->next; child != NULL; child = child->next) {

              child->handler_context = NULL;
          }
          database->root.handler_context = NULL;
          return 0;
      }
   }

   return roadmap_db_call_map_one (database->model, &database->root, "/");
}


static void roadmap_db_call_activate (roadmap_db_database *database) {

   int done;
   char *section;
   roadmap_db *child;
   roadmap_db_model *registered;


   /* For each module declared in the model, search for the matching table
    * and activate it. If no table has been found, de-activate the currently
    * activated table (i.e. cleanup the previous context).
    */
   for (registered = database->model;
        registered != NULL;
        registered = registered->next) {

      section = registered->section;

      if ((section == NULL) ||
          (registered->handler->activate == NULL)) continue;

      done = 0;

      for (child = database->root.first; child != NULL; child = child->next) {

         if (strcasecmp (section, child->head->name) == 0) {

            (*(registered->handler->activate)) (child->handler_context);
            done = 1;
            break;
         }
      }

      if (done) continue; /* Process the next module. */


      /* This module has no matching table. Maybe it's the root?
       * Being able to "activate" the root may be useful when a global
       * context is maintained for the map.
       */
      if ((section[0] == '/') && (section[1] == 0)) {

         (*(registered->handler->activate)) (database->root.handler_context);
         continue;
      }

      /* There is nothing matching this handler whatsoever:
       * de-activate what had been activated before, if any.
       */
      (* (registered->handler->activate)) (NULL);
   }
}


static void roadmap_db_call_unmap_one
               (roadmap_db_model *model, roadmap_db *this, char *section) {

   roadmap_db_model *registered;

   for (registered = model;
        registered != NULL;
        registered = registered->next) {

      if (strcasecmp (registered->section, section) == 0) {

         if ((this->handler_context != NULL) &&
             (registered->handler->unmap != NULL)) {
            (* (registered->handler->unmap)) (this->handler_context);
         }
         break;
      }
   }
   this->handler_context = NULL;
}

static void roadmap_db_call_unmap (roadmap_db_database *database) {

   roadmap_db *child;

   for (child = database->root.first; child != NULL; child = child->next) {

      roadmap_db_call_unmap_one (database->model, child, child->head->name);
   }

   roadmap_db_call_unmap_one (database->model, &database->root, "/");
}


static void roadmap_db_close_database (roadmap_db_database *database) {

   if (database->base != NULL) {

      roadmap_db_call_unmap (database);

      roadmap_db_free_subtree (&database->root);
   }

   if (database->file != NULL) {
      roadmap_file_unmap (&database->file);
   }

   if (database->next != NULL) {
      database->next->previous = database->previous;
   }
   if (database->previous == NULL) {
      RoadmapDatabaseFirst = database->next;
   } else {
      database->previous->next = database->next;
   }

   free(database->name);
   free(database->path);
   free(database);
}


roadmap_db_model *roadmap_db_register
                      (roadmap_db_model *model,
                       char *section,
                       roadmap_db_handler *handler) {

   roadmap_db_model *registered;

   /* Check there is not a handler already: */

   for (registered = model;
        registered != NULL;
        registered = registered->next) {

      if (section == registered->section) {
         roadmap_log (ROADMAP_FATAL,
                      "handler %s conflicts with %s for section %s",
                      handler->name,
                      registered->handler->name,
                      section);
      }

      if (strcasecmp (section, registered->section) == 0) {
         roadmap_log (ROADMAP_FATAL,
                      "handler %s conflicts with %s for section %s",
                      handler->name,
                      registered->handler->name,
                      section);
      }
   }

   registered = malloc (sizeof(roadmap_db_model));

   roadmap_check_allocated(registered);

   registered->section = section;
   registered->handler = handler;
   registered->next    = model;

   return registered;
}


int roadmap_db_open (const char *path,
                     const char *name,
                     roadmap_db_model *model) {

   RoadMapFileContext   file;
   roadmap_db_database *database = roadmap_db_find (path, name);


   if (database != NULL) {

      roadmap_db_call_activate (database);
      return 1; /* Already open. */
   }

   if (roadmap_file_map (path, name, "r", &file) == NULL) {

      roadmap_log (ROADMAP_INFO,
                   "cannot open database file %s in %s", name, path);
      return 0;
   }

   roadmap_log (ROADMAP_INFO, "Opening database file %s in %s", name, path);

   database = malloc(sizeof(*database));
   roadmap_check_allocated(database);

   database->file = file;
   database->name = strdup(name);
   database->path = strdup(path);
   database->base = roadmap_file_base (file);
   database->size = roadmap_file_size (file);

   if (RoadmapDatabaseFirst != NULL) {
      RoadmapDatabaseFirst->previous = database;
   }
   database->next       = RoadmapDatabaseFirst;
   database->previous   = NULL;
   RoadmapDatabaseFirst = database;


   database->model = model;

   database->root.head = (struct roadmap_db_section *) database->base;
   database->root.parent = NULL;
   database->root.level = 0;

   roadmap_db_make_tree (database, &database->root);

   if (! roadmap_db_call_map  (database)) {
      roadmap_db_close_database (database);
      return 0;
   }

   roadmap_db_call_activate (database);

   return 1;
}


void roadmap_db_activate (const char *path, const char *name) {

   roadmap_db_database *database = roadmap_db_find (path, name);

   if (database == NULL) {
      roadmap_log
         (ROADMAP_ERROR, "cannot activate database %s (not found)", name);
      return;
   }

   roadmap_log (ROADMAP_DEBUG, "Activating database %s", name);
   roadmap_db_call_activate (database);
}


roadmap_db *roadmap_db_get_subsection (roadmap_db *parent, char *path) {

   int found;
   unsigned length;
   roadmap_db *child;

   char *item;
   char *next;
   char *separator;
   char name[sizeof(parent->head->name)];

   for (item = path; *item != 0; item = next) {

      /* Retrieve the name of the next section in the path. */

      separator = strchr (item, '.');

      if (separator == NULL) {

         length = strlen(item);
         next = "";

      } else {

         length = (unsigned)(separator - path);
         next = separator + 1;
      }

      if (length >= sizeof(name)) {
         roadmap_log (ROADMAP_ERROR, "element too long in path %s", path);
         return NULL;
      }
      strncpy (name, item, length);
      name[length] = 0;


      /* Search this section. */

      found = 0;

      for (child = roadmap_db_get_first (parent);
           child != NULL;
           child = roadmap_db_get_next (child)) {

         if (strcasecmp (name, child->head->name) == 0) {
            found = 1;
            break;
         }
      }

      if (! found) return NULL;

      parent = child;
   }

   return parent;
}


roadmap_db *roadmap_db_get_first (roadmap_db *parent) {

   if (parent == NULL) {
      return NULL;
   }

   return parent->first;
}


char *roadmap_db_get_name (roadmap_db *section) {

   return section->head->name;
}


int roadmap_db_get_level (roadmap_db *section) {

   return section->level;
}


unsigned roadmap_db_get_size  (roadmap_db *section) {

   if (section->head->count == 0) {
      return 0;
   }
   return (unsigned) (section->head->size);
}


int roadmap_db_get_count (roadmap_db *section) {

   return section->head->count;
}


char *roadmap_db_get_data  (roadmap_db *section) {

   return (char *)(section->head + 1);
}


roadmap_db *roadmap_db_get_next  (roadmap_db *section) {

   return section->next;
}


void roadmap_db_close (const char *path, const char *name) {

   roadmap_db_database *database = roadmap_db_find (path, name);

   if (database != NULL) {
      roadmap_db_close_database (database);
   }
}


void roadmap_db_end (void) {

   roadmap_db_database *database;

   for (database = RoadmapDatabaseFirst;
        database != NULL;
        database = database->next) {

      roadmap_db_close_database (database);
   }
}

