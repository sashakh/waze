/* buildmap_place.c - Build a place table & index for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2004 Stephen Woodbridge <woodbri@swoodbridge.com>
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
 *   void buildmap_place_initialize (void);
 *   int  buildmap_place_add (int name, int cfcc, int point);
 *   int  buildmap_place_get_sorted  (int place);
 *   int buildmap_place_get_name_sorted (int place);
 *   BuildMapPlace *buildmap_place_get_record (int place);
 *   BuildMapPlace *buildmap_place_get_record_sorted (int place);
 *   void buildmap_place_find_sorted (int name);
 *   void buildmap_place_get_position 
 *           (int place, int *longitude, int *latitude);
 *   int buildmap_place_get_point_sorted (int place);
 *   void buildmap_place_get_position_sorted
 *           (int place, int *longitude, int *latitude);
 *   void buildmap_place_get_square_sorted (int place);
 *   int buildmap_place_compare (const void *r1, const void *r2);
 *   void buildmap_place_sort (void);
 *   void buildmap_place_save    (void);
 *   void buildmap_place_summary (void);
 *   void buildmap_place_reset   (void);
 *
 * These functions are used to build a table of places from
 * the various data sources. The objective is double: (1) reduce 
 * the size of the data by sharing all duplicated information and
 * (2) produce the index data to serve as the basis for a fast
 * search mechanism for places in roadmap.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_db_place.h"

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_point.h"
#include "buildmap_square.h"
#include "buildmap_place.h"


typedef struct {
   int name;
   int sorted;
   int cfcc;
   int point;
} BuildMapPlace;


static int PlaceCount = 0;
static int PlaceTableSize = 0;
static BuildMapPlace *Place[BUILDMAP_BLOCK] = {NULL};

static RoadMapHash *PlaceByName = NULL;

static int *SortedPlace = NULL;


void buildmap_place_initialize (void) {

   PlaceByName = roadmap_hash_new ("PlaceByName", BUILDMAP_BLOCK);

   Place[0] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapPlace));
   if (Place[0] == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   PlaceTableSize = 0;
   PlaceCount = 0;
}


int buildmap_place_add (int name, int cfcc, int point) {

   int block;
   int offset;
   BuildMapPlace *this_place;


   block = PlaceCount / BUILDMAP_BLOCK;
   offset = PlaceCount % BUILDMAP_BLOCK;

   if (Place[block] == NULL) {

      /* We need to add a new block to the table. */

      Place[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapPlace));
      if (Place[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }

      roadmap_hash_resize (PlaceByName, (block+1) * BUILDMAP_BLOCK);
   }

   this_place = Place[block] + offset;

   if ((point < 0)) {
     buildmap_fatal (0, "invalid point");
   }
   this_place->name = name;
   this_place->cfcc = cfcc;
   this_place->point = point;

   roadmap_hash_add (PlaceByName, name, PlaceCount);

   return PlaceCount++;
}


