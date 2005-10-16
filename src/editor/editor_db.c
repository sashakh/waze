/* editor_db.c - databse layer
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
 *
 * SYNOPSYS:
 *
 *   See editor_db.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../roadmap.h"
#include "../roadmap_file.h"
#include "../roadmap_path.h"
#include "../roadmap_county.h"
#include "../roadmap_line.h"
#include "../roadmap_locator.h"
#include "../buildmap.h"

#include "editor_db.h"
#include "editor_point.h"
#include "editor_shape.h"
#include "editor_line.h"
#include "editor_square.h"
#include "editor_street.h"
#include "editor_dictionary.h"
#include "editor_route.h"
#include "editor_override.h"
#include "editor_trkseg.h"
#include "editor_log.h"

/* Use the biggest item size to set the default block size */
#define DB_DEFAULT_BLOCK_SIZE (2 * sizeof (editor_db_square))

/* TODO create a generic cache system - this was copied from roadmap_locator */

#define EDITOR_CACHE_SIZE 10

struct editor_cache_entry {

   int          fips;
   unsigned int last_access;
};

static struct editor_cache_entry *EditorCache = NULL;
static int EditorCacheSize = 0;
static int EditorActiveCounty;
static int EditorNoCounty = -1;
static roadmap_db_model *EditorDBModel;

static editor_db_header *ActiveDBHeader;
static char *ActiveBlocks;

static void editor_header_activate (void *context) {
   ActiveDBHeader = (editor_db_header *) context;
}

static void editor_blocks_activate (void *context) {
   ActiveBlocks = (char *) context;
}

roadmap_db_handler EditorHeaderHandler = {
   "header",
   editor_map,
   editor_header_activate,
   editor_unmap
};

roadmap_db_handler EditorBlocksHandler = {
   "data_blocks",
   editor_map,
   editor_blocks_activate,
   editor_unmap
};


static void editor_db_configure (void) {

   if (EditorCache == NULL) {

      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "data_blocks", &EditorBlocksHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "override", &EditorOverrideHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "route", &EditorRouteHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "trkseg", &EditorTrksegHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "squares", &EditorSquaresHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "lines", &EditorLinesHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "ranges", &EditorRangeHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "streets", &EditorStreetHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "shape", &EditorShapeHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "points_del", &EditorPointsDelHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "points", &EditorPointsHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "header", &EditorHeaderHandler);
      EditorDBModel =
         roadmap_db_register
            (EditorDBModel, "strings", &EditorDictionaryHandler);

      EditorCacheSize = roadmap_option_cache ();
      if (EditorCacheSize < EDITOR_CACHE_SIZE) {
         EditorCacheSize = EDITOR_CACHE_SIZE;
      }
      EditorCache = (struct editor_cache_entry *)
         calloc (EditorCacheSize, sizeof(struct editor_cache_entry));
      roadmap_check_allocated (EditorCache);
   }
}

static void editor_db_remove (int index) {

   char map_name[64];

   snprintf (map_name, sizeof(map_name),
             "edt%05d", EditorCache[index].fips);
   roadmap_db_close (map_name);

   EditorCache[index].fips = 0;
   EditorCache[index].last_access = 0;
}

static unsigned int editor_db_new_access (void) {

   static unsigned int EditorDBAccessCount = 0;

   int i;


   EditorDBAccessCount += 1;

   if (EditorDBAccessCount == 0) { /* Rollover. */

      for (i = 0; i < EditorCacheSize; i++) {

         if (EditorCache[i].fips > 0) {
            editor_db_remove (i);
         }
      }
      EditorActiveCounty = 0;

      EditorDBAccessCount += 1;
   }

   return EditorDBAccessCount;
}


static int editor_db_allocate_new_block
                     (editor_db_section *section, int block_id) {
   
   if (section->max_blocks == block_id) return -1;

   if (ActiveDBHeader->num_used_blocks == ActiveDBHeader->num_total_blocks) {
      return -1;
   }

   section->blocks[block_id] = ActiveDBHeader->num_used_blocks++;

   return 0;
}


