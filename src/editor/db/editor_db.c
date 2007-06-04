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

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_county.h"
#include "roadmap_line.h"
#include "roadmap_locator.h"
#include "roadmap_metadata.h"
#include "roadmap_messagebox.h"
#include "buildmap.h"

#include "../editor_log.h"

#include "editor_db.h"
#include "editor_point.h"
#include "editor_marker.h"
#include "editor_shape.h"
#include "editor_line.h"
#include "editor_square.h"
#include "editor_street.h"
#include "editor_dictionary.h"
#include "editor_route.h"
#include "editor_override.h"
#include "editor_trkseg.h"

/* Use the biggest item size to set the default block size */
#define DB_DEFAULT_BLOCK_SIZE (2 * sizeof (editor_db_square))

/* TODO create a generic cache system - this was copied from roadmap_locator */

#define EDITOR_CACHE_SIZE 10
#define FLUSH_SIZE 300

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
   
   int current_fips = roadmap_locator_active ();
   
   ActiveDBHeader = (editor_db_header *) context;
   
   if (roadmap_locator_activate (ActiveDBHeader->fips) != ROADMAP_US_OK) {

      roadmap_messagebox
         ("Error", "Can't activate roadmap's internal database");
      ActiveDBHeader = NULL;
      return;
   }

   if (strcmp
         (ActiveDBHeader->rm_map_date,
          roadmap_metadata_get_attribute ("Version", "Date"))) {

      roadmap_messagebox
         ("Error",
          "RoadMap database does match Editor data.\n Delete the editor database.");
      ActiveDBHeader = NULL;
      return;
   }

   if ((current_fips >= 0) && (current_fips != roadmap_locator_active ())) {
      roadmap_locator_activate (current_fips);
   }
}

static void editor_header_unmap (void *context) {
   if (ActiveDBHeader == context) ActiveDBHeader = NULL;
}

static void editor_blocks_activate (void *context) {
   ActiveBlocks = (char *) context;
}

roadmap_db_handler EditorHeaderHandler = {
   "header",
   editor_map,
   editor_header_activate,
   editor_header_unmap
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
            (EditorDBModel, "markers", &EditorMarkersHandler);
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
   
   if (section->max_blocks == block_id) {
      editor_log (ROADMAP_ERROR,
                  "editor_db_allocate_new_block - reached max section blocks.");
      return -1;
   }


   if (ActiveDBHeader->num_used_blocks == ActiveDBHeader->num_total_blocks) {
      editor_log (ROADMAP_ERROR,
                  "editor_db_allocate_new_block - no free blocks, need to grow.");
      return -1;
   }

   section->blocks[block_id] = ActiveDBHeader->num_used_blocks++;

   return 0;
}


static buildmap_db *add_db_section(buildmap_db *parent,
                                   const char *name,
                                   const void* private_init,
                                   int private_header_size,
                                   int item_size,
                                   int max_items) {

   buildmap_db *root;
   editor_db_section *section;
   char *private_data;
   int max_blocks;
   int i;

   max_blocks = (item_size * max_items)/DB_DEFAULT_BLOCK_SIZE + 1;

   root = buildmap_db_add_section (parent, name);
   buildmap_db_add_data
      (root, 1, sizeof(editor_db_section) + private_header_size);
   
   /* we already got one block allocated in editor_db_section */
   buildmap_db_add_data (root, max_blocks-1, sizeof(int));

   private_data = buildmap_db_get_data (root);
   section = (editor_db_section *) (private_data + private_header_size);

   if (private_init != NULL) {
      memcpy (private_data, private_init, private_header_size);
   }

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
      (root, "references", NULL, 0, sizeof(struct ed_dictionary_reference),
       DICTIONARY_INDEX_SIZE * 10);

   add_db_section
      (root, "trees", NULL, 0, sizeof(struct ed_dictionary_tree),
       DICTIONARY_INDEX_SIZE);

   add_db_section
      (root, "data", NULL, 0, 1, DICTIONARY_DATA_SIZE);
}


