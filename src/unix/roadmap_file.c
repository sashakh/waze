/* roadmap_file.c - a module to open/read/close a roadmap database file.
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
 *   See roadmap_file.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "roadmap.h"
#include "roadmap_file.h"

#define ROADMAP_PATH_COUNT_MAX 16

struct RoadMapFileContextStructure {

   int   fd;
   void *base;
   int   size;

};

static char *RoadMapFilePathString = NULL;
static char *RoadMapFilePath[ROADMAP_PATH_COUNT_MAX] = {NULL};
static int   RoadMapFilePathCount = 0;
static int   RoadMapFileMaxPath = 0;


static const char *roadmap_file_home (void) {

   static char *RoadMapFileHome = NULL;

   if (RoadMapFileHome == NULL) {

      RoadMapFileHome = getenv("HOME");

      if (RoadMapFileHome == NULL) {
         RoadMapFileHome = "";
      }
   }

   return RoadMapFileHome;
}


static char *roadmap_file_cat (const char *s1, const char *s2) {
    
    char *result = malloc (strlen(s1) + strlen(s2) + 4);

    roadmap_check_allocated (result);

    strcpy (result, s1);
    strcat (result, "/");
    strcat (result, s2);
    
    return result;
}


const char *roadmap_file_default_path (void) {

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


const char *roadmap_file_user (void) {

    static char  RoadMapDirectory[] = ".roadmap";
    static char *RoadMapUser = NULL;

    if (RoadMapUser == NULL) {
        RoadMapUser = roadmap_file_cat (roadmap_file_home(), RoadMapDirectory);
        mkdir (RoadMapUser, 0770);
    }

    return RoadMapUser;
}


const char *roadmap_file_trips (void) {
    
    static char  RoadMapDefaultTrips[] = ".roadmap/trips";
    static char *RoadMapTrips = NULL;
    
    if (RoadMapTrips == NULL) {
        
        RoadMapTrips = getenv("ROADMAP_TRIPS");
        
        if (RoadMapTrips == NULL) {
            RoadMapTrips = roadmap_file_cat (roadmap_file_home(), RoadMapDefaultTrips);
        }
        
        mkdir (RoadMapTrips, 0770);
    }
    
    return RoadMapTrips;
}
            

void roadmap_file_set_path (const char *path) {

   int i;
   int length;
   char *p;


   while (*path == ',') path += 1;
   if (*path == 0) return; /* Ignore empty path: default is better. */


   if (RoadMapFilePathString != NULL) {

      free (RoadMapFilePathString);
      RoadMapFilePathString = NULL;
   }
   for (i = 0; i < ROADMAP_PATH_COUNT_MAX; ++i) {
      RoadMapFilePath[i] = NULL;
   }
   RoadMapFilePathCount = 0;


   RoadMapFilePathString = strdup (path);

   RoadMapFilePath[0] = RoadMapFilePathString;
   i = 0;

   for (p = strchr(RoadMapFilePathString, ',');
        p != NULL;
        p = strchr (p, ',')) {

      *(p++) = 0;
      if (*p == 0) break;

      if (*p != ',') {
         RoadMapFilePath[++i] = p;
         if (i >= ROADMAP_PATH_COUNT_MAX - 1) break;
      }
   }
   RoadMapFilePathCount = ++i;

   RoadMapFileMaxPath = 0;

   while (--i >= 0) {
      length = strlen(RoadMapFilePath[i]);
      if (length > RoadMapFileMaxPath) {
         RoadMapFileMaxPath = length;
      }
   }
   length = strlen(roadmap_file_user());
   if (length > RoadMapFileMaxPath) {
      RoadMapFileMaxPath = length;
   }
}


const char *roadmap_file_path_first (void) {

   if (RoadMapFilePathString == NULL) {
      roadmap_file_set_path (roadmap_file_default_path());
   }

   return RoadMapFilePath[0];
}


const char *roadmap_file_path_next  (const char *current) {

   int i;

   for (i = 0; RoadMapFilePath[i] != NULL; ++i) {

      if (RoadMapFilePath[i] == current) {
         return RoadMapFilePath[i+1];
      }
   }

   return NULL;
}


const char *roadmap_file_path_last (void) {

   if (RoadMapFilePathString == NULL) {
      roadmap_file_set_path (roadmap_file_default_path());
   }

   if (RoadMapFilePathCount > 0) {
      return RoadMapFilePath[RoadMapFilePathCount-1];
   }
   return NULL;
}


const char *roadmap_file_path_previous (const char *current) {

   int i;

   for (i = 1; RoadMapFilePath[i] != NULL; ++i) {

      if (RoadMapFilePath[i] == current) {
         return RoadMapFilePath[i-1];
      }
   }
   return NULL;
}


char *roadmap_file_join (const char *path, const char *name) {

   if (path == NULL) {
      return strdup (name);
   }
   return roadmap_file_cat (path, name);
}


