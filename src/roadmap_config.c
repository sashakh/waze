/* roadmap_config.c - A module to handle all RoadMap configuration issues.
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
 *   See roadmap_config.h.
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_config.h"


typedef struct RoadMapConfigEnumRecord RoadMapConfigEnum;

struct RoadMapConfigEnumRecord {

   RoadMapConfigEnum *next;

   char *value;
};


#define ROADMAP_CONFIG_CLEAN    0
#define ROADMAP_CONFIG_SHARED   1
#define ROADMAP_CONFIG_DIRTY    2

typedef struct RoadMapConfigItemRecord RoadMapConfigItem;

struct RoadMapConfigItemRecord {

   RoadMapConfigItem *next;

   char *category;
   char *name;
   char *default_value;
   char *value;

   int   type;
   int   state; /* CLEAN, SHARED, DIRTY. */

   union {
      RoadMapConfigEnum *enumeration_values;
   } detail;
};


typedef struct {

   char              *name;
   int                state; /* CLEAN, SHARED or DIRTY. */
   RoadMapConfigItem *first_item;
   RoadMapConfigItem *last_item;

} RoadMapConfig;


static RoadMapConfig RoadMapConfigFiles[] = {
   {"session",     ROADMAP_CONFIG_CLEAN, NULL},
   {"preferences", ROADMAP_CONFIG_CLEAN, NULL},
   {"schema",      ROADMAP_CONFIG_CLEAN, NULL},
   {NULL, 0, NULL}
};



static RoadMapConfig *roadmap_config_search_file (const char *name) {

   RoadMapConfig *file;

   for (file = RoadMapConfigFiles; file->name != NULL; ++file) {
      if (strcmp (name, file->name) == 0) {
         return file;
      }
   }

   roadmap_log (ROADMAP_FATAL, "%s: not a valid config file name");
   return NULL;
}


static RoadMapConfigItem *roadmap_config_search_item (RoadMapConfig *config,
                                                      const char *category,
                                                      const char *name) {

   RoadMapConfigItem *item;

   for (item = config->first_item; item != NULL; item = item->next) {

      if ((strcmp (item->name, name) == 0) &&
          (strcmp (item->category, category) == 0)) {

         return item;
      }
   }
   return NULL;
}


static RoadMapConfigItem *roadmap_config_new_item
                             (RoadMapConfig *config,
                              const char    *category,
                              const char    *name,
                              const char    *default_value,
                              int            item_type) {

   RoadMapConfigItem *new_item;

   new_item = roadmap_config_search_item (config, category, name);

   if (new_item != NULL) {

      /* Not so new. Update the type & default value, if any. */

      if ((default_value[0] != 0) && (new_item->default_value[0] == 0)) {
         new_item->default_value = strdup(default_value);
      }

      if (item_type != ROADMAP_CONFIG_STRING && new_item->type != item_type) {

         if (new_item->type != ROADMAP_CONFIG_STRING) {
            roadmap_log (ROADMAP_ERROR,
                         "type conflict for %s.%s: % replaced by %d",
                         category, name, new_item->type, item_type);
         } else {
            new_item->type = item_type;
         }
      }

      return new_item;
   }

   new_item = calloc (1, sizeof(RoadMapConfigItem));

   if (new_item == NULL) {
      roadmap_log (ROADMAP_FATAL, "no more memory");
   }
   new_item->name     = strdup(name);
   new_item->category = strdup(category);

   if (default_value[0] == 0) {
      new_item->default_value = "";
   } else {
      new_item->default_value = strdup(default_value);
   }

   new_item->value = NULL;
   new_item->state = ROADMAP_CONFIG_CLEAN;
   new_item->type  = item_type;

   new_item->next = config->first_item;

   if (config->last_item == NULL) {
      config->last_item = new_item;
   }
   config->first_item = new_item;

   return new_item;
}


static void roadmap_config_add_enumeration_value (RoadMapConfigItem *item,
                                                  const char *value) {

   RoadMapConfigEnum *new_value;
   RoadMapConfigEnum *last = item->detail.enumeration_values;

   new_value = malloc (sizeof(RoadMapConfigEnum));
   if (new_value == NULL) {
      roadmap_log (ROADMAP_FATAL, "no more memory");
   }
   new_value->next = NULL;
   new_value->value = strdup(value);

   if (last == NULL) {

      item->detail.enumeration_values = new_value;

   } else {

      while (last->next != NULL) last = last->next;

      last->next = new_value;
   }
}


void roadmap_config_declare (const char *config,
                             const char *category,
                             const char *name,
                             const char *default_value) {

   RoadMapConfig *file = roadmap_config_search_file (config);

   roadmap_config_new_item
      (file, category, name, default_value, ROADMAP_CONFIG_STRING);
}


