/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2009 Danny Backx.
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

/*
 * buildmap.h - General definition for the map builder tool.
 */

#ifndef INCLUDED__ROADMAP_BUILDMAP__H
#define INCLUDED__ROADMAP_BUILDMAP__H

#include "roadmap_types.h"
#include "roadmap_db.h"

#define BUILDMAP_BLOCK  4096 /* memory allocation block. */


void buildmap_set_source (const char *name);
void buildmap_set_line   (int line);

void buildmap_fatal    (int column, const char *format, ...);
void buildmap_error    (int column, const char *format, ...);
void buildmap_progress (int done, int estimated);
void buildmap_info     (const char *format, ...);
void buildmap_summary  (int verbose, const char *format, ...);
void buildmap_verbose  (const char *format, ...);
int  buildmap_is_verbose (void);
void buildmap_message_adjust_level (int level);

int buildmap_get_error_count (void);
int buildmap_get_error_total (void);

void buildmap_check_allocated_with_source_line
                (char *source, int line, const void *allocated);
#define buildmap_check_allocated(p) \
            buildmap_check_allocated_with_source_line(__FILE__,__LINE__,p)


typedef struct dictionary_volume *BuildMapDictionary;

BuildMapDictionary buildmap_dictionary_open (char *name);

RoadMapString buildmap_dictionary_add
                (BuildMapDictionary dictionary, const char *string, int length);

RoadMapString buildmap_dictionary_locate
                (BuildMapDictionary dictionary, const char *string);
char *buildmap_dictionary_get
         (BuildMapDictionary dictionary, RoadMapString index);


struct buildmap_db_section {
   struct buildmap_db_section *parent;
   struct buildmap_db_section *first;   /* .. child. */
   struct buildmap_db_section *last;    /* .. child. */
   struct buildmap_db_section *next;    /* .. sibling. */
   struct roadmap_db_section  *head;
   int head_offset;
   int data;
};

typedef struct buildmap_db_section buildmap_db;


typedef void (*buildmap_db_action) (void);

typedef struct {

   const char        *name;
   buildmap_db_action sort;
   buildmap_db_action save;
   buildmap_db_action summary;
   buildmap_db_action reset;

} buildmap_db_module;


int buildmap_db_open (const char *path, const char *name);

void buildmap_db_register (const buildmap_db_module *module);

buildmap_db *buildmap_db_add_section (buildmap_db *parent, const char *name);
int          buildmap_db_add_data (buildmap_db *section, int count, int size);
void        *buildmap_db_get_data (buildmap_db *section);

buildmap_db *buildmap_db_add_child (buildmap_db *parent,
                                    char *name,
                                    int count,
                                    int size);

/* The functions that call the registered actions: */
void buildmap_db_sort    (void);
void buildmap_db_save    (void);
void buildmap_db_summary (void);
void buildmap_db_reset   (void);

void buildmap_db_close (void);

#endif // INCLUDED__ROADMAP_BUILDMAP__H

