/**
 * @brief roadmap_path.c - a module to handle file path in an OS independent way.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2008, Danny Backx.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <ftw.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_scan.h"
#include "roadmap_config.h"
#include "roadmap_list.h"

static RoadMapConfigDescriptor RoadMapConfigPathTrips =
                        ROADMAP_CONFIG_ITEM("General", "TripsPath");


const char *RoadMapPathCurrentDirectory = ".";

typedef struct RoadMapPathRecord *RoadMapPathList;

struct RoadMapPathRecord {

   RoadMapPathList next;

   char  *name;
   int    count;
   RoadMapList itemlist;
   char  *preferred;
};

typedef struct {
   RoadMapListItem link;
   char *path;
} RoadMapPathItem ;

static RoadMapPathList RoadMapPaths = NULL;


/* This list exist because the user context is a special case
 * that we want to handle in a standard way.
 */
static const char *RoadMapPathUser[] = {
   "~/.roadmap",
   NULL
};
static const char RoadMapPathUserPreferred[] = "~/.roadmap";


/* The hardcoded path for configuration files (the "config" path).
 * (Note that the user directory (~/.roadmap) does not appear here
 * as it is implicitely used by the callers--see above.)
 */ 
static const char *RoadMapPathConfig[] = {
#ifdef ROADMAP_CONFIG_DIR
   ROADMAP_CONFIG_DIR,
#endif
#ifdef QWS
   /* This is for the Sharp Zaurus PDA.. */
   "/opt/QtPalmtop/share/roadmap",
   "/mnt/cf/QtPalmtop/share/roadmap",
   "/mnt/card/QtPalmtop/share/roadmap",
#else
   /* This is for standard Unix configurations. */
   "/etc/roadmap",
   "/usr/local/share/roadmap",
   "/usr/share/roadmap",
#endif
   NULL
};
static const char RoadMapPathConfigPreferred[] =
#ifdef ROADMAP_CONFIG_DIR
                        ROADMAP_CONFIG_DIR;
#else
#ifdef QWS
                      "/mnt/cf/QtPalmtop/share/roadmap";
#else
                      "/usr/local/share/roadmap";
#endif
#endif


/* The default path for the map files (the "maps" path): */
static const char *RoadMapPathMaps[] = {
#ifdef ROADMAP_MAP_DIR
   ROADMAP_MAP_DIR,
#endif
#ifdef QWS
   /* This is for the Sharp Zaurus PDA.. */
   "/opt/QtPalmtop/share/roadmap/...",
   "/mnt/cf/QtPalmtop/share/roadmap/...",
   "/mnt/card/QtPalmtop/share/roadmap/...",
#else
   /* This is for standard Unix configurations. */
   "&/maps/...",
   "/var/lib/roadmap/...",
   "/usr/lib/roadmap/...",

   /* These paths are not really compliant with the FHS, but we
    * keep them for compatibility with older versions of RoadMap.
    */
   "/usr/local/share/roadmap/...",
   "/usr/share/roadmap/...",
#endif
   NULL
};
static const char RoadMapPathMapsPreferred[] =
#ifdef ROADMAP_MAP_DIR
                        ROADMAP_MAP_DIR;
#else
#ifdef QWS
                      "/mnt/cf/QtPalmtop/share/roadmap";
#else
                      "/var/lib/roadmap";
#endif
#endif

/* The default path for the icon files (the "icons" path): */
static const char *RoadMapPathIcons[] = {
   "~/.roadmap/pixmaps",
#ifdef ROADMAP_CONFIG_DIR
   ROADMAP_CONFIG_DIR "/pixmaps",
#endif
   "/usr/local/share/pixmaps",
   "/usr/share/pixmaps",
   NULL
};

/* The default path for the skin files (the "skin" path): */
static const char *RoadMapPathSkin[] =  {
	"&\\skins\\default\\day",
	"&\\skins\\default",
	NULL
};
static const char RoadMapPathSkinPreferred[] = "&\\skins";


static char *roadmap_path_expand (const char *item, size_t length);
static void roadmap_path_addlist(RoadMapList *list, char *path);

/**
 * @brief
 * @param name
 * @param items
 * @param preferred
 * @return
 */
