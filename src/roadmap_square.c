/* roadmap_square.c - Manage a county area, divided in small squares.
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
 *   See roadmap_square.h.
 *
 * These functions are used to retrieve the squares that make the county
 * area. A special square (ROADMAP_SQUARE_GLOBAL) is used to describe the
 * global county area (vs. a piece of it).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_dbread.h"
#include "roadmap_db_square.h"

#include "roadmap_square.h"

static char *RoadMapSquareType = "RoadMapSquareContext";


typedef struct {

   char *type;

   RoadMapGlobal *SquareGlobal;
   RoadMapSquare *Square;

   int *SquareGrid;
   int  SquareGridCount;

} RoadMapSquareContext;

static RoadMapSquareContext *RoadMapSquareActive = NULL;


static void *roadmap_square_map (roadmap_db *root) {

   RoadMapSquareContext *context;

   int i;
   int count;
   roadmap_db *global_table;
   roadmap_db *square_table;


   context = malloc(sizeof(RoadMapSquareContext));
   roadmap_check_allocated(context);

   context->type = RoadMapSquareType;

   global_table  = roadmap_db_get_subsection (root, "global");
   square_table = roadmap_db_get_subsection (root, "data");

   context->SquareGlobal = (RoadMapGlobal *) roadmap_db_get_data (global_table);
   context->Square       = (RoadMapSquare *) roadmap_db_get_data (square_table);

   count = context->SquareGlobal->count_longitude
              * context->SquareGlobal->count_latitude;

   context->SquareGrid = (int *) calloc (count, sizeof(int));
   roadmap_check_allocated(context->SquareGrid);

   context->SquareGridCount = count;

   for (i = count - 1; i >= 0; --i) {
      context->SquareGrid[i] = -1;
   }

   for (i = context->SquareGlobal->count_squares - 1; i >= 0; --i) {
      context->SquareGrid[context->Square[i].position] = i;
   }

   return context;
}

static void roadmap_square_activate (void *context) {

   RoadMapSquareContext *square_context = (RoadMapSquareContext *) context;

   if (square_context->type != RoadMapSquareType) {
      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   RoadMapSquareActive = square_context;
}

static void roadmap_square_unmap (void *context) {

   RoadMapSquareContext *square_context = (RoadMapSquareContext *) context;

   if (square_context->type != RoadMapSquareType) {
      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapSquareActive == square_context) {
      RoadMapSquareActive = NULL;
   }
   free(square_context->SquareGrid);
   free(square_context);
}

roadmap_db_handler RoadMapSquareHandler = {
   "square",
   roadmap_square_map,
   roadmap_square_activate,
   roadmap_square_unmap
};



static int roadmap_square_is_valid (square) {

   if (square < 0 || square >= RoadMapSquareActive->SquareGridCount) {
      roadmap_log (ROADMAP_ERROR, "invalid square index %d", square);
      return 0;
   }
   return 1;
}


int roadmap_square_count (void) {

   return RoadMapSquareActive->SquareGridCount;
}


static int roadmap_square_on_grid (const RoadMapPosition *position) {

   int x;
   int y;

   RoadMapGlobal *global = RoadMapSquareActive->SquareGlobal;


   x = (position->longitude - global->min_longitude) / global->step_longitude;
   if (x < 0 || x > global->count_longitude) {
      return -1;
   }

   y = (position->latitude - global->min_latitude)  / global->step_latitude;
   if (y < 0 || y > global->count_latitude) {
      return -1;
   }

   if (x >= global->count_longitude) {
      x = global->count_longitude - 1;
   }
   if (y >= global->count_latitude) {
      y = global->count_latitude - 1;
   }
   return (x * global->count_latitude) + y;
}


static int roadmap_square_location (const RoadMapPosition *position) {

   int square;
   int *grid = RoadMapSquareActive->SquareGrid;
   int  grid_count = RoadMapSquareActive->SquareGridCount;
   RoadMapSquare *this_square;
   RoadMapSquare *base_square = RoadMapSquareActive->Square;

   square = roadmap_square_on_grid (position);
   if (square < 0 || square >= grid_count) {
      return -1;
   }

   /* The computation above may have rounding errors: adjust. */

   this_square = base_square + grid[square];

   while (grid[square] < 0 ||
          this_square->max_longitude <= position->longitude) {

      if (position->longitude ==
          RoadMapSquareActive->SquareGlobal->max_longitude) break;

      if (++square >= grid_count) return -1;
      this_square = base_square + grid[square];
   }

   while (grid[square] < 0 ||
          this_square->min_longitude > position->longitude) {

      if (--square < 0) return -1;
      this_square = base_square + grid[square];
   }

   while (grid[square] < 0 ||
          this_square->max_latitude <= position->latitude) {

      if (position->latitude ==
          RoadMapSquareActive->SquareGlobal->max_latitude) break;

      if (++square >= grid_count) return -1;
      this_square = base_square + grid[square];
   }

   while (grid[square] < 0 ||
          this_square->min_latitude > position->latitude) {

      if (--square < 0) return -1;
      this_square = base_square + grid[square];
   }

   return square;
}