void roadmap_config_declare_enumeration (const char *config,
                                         const char *category,
                                         const char *name,
                                         const char *enumeration_value, ...) {

   char *p;
   va_list ap;
   RoadMapConfig *file = roadmap_config_search_file (config);
   RoadMapConfigItem *item;
   RoadMapConfigEnum *next;
   RoadMapConfigEnum *enumerated;


   item = roadmap_config_new_item
             (file, category, name, enumeration_value, ROADMAP_CONFIG_ENUM);

   /* Replace the enumeration list. */

   for (enumerated = item->detail.enumeration_values;
        enumerated != NULL;
        enumerated = next) {
      next = enumerated->next;
      free (enumerated->value);
      free (enumerated);
   }
   item->detail.enumeration_values = NULL;

   roadmap_config_add_enumeration_value (item, enumeration_value);

   va_start(ap, enumeration_value);
   for (p = va_arg(ap, char *); p != NULL; p = va_arg(ap, char *)) {
      roadmap_config_add_enumeration_value (item, p);
   }
}


void roadmap_config_declare_color (const char *config,
                                   const char *category,
                                   const char *name,
                                   const char *default_value) {

   RoadMapConfig *file = roadmap_config_search_file (config);

   roadmap_config_new_item
      (file, category, name, default_value, ROADMAP_CONFIG_COLOR);
}


void *roadmap_config_first (char *config) {

   return roadmap_config_search_file(config)->first_item;
}


int roadmap_config_get_type (void *cursor) {

   RoadMapConfigItem *item = (RoadMapConfigItem *)cursor;

   if (item == NULL) {
      return ROADMAP_CONFIG_STRING;
   }

   return item->type;
}


void *roadmap_config_scan
         (void *cursor, char **category, char **name, char **value) {

   RoadMapConfigItem *item = (RoadMapConfigItem *)cursor;

   if (item == NULL) {
      return NULL;
   }

   *category = item->category;
   *name     = item->name;
   if (item->value != NULL) {
      *value = item->value;
   } else {
      *value = item->default_value;
   }

   return item->next;
}


void *roadmp_config_get_enumeration (void *cursor) {

   RoadMapConfigItem *item = (RoadMapConfigItem *)cursor;

   if (item == NULL) {
      return NULL;
   }

   if (item->type != ROADMAP_CONFIG_ENUM) {
      return NULL;
   }

   return item->detail.enumeration_values;
}


char *roadmp_config_get_enumeration_value (void *enumeration) {

   RoadMapConfigEnum *item = (RoadMapConfigEnum *)enumeration;

   if (item == NULL) { 
      return NULL;
   }

   return item->value;
}


void *roadmp_config_get_enumeration_next (void *enumeration) {

   RoadMapConfigEnum *item = (RoadMapConfigEnum *)enumeration;

   if (item == NULL) { 
      return NULL;
   }

   return item->next;
}


static void roadmap_config_set_item (RoadMapConfigItem *item, char *value) {

   if (item->value != NULL) {
      if (value != NULL) {
         if (strcmp (item->value, value) == 0) {

            return; /* Nothing was changed. */
         }
      }
      free(item->value);
   }
   if (value != NULL) {
      item->value = strdup(value);
   } else {
      item->value = NULL;
   }

   item->state = ROADMAP_CONFIG_DIRTY;
}


static char *roadmap_config_skip_until (char *p, char c) {

   while (*p != '\n' && *p != c && *p > 0) p++;
   return p;
}

static char *roadmap_config_skip_spaces (char *p) {

   while (*p == ' ' || *p == '\t') p++;
   return p;
}


static FILE *roadmap_config_open (const char *path, char *name, char *mode) {

   FILE *file;
   char *full_name;

   if (*mode == 'w') {
      mkdir  (path, 0755);
   }
   full_name = roadmap_file_join (path, name);

   file = fopen (full_name, mode);

   if (file == NULL) {
      if (*mode == 'w') {
         roadmap_log (ROADMAP_FATAL, "cannot open file %s", full_name);
      }
   }

   free (full_name);

   return file;
}