buildmap_db *add_db_section
         (buildmap_db *parent, const char *name, int item_size, int max_items) {

   buildmap_db *root;
   editor_db_section *section;
   int max_blocks;
   int i;

   max_blocks = (item_size * max_items)/DB_DEFAULT_BLOCK_SIZE + 1;

   root = buildmap_db_add_section (parent, name);
   buildmap_db_add_data (root, 1, sizeof(editor_db_section));
   
   /* we already got one block allocated in editor_db_section */
   buildmap_db_add_data (root, max_blocks-1, sizeof(int));

   section = (editor_db_section *) buildmap_db_get_data (root);

   section->num_items = 0;
   section->max_items = max_items;
   section->max_blocks = max_blocks;
   section->item_size = item_size;
   section->items_per_block = DB_DEFAULT_BLOCK_SIZE / item_size;

   for (i=0; i < section->max_blocks; i++) {
      section->blocks[i] = -1;
   }

   return root;
}


static void add_db_string_section (buildmap_db *parent, const char *name) {

   buildmap_db *root;

   root = buildmap_db_add_section (parent, name);

   add_db_section
      (root, "references", sizeof(struct ed_dictionary_reference),
       DICTIONARY_INDEX_SIZE * 10);

   add_db_section
      (root, "trees", sizeof(struct ed_dictionary_tree),
       DICTIONARY_INDEX_SIZE);

   add_db_section
      (root, "data", 1, DICTIONARY_DATA_SIZE);
}


int editor_db_create (int fips) {

   char name[100];
   buildmap_db *root;
   editor_db_header *header;
   int square_count;
   const RoadMapArea *edges;

   editor_log_push ("editor_db_create");

   if (roadmap_locator_activate (fips) != ROADMAP_US_OK) {
      editor_log (ROADMAP_ERROR, "Can't activate RoadMap fips: %d", fips);
      editor_log_pop ();
      return -1;
   }

   edges = roadmap_county_get_edges (fips);

   if (edges == NULL) {
      editor_log (ROADMAP_ERROR, "Can't get edges of fips: %d", fips);
      editor_log_pop ();
      return -1;
   }

   snprintf (name, sizeof(name), "edt%05d", fips);
   
   if (buildmap_db_open (roadmap_path_user(), name) == -1) {
      editor_log (ROADMAP_ERROR, "Can't create new database: %s/%s",
            roadmap_path_user(), name);
      editor_log_pop ();
      return -1;
   }


   root = buildmap_db_add_section (NULL, "header");
   buildmap_db_add_data (root, 1, sizeof(editor_db_header));
   header = (editor_db_header *) buildmap_db_get_data (root);

   memset (header, 0, sizeof (editor_db_header));

   header->fips = fips;
   header->edges = *edges;
   header->block_size = DB_DEFAULT_BLOCK_SIZE;
   header->num_total_blocks = DB_DEFAULT_INITIAL_BLOCKS;

   add_db_section (NULL, "points", sizeof(editor_db_point), EDITOR_MAX_POINTS);
   add_db_section (NULL, "points_del", sizeof(editor_db_del_point), EDITOR_MAX_POINTS);
   add_db_section (NULL, "shape", sizeof(editor_db_shape), EDITOR_MAX_SHAPES);
   add_db_section (NULL, "lines", sizeof(editor_db_line), EDITOR_MAX_LINES);
   square_count = ((edges->east - edges->west) / EDITOR_DB_LONGITUDE_STEP + 1) *
                  ((edges->north - edges->south) / EDITOR_DB_LATITUDE_STEP + 1);

   add_db_section (NULL, "squares", sizeof(editor_db_square), square_count);
   add_db_section
      (NULL, "streets", sizeof(editor_db_street), EDITOR_MAX_STREETS);
   add_db_section (NULL, "ranges", sizeof(editor_db_range), EDITOR_MAX_LINES*2);

   add_db_section
      (NULL, "trkseg", sizeof (editor_db_trkseg), EDITOR_MAX_LINES*2);

   add_db_section
      (NULL, "route", sizeof (editor_db_route_segment), EDITOR_MAX_LINES*2);

   root = buildmap_db_add_section (NULL, "override");
   add_db_section (root, "index", sizeof(int), roadmap_line_count ());
   add_db_section (root, "data", sizeof(editor_db_override),
         EDITOR_MAX_LINES);

   root = buildmap_db_add_section (NULL, "strings");
   add_db_string_section (root, "streets");
   add_db_string_section (root, "cities");
   add_db_string_section (root, "types");
   add_db_string_section (root, "zips");
   add_db_string_section (root, "t2s");

   root = buildmap_db_add_section (NULL, "data_blocks");

   header->file_size =
      buildmap_db_add_data (root, DB_DEFAULT_INITIAL_BLOCKS,
         DB_DEFAULT_BLOCK_SIZE);

   buildmap_db_close ();
   
   if (EditorNoCounty == fips) {
      EditorNoCounty = -1;
   }

   editor_log_pop ();
   return 0;
}


