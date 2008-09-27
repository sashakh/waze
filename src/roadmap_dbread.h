/* roadmap_dbread.h - General API for accessing a RoadMap map file.
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

#ifndef _ROADMAP_DBREAD__H_
#define _ROADMAP_DBREAD__H_

#include "roadmap_db.h"

struct roadmap_db_tree {
   struct roadmap_db_tree    *parent;
   struct roadmap_db_tree    *first;   /* .. child. */
   struct roadmap_db_tree    *last;    /* .. child. */
   struct roadmap_db_tree    *next;    /* .. sibling. */
   struct roadmap_db_section *head;
   int level;

   void *handler_context;
};

typedef struct roadmap_db_tree roadmap_db;


typedef void * (*roadmap_db_mapper)  (roadmap_db *root);
typedef void (*roadmap_db_activator) (void *context);
typedef void (*roadmap_db_unmapper)  (void *context);

typedef struct {

   char *name;

   roadmap_db_mapper    map;
   roadmap_db_activator activate;
   roadmap_db_unmapper  unmap;

} roadmap_db_handler;

typedef struct roadmap_db_model_s {

   char *section;

   roadmap_db_handler *handler;

   struct roadmap_db_model_s *next;
} roadmap_db_model;

roadmap_db_model *roadmap_db_register
     (roadmap_db_model *model, char *section, roadmap_db_handler *handler);


int  roadmap_db_open (char *name, roadmap_db_model *model, const char* mode);

void roadmap_db_activate (char *name);

roadmap_db *roadmap_db_get_subsection (roadmap_db *parent, char *path);

roadmap_db *roadmap_db_get_first (roadmap_db *parent);
char       *roadmap_db_get_name  (roadmap_db *section);
unsigned    roadmap_db_get_size  (roadmap_db *section);
int         roadmap_db_get_count (roadmap_db *section);
int         roadmap_db_get_level (roadmap_db *section);
char       *roadmap_db_get_data  (roadmap_db *section);
roadmap_db *roadmap_db_get_next  (roadmap_db *section);

void roadmap_db_sync (char *name);
void roadmap_db_close (char *name);
void roadmap_db_end   (void);

#endif // _ROADMAP_DBREAD__H_