int editor_db_create (int fips) {

   char name[100];
   char path[100];
   const char *map_creation_date;
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

   snprintf (path, sizeof(path), "%s/maps", roadmap_path_user());
   snprintf (name, sizeof(name), "edt%05d", fips);
   
   roadmap_path_create (path);

   if (roadmap_file_exists (path, name)) {
      editor_log (ROADMAP_ERROR, "Trying to create a new database which already exists! '%s/%s'", path, name);
      editor_log_pop ();
      return -1;
   }
      
   if (buildmap_db_open (path, name) == -1) {
      editor_log (ROADMAP_ERROR, "Can't create new database: %s/%s",
            path, name);
      editor_log_pop ();
      return -1;
   }

   map_creation_date = roadmap_metadata_get_attribute ("Version", "Date");

   if (!strlen (map_creation_date) ||
         (strlen (map_creation_date) >= (sizeof (header->rm_map_date) - 1))) {

      editor_log
         (ROADMAP_ERROR,
          "Can't create new database (RM map has no valid timestamp): %s/%s",
            path, name);
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
   strcpy (header->rm_map_date, map_creation_date);

   add_db_section
      (NULL, "points", NULL, 0, sizeof(editor_db_point), EDITOR_MAX_POINTS);
   add_db_section
      (NULL, "markers", NULL, 0, sizeof(editor_db_marker), EDITOR_MAX_STREETS);
   add_db_section
      (NULL, "points_del", NULL, 0,
       sizeof(editor_db_del_point), EDITOR_MAX_POINTS);
   add_db_section
      (NULL, "shape", NULL, 0, sizeof(editor_db_shape), EDITOR_MAX_SHAPES);
   add_db_section
      (NULL, "lines", NULL, 0, sizeof(editor_db_line), EDITOR_MAX_LINES);
   
   square_count = ((edges->east - edges->west) / EDITOR_DB_LONGITUDE_STEP + 1) *
                  ((edges->north - edges->south) / EDITOR_DB_LATITUDE_STEP + 1);
   add_db_section
      (NULL, "squares", NULL, 0, sizeof(editor_db_square), square_count);
   add_db_section
      (NULL, "streets", NULL, 0, sizeof(editor_db_street), EDITOR_MAX_STREETS);
   add_db_section
      (NULL, "ranges", NULL, 0, sizeof(editor_db_range), EDITOR_MAX_LINES*2);

   add_db_section
      (NULL, "trkseg", &editor_db_trkseg_private_init,
       sizeof(editor_db_trkseg_private), sizeof(editor_db_trkseg),
       EDITOR_MAX_LINES*2);

   add_db_section
      (NULL, "route", NULL, 0,
       sizeof(editor_db_route_segment), EDITOR_MAX_LINES*2);

   root = buildmap_db_add_section (NULL, "override");
   add_db_section (root, "index", NULL, 0, sizeof(int), roadmap_line_count ());
   add_db_section
      (root, "data", NULL, 0, sizeof(editor_db_override), EDITOR_MAX_LINES);

   root = buildmap_db_add_section (NULL, "strings");
   add_db_string_section (root, "streets");
   add_db_string_section (root, "cities");
   add_db_string_section (root, "types");
   add_db_string_section (root, "zips");
   add_db_string_section (root, "t2s");
   add_db_string_section (root, "notes");

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
   snprintf (map_name, sizeof(map_name), "edt%05d", fips);

   /* Look for the oldest entry in the cache. */

   for (i = EditorCacheSize-1; i >= 0; --i) {

      if (EditorCache[i].fips == fips) {

         roadmap_db_activate (map_name);

         if (ActiveDBHeader == NULL) return -1;

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

   if (ActiveDBHeader == NULL) return -1;
    
   EditorCache[oldest].fips = fips;
   EditorCache[oldest].last_access = access;

   EditorActiveCounty = fips;
   
   return 0;
}


void editor_db_sync (int fips) {

   int i;

   for (i = EditorCacheSize-1; i >= 0; --i) {

      if (EditorCache[i].fips == fips) {
         char map_name[64];

         snprintf (map_name, sizeof(map_name),
               "edt%05d", EditorCache[i].fips);
         roadmap_db_sync (map_name);
      }
   }
}


int editor_db_activate (int fips) {

   int res;

   if (EditorActiveCounty == fips) {
       return 0;
   }

   if (fips == -1) return -1;

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


void editor_db_close (int fips) {

   int i;

   for (i = EditorCacheSize-1; i >= 0; --i) {

      if (EditorCache[i].fips == fips) {
         editor_db_remove (i);
      }
   }

   if (EditorActiveCounty == fips) {
      EditorActiveCounty = 0;
   }
}


void editor_db_delete (int fips) {

   char name[100];
   char path[100];

   snprintf (path, sizeof(path), "%s/maps", roadmap_path_user());
   snprintf (name, sizeof(name), "edt%05d%s", fips, ROADMAP_DB_TYPE);
   
   if (roadmap_file_exists (path, name)) {

      char **files;
      char **cursor;
      char *directory;

      /* Delete notes wav files */
      /* FIXME this is broken for multiple counties */
      directory = roadmap_path_join (roadmap_path_user (), "markers");
      files = roadmap_path_list (directory, ".wav");

      for (cursor = files; *cursor != NULL; ++cursor) {

         char *full_name = roadmap_path_join (directory, *cursor);
         roadmap_file_remove (NULL, full_name);

         free (full_name);
      }

      free (directory);

      /* Remove the actual editor file */
      roadmap_file_remove (path, name);
   }
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
   static int flush_count;

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

   if (++flush_count == FLUSH_SIZE) {
      flush_count = 0;
      editor_db_sync (ActiveDBHeader->fips);
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

   if (ActiveDBHeader == NULL) return NULL;

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
   char path[100];

   editor_log (ROADMAP_ERROR, "editor_db_grow - total:%d used:%d.",
        ActiveDBHeader->num_total_blocks,
   	ActiveDBHeader->num_used_blocks);

   /* NOTE that after the call to editor_db_remove(),
    * ActiveDBHeader pointer becomes invalid.
    */

   if (ActiveDBHeader->num_used_blocks <
         (ActiveDBHeader->num_total_blocks - 10)) {

      return 0;
   }

   snprintf (path, sizeof(path), "%s/maps", roadmap_path_user());
   snprintf (map_name, sizeof(map_name), "edt%05d%s",
               ActiveDBHeader->fips, ROADMAP_DB_TYPE);

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

   if (roadmap_file_truncate (path, map_name, file_size) == -1) {
      
      editor_log (ROADMAP_ERROR, "Can't grow database.");
      return -1;
   }

   return editor_db_open (fips);
}


int editor_db_locator(const RoadMapPosition *position) {

   RoadMapArea *edges = editor_db_get_active_edges ();
   static int *fips;
   int count;

   if (edges != NULL) {

      if ((position->longitude >= edges->west) &&
            (position->longitude <= edges->east) &&
            (position->latitude >= edges->south) &&
            (position->latitude <= edges->north)) {
         return ActiveDBHeader->fips;
      }
   }
   
   count = roadmap_locator_by_position (position, &fips);

   if (count) return fips[0];

   /* FIXME this is a hack until I figure out why we get some -1 fips */
   editor_log (ROADMAP_ERROR, "editor_db_locator - can't find fips.");
   return 77001;
   //return -1;
}


void editor_db_check_grow (void) {

   if (ActiveDBHeader == NULL) return;

   if (ActiveDBHeader->num_used_blocks >=
         (ActiveDBHeader->num_total_blocks - 10)) {

      editor_db_grow ();
   }
}


void *editor_map (roadmap_db *root) {
   return roadmap_db_get_data (root);
}


void editor_unmap (void *context) {}

