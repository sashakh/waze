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
 */

#ifndef INCLUDE__ROADMAP_FILE__H
#define INCLUDE__ROADMAP_FILE__H

#include <stdio.h>

struct RoadMapFileContextStructure;
typedef struct RoadMapFileContextStructure *RoadMapFileContext;

void roadmap_file_set_path  (const char *path);

const char *roadmap_file_path_first (void);
const char *roadmap_file_path_next  (const char *current);

const char *roadmap_file_path_last     (void);
const char *roadmap_file_path_previous (const char *current);

const char *roadmap_file_unique       (const char *base);

const char *roadmap_file_user         (void);
const char *roadmap_file_trips        (void);
const char *roadmap_file_default_path (void);

char *roadmap_file_join (const char *path, const char *name);
FILE *roadmap_file_open (const char *path, const char *name, const char *mode);
void  roadmap_file_remove (const char *path, const char *name);

void  roadmap_file_save (const char *path, const char *name,
                         void *data, int length);

int  roadmap_file_map   (const char *name,
                         int sequence, RoadMapFileContext *file);
void roadmap_file_unmap (RoadMapFileContext *file);

void *roadmap_file_base (RoadMapFileContext file);
int   roadmap_file_size (RoadMapFileContext file);

const char *roadmap_file_parent_directory (const char *path, const char *name);
void        roadmap_file_create_directory (const char *path);
void        roadmap_file_release (const char *path);

#endif // INCLUDE__ROADMAP_FILE__H

