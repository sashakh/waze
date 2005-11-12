/* editor_db.h - database layer
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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

#ifndef INCLUDE__EDITOR_DB__H
#define INCLUDE__EDITOR_DB__H

#include "../roadmap_types.h"
#include "../roadmap_dbread.h"

#define EDITOR_MAX_POINTS 10000
#define EDITOR_MAX_SHAPES 100000
#define EDITOR_MAX_STREETS 500
#define EDITOR_MAX_LINES 5000
#define EDITOR_MAX_LINES_DEL 1000

#define DB_DEFAULT_INITIAL_BLOCKS 1000

typedef struct editor_db_section_s {
   int num_items;
   int max_blocks;
   int max_items;
   int item_size;
   int items_per_block;
   int blocks[1]; /* dynamic */
} editor_db_section;

typedef struct editor_db_header_s {
   int fips;
   RoadMapArea edges;
   int cfccs; /* Bitmap of cfccs we override */
   int block_size;
   int num_total_blocks;
   int num_used_blocks;
   int file_size;
   int current_trkseg;
} editor_db_header;

typedef void (*editor_item_init)  (void *item);

void *editor_map (roadmap_db *root);
void editor_unmap (void *context);

int editor_db_create (int fips);

int editor_db_activate (int fips);

void editor_db_mark_cfcc (int cfcc);
int editor_db_is_cfcc_marked (int cfcc);
int editor_db_add_item (editor_db_section *section, void *data);
int editor_db_get_item_count (editor_db_section *section);
void *editor_db_get_item (editor_db_section *section, int item_id, int create, editor_item_init init);
int editor_db_get_block_size (void);
void *editor_db_get_last_item (editor_db_section *section);
int editor_db_allocate_items (editor_db_section *section, int count);
int editor_db_insert_item (editor_db_section *section, void *data, int pos);
int editor_db_grow (void);
RoadMapArea *editor_db_get_active_edges(void);
void editor_db_update_current_trkseg (int trkseg);
int editor_db_get_current_trkseg (void);

#endif // INCLUDE__EDITOR_DB__H

