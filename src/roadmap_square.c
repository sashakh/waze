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

   short *SquareGrid; /* keep small: large grids can use a lot of them */
   int  SquareGridCount;
   int SquareGridBitmapped;
   int SquareLastLookup;  /* lookup cache for bitmapped grids */
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

   /* See if the grid seems like it will be very sparse, by
    * comparing the number of squares in the whole grid to those
    * that actually contain features.  In practice, the ratio is
    * either very low (less than 10) for small maps of
    * "typically" featured areas, or very high (more than 1000)
    * for huge maps containing nothing but boundary lines (where
    * only the endpoints of the lines count as features).  So the
    * value used for comparison isn't really very critical.
    */
   if (count / context->SquareGlobal->count_squares < 50) {

       /* Allocate the entire grid representing the area
        * covered by these squares.
        */
       context->SquareGridBitmapped = 0;
       context->SquareGrid = (short *) calloc (count, sizeof(short));
       roadmap_check_allocated(context->SquareGrid);

       context->SquareGridCount = count;

       for (i = context->SquareGlobal->count_squares - 1; i >= 0; --i) {
          /* store "i + 1" so that 0 can be the "invalid" marker. 
           * we'll subtract 1 every time we dereference, and
           * compare against negative.
           */
          context->SquareGrid[context->Square[i].position] = i + 1;
       }

   } else {
       /* For very sparse grids, we save memory by only
        * allocating a bitmap.  the bitmap will tell us whether
        * we need to linear search the list for our square.
        */
       context->SquareGridBitmapped = 1;
       context->SquareGrid = (short *) calloc((count / 16) + 1, sizeof(short));
       roadmap_check_allocated(context->SquareGrid);

       context->SquareGridCount = count;

       for (i = context->SquareGlobal->count_squares - 1; i >= 0; --i) {
          int index = context->Square[i].position / 16;
          int bit  = context->Square[i].position % 16;
          context->SquareGrid[index] |= (1 << bit);
       }


   }

   context->SquareLastLookup = -1;

   return context;
}

int grid_index(int square)
{

   if (!RoadMapSquareActive->SquareGridBitmapped) {

      return RoadMapSquareActive->SquareGrid[square] - 1;

   } else {
      int i;
      int index;
      int bit;
      static int last_index;

      if (square == RoadMapSquareActive->SquareLastLookup) {
         return last_index;
      }

      RoadMapSquareActive->SquareLastLookup = square;

      index = square / 16;
      bit  = square % 16;

      if ((RoadMapSquareActive->SquareGrid[index] & (1 << bit)) == 0)
         return last_index = -1;

      for (i = RoadMapSquareActive->SquareGlobal->count_squares - 1;
               i >= 0; --i) {
         if (RoadMapSquareActive->Square[i].position == square) {
            return last_index = i;
         }
      }

      roadmap_log (ROADMAP_WARNING, "bitmapping BUG");
      return last_index = -1;
   }

}