static RoadMapPathList roadmap_path_list_create(const char *name,
                                     const char **items,
                                     const char *preferred) {


   RoadMapPathList new_path;
   char *p;

   new_path = malloc (sizeof(struct RoadMapPathRecord));
   roadmap_check_allocated(new_path);

   new_path->next  = RoadMapPaths;
   new_path->name  = strdup(name);
   ROADMAP_LIST_INIT(&new_path->itemlist);
   new_path->preferred = NULL;

   while (items && *items) {
      p = roadmap_path_expand (*items, strlen(*items));
      roadmap_path_addlist(&new_path->itemlist, p);
      items++;
   }
   if (preferred) {
      new_path->preferred = roadmap_path_expand (preferred, strlen(preferred));
   }

   RoadMapPaths = new_path;

   return new_path;
}

/**
 * @brief
 */
struct {
    char *name;
    const char **pathlist;
    const char *pathpreferred;
} RoadMapPathLists[] = {
      {"user",      RoadMapPathUser,    RoadMapPathUserPreferred },
      {"config",    RoadMapPathConfig,  RoadMapPathConfigPreferred },
      {"maps",      RoadMapPathMaps,    RoadMapPathMapsPreferred },
      {"icons",     RoadMapPathIcons,   NULL},
      {"skin",      RoadMapPathSkin,    RoadMapPathSkinPreferred },
      {"features",  NULL,               NULL},
};

/**
 * @brief look up the directory path(s) associated with a given keyword
 * @param name the keyword
 * @param init_ok create the file if it doesn't exist
 * @return
 */
static RoadMapPathList roadmap_path_find (const char *name, int init_ok) {

   RoadMapPathList cursor;

   for (cursor = RoadMapPaths; cursor != NULL; cursor = cursor->next) {
      if (strcasecmp(cursor->name, name) == 0)
          return cursor;
   }

   if (init_ok) {  /* not found, try to create it */
        unsigned int i;
        for (i = 0;
            i < sizeof(RoadMapPathLists)/sizeof(RoadMapPathLists[0]); i++) {
            if (strcmp(name, RoadMapPathLists[i].name) == 0) {
                return roadmap_path_list_create
                        (name, RoadMapPathLists[i].pathlist,
                            RoadMapPathLists[i].pathpreferred);
            }
        }
   }

   return NULL;
}


/* Directory path strings operations. -------------------------------------- */

static char *roadmap_path_cat (const char *s1, const char *s2) {
    
    char *result = malloc (strlen(s1) + strlen(s2) + 4);

    roadmap_check_allocated (result);

    strcpy (result, s1);
    strcat (result, "/");
    strcat (result, s2);
    
    return result;
}


char *roadmap_path_join (const char *path, const char *name) {

   if (path == NULL || path[0] == 0) {
      return strdup (name);
   }
   return roadmap_path_cat (path, name);
}


