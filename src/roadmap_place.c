/* roadmap_place.c - Manage placename points.
 *
 * LICENSE:
 *
 *   Copyright 2004 Stephen Woodbridge
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
 *   int  roadmap_place_in_square (int square, int cfcc, int *first, int *last);
 *   void roadmap_place_point   (int place, RoadMapPosition *position);
 *
 *   int  roadmap_place_count (void);
 *
 * These functions are used to retrieve the points that make the places.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_db_place.h"

#include "roadmap_point.h"
#include "roadmap_place.h"
#include "roadmap_square.h"


static char *RoadMapPlaceType = "RoadMapPlaceContext";

typedef struct {

   char *type;

   RoadMapPlace *Place;
   int           PlaceCount;

   RoadMapPlaceBySquare *PlaceBySquare;
   int                   PlaceBySquareCount;

} RoadMapPlaceContext;

static RoadMapPlaceContext *RoadMapPlaceActive = NULL;


static void *roadmap_place_map (roadmap_db *root) {

   RoadMapPlaceContext *context;

   roadmap_db *place_table;
   roadmap_db *square_table;


   context = (RoadMapPlaceContext *) malloc (sizeof(RoadMapPlaceContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }
   context->type = RoadMapPlaceType;

   place_table   = roadmap_db_get_subsection (root, "data");
   square_table  = roadmap_db_get_subsection (root, "bysquare");

   context->Place = (RoadMapPlace *) roadmap_db_get_data (place_table);
   context->PlaceCount = roadmap_db_get_count (place_table);

   if (context->PlaceCount !=
       roadmap_db_get_size (place_table) / sizeof(RoadMapPlace)) {
      roadmap_log (ROADMAP_ERROR, "invalid place/data structure");
      goto roadmap_place_map_abort;
   }

   context->PlaceBySquare =
      (RoadMapPlaceBySquare *) roadmap_db_get_data (square_table);
   context->PlaceBySquareCount = roadmap_db_get_count (square_table);

   if (context->PlaceBySquareCount !=
       roadmap_db_get_size (square_table) / sizeof(RoadMapPlaceBySquare)) {
      roadmap_log (ROADMAP_ERROR, "invalid place/bysquare structure");
      goto roadmap_place_map_abort;
   }

   return context;

roadmap_place_map_abort:

   free(context);
   return NULL;
}

static void roadmap_place_activate (void *context) {

   RoadMapPlaceContext *place_context = (RoadMapPlaceContext *) context;

   if (place_context->type != RoadMapPlaceType) {
      roadmap_log (ROADMAP_FATAL, "invalid place context activated");
   }
   RoadMapPlaceActive = place_context;
}

static void roadmap_place_unmap (void *context) {

   RoadMapPlaceContext *place_context = (RoadMapPlaceContext *) context;

   if (place_context->type != RoadMapPlaceType) {
      roadmap_log (ROADMAP_FATAL, "unmapping invalid place context");
   }
   free (place_context);
}

roadmap_db_handler RoadMapPlaceHandler = {
   "dictionary",
   roadmap_place_map,
   roadmap_place_activate,
   roadmap_place_unmap
};


int roadmap_place_in_square (int square, int cfcc, int *first, int *last) {

   int  next;
   int *index;

   if (cfcc <= 0 || cfcc > ROADMAP_CATEGORY_RANGE) {
      roadmap_log (ROADMAP_FATAL, "illegal cfcc %d", cfcc);
   }
   square = roadmap_square_index(square);
   if (square < 0) {
      return 0;   /* This square is empty. */
   }

   index = RoadMapPlaceActive->PlaceBySquare[square].first;

   if (index[cfcc-1] < 0) {
      return 0;
   }
   *first = index[cfcc-1];

   for (next = -1; next < 0 && cfcc < ROADMAP_CATEGORY_RANGE; cfcc++) {
      next  = index[cfcc];
   }

   if (next > 0) {
      *last = next - 1;
   } else {
      *last = RoadMapPlaceActive->PlaceBySquare[square].last;
   }

   return 1;
}


void roadmap_place_point   (int place, RoadMapPosition *position) {

#ifdef DEBUG
   if (place < 0 || place >= RoadMapPlaceActive->PlaceCount) {
      roadmap_log (ROADMAP_FATAL, "illegal place index %d", place);
   }
#endif
   roadmap_point_position (RoadMapPlaceActive->Place[place], position);
}


int  roadmap_place_count (void) {

   return RoadMapPlaceActive->PlaceCount;
}