static void roadmap_square_activate (void *context) {

   RoadMapSquareContext *square_context = (RoadMapSquareContext *) context;

   if ((square_context != NULL) &&
       (square_context->type != RoadMapSquareType)) {
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



static int roadmap_square_is_valid (int square) {

   if (square < 0 || square >= RoadMapSquareActive->SquareGridCount) {
      roadmap_log (ROADMAP_ERROR, "invalid square index %d", square);
      return 0;
   }
   return 1;
}


int roadmap_square_count (void) {

   if (RoadMapSquareActive == NULL) return 0;

   return RoadMapSquareActive->SquareGridCount;
}


static int roadmap_square_on_grid (const RoadMapPosition *position) {

   int x;
   int y;

   RoadMapGlobal *global = RoadMapSquareActive->SquareGlobal;

   if (position->longitude > global->edges.east ||
       position->latitude > global->edges.north ||
       position->longitude < global->edges.west ||
       position->latitude < global->edges.south) {
      return -1;
   }

   x = (position->longitude - global->edges.west) / global->step_longitude;
   if (x < 0 || x > global->count_longitude) {
      return -1;
   }

   y = (position->latitude - global->edges.south)  / global->step_latitude;
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

   int square, newsq, index;
   int  grid_count = RoadMapSquareActive->SquareGridCount;
   RoadMapSquare *this_square;
   RoadMapSquare *base_square = RoadMapSquareActive->Square;
   RoadMapGlobal *global = RoadMapSquareActive->SquareGlobal;

   square = roadmap_square_on_grid (position);
   if (square < 0 || square >= grid_count) {
      return -1;
   }

   /* The computation above may have rounding errors: adjust. */
   index = grid_index(square);
   this_square = base_square + index;

   if (index >= 0) {
      /* check against the found edges, and move if we're outside */

      if (position->longitude > this_square->edges.east) { /* go east */
         newsq = square + 1;  /* east */
         if (newsq < grid_count)
            return newsq;
      }
      if (position->latitude > this_square->edges.north) { /* go north */
         newsq = square + global->count_longitude;;  /* north */
         if (newsq < grid_count)
            return newsq;
      }
      if (position->longitude < this_square->edges.west) { /* go west */
         newsq = square - 1;  /* west */
         if (newsq >= 0)
            return newsq;
      }
      if (position->latitude < this_square->edges.south) { /* go south */
         newsq = square - global->count_longitude;;  /* south */
         if (newsq >= 0)
            return newsq;
      }

      return square;

   } else {
      /* our square is invalid.
       * check against our neighbors' edges (if they're valid) and
       * move if appropriate.
       */
      newsq = square + 1;  /* east */
      if (newsq < grid_count)  {
         index = grid_index(newsq);
         if (index >= 0 && 
               /* moved east, so see if we're good to the west now */
               position->longitude > (base_square + index)->edges.west) {
            return newsq;
         }
      }
      newsq = square + global->count_longitude;;  /* north */
      if (newsq < grid_count)  {
         index = grid_index(newsq);
         if (index >= 0 &&
               /* moved north, so see if we're good to the south now */
               position->latitude > (base_square + index)->edges.south) {
            return newsq;
         }
      }
      newsq = square - 1;  /* west */
      if (newsq >= 0)  {
         index = grid_index(newsq);
         if (index >= 0 &&
               /* moved west, so see if we're good to the east now */
               position->longitude < (base_square + index)->edges.east) {
            return newsq;
         }
      }
      newsq = square - global->count_longitude;;  /* south */
      if (newsq >= 0)  {
         index = grid_index(newsq);
         if (index >= 0 && 
               /* moved south, so see if we're good to the north now */
               position->latitude < (base_square + index)->edges.north) {
            return newsq;
         }
      }
   }

   return square;
}

int roadmap_square_search (const RoadMapPosition *position) {

   int square;
   RoadMapSquare *this_square;


   if (RoadMapSquareActive == NULL) return ROADMAP_SQUARE_OTHER;

   square = roadmap_square_location (position);

   if (square < 0) {
      return ROADMAP_SQUARE_OTHER;
   }
   if (grid_index(square) < 0) {
      return ROADMAP_SQUARE_GLOBAL;
   }

   this_square = RoadMapSquareActive->Square + grid_index(square);

   if ((this_square->edges.west > position->longitude) ||
       (this_square->edges.east < position->longitude) ||
       (this_square->edges.south > position->latitude) ||
       (this_square->edges.north < position->latitude)) {

      return ROADMAP_SQUARE_GLOBAL;
   }

   return square;
}


void  roadmap_square_min (int square, RoadMapPosition *position) {

   if (RoadMapSquareActive == NULL) return;

   /* Default values. */
   position->longitude = RoadMapSquareActive->SquareGlobal->edges.west;
   position->latitude  = RoadMapSquareActive->SquareGlobal->edges.south;

   if (square == ROADMAP_SQUARE_GLOBAL) {
      return;
   }

   if (! roadmap_square_is_valid (square)) {
      return;
   }

   square = grid_index(square);
   if (square < 0) {
      return;
   }
   position->longitude = RoadMapSquareActive->Square[square].edges.west;
   position->latitude  = RoadMapSquareActive->Square[square].edges.south;
}


void  roadmap_square_edges (int square, RoadMapArea *edges) {

   if (RoadMapSquareActive == NULL) {

      edges->west = 0;
      edges->east = 0;
      edges->north = 0;
      edges->south = 0;

      return;
   }

   if (square == ROADMAP_SQUARE_GLOBAL) {

      RoadMapGlobal *global = RoadMapSquareActive->SquareGlobal;

      *edges = global->edges;

      return;
   }

   if (! roadmap_square_is_valid (square)) {
      return;
   }
   square = grid_index(square);

   {
      RoadMapSquare *square_item = RoadMapSquareActive->Square + square;

      *edges = square_item->edges;
   }
}


int roadmap_square_index (int square) {

   if (RoadMapSquareActive == NULL) return -1;

   if (! roadmap_square_is_valid (square)) {
      return -1;
   }

   return grid_index(square);
}


int roadmap_square_from_index (int index) {

   if (RoadMapSquareActive == NULL) return -1;

   if (index < 0 ||
       index >= RoadMapSquareActive->SquareGlobal->count_squares) {
      return -1;
   }

   return RoadMapSquareActive->Square[index].position;
}


int roadmap_square_view (int **in_view) {

   RoadMapGlobal *global;

   static int *squares = NULL;
   static int max_view_squares = ROADMAP_MIN_VISIBLE_SQUARES;

   RoadMapArea screen;
   int x0;
   int x1;
   int x;
   int y0;
   int y1;
   int y;
   int count;
   int gindex;


   if (RoadMapSquareActive == NULL) return 0;

   global = RoadMapSquareActive->SquareGlobal;

   roadmap_math_screen_edges (&screen);

   x0 = (screen.west - global->edges.west) / global->step_longitude;
   x1 = (screen.east - global->edges.west) / global->step_longitude;
   if ((x1 < 0) || (x0 >= global->count_longitude)) {
      return 0;
   }
   if (x0 < 0) {
      x0 = 0;
   }
   if (x1 >= global->count_longitude) {
      x1 = global->count_longitude - 1;
   }

   y0 = (screen.north  - global->edges.south)  / global->step_latitude;
   y1 = (screen.south  - global->edges.south)  / global->step_latitude;
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

   if (squares == NULL) {
      squares = malloc (max_view_squares * sizeof(int));
      roadmap_check_allocated(squares);
   }

   for (x = x0; x <= x1; ++x) {

      for (y = y1; y <= y0; ++y) {

         gindex = (x * global->count_latitude) + y;

         if (grid_index(gindex) >= 0) {

            squares[count] = gindex;
            count  += 1;
            if (count >= max_view_squares) {
	       if (max_view_squares >= ROADMAP_MAX_VISIBLE_SQUARES) {
        	    roadmap_log (ROADMAP_ERROR,
                            "too many square are visible: %d is not enough",
                            max_view_squares);
   		    *in_view = squares;
        	    return max_view_squares;
	       }
	       max_view_squares *= 4;
               roadmap_log (ROADMAP_DEBUG,
                            "growing space for visible squares to %d",
                            max_view_squares);
	       squares = realloc (squares, max_view_squares * sizeof(int));
	       roadmap_check_allocated(squares);
            }
         }
      }
   }

   *in_view = squares;
   return count;
}

