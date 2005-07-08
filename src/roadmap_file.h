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
 */

#ifndef INCLUDE__ROADMAP_FILE__H
#define INCLUDE__ROADMAP_FILE__H


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

void  roadmap_file_save (const char *path, const char *name,
                         void *data, int length);

void roadmap_file_append (const char *path, const char *name,
                          void *data, int length);


FILE *roadmap_file_fopen (const char *path, const char *name, const char *mode);


/* The following file operations hide the OS file mapping primitives. */

struct RoadMapFileContextStructure;
typedef struct RoadMapFileContextStructure *RoadMapFileContext;

const char *roadmap_file_map (const char *set,
                              const char *name,
                              const char *sequence, RoadMapFileContext *file);

void *roadmap_file_base (RoadMapFileContext file);
int   roadmap_file_size (RoadMapFileContext file);

void roadmap_file_unmap (RoadMapFileContext *file);

const char *roadmap_file_unique (const char *base);

#endif // INCLUDE__ROADMAP_FILE__H