char *roadmap_path_parent (const char *path, const char *name) {

   char *separator;
   char *full_name = roadmap_path_join (path, name);

   separator = strrchr (full_name, '/');
   if (separator == NULL) {
      roadmap_path_free(full_name);
      return strdup(RoadMapPathCurrentDirectory);
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

   static char *RoadMapPathHome = NULL;

   if (RoadMapPathHome == NULL) {

      RoadMapPathHome = getenv("HOME");

      if (RoadMapPathHome == NULL) {
         RoadMapPathHome = "";
      }
   }

   return RoadMapPathHome;
}


const char *roadmap_path_user (void) {

    static const char *RoadMapUser = NULL;

    if (RoadMapUser == NULL) {
        RoadMapUser = roadmap_path_first ("user");
        mkdir (RoadMapUser, 0770);
    }
    return RoadMapUser;
}


const char *roadmap_path_trips (void) {
    
    static const char *RoadMapTrips = NULL;

    if (RoadMapTrips == NULL) {

        RoadMapTrips = roadmap_config_get(&RoadMapConfigPathTrips);
        RoadMapTrips = roadmap_path_expand (RoadMapTrips, strlen(RoadMapTrips));

        mkdir (RoadMapTrips, 0770);
    }
    return RoadMapTrips;
}
            

static char *roadmap_path_expand (const char *item, size_t length) {

   const char *expansion;
   size_t expansion_length;
   char *expanded;

   switch (item[0]) {
      case '~': expansion = roadmap_path_home(); item += 1; length -= 1; break;
      case '&': expansion = roadmap_path_user(); item += 1; length -= 1; break;
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

/* bah.  globals.  too bad ftw() doesn't take a "void *cookie" argument */
static RoadMapList *RoadMapPathFTWDirList;
static int RoadMapPathFTWPathLen;

static int roadmap_path_ftw_cb
        (const char *path, const struct stat *sb, int flag) {

   const char *p = path;
   RoadMapPathItem *pathitem;

   if (flag != FTW_D) return 0;

   /* skip hidden dirs, but only those deeper than where we started */
   while ((p = strstr(p, "/.")) != NULL) {
      if (p - path >= RoadMapPathFTWPathLen) return 0;
      p++;
   }

   pathitem = malloc(sizeof(RoadMapPathItem));
   roadmap_check_allocated(pathitem);
   pathitem->path = strdup(path);
   roadmap_list_append(RoadMapPathFTWDirList, &pathitem->link);

   return 0;
}

static void roadmap_path_addlist(RoadMapList *list, char *path) {
   int len;
   len = strlen(path);
   if (len > 4 && strcmp(&path[len-4], "/...") == 0) {
      path[len-4] = '\0';
      RoadMapPathFTWDirList = list;
      RoadMapPathFTWPathLen = len - 4;
      ftw(path, roadmap_path_ftw_cb, 30);
      free(path);
   } else {
      RoadMapPathItem *pathitem;
      pathitem = malloc(sizeof(RoadMapPathItem));
      roadmap_check_allocated(pathitem);

      pathitem->path = path;
      roadmap_list_append(list, &pathitem->link);
   }
}

void roadmap_path_set (const char *name, const char *path) {

   const char *item;
   const char *next_item;
   RoadMapListItem *elem, *tmp;

   while (*path == ',') path += 1;
   if (*path == 0) return; /* Ignore empty path: current is better. */

   RoadMapPathList path_list = roadmap_path_find (name, 0);
   if (path_list == NULL) {
       path_list = roadmap_path_list_create(name, NULL, NULL);
       if (path_list == NULL) {
          roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
       }
   } else {
       /* free any current contents */
       ROADMAP_LIST_FOR_EACH(&path_list->itemlist, elem, tmp) {
          free (((RoadMapPathItem *)elem)->path);
          free (roadmap_list_remove(elem));
       }
   }

   /* Extract and expand each item of the path.
    */
   for (item = path; item != NULL; item = next_item) {

      char *p;
      next_item = strchr (item, ',');

      if (next_item == NULL) {
         p = roadmap_path_expand (item, strlen(item));
      } else {
         p = roadmap_path_expand (item, (size_t)(next_item - item));
      }

      roadmap_path_addlist(&path_list->itemlist, p);

      if (next_item) {
         next_item++;
         if (!*next_item) next_item = NULL;
      }
   }
}

/**
 * @brief look up the path list, return the first element of that list
 * @param name the keyword used to look up the list
 * @return first element
 */
const char *roadmap_path_first (const char *name) {

   RoadMapPathList path_list = roadmap_path_find (name, 1);
   RoadMapPathItem *pi;

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   if ( ! ROADMAP_LIST_EMPTY(&path_list->itemlist)) {
      pi = (RoadMapPathItem *)ROADMAP_LIST_FIRST(&path_list->itemlist);
      return pi->path;
   }
   return NULL;
}

/**
 * @brief called after roadmap_path_first, return subsequent elements of the list
 * @param name keyword
 * @param current the previous path
 * @return the next one
 */
const char *roadmap_path_next  (const char *name, const char *current) {

   RoadMapPathList path_list = roadmap_path_find (name, 1);
   RoadMapPathItem *pi;
   RoadMapListItem *elem, *tmp;

   ROADMAP_LIST_FOR_EACH(&path_list->itemlist, elem, tmp) {
      if (((RoadMapPathItem *)elem)->path == current) {
          if (elem != ROADMAP_LIST_LAST(&path_list->itemlist)) {
             pi = (RoadMapPathItem *)ROADMAP_LIST_NEXT(elem);
             return pi->path;
          }
      }
   }

   return NULL;
}


const char *roadmap_path_last (const char *name) {

   RoadMapPathList path_list = roadmap_path_find (name, 1);
   RoadMapPathItem *pi;

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   if ( ! ROADMAP_LIST_EMPTY(&path_list->itemlist)) {
      pi = (RoadMapPathItem *)ROADMAP_LIST_LAST(&path_list->itemlist);
      return pi->path;
   }

   return NULL;
}


const char *roadmap_path_previous (const char *name, const char *current) {

   RoadMapPathList path_list = roadmap_path_find (name, 1);
   RoadMapPathItem *pi;
   RoadMapListItem *elem, *tmp;

   ROADMAP_LIST_FOR_EACH(&path_list->itemlist, elem, tmp) {
      if (((RoadMapPathItem *)elem)->path == current) {
          if (elem != ROADMAP_LIST_FIRST(&path_list->itemlist)) {
             pi = (RoadMapPathItem *)ROADMAP_LIST_PREV(elem);
             return pi->path;
          } else {
             return NULL;
          }
      }
   }
   return NULL;
}


/* This function always return a hardcoded default location,
 * which is the recommended location for these objects.
 */
const char *roadmap_path_preferred (const char *name) {

   RoadMapPathList path_list = roadmap_path_find (name, 1);

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   return path_list->preferred;
}


void roadmap_path_create (const char *path) {

   char command[256];

   snprintf (command, sizeof(command), "mkdir -p %s", path);
   system (command);
}


static char *RoadMapPathEmptyList = NULL;

/**
 * @brief create a list of files matching the specified path and extension
 * There is no wildcarding mechanism here, except all files in this directory with the
 * specified extension are a match.
 * @param path is the directory name in which to look for files
 * @param extension can be empty or NULL
 * @return a list of file names
 */
char **roadmap_path_list (const char *path, const char *extension) {

   char  *match;
   int    length;
   size_t count;
   char  **result;
   char  **cursor;

   DIR *directory;
   struct dirent *entry;


   directory = opendir (path);
   if (directory == NULL) return &RoadMapPathEmptyList;

   count = 0;
   while ((entry = readdir(directory)) != NULL) ++count;

   cursor = result = calloc (count+1, sizeof(char *));
   roadmap_check_allocated (result);

   rewinddir (directory);
   if (extension != NULL) {
      length = strlen(extension);
   } else {
      length = 0;
   }

   while ((entry = readdir(directory)) != NULL) {

      if (entry->d_name[0] == '.') continue;

      /* Don't allow file names ending in ~, they're likely to be backup files */
      int l = strlen(entry->d_name);
      if (entry->d_name[l-1] == '~')
	      continue;

      if (length > 0) {
         
         match = entry->d_name + strlen(entry->d_name) - length;

         if (! strcmp (match, extension)) {
            *(cursor++) = strdup (entry->d_name);
	 roadmap_log (ROADMAP_WARNING, "--> %s", entry->d_name);
         }
      } else {
         *(cursor++) = strdup (entry->d_name);
	 roadmap_log (ROADMAP_WARNING, "--> %s", entry->d_name);
      }
   }
   *cursor = NULL;
   closedir(directory);

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
   if (path != RoadMapPathCurrentDirectory) {
      free ((char *) path);
   }
}


const char *roadmap_path_search_icon (const char *name) {

   char result[256];
   const char *path;

   sprintf (result, "rm_%s.png", name);
   for (path = roadmap_scan ("icons", result);
       path != NULL;
       path = roadmap_scan_next ("icons", result, path)) {
        return roadmap_path_cat(path, result);
   }

   return NULL; /* Not found. */
}


int roadmap_path_is_full_path (const char *name) {
   return name[0] == '/';
}


int roadmap_path_is_directory (const char *name) {

   struct stat file_attributes;

   if (stat (name, &file_attributes) != 0) {
      return 0;
   }

   return S_ISDIR(file_attributes.st_mode);
}


const char *roadmap_path_skip_separator (const char *name) {

   if (*name == '/') {
      return name + 1;
   }
   return NULL; // Not a valid path separator.
}


const char *roadmap_path_temporary (void) {

   return "/var/tmp";
}

