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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"


static char **RoadMapPathItem  = NULL;
static int    RoadMapPathCount = 0;


static const char *roadmap_path_home (void) {

   static char *RoadMapPathHome = NULL;

   if (RoadMapPathHome == NULL) {

      RoadMapPathHome = getenv("HOME");

      if (RoadMapPathHome == NULL) {
         RoadMapPathHome = "";
      }
   }

   return RoadMapPathHome;
}


static char *roadmap_path_cat (const char *s1, const char *s2) {
    
    char *result = malloc (strlen(s1) + strlen(s2) + 4);

    roadmap_check_allocated (result);

    strcpy (result, s1);
    strcat (result, "/");
    strcat (result, s2);
    
    return result;
}


const char *roadmap_path_default (void) {

#ifdef QWS
    /* This is for the Sharp Zaurus PDA.. */
    return ".,"
           "/opt/QtPalmtop/share/roadmap,"
           "/mnt/cf/QtPalmtop/share/roadmap,"
           "/mnt/card/QtPalmtop/share/roadmap";
#else
    return ".,"
           "/usr/local/share/roadmap,"
           "/usr/share/roadmap,"
           "&/maps";
#endif
}


const char *roadmap_path_user (void) {

    static char  RoadMapDirectory[] = ".roadmap";
    static char *RoadMapUser = NULL;

    if (RoadMapUser == NULL) {
        RoadMapUser = roadmap_path_cat (roadmap_path_home(), RoadMapDirectory);
        mkdir (RoadMapUser, 0770);
    }
    return RoadMapUser;
}


const char *roadmap_path_trips (void) {
    
    static char  RoadMapDefaultTrips[] = ".roadmap/trips";
    static char *RoadMapTrips = NULL;
    
    if (RoadMapTrips == NULL) {
        
        RoadMapTrips = getenv("ROADMAP_TRIPS");
        
        if (RoadMapTrips == NULL) {
            RoadMapTrips = roadmap_path_cat (roadmap_path_home(), RoadMapDefaultTrips);
        }
        
        mkdir (RoadMapTrips, 0770);
    }
    return RoadMapTrips;
}
            

void roadmap_path_set (const char *path) {

   int i;
   int length;
   int expand_length;
   const char *item;
   const char *expand;
   const char *next_item;


   while (*path == ',') path += 1;
   if (*path == 0) return; /* Ignore empty path: current is better. */


   if (RoadMapPathItem != NULL) {

      /* This replaces a path that was already set. */

      for (i = RoadMapPathCount-1; i >= 0; --i) {
         free (RoadMapPathItem[i]);
      }
      free (RoadMapPathItem);
   }


   /* Count the number of items in this path string. */

   RoadMapPathCount = 0;
   for (item = path-1; item != NULL; item = strchr (item+1, ',')) {
      RoadMapPathCount += 1;
   }

   RoadMapPathItem = calloc (RoadMapPathCount, sizeof(char *));
   roadmap_check_allocated(RoadMapPathItem);


   /* Extract and expand each item of the path.
    * Ignore directories that do not exist yet.
    */
   for (i = 0, item = path-1; item != NULL; item = next_item) {

      item += 1;
      next_item = strchr (item, ',');

      switch (item[0]) {
         case '~': expand = roadmap_path_home(); item += 1; break;
         case '&': expand = roadmap_path_user(); item += 1; break;
         default:  expand = "";
      }
      expand_length = strlen(expand);

      if (next_item == NULL) {
         length = strlen(item);
      } else {
         length = next_item - item;
      }

      RoadMapPathItem[i] = malloc (length + expand_length + 1);
      roadmap_check_allocated(RoadMapPathItem[i]);

      strcpy (RoadMapPathItem[i], expand);
      strncat (RoadMapPathItem[i], item, length);

      (RoadMapPathItem[i])[length+expand_length] = 0;

      if (roadmap_file_exists(NULL, RoadMapPathItem[i])) {
         ++i;
      } else {
         free (RoadMapPathItem[i]);
         RoadMapPathItem[i] = NULL;
      }
   }
   RoadMapPathCount = i;
}


const char *roadmap_path_first (void) {

   if (RoadMapPathItem == NULL) {
      roadmap_path_set (roadmap_path_default());
   }

   if (RoadMapPathCount > 0) {
      return RoadMapPathItem[0];
   }

   return NULL;
}


const char *roadmap_path_next  (const char *current) {

   int i;

   for (i = 0; i < RoadMapPathCount-1; ++i) {

      if (RoadMapPathItem[i] == current) {
         return RoadMapPathItem[i+1];
      }
   }

   return NULL;
}


const char *roadmap_path_last (void) {

   if (RoadMapPathItem == NULL) {
      roadmap_path_set (roadmap_path_default());
   }

   if (RoadMapPathCount > 0) {
      return RoadMapPathItem[RoadMapPathCount-1];
   }
   return NULL;
}


const char *roadmap_path_previous (const char *current) {

   int i;

   for (i = RoadMapPathCount-1; i > 0; --i) {

      if (RoadMapPathItem[i] == current) {
         return RoadMapPathItem[i-1];
      }
   }
   return NULL;
}


char *roadmap_path_join (const char *path, const char *name) {

   if (path == NULL) {
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


void roadmap_path_create (const char *path) {

   char command[256];

   snprintf (command, sizeof(command), "mkdir -p %s", path);
   system (command);
}


static char *RoadMapPathEmptyList = NULL;

char **roadmap_path_list (const char *path, const char *extension) {

   char *match;
   int   length;
   int   count;
   char **result;
   char **cursor;

   DIR *directory;
   struct dirent *entry;


   directory = opendir (path);
   if (directory == NULL) return &RoadMapPathEmptyList;

   count = 0;
   while ((entry = readdir(directory)) != NULL) ++count;

   cursor = result = calloc (count+1, sizeof(char *));
   roadmap_check_allocated (result);

   rewinddir (directory);
   length = strlen(extension);

   while ((entry = readdir(directory)) != NULL) {

      match = entry->d_name + strlen(entry->d_name) - length;

      if (! strcmp (match, extension)) {
         *(cursor++) = strdup (entry->d_name);
      }
   }
   *cursor = NULL;

   return result;
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

