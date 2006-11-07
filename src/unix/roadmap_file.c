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
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_file.h"


struct RoadMapFileContextStructure {

   int   fd;
   void *base;
   int   size;
   char *name;
};


FILE *roadmap_file_fopen (const char *path,
                          const char *name,
                          const char *mode) {

   int   silent;
   FILE *file;
   struct stat stat_buffer;
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
   if (file != NULL && fstat (fileno(file), &stat_buffer) == 0) {
      /* read-only opens will succeed on directories */
      if (!S_ISREG(stat_buffer.st_mode)) {
         fclose(file);
         file = NULL;
      }
   }

   if ((file == NULL) && (! silent)) {
      roadmap_log (ROADMAP_ERROR, "cannot open file %s", full_name);
   }

   roadmap_path_free (full_name);
   return file;
}


void roadmap_file_remove (const char *path, const char *name) {

   const char *full_name = roadmap_path_join (path, name);

   remove(full_name);
   roadmap_path_free (full_name);
}


int roadmap_file_exists (const char *path, const char *name) {

   int   status;
   const char *full_name = roadmap_path_join (path, name);
   struct stat stat_buffer;

   status = stat (full_name, &stat_buffer);

   roadmap_path_free (full_name);

   return (status == 0);
}


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

      write (fd, data, (size_t)length);
      close(fd);
   }
}


int roadmap_file_truncate (const char *path, const char *name, int length) {

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

      write (fd, data, (size_t)length);
      close(fd);
   }
}

int roadmap_file_rename (const char *path, const char *name, const char *new) {

   const char *full_name, *full_new;
   int ret = 1;

   full_name = roadmap_path_join (path, name);
   full_new = roadmap_path_join (path, new);

   if (rename(full_name, full_new)) ret = 0;

   roadmap_path_free (full_name);
   roadmap_path_free (full_new);

   return ret;
}

/*
 * Make backup file: append "~"
 */
void roadmap_file_backup(const char *path, const char *name) {

    int len;
    char *newname;
    
    len = strlen(name);
    newname = malloc(len + 2);  /* '~' + '\0' */

    roadmap_check_allocated(newname);

    strcpy(newname, name);
    strcpy(&newname[len], "~");

    roadmap_file_rename(path, name, newname);

    free(newname);
}

const char *roadmap_file_unique (const char *base) {

    static int    UniqueNameCounter = 0;
    static char  *UniqueNameBuffer = NULL;
    static size_t UniqueNameBufferLength = 0;

    size_t length;
    
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


const char *roadmap_file_map (const char *path,
                              const char *name,
                              const char *mode,
                              RoadMapFileContext *file) {

   RoadMapFileContext context;

   struct stat stat_result;
   int open_mode;
   int map_mode;
   int map_flags;


   context = malloc (sizeof(*context));
   roadmap_check_allocated(context);

   context->fd = -1;
   context->base = NULL;
   context->size = 0;

   if (strcmp(mode, "r") == 0) {
      open_mode = O_RDONLY;
      map_mode = PROT_READ;
      map_flags = MAP_PRIVATE;
   } else if (strchr (mode, 'w') != NULL) {
      open_mode = O_RDWR;
      map_mode = PROT_READ|PROT_WRITE;
      map_flags = MAP_SHARED;
   } else {
      roadmap_log (ROADMAP_ERROR,
                   "invalid file access mode %s for file %s", mode, name);
      return NULL;
   }

   if (name[0] == '/') {
      context->name = strdup(name);
   } else {
      context->name = roadmap_path_join (path, name);
   }

   context->fd = open (context->name, open_mode, 0666);

   if (context->fd < 0) {
      roadmap_log (ROADMAP_INFO, "cannot open file %s", context->name);
      roadmap_file_unmap (&context);
      return NULL;
   }

   if (fstat (context->fd, &stat_result) != 0) {
      roadmap_log (ROADMAP_ERROR, "cannot stat file %s", context->name);
      roadmap_file_unmap (&context);
      return NULL;
   }
   context->size = stat_result.st_size;

   context->base = mmap (NULL,
                         (size_t)stat_result.st_size,
                         map_mode,
                         map_flags,
                         context->fd,
                         0);

   if (context->base == MAP_FAILED) {
      roadmap_log (ROADMAP_ERROR, "cannot map file %s", context->name);
      roadmap_file_unmap (&context);
      return NULL;
   }

   *file = context;

   return context->base;
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


void roadmap_file_unmap (RoadMapFileContext *file) {

   RoadMapFileContext context = *file;

   if (context->base != NULL) {
      munmap (context->base, (size_t)context->size);
   }

   if (context->fd >= 0) {
      close (context->fd);
   }
   free(context->name);
   free(context);
   *file = NULL;
}


RoadMapFile roadmap_file_open  (const char *name, const char *mode) {

   int unix_mode = 0;

   if (strcmp(mode, "r") == 0) {
      unix_mode = O_RDONLY;
   } else if (strchr (mode, 'w') != NULL) {
      unix_mode = O_RDWR;
   } else {
      roadmap_log (ROADMAP_ERROR,
                   "%s: invalid file access mode %s", name, mode);
      return -1;
   }

   return (RoadMapFile) open (name, unix_mode);
}


int roadmap_file_read  (RoadMapFile file, void *data, int size) {
   int count = read ((int)file, data, (size_t)size);
   if (count <= 0) return -1;
   return count;
}

int roadmap_file_write (RoadMapFile file, const void *data, int length) {
   return write ((int)file, data, (size_t)length);
}

void  roadmap_file_close (RoadMapFile file) {
   close ((int)file);
}