static int editor_db_open (int fips) {

   int i;
   int access;
   int oldest = 0;

   char map_name[64];


   if (fips <= 0) {
      return -1;
   }
   snprintf (map_name, sizeof(map_name), "%s/edt%05d",
         roadmap_path_user(), fips);

   /* Look for the oldest entry in the cache. */

   for (i = EditorCacheSize-1; i >= 0; --i) {

      if (EditorCache[i].fips == fips) {

         roadmap_db_activate (map_name);
         EditorActiveCounty = fips;

         return 0;
      }

      if (EditorCache[i].last_access
             < EditorCache[oldest].last_access) {

         oldest = i;
      }
   }

   if (EditorCache[oldest].fips > 0) {
       editor_db_remove (oldest);
       if (EditorCache[oldest].fips == EditorActiveCounty) {
           EditorActiveCounty = 0;
       }
   }

   access = editor_db_new_access ();

   if (! roadmap_db_open (map_name, EditorDBModel, "rw")) {
      return -1;
   }

   EditorCache[oldest].fips = fips;
   EditorCache[oldest].last_access = access;

   EditorActiveCounty = fips;
   
   return 0;
}


int editor_db_activate (int fips) {

   int res;

   if (EditorActiveCounty == fips) {
       return 0;
   }

   if (EditorNoCounty == fips) {
      return -1;
   }

   editor_db_configure();

   res = editor_db_open (fips);

   if (res == -1) {
      EditorNoCounty = fips;
   }

   return res;
}


void editor_db_mark_cfcc (int cfcc) {
   assert ((unsigned)cfcc < 8*sizeof(cfcc));
   ActiveDBHeader->cfccs |= (1 << cfcc);
}


int editor_db_is_cfcc_marked (int cfcc) {
   assert ((unsigned)cfcc < 8*sizeof(cfcc));
   return ActiveDBHeader->cfccs & (1 << cfcc);
}


int editor_db_add_item (editor_db_section *section, void *data) {
   
   int block = section->num_items / section->items_per_block;
   int block_offset = section->num_items % section->items_per_block;
   char *item_addr;

   if ((section->num_items == 0) ||
         ((section->num_items % section->items_per_block) == 0)) {
      if (editor_db_allocate_new_block (section, block) == -1) {
         return -1;
      }
   }

   if (data != NULL) {
      item_addr = ActiveBlocks +
         section->blocks[block] * ActiveDBHeader->block_size +
         block_offset * section->item_size;

      memcpy (item_addr, data, section->item_size);
   }

   return section->num_items++;
}


int editor_db_insert_item (editor_db_section *section, void *data, int pos) {

   int i;

   assert ((pos >= 0) && (pos <= section->num_items));

   if (editor_db_add_item (section, data) == -1) {
      editor_db_grow ();
      if (editor_db_add_item (section, data) == -1) {
         return -1;
      }
   }

   if (pos == (section->num_items-1)) return 0;

   //TODO optimize with memove
   
   for (i=section->num_items-1; i>pos; i--) {

      memcpy (editor_db_get_item (section, i, 0, NULL),
              editor_db_get_item (section, i-1, 0, NULL),
              section->item_size);
   }

   memcpy (editor_db_get_item (section, pos, 0, NULL),
           data,
           section->item_size);

   return 0;
}


