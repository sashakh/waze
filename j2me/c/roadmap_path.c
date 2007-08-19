/* roadmap_path.c - a module to handle file path in an OS independent way.
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
 *   See roadmap_path.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"


typedef struct RoadMapPathRecord *RoadMapPathList;

struct RoadMapPathRecord {

   RoadMapPathList next;

   char  *name;
   int    count;
   char **items;
   char  *preferred;
};

static RoadMapPathList RoadMapPaths = NULL;


/* This list exist because the user context is a special case
 * that we want to handle in a standard way.
 */
static const char *RoadMapPathUser[] = {
   "",
   NULL
};
static const char *RoadMapPathUserPreferred = "./roadmap";


/* Skins directories */
static const char *RoadMapPathSkin[] = {
   "",
   "./roadmap/skins/default/day",
   "./roadmap/skins/default",
   NULL
};
static const char *RoadMapPathSkinPreferred = "./roadmap/skins";

/* The hardcoded path for configuration files (the "config" path).
 * (Note that the user directory (~/.roadmap) does not appear here
 * as it is implicitely used by the callers--see above.)
 */ 
static const char *RoadMapPathConfig[] = {
   /* This is for standard Unix configurations. */
   "rs:/",
   "/",
   NULL
};
static const char *RoadMapPathConfigPreferred =
                      "./roadmap";


/* The default path for the map files (the "maps" path): */
static const char *RoadMapPathMaps[] = {
   /* This is for standard Unix configurations. */
   "/",
   "file:///e:/freemap",
   "file:///root1/RoadMap",
   NULL
};
static const char *RoadMapPathMapsPreferred =
                      "/";


static char *roadmap_path_expand (const char *item, size_t length);

static void roadmap_path_list_create(const char *name,
                                     const char *items[],
                                     const char *preferred) {


   size_t i;
   size_t count;
   RoadMapPathList new_path;

   for (count = 0; items[count] != NULL; ++count) ;

   new_path = malloc (sizeof(struct RoadMapPathRecord));
   roadmap_check_allocated(new_path);

   new_path->next  = RoadMapPaths;
   new_path->name  = strdup(name);
   new_path->count = (int)count;

   new_path->items = calloc (count, sizeof(char *));
   roadmap_check_allocated(new_path->items);

   for (i = 0; i < count; ++i) {
      new_path->items[i] = roadmap_path_expand (items[i], strlen(items[i]));
   }
   new_path->preferred  = roadmap_path_expand (preferred, strlen(preferred));

   RoadMapPaths = new_path;
}

static RoadMapPathList roadmap_path_find (const char *name) {

   RoadMapPathList cursor;

   if (RoadMapPaths == NULL) {

      /* Add the hardcoded configuration. */

      roadmap_path_list_create ("user",   RoadMapPathUser,
                                          RoadMapPathUserPreferred);

      roadmap_path_list_create ("config", RoadMapPathConfig,
                                          RoadMapPathConfigPreferred);

      roadmap_path_list_create ("skin",   RoadMapPathSkin,
                                          RoadMapPathSkinPreferred);

      roadmap_path_list_create ("maps",   RoadMapPathMaps,
                                          RoadMapPathMapsPreferred);
   }

   for (cursor = RoadMapPaths; cursor != NULL; cursor = cursor->next) {
      if (strcasecmp(cursor->name, name) == 0) break;
   }
   return cursor;
}


/* Directory path strings operations. -------------------------------------- */

static char *roadmap_path_cat (const char *s1, const char *s2) {
    
    char *result = malloc (strlen(s1) + strlen(s2) + 4);

    roadmap_check_allocated (result);

    strcpy (result, s1);
    if (strcmp(s1, "/")) strcat (result, "/");
    strcat (result, s2);
    
    return result;
}


char *roadmap_path_join (const char *path, const char *name) {

   if ((path == NULL) || (!*path)) {
      return strdup (name);
   }
   return roadmap_path_cat (path, name);
}


char *roadmap_path_parent (const char *path, const char *name) {

   char *separator;
   char *full_name = roadmap_path_join (path, name);

   separator = strrchr (full_name, '/');
   if (separator == NULL) {
      return ".";
   }

   *separator = 0;

   return full_name;
}


char *roadmap_path_skip_directories (const char *name) {

   char *result = strrchr (name, '/');

   if (result == NULL) return (char *)name;

   return result + 1;
}


char *roadmap_path_remove_extension (const char *name) {

   char *result;
   char *p;


   result = strdup(name);
   roadmap_check_allocated(result);

   p = roadmap_path_skip_directories (result);
   p = strrchr (p, '.');
   if (p != NULL) *p = 0;

   return result;
}


/* The standard directory paths: ------------------------------------------- */

static const char *roadmap_path_home (void) {

   return ".";
}


const char *roadmap_path_user (void) {

    return "rs:/";
}


