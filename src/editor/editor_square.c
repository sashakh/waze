/* editor_point.c - point databse layer
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
 *   See editor_point.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../roadmap.h"
#include "../roadmap_file.h"
#include "../roadmap_path.h"
#include "../roadmap_math.h"

#include "editor_log.h"
#include "editor_db.h"
#include "editor_point.h"
#include "editor_shape.h"
#include "editor_trkseg.h"

#include "editor_square.h"

static editor_db_section *ActiveSquaresDB;
static int longitude_count;

/* FIXME: depends on the header section activation before the squares activation */
static void editor_squares_activate (void *context) {
   RoadMapArea *edges = editor_db_get_active_edges ();

   ActiveSquaresDB = (editor_db_section *) context;
   longitude_count = abs(
          (edges->east - edges->west) / EDITOR_DB_LONGITUDE_STEP + 1);
}


roadmap_db_handler EditorSquaresHandler = {
   "squares",
   editor_map,
   editor_squares_activate,
   editor_unmap
};


static void editor_square_init_item (void *item) {
   
   editor_db_square *square = (editor_db_square *)item;
   
   square->cfccs = 0;
   square->section.num_items = 0;
   square->section.items_per_block = editor_db_get_block_size ();
   square->section.max_blocks = MAX_BLOCKS_PER_SQUARE;
   square->section.item_size = sizeof(int);
   square->section.max_items =
      editor_db_get_block_size () * MAX_BLOCKS_PER_SQUARE / sizeof(int);
}


static editor_db_square *editor_square_get_by_position (
                                       const RoadMapPosition *position,
                                       int create) {

   int square;

   editor_square_find_by_position (position, &square, 1, 0);
   
   return (editor_db_square *)editor_db_get_item
      (ActiveSquaresDB, square, create, editor_square_init_item);
}


static editor_db_square *editor_square_get (int square) {

   if (square < 0 || square >= ActiveSquaresDB->max_items)
      return NULL;

   return (editor_db_square *)editor_db_get_item
                                  (ActiveSquaresDB, square, 0, NULL);
}

void editor_square_add_line
         (int line_id,
          int p_from,
          int p_to,
          int trkseg,
          int cfcc) {
   
   RoadMapPosition position;
   editor_db_square *cur_square;
   editor_db_square *square;
   int trk_from;
   int first_shape;
   int last_shape;
   int i;

   editor_point_position (p_from, &position);

   cur_square = editor_square_get_by_position (&position, 1);

   if (editor_db_add_item (&cur_square->section, &line_id) == -1) {
      editor_db_grow ();
      if (editor_db_add_item (&cur_square->section, &line_id) == -1) {
         editor_log (ROADMAP_ERROR, "Can't add line to square. Line is lost forever!");
         return;
      }
   }

   cur_square->cfccs |= 1<<cfcc;

   editor_trkseg_get
      (trkseg, &trk_from, NULL, &first_shape, &last_shape, NULL);

   if (first_shape > -1) {
      editor_point_position (trk_from, &position);

      for (i=first_shape; i<=last_shape; i++) {

         editor_shape_position (i, &position);
         square = editor_square_get_by_position (&position, 1);

         if (square != cur_square) {

            int *last_item = (int *)editor_db_get_last_item (&square->section);
      
            if ((last_item == NULL) || (*last_item != line_id)) {
               editor_db_add_item (&square->section, &line_id);
               square->cfccs |= 1<<cfcc;
               cur_square = square;
            }
         }
      }
   }

   editor_point_position (p_to, &position);
   square = editor_square_get_by_position (&position, 1);

   if (square != cur_square) {

      int *last_item = (int *)editor_db_get_last_item (&square->section);
      
      if ((last_item == NULL) || (*last_item != line_id)) {
         editor_db_add_item (&square->section, &line_id);
         square->cfccs |= 1<<cfcc;
      }
   }
}


void editor_square_view (int *x0, int *y0, int *x1, int *y1) {

   RoadMapArea screen;
   RoadMapArea *edges = editor_db_get_active_edges ();

   roadmap_math_screen_edges (&screen);

   *x0 = abs((screen.west - edges->west) / EDITOR_DB_LONGITUDE_STEP);
   *x1 = abs((screen.east - edges->west) / EDITOR_DB_LONGITUDE_STEP);

   *y1 = abs((screen.north  - edges->south)  / EDITOR_DB_LATITUDE_STEP);
   *y0 = abs((screen.south  - edges->south)  / EDITOR_DB_LATITUDE_STEP);
}


int editor_square_find (int x, int y) {
   
   int square = y * longitude_count + x;

   if (square < 0 || square >= ActiveSquaresDB->max_items) {
      return -1;
   }

   if (editor_db_get_item (ActiveSquaresDB, square, 0, NULL) == NULL) {
      return -1;
   }

   return square;
}


int editor_square_get_num_lines (int square) {

   editor_db_square *square_db = editor_square_get (square);

   if (square_db == NULL) return 0;

   return square_db->section.num_items;
}


int editor_square_get_line (int square, int i) {

   editor_db_square *square_db = editor_square_get (square);

   if (square_db == NULL) return 0;

   return *(int *)editor_db_get_item (&square_db->section, i, 0, NULL);
}


int editor_square_get_cfccs (int square) {

   editor_db_square *square_db = editor_square_get (square);

   if (square_db == NULL) return 0;

   return square_db->cfccs;
}


int editor_square_find_by_position
      (const RoadMapPosition *position, int *squares, int size, int distance) {
#define ED_NEAR_DISTANCE 1000

   RoadMapArea *edges = editor_db_get_active_edges ();
   RoadMapArea square_edges;
   int x;
   int y;
   int square;
   int count = 0;

   x = abs((position->longitude - edges->west) / EDITOR_DB_LONGITUDE_STEP);
   y = abs((position->latitude - edges->south)  / EDITOR_DB_LATITUDE_STEP);

   square = y * longitude_count + x;

   if (square < 0 || square >= ActiveSquaresDB->max_items)
      square = 0;

   squares[count++] = square;

   if (!distance) return count;

   square_edges.west = edges->west + x * EDITOR_DB_LONGITUDE_STEP;
   square_edges.east = square_edges.west + EDITOR_DB_LONGITUDE_STEP;
   square_edges.south = edges->south + y * EDITOR_DB_LATITUDE_STEP;
   square_edges.north = square_edges.south + EDITOR_DB_LATITUDE_STEP;

   if (distance & ED_SQUARE_NEAR) {
      
      if ((position->longitude - square_edges.west) < ED_NEAR_DISTANCE) {

         if ((square -1) >= 0) {
         
            squares[count++] = square - 1;
         }
      }

      if (count == size) return count;

      if ((square_edges.east - position->longitude) < ED_NEAR_DISTANCE) {

         if ((square + 1) < ActiveSquaresDB->max_items) {
         
            squares[count++] = square + 1;
         }
      }

      if (count == size) return count;

      if ((position->latitude - square_edges.south) < ED_NEAR_DISTANCE) {

         if ((square - longitude_count) >= 0) {
         
            squares[count++] = square - longitude_count;
         }
      }

      if (count == size) return count;

      if ((square_edges.north - position->latitude) < ED_NEAR_DISTANCE) {

         if ((square + longitude_count) < ActiveSquaresDB->max_items) {
         
            squares[count++] = square + longitude_count;
         }
      }
   }

   return count;
}