int  buildmap_place_find_sorted (int name) {

   int index;
   BuildMapPlace *this_place;

   if (SortedPlace == NULL) {
      buildmap_fatal (0, "Places not sorted yet");
   }

   for (index = roadmap_hash_get_first (PlaceByName, name);
        index >= 0;
        index = roadmap_hash_get_next (PlaceByName, index)) {

      this_place = Place[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      if (this_place->name == name) {
         return this_place->sorted;
      }
   }

   return -1;
}


static BuildMapPlace *buildmap_place_get_record (int place) {

   if ((place < 0) || (place > PlaceCount)) {
      buildmap_fatal (0, "invalid place index %d", place);
   }

   return Place[place/BUILDMAP_BLOCK] + (place % BUILDMAP_BLOCK);
}


static BuildMapPlace *buildmap_place_get_record_sorted (int place) {

   if ((place < 0) || (place > PlaceCount)) {
      buildmap_fatal (0, "invalid place index %d", place);
   }

   if (SortedPlace == NULL) {
      buildmap_fatal (0, "Places not sorted yet");
   }

   place = SortedPlace[place];

   return Place[place/BUILDMAP_BLOCK] + (place % BUILDMAP_BLOCK);
}


int buildmap_place_get_point_sorted (int place) {

   BuildMapPlace *this_place = buildmap_place_get_record_sorted (place);

   return this_place->point;
}

void buildmap_place_get_position (int place, int *longitude, int *latitude) {

   BuildMapPlace *this_place = buildmap_place_get_record (place);

   *longitude = buildmap_point_get_longitude (this_place->point);
   *latitude  = buildmap_point_get_latitude  (this_place->point);
}


void buildmap_place_get_position_sorted
          (int place, int *longitude, int *latitude) {

   BuildMapPlace *this_place = buildmap_place_get_record_sorted (place);

   *longitude = buildmap_point_get_longitude_sorted (this_place->point);
   *latitude  = buildmap_point_get_latitude_sorted  (this_place->point);
}


int  buildmap_place_get_sorted (int place) {

   BuildMapPlace *this_place = buildmap_place_get_record (place);

   if (SortedPlace == NULL) {
      buildmap_fatal (0, "Places not sorted yet");
   }

   return this_place->sorted;
}


int buildmap_place_get_name_sorted (int place) {

   return buildmap_place_get_record_sorted(place)->name;
}


int buildmap_place_get_square_sorted (int place) {

   return buildmap_point_get_square_sorted
             (buildmap_place_get_record_sorted(place)->point);
}


static int buildmap_place_compare (const void *r1, const void *r2) {

   int square1;
   int square2;

   int index1 = *((int *)r1);
   int index2 = *((int *)r2);

   BuildMapPlace *record1;
   BuildMapPlace *record2;

   record1 = Place[index1/BUILDMAP_BLOCK] + (index1 % BUILDMAP_BLOCK);
   record2 = Place[index2/BUILDMAP_BLOCK] + (index2 % BUILDMAP_BLOCK);


   /* The Places are first sorted by square.
    * Within a square, Places are sorted by category.
    * Within a category, Places are sorted by point.
    */

   square1 = buildmap_point_get_square_sorted (record1->point);
   square2 = buildmap_point_get_square_sorted (record2->point);

   if (square1 != square2) {
      return square1 - square2;
   }

   if (record1->cfcc != record2->cfcc) {
      return record1->cfcc - record2->cfcc;
   }

   return record1->point - record2->point;
}

void buildmap_place_sort (void) {

   int i;
   int j;
   BuildMapPlace *one_place;

   if (SortedPlace != NULL) return; /* Sort was already performed. */

   buildmap_point_sort ();

   buildmap_info ("sorting places...");

   SortedPlace = malloc (PlaceCount * sizeof(int));
   if (SortedPlace == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   /*
    * because we have sorted the points above, this loop will get the
    * unsorted point_id for each place and replace it with the new
    * sorted point_id, so we keep the place items in valid.
    *
    * Also note that we load up SortPlace with sequential indexes to
    * the unsorted places. After the qsort() below this array will have
    * the indexes reordered by the sorted order.
    */
   
   for (i = 0; i < PlaceCount; ++i) {
      SortedPlace[i] = i;
      one_place = Place[i/BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);
      one_place->point = buildmap_point_get_sorted (one_place->point);
   }

   qsort (SortedPlace, PlaceCount, sizeof(int), buildmap_place_compare);

   /* 
    * Now that we have indexes to the places sorted by Place we can 
    * populate the ->sorted field with the appropriate index value.
    */
   for (i = 0; i < PlaceCount; ++i) {
      j = SortedPlace[i];
      one_place = Place[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);
      one_place->sorted = i;
   }

}


void buildmap_place_save (void) {

   int i;
   int j;
   int k;
   int square;
   int square_current;
   int cfcc_current;
   int square_count;

   BuildMapPlace *one_place;

   RoadMapPlace *db_places;
   RoadMapPlaceBySquare *db_square;

   buildmap_db *root;
   buildmap_db *data_table;
   buildmap_db *square_table;


   buildmap_info ("saving places...");

   square_count = buildmap_square_get_count();


   /* Create the database space */

   root = buildmap_db_add_section (NULL, "place");

   data_table = buildmap_db_add_section (root, "data");
   buildmap_db_add_data (data_table, PlaceCount, sizeof(RoadMapPlace));

   square_table = buildmap_db_add_section (root, "bysquare");
   buildmap_db_add_data (square_table,
                         square_count, sizeof(RoadMapPlaceBySquare));

   db_places  = (RoadMapPlace *) buildmap_db_get_data (data_table);
   db_square  = (RoadMapPlaceBySquare *) buildmap_db_get_data (square_table);


   for (i = 0; i < square_count; ++i) {
      for (k = 0; k < ROADMAP_CATEGORY_RANGE; k++) {
         db_square[i].first[k] = -1;
      }
   }


   square_current = -1;
   cfcc_current   = -1;

   for (i = 0; i < PlaceCount; ++i) {

      j = SortedPlace[i];

      one_place = Place[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      db_places[i] = one_place->point;

      square = buildmap_point_get_square_sorted (one_place->point);

      if (square != square_current) {

         if (square < square_current) {
            buildmap_fatal (0, "abnormal square order: %d following %d",
                               square, square_current);
         }
         if (square_current >= 0) {
            db_square[square_current].last = i - 1;
         }
         square_current = square;

         cfcc_current = -1; /* Force cfcc change. */
      }

      if (one_place->cfcc != cfcc_current) {

         if (one_place->cfcc < cfcc_current) {
            buildmap_fatal (0, "abnormal cfcc order: %d following %d",
                               one_place->cfcc, cfcc_current);
         }
         if (one_place->cfcc <= 0 || 
                one_place->cfcc > BUILDMAP_MAX_PLACE_CFCC) {
            buildmap_fatal (0, "illegal cfcc value");
         }
         cfcc_current = one_place->cfcc;

         db_square[square].first[cfcc_current-1] = i;
      }
   }

   db_square[square_current].last = PlaceCount - 1;
}


void buildmap_place_summary (void) {

   fprintf (stderr,
            "-- place table statistics: %d places\n",
            PlaceCount);
}


void buildmap_place_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; ++i) {
      if (Place[i] != NULL) {
         free(Place[i]);
         Place[i] = NULL;
      }
   }

   free (SortedPlace);
   SortedPlace = NULL;

   PlaceCount = 0;
   PlaceTableSize = 0;

   PlaceByName = NULL;
}