const char *roadmap_path_trips (void) {
    
    static char  RoadMapDefaultTrips[] = ".roadmap/trips";
    static char *RoadMapTrips = NULL;
    
    if (RoadMapTrips == NULL) {
        
            RoadMapTrips =
               roadmap_path_cat (roadmap_path_home(), RoadMapDefaultTrips);
    }
    return RoadMapTrips;
}
            

static char *roadmap_path_expand (const char *item, size_t length) {

   const char *expansion;
   size_t expansion_length;
   char *expanded;

   switch (item[0]) {
      case '~': expansion = roadmap_path_home(); item++; length--; break;
      case '&': expansion = roadmap_path_user(); item++; length--; break;
      default:  expansion = "";
   }
   expansion_length = strlen(expansion);

   expanded = malloc (length + expansion_length + 1);
   roadmap_check_allocated(expanded);

   strcpy (expanded, expansion);
   strncat (expanded, item, length);

   expanded[length+expansion_length] = 0;

   return expanded;
}


/* Path lists operations. -------------------------------------------------- */

void roadmap_path_set (const char *name, const char *path) {

   int i;
   size_t count;
   const char *item;
   const char *next_item;

   RoadMapPathList path_list = roadmap_path_find (name);


   if (path_list == NULL) {
      roadmap_log(ROADMAP_FATAL, "unknown path set '%s'", name);
   }

   while (*path == ',') path += 1;
   if (*path == 0) return; /* Ignore empty path: current is better. */


   if (path_list->items != NULL) {

      /* This replaces a path that was already set. */

      for (i = path_list->count-1; i >= 0; --i) {
         free (path_list->items[i]);
      }
      free (path_list->items);
   }


   /* Count the number of items in this path string. */

   count = 0;
   for (item = path-1; item != NULL; item = strchr (item+1, ',')) {
      count += 1;
   }

   path_list->items = calloc (count, sizeof(char *));
   roadmap_check_allocated(path_list->items);


   /* Extract and expand each item of the path.
    * Ignore directories that do not exist yet.
    */
   for (i = 0, item = path-1; item != NULL; item = next_item) {

      item += 1;
      next_item = strchr (item, ',');

      if (next_item == NULL) {
         path_list->items[i] = roadmap_path_expand (item, strlen(item));
      } else {
         path_list->items[i] =
            roadmap_path_expand (item, (size_t)(next_item - item));
      }

      if (roadmap_file_exists(NULL, path_list->items[i])) {
         ++i;
      } else {
         free (path_list->items[i]);
         path_list->items[i] = NULL;
      }
   }
   path_list->count = i;
}


const char *roadmap_path_first (const char *name) {

   RoadMapPathList path_list = roadmap_path_find (name);

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   if (path_list->count > 0) {
      return path_list->items[0];
   }

   return NULL;
}


const char *roadmap_path_next  (const char *name, const char *current) {

   int i;
   RoadMapPathList path_list = roadmap_path_find (name);


   for (i = 0; i < path_list->count-1; ++i) {

      if (path_list->items[i] == current) {
         return path_list->items[i+1];
      }
   }

   return NULL;
}


const char *roadmap_path_last (const char *name) {

   RoadMapPathList path_list = roadmap_path_find (name);

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   if (path_list->count > 0) {
      return path_list->items[path_list->count-1];
   }
   return NULL;
}


const char *roadmap_path_previous (const char *name, const char *current) {

   int i;
   RoadMapPathList path_list = roadmap_path_find (name);

   for (i = path_list->count-1; i > 0; --i) {

      if (path_list->items[i] == current) {
         return path_list->items[i-1];
      }
   }
   return NULL;
}


/* This function always return a hardcoded default location,
 * which is the recommended location for these objects.
 */
const char *roadmap_path_preferred (const char *name) {

   RoadMapPathList path_list = roadmap_path_find (name);

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   return path_list->preferred;
}


void roadmap_path_create (const char *path) {
}


static char *RoadMapPathEmptyList = NULL;

char **roadmap_path_list (const char *path, const char *extension) {

   return &RoadMapPathEmptyList;
}


void   roadmap_path_list_free (char **list) {

   char **cursor;

   if ((list == NULL) || (list == &RoadMapPathEmptyList)) return;

   for (cursor = list; *cursor != NULL; ++cursor) {
      free (*cursor);
   }
   free (list);
}


void roadmap_path_free (const char *path) {
   free ((char *) path);
}


const char *roadmap_path_search_icon (const char *name) {

   static char result[256];

   sprintf (result, "%s/pixmaps/rm_%s.png", roadmap_path_home(), name);
   if (roadmap_file_exists(NULL, result)) return result;

   sprintf (result, "/usr/local/share/pixmaps/rm_%s.png", name);
   if (roadmap_file_exists(NULL, result)) return result;

   sprintf (result, "/usr/share/pixmaps/rm_%s.png", name);
   if (roadmap_file_exists(NULL, result)) return result;

   return NULL; /* Not found. */
}


int roadmap_path_is_full_path (const char *name) {
   return name[0] == '/';
}


int roadmap_path_is_directory (const char *name) {

   return 0;
}


const char *roadmap_path_temporary (void) {

   return "/tmp";
}