FILE *roadmap_file_open (const char *path, const char *name, const char *mode) {

   int   silent;
   FILE *file;
   char *full_name = roadmap_file_join (path, name);

   if (mode[0] == 's') {
      /* This special mode is a "lenient" read: do not complain
       * if the file does not exist.
       */
      silent = 1;
      ++mode;
   } else {
      silent = 0;
   }

   file = fopen (full_name, mode);

   if ((file == NULL) && (! silent)) {
      roadmap_log (ROADMAP_ERROR, "cannot open file %s", full_name);
   }

   free (full_name);
   return file;
}


void roadmap_file_remove (const char *path, const char *name) {

   char *full_name = roadmap_file_join (path, name);

   remove(full_name);
   free (full_name);
}


void roadmap_file_save (const char *path, const char *name,
                        void *data, int length) {

   int   fd;
   char *full_name = roadmap_file_join (path, name);

   fd = open (full_name, O_CREAT+O_WRONLY, 0666);
   free (full_name);

   if (fd >= 0) {

      write (fd, data, length);
      close(fd);
   }
}


const char *roadmap_file_unique (const char *base) {
    
    static char *UniqueNameBuffer = NULL;
    static int   UniqueNameBufferLength = 0;

    int length;
    
    length = strlen(base + 16);
    
    if (length > UniqueNameBufferLength) {

        if (UniqueNameBuffer != NULL) {
            free(UniqueNameBuffer);
        }
        UniqueNameBuffer = malloc (length);
        
        roadmap_check_allocated(UniqueNameBuffer);
        
        UniqueNameBufferLength = length;
    }
    
    sprintf (UniqueNameBuffer, "%s%d", base, getpid());
    
    return UniqueNameBuffer;
}


int roadmap_file_map (const char *name,
                      int sequence, RoadMapFileContext *file) {

   int i;
   char *path;
   char *full_name;

   RoadMapFileContext context;

   struct stat state_result;


   if (RoadMapFilePathString == NULL) {
      roadmap_file_set_path (roadmap_file_default_path());
   }

   if (sequence >= RoadMapFilePathCount) {
      return -1;
   }

   full_name = malloc ((2 * RoadMapFileMaxPath) + strlen(name) + 4);
   roadmap_check_allocated(full_name);

   context = malloc (sizeof(*context));
   roadmap_check_allocated(context);

   context->fd = -1;
   context->base = NULL;
   context->size = 0;

   if (name[0] == '/') {

      i = sequence;
      context->fd = open (name, O_RDONLY, 0666);

   } else {

      for (i = sequence; RoadMapFilePath[i] != NULL; ++i) {

         path = RoadMapFilePath[i];

         if (path[0] == '~') {

            strcpy (full_name, roadmap_file_home());
            strcat (full_name, path+1);

         } else if (path[0] == '&') {

            strcpy (full_name, roadmap_file_user());
            strcat (full_name, path+1);

         } else {
            strcpy (full_name, path);
         }

         strcat (full_name, "/");
         strcat (full_name, name);

         context->fd = open (full_name, O_RDONLY, 0666);

         if (context->fd >= 0) break;
      }
      free (full_name);
   }

   if (context->fd < 0) {
      if (sequence == 0) {
         roadmap_log (ROADMAP_INFO, "cannot open file %s", name);
      }
      return -1;
   }

   if (fstat (context->fd, &state_result) != 0) {
      if (sequence == 0) {
         roadmap_log (ROADMAP_ERROR, "cannot stat file %s", name);
      }
      roadmap_file_unmap (file);
      return -1;
   }
   context->size = state_result.st_size;

   context->base =
      mmap (NULL, state_result.st_size, PROT_READ, MAP_PRIVATE, context->fd, 0);

   if (context->base == NULL) {
      roadmap_log (ROADMAP_ERROR, "cannot map file %s", name);
      roadmap_file_unmap (file);
      return -1;
   }

   *file = context;

   return i + 1; /* Indicate the next directory in the path. */
}


void *roadmap_file_base (RoadMapFileContext file){

   if (file == NULL) {
      return NULL;
   }
   return file->base;
}


int   roadmap_file_size (RoadMapFileContext file){

   if (file == NULL) {
      return 0;
   }
   return file->size;
}


void roadmap_file_unmap (RoadMapFileContext *file) {

   RoadMapFileContext context = *file;

   if (context->base != NULL) {
      munmap (context->base, context->size);
   }

   if (context->fd >= 0) {
      close (context->fd);
   }
   free(context);
   *file = NULL;
}


const char *roadmap_file_parent_directory (const char *path, const char *name) {

   char *separator;
   char *full_name = roadmap_file_join (path, name);

   separator = strrchr (full_name, '/');
   if (separator == NULL) {
      return ".";
   }

   *separator = 0;

   return full_name;
}


void roadmap_file_create_directory (const char *path) {

   char command[256];

   snprintf (command, sizeof(command), "mkdir -p %s", path);
   system (command);
}


void roadmap_file_release (const char *path) {
   free ((void *)path);
}

