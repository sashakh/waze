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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <javax/microedition/io.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_file.h"


struct RoadMapFileContextStructure {

   FILE  *fd;
   void *base;
   int   size;
};


FILE *roadmap_file_fopen (const char *path,
                          const char *name,
                          const char *mode) {

   int   silent;
   FILE *file;
   const char *full_name = roadmap_path_join (path, name);

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
   roadmap_log (ROADMAP_DEBUG, "After fopen for: %s, file=%x", full_name, file);

   if ((file == NULL) && (! silent)) {
      roadmap_log (ROADMAP_ERROR, "cannot open file %s", full_name);
   }

   roadmap_path_free (full_name);
   return file;
}


void roadmap_file_remove (const char *path, const char *name) {

   assert("roadmap_file_remove");
   const char *full_name = roadmap_path_join (path, name);

   //remove(full_name);
   roadmap_path_free (full_name);
}

int roadmap_file_exists (const char *path, const char *name) {

   int   status;
   const char *full_name = roadmap_path_join (path, name);

   FILE *f = fopen (full_name, "r");

   status = (f != NULL);

   if (f) fclose(f);

   return status;
}

/*
int roadmap_file_length (const char *path, const char *name) {

   int   status;
   const char *full_name = roadmap_path_join (path, name);
   struct stat stat_buffer;

   status = stat (full_name, &stat_buffer);
   roadmap_path_free (full_name);

   if (status == 0) {
      return stat_buffer.st_size;
   }
   return -1;
}


void roadmap_file_save (const char *path, const char *name,
                        void *data, int length) {

   int   fd;
   const char *full_name = roadmap_path_join (path, name);

   fd = open (full_name, O_CREAT+O_WRONLY+O_TRUNC, 0666);
   roadmap_path_free (full_name);

   if (fd >= 0) {

      write (fd, data, length);
      close(fd);
   }
}


int roadmap_file_truncate (const char *path, const char *name,
                           int length) {

   int   res;
   const char *full_name = roadmap_path_join (path, name);

   res = truncate (full_name, length);
   roadmap_path_free (full_name);

   return res;
}


void roadmap_file_append (const char *path, const char *name,
                          void *data, int length) {

   int   fd;
   const char *full_name = roadmap_path_join (path, name);

   fd = open (full_name, O_CREAT+O_WRONLY+O_APPEND, 0666);
   roadmap_path_free (full_name);

   if (fd >= 0) {

      write (fd, data, length);
      close(fd);
   }
}


const char *roadmap_file_unique (const char *base) {

    static int   UniqueNameCounter = 0;
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
    
    sprintf (UniqueNameBuffer,
             "%s%d_%d", base, getpid(), UniqueNameCounter);

    UniqueNameCounter += 1;

    return UniqueNameBuffer;
}

*/

const char *roadmap_file_map (const char *set,
                              const char *name,
                              const char *sequence,
                              const char *mode,
                              RoadMapFileContext *file) {

   RoadMapFileContext context;

   context = malloc (sizeof(*context));
   roadmap_check_allocated(context);

   context->fd = NULL;
   context->base = NULL;
   context->size = 0;

   if (strcmp(mode, "r")) {
      roadmap_log (ROADMAP_ERROR,
                   "%s: invalid file access mode %s", name, mode);
      free (context);
      return NULL;
   }

   if (name[0] == '/') {

      context->fd = fopen (name, mode);
      sequence = ""; /* Whatever, but NULL. */

   } else {

      char *full_name;

      int full_name_size;
      int name_size = strlen(name);
      int size;


      if (sequence == NULL) {
         sequence = roadmap_path_first(set);
      } else {
         sequence = roadmap_path_next(set, sequence);
      }
      if (sequence == NULL) {
         free (context);
         return NULL;
      }

      full_name_size = 512;
      full_name = malloc (full_name_size);
      roadmap_check_allocated(full_name);

      do {
         size = strlen(sequence) + name_size + 2;

         if (size >= full_name_size) {
            full_name = realloc (full_name, size);
            roadmap_check_allocated(full_name);
            full_name_size = size;
         }

         strcpy (full_name, sequence);
         strcat (full_name, "/");
         strcat (full_name, name);

         if (!strncmp(full_name, "file:", 5)) {
            printf ("Trying to open: %s %x\n", full_name, (int)context->fd);
            context->fd = NOPH_Connector_openFILEInputStream(full_name);
            if (context->fd) {
               printf ("Open ok!, size: %d\n", favail(context->fd));
            }
         } else {
            context->fd = fopen (full_name, mode);
         }

         if (context->fd > 0) break;

         sequence = roadmap_path_next(set, sequence);

      } while (sequence != NULL);

      free (full_name);
   }

   if (context->fd <= 0) {
      if (sequence == 0) {
         roadmap_log (ROADMAP_INFO, "cannot open file %s", name);
      }
      roadmap_file_unmap (&context);
      return NULL;
   }

   if ((context->size = favail(context->fd)) <= 0) {
      roadmap_file_unmap (&context);
      return NULL;
   }

   context->base = malloc(context->size);

   if ((context->base == NULL) ||
       (fread(context->base, context->size, 1, context->fd) != context->size)) {
      roadmap_log (ROADMAP_ERROR, "cannot map file %s", name);
      roadmap_file_unmap (&context);
      return NULL;
   }

   *file = context;

   return sequence; /* Indicate the next directory in the path. */
}


void *roadmap_file_base (RoadMapFileContext file){

   if (file == NULL) {
      return NULL;
   }
   return file->base;
}


int roadmap_file_size (RoadMapFileContext file){

   if (file == NULL) {
      return 0;
   }
   return file->size;
}


int roadmap_file_sync (RoadMapFileContext file) {

   return 0;
}


void roadmap_file_unmap (RoadMapFileContext *file) {

   RoadMapFileContext context = *file;

   if (context->base != NULL) {
      free(context->base);
      context->base = NULL;
   }

   if (context->fd > 0) {
      fclose (context->fd);
   }
   free(context);
   *file = NULL;
}


RoadMapFile roadmap_file_open  (const char *name, const char *mode) {

   return -1;
}


int roadmap_file_read  (RoadMapFile file, void *data, int size) {
   return -1;
}

int roadmap_file_write (RoadMapFile file, const void *data, int length) {
   return -1;
}

void  roadmap_file_close (RoadMapFile file) {
}

int roadmap_file_free_space (const char *path) {
   return -1;
}