int roadmap_square_search (const RoadMapPosition *position) {

   RoadMapSquare *this_square;
   int *grid = RoadMapSquareActive->SquareGrid;

   int square = roadmap_square_location (position);

   if (square < 0 || square >= RoadMapSquareActive->SquareGridCount) {
      return ROADMAP_SQUARE_OTHER;
   }
   if (grid[square] < 0) {
      return ROADMAP_SQUARE_GLOBAL;
   }

   this_square = RoadMapSquareActive->Square + grid[square];

   if ((this_square->min_longitude > position->longitude) ||
       (this_square->max_longitude < position->longitude) ||
       (this_square->min_latitude > position->latitude) ||
       (this_square->max_latitude < position->latitude)) {

      return ROADMAP_SQUARE_GLOBAL;
   }

   return square;
}


void  roadmap_square_min (int square, RoadMapPosition *position) {

   RoadMapGlobal *global = RoadMapSquareActive->SquareGlobal;


   /* Default values. */
   position->longitude = global->min_longitude;
   position->latitude  = global->min_latitude;

   if (square == ROADMAP_SQUARE_GLOBAL) {
      return;
   }

   if (! roadmap_square_is_valid (square)) {
      return;
   }

   square = RoadMapSquareActive->SquareGrid[square];
   if (square < 0) {
      return;
   }
   position->longitude = RoadMapSquareActive->Square[square].min_longitude;
   position->latitude  = RoadMapSquareActive->Square[square].min_latitude;
}


void  roadmap_square_edges
         (int square, int *west, int *east, int *north, int *south) {


   if (RoadMapSquareActive == NULL) {

      *west = 0;
      *east = 0;
      *north = 0;
      *south = 0;

      return;
   }

   if (square == ROADMAP_SQUARE_GLOBAL) {

      RoadMapGlobal *global = RoadMapSquareActive->SquareGlobal;

      *west = global->min_longitude;
      *east = global->max_longitude;
      *north = global->max_latitude;
      *south = global->min_latitude;

      return;
   }

   if (! roadmap_square_is_valid (square)) {
      return;
   }
   square = RoadMapSquareActive->SquareGrid[square];

   {
      RoadMapSquare *square_item = RoadMapSquareActive->Square + square;

      *west =  square_item->min_longitude;
      *east =  square_item->max_longitude;
      *north = square_item->max_latitude;
      *south = square_item->min_latitude;
   }
}


int roadmap_square_index (int square) {

   if (! roadmap_square_is_valid (square)) {
      return -1;
   }

   return RoadMapSquareActive->SquareGrid[square];
}


int roadmap_square_from_index (int index) {

   if (RoadMapSquareActive == NULL) {
      return -1;
   }

   if (index < 0 ||
       index >= RoadMapSquareActive->SquareGlobal->count_squares) {
      return -1;
   }

   return RoadMapSquareActive->Square[index].position;
}


int roadmap_square_view (int *square, int size) {

   int *grid = RoadMapSquareActive->SquareGrid;
   RoadMapGlobal *global = RoadMapSquareActive->SquareGlobal;

   int west;
   int east;
   int north;
   int south;
   int x0;
   int x1;
   int x;
   int y0;
   int y1;
   int y;
   int count;
   int grid_index;


   roadmap_math_screen_edges (&west, &east, &north, &south);

   x0 = (west - global->min_longitude) / global->step_longitude;
   x1 = (east - global->min_longitude) / global->step_longitude;
   if ((x1 < 0) || (x0 >= global->count_longitude)) {
      return 0;
   }
   if (x0 < 0) {
      x0 = 0;
   }
   if (x1 >= global->count_longitude) {
      x1 = global->count_longitude - 1;
   }

   y0 = (north  - global->min_latitude)  / global->step_latitude;
   y1 = (south  - global->min_latitude)  / global->step_latitude;
   if ((y0 < 0) || (y1 >= global->count_latitude)) {
      return 0;
   }
   if (y1 < 0) {
      y1 = 0;
   }
   if (y0 >= global->count_latitude) {
      y0 = global->count_latitude - 1;
   }

   count = 0;

   for (x = x0; x <= x1; ++x) {

      for (y = y1; y <= y0; ++y) {

         grid_index = (x * global->count_latitude) + y;

         if (grid[grid_index] >= 0) {

            square[count] = grid_index;
            count  += 1;
            if (count >= size) {
               roadmap_log (ROADMAP_ERROR,
                            "too many square are visible: %d is not enough",
                            size);
               return size;
            }
         }
      }
   }

   return count;
}