int editor_db_get_item_count (editor_db_section *section) {
   return section->num_items;
}


void *editor_db_get_item (editor_db_section *section,
                           int item_id, int create, editor_item_init init) {

   int block = item_id / section->items_per_block;
   int block_offset = item_id % section->items_per_block;

   assert (item_id < section->max_items);

   if (section->blocks[block] == -1) {
      
      if (!create) return NULL;
      
      if (editor_db_allocate_new_block (section, block) == -1) {
         return NULL;
      }
      
      if (init != NULL) {
         int i;
         char *addr = ActiveBlocks +
            section->blocks[block] * ActiveDBHeader->block_size;

         for (i=0; i<section->items_per_block; i++) {
            (*init) (addr + i * section->item_size);
         }
      }
   }

   return ActiveBlocks +
      section->blocks[block] * ActiveDBHeader->block_size +
      block_offset * section->item_size;
}


RoadMapArea *editor_db_get_active_edges(void) {
   return &ActiveDBHeader->edges;
}


int editor_db_get_block_size (void) {
   return ActiveDBHeader->block_size;
}


void *editor_db_get_last_item (editor_db_section *section) {
   
   int num_items = section->num_items;

   if (num_items == 0) return NULL;

   return editor_db_get_item (section, num_items-1, 0, NULL);
}


int editor_db_allocate_items (editor_db_section *section, int count) {

   int block = section->num_items / section->items_per_block;
   int block_offset = section->num_items % section->items_per_block;
   int first_item_id;

   if (count > section->items_per_block) return -1;

   if ((section->num_items == 0) || (block_offset == 0)) {
      if (editor_db_allocate_new_block (section, block) == -1) {
         return -1;
      }
   }

   if ((block_offset + count * section->item_size) >
         ActiveDBHeader->block_size) {
      block++;
      if (editor_db_allocate_new_block (section, block) == -1) {
         return -1;
      }
      section->num_items = block * section->items_per_block;
      block_offset = 0;
   }

   first_item_id = section->num_items;
   section->num_items += count;

   return first_item_id;
}


/* FIXME
 * Hack alert!
 * This should be rewritten so it uses the roadmap_db layer to grow.
 * Currently, it just grows the file and changes the blocks number in
 * the editor header section. The roadmap_db layer doesn't know about
 * the new size which is bad.
 */
int editor_db_grow (void) {

   int i;
   int file_size;
   int fips = ActiveDBHeader->fips;
   char map_name[255];

   /* NOTE that after the call to editor_db_remove(),
    * ActiveDBHeader pointer becomes invalid.
    */

   if (ActiveDBHeader->num_used_blocks < ActiveDBHeader->num_total_blocks) {
      return 0;
   }

   snprintf (map_name, sizeof(map_name), "edt%05d.rdm", ActiveDBHeader->fips);

   ActiveDBHeader->num_total_blocks += DB_DEFAULT_INITIAL_BLOCKS;
   ActiveDBHeader->file_size +=
      (DB_DEFAULT_INITIAL_BLOCKS * ActiveDBHeader->block_size);

   file_size = ActiveDBHeader->file_size;

   for (i = EditorCacheSize-1; i >= 0; --i) {

      if (EditorCache[i].fips == ActiveDBHeader->fips) {
         editor_db_remove (i);
         break;
      }
   }

   assert (i >= 0);

   if (roadmap_file_truncate
         (roadmap_path_user(), map_name, file_size) == -1) {
      
      editor_log (ROADMAP_ERROR, "Can't grow database.");
      return -1;
   }

   return editor_db_open (fips);
}


void *editor_map (roadmap_db *root) {
   return roadmap_db_get_data (root);
}

void editor_unmap (void *context) {}