static void roadmap_config_load
               (const char *path, RoadMapConfig *config, int intended_state) {

   char *p;
   FILE *file;
   char  line[1024];

   char *category;
   char *name;
   char *value;

   RoadMapConfigItem *item;


   file = roadmap_config_open (path, config->name, "r");

   if (file == NULL) return;


   while (!feof(file)) {

      if (fgets (line, sizeof(line), file) == NULL) break;

      p = roadmap_config_skip_spaces (line);

      if (*p == '\n' || *p == '#') continue;

      category = p;

      p = roadmap_config_skip_until (p, '.');
      if (*p != '.') continue;
      *(p++) = 0;

      name = p;

      p = roadmap_config_skip_until (p, ':');
      if (*p != ':') continue;
      *(p++) = 0;

      p = roadmap_config_skip_spaces (p);
      value = p;

      p = roadmap_config_skip_until (p, 0);
      *p = 0;

      item = roadmap_config_new_item
                (config, category, name, "", ROADMAP_CONFIG_STRING);
      if (item->value != NULL) {
         free(item->value);
      }
      item->value = strdup(value);
      item->state = intended_state;
   }
   fclose (file);

   config->state = ROADMAP_CONFIG_CLEAN;
}


static void roadmap_config_update
               (const char *path, RoadMapConfig *config, int force) {

   FILE *file;
   char *value;
   RoadMapConfigItem *item;

   if (force || (config->state == ROADMAP_CONFIG_DIRTY)) {

      file = roadmap_config_open (roadmap_file_user(), config->name, "w");

      for (item = config->first_item; item != NULL; item = item->next) {

         if ((! force) && (item->state == ROADMAP_CONFIG_SHARED)) continue;

         if (item->value != NULL) {
            value = item->value;
         } else {
            value = item->default_value;
         }
         fprintf (file, "%s.%s: %s\n", item->category, item->name, value);
      }

      fclose (file);
      config->state = ROADMAP_CONFIG_CLEAN;
   }
}


void  roadmap_config_initialize (void) {

   const char *p;
   RoadMapConfig *file;

   roadmap_config_declare
      ("preferences", "General", "Database", roadmap_file_default_path());

   for (file = RoadMapConfigFiles; file->name != NULL; ++file) {

      for (p = roadmap_file_path_last();
           p != NULL;
           p = roadmap_file_path_previous(p)) {

         roadmap_config_load (p, file, ROADMAP_CONFIG_SHARED);
      }

      roadmap_config_load (roadmap_file_user(), file, ROADMAP_CONFIG_CLEAN);
   }
}


void roadmap_config_save (int force) {

   RoadMapConfig *file;

   for (file = RoadMapConfigFiles; file->name != NULL; ++file) {
      roadmap_config_update (roadmap_file_user(), file, force);
   }
}


char *roadmap_config_get (char *category, char *name) {

   RoadMapConfig *file;
   RoadMapConfigItem *item;

   for (file = RoadMapConfigFiles; file->name != NULL; ++file) {

      item = roadmap_config_search_item (file, category, name);
      if (item != NULL) {

         if (item->value != NULL) {
            return item->value;
         }
         return item->default_value;
      }
   }

   return "";
}


int roadmap_config_get_integer(char *category, char *name) {

   return atoi (roadmap_config_get (category, name));
}


void  roadmap_config_set (char *category, char *name, char *value) {

   RoadMapConfig *file;
   RoadMapConfigItem *item;

   for (file = RoadMapConfigFiles; file->name != NULL; ++file) {

      item = roadmap_config_search_item (file, category, name);

      if (item != NULL) {

         roadmap_config_set_item (item, value);

         file->state = ROADMAP_CONFIG_DIRTY;
         return;
      }
   }
}


/* The following two functions are special because positions are always
 * session items. The reason is only that I am anal: I don't see any reason
 * for having a position in the schema or preferences.
 */
void roadmap_config_get_position (char *name, RoadMapPosition *position) {

   char *center;
   RoadMapConfig *file;
   RoadMapConfigItem *item;

   file = roadmap_config_search_file ("session");
   item = roadmap_config_search_item (file, "Locations", name);

   if (item->value != NULL) {
      center = item->value;
   } else {
      center = item->default_value;
   }

   if (center != NULL && center[0] != 0) {

      char *latitude_ascii;
      char buffer[128];


      strcpy (buffer, center);
      latitude_ascii = strchr (buffer, ',');
      if (latitude_ascii != NULL) {
         *(latitude_ascii++) = 0;
      }

      position->longitude = atoi(buffer);
      position->latitude  = atoi(latitude_ascii);

   } else {

      position->longitude = 0;
      position->latitude  = 0;
   }
}

void  roadmap_config_set_position (char *name, RoadMapPosition *position) {

   char buffer[128];
   RoadMapConfig *file;
   RoadMapConfigItem *item;

   file = roadmap_config_search_file ("session");
   item = roadmap_config_search_item (file, "Locations", name);

   if (item != NULL) {

      sprintf (buffer, "%d,%d", position->longitude, position->latitude);
      roadmap_config_set_item (item, buffer);
      file->state = ROADMAP_CONFIG_DIRTY;
   }
}

