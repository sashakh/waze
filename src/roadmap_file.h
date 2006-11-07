/* roadmap_file.h - a module to open/read/close a roadmap database file.
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
 *
 * DESCRIPTION:
 *
 *   The roadmap_file_unique() function returns a unique file name.
 *   The name is built so that two processes generate different names
 *   and the same process never generates the same name twice.
 *
 *   The functions roadmap_file_open, roadmap_file_read, roadmap_file_write
 *   and roadmap_file_close do what one would assume from their names.
 *   Please note however that roadmap_file_read must return an error when
 *   trying to read beyond the end of the file.
 *
 *   The function roadmap_file_map() maps the given file into memory and
 *   returns the base address of the mapping, or NULL if the mapping failed.
 *
 *   Any function that takes a path and name parameter follows the
 *   conventions as set for roadmap_path_join(), i.e. the path parameter
 *   is ignored if either NULL or empty. In addition, roadmap_file_map()
 *   ignores the path parameter if the name is a full file name.
 */

#ifndef INCLUDED__ROADMAP_FILE__H
#define INCLUDED__ROADMAP_FILE__H


#ifdef _WIN32

#include <windows.h>

typedef HANDLE RoadMapFile; /* WIN32 style. */
#define ROADMAP_FILE_IS_VALID(f) (f != INVALID_HANDLE_VALUE)

#else

#include <stdio.h>

typedef int RoadMapFile; /* UNIX style. */
#define ROADMAP_FILE_IS_VALID(f) (f != (RoadMapFile)-1)

#endif


RoadMapFile roadmap_file_open  (const char *name, const char *mode);
int   roadmap_file_read  (RoadMapFile file, void *data, int size);
int   roadmap_file_write (RoadMapFile file, const void *data, int length);
void  roadmap_file_close (RoadMapFile file);

void  roadmap_file_remove (const char *path, const char *name);
int   roadmap_file_exists (const char *path, const char *name);
int   roadmap_file_length (const char *path, const char *name);
int   roadmap_file_rename (const char *path, const char *name, const char *newname);
void  roadmap_file_backup (const char *path, const char *name);

void  roadmap_file_save (const char *path, const char *name,
                         void *data, int length);

void roadmap_file_append (const char *path, const char *name,
                          void *data, int length);

int roadmap_file_truncate (const char *path, const char *name, int length);


FILE *roadmap_file_fopen (const char *path, const char *name, const char *mode);


/* The following file operations hide the OS file mapping primitives. */

struct RoadMapFileContextStructure;
typedef struct RoadMapFileContextStructure *RoadMapFileContext;

const char *roadmap_file_map (const char *path,
                              const char *name,
                              const char *mode,
                              RoadMapFileContext *file);

void *roadmap_file_base (RoadMapFileContext file);
int   roadmap_file_size (RoadMapFileContext file);

void roadmap_file_unmap (RoadMapFileContext *file);

const char *roadmap_file_unique (const char *base);

#endif // INCLUDED__ROADMAP_FILE__H

