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

/**
 * @file
 * @brief Build a table of all points referenced in lines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_db_point.h"

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_square.h"

/**
 * @brief internal structure of the point database while building a map
 */
typedef struct {
   int longitude;
   int latitude;
   int sorted;
   int squareid;           /* Before sorting. */
   unsigned short square;  /* After sorting. */
} BuildMapPoint;


static int PointCount = 0;
static BuildMapPoint *Point[BUILDMAP_BLOCK] = {NULL};

static RoadMapHash *PointByPosition = NULL;

static int *SortedPoint = NULL;

static void buildmap_point_register (void);

/**
 * @brief initialize the point module
 */
static void buildmap_point_initialize (void) {

   PointByPosition =
      roadmap_hash_new ("PointByPosition", BUILDMAP_BLOCK);

   Point[0] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapPoint));
   if (Point[0] == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   PointCount = 0;

   buildmap_point_register();
}

/**
 * @brief add a point to buildmap's tables
 * @param longitude the longitude of this point
 * @param latitude the latitude of this point
 * @return the point count
 */
int buildmap_point_add (int longitude, int latitude) {

   int i;
   int block;
   int offset;
   BuildMapPoint *this_point;


   if (PointByPosition == NULL) buildmap_point_initialize();


   /* First check if the point is already known. */

   for (i = roadmap_hash_get_first (PointByPosition, longitude);
        i >= 0;
        i = roadmap_hash_get_next (PointByPosition, i)) {

      this_point = Point[i / BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);

      if ((this_point->latitude == latitude) &&
          (this_point->longitude == longitude)) {
          
         return i;
      }
   }


   /* This is a new point: create a new entry. */

   block = PointCount / BUILDMAP_BLOCK;
   offset = PointCount % BUILDMAP_BLOCK;

   if (block >= BUILDMAP_BLOCK) {
      buildmap_fatal (0,
         "Underdimensioned point table (block %d, BUILDMAP_BLOCK %d)",
	 block, BUILDMAP_BLOCK);
   }

   if (Point[block] == NULL) {

      /* We need to add a new block to the table. */

      Point[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapPoint));
      if (Point[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }

      roadmap_hash_resize (PointByPosition, (block+1) * BUILDMAP_BLOCK);
   }

   roadmap_hash_add (PointByPosition, longitude, PointCount);

   this_point = Point[block] + offset;

   this_point->longitude = longitude;
   this_point->latitude   = latitude;
   this_point->sorted = -1;
   this_point->square = -1;


   /* Compute the geographic limits of the area. This will be used later
    * to compute the list of squares.
    */

   buildmap_square_adjust_limits(longitude, latitude);

   return PointCount++;
}

/**
 * @brief get the id of a point
 * @param point the point to query
 * @return id
 */
static BuildMapPoint *buildmap_point_get (int pointid) {

   BuildMapPoint *this_point;

   if ((pointid < 0) || (pointid > PointCount)) {
      buildmap_fatal (0, "invalid point index %d", pointid);
   }

   this_point = Point[pointid/BUILDMAP_BLOCK] + (pointid % BUILDMAP_BLOCK);

   return this_point;
}

/**
 * @brief get the square of a point
 * @param point the point to query
 * @return square
 */
int buildmap_point_get_square (int pointid) {

   return buildmap_point_get(pointid)->square;
}

/**
 * @brief get the longitude of a point
 * @param point the point to query
 * @return longitude
 */
int buildmap_point_get_longitude (int pointid) {

   return buildmap_point_get(pointid)->longitude;
}

/**
 * @brief get the latitude of a point
 * @param point the point to query
 * @return latitude
 */
int buildmap_point_get_latitude  (int pointid) {

   return buildmap_point_get(pointid)->latitude;
}

/**
 * @brief get the id of a point
 * @param point the point to query
 * @return id
 */
int buildmap_point_get_sorted (int pointid) {

   if (SortedPoint == NULL) {
      buildmap_fatal (0, "points have not been sorted yet");
   }

   return buildmap_point_get(pointid)->sorted;
}

/**
 * @brief get the square of a sorted point
 * @param point the point to query
 * @return square
 */
int buildmap_point_get_square_sorted (int point) {

   if (SortedPoint == NULL) {
      buildmap_fatal (0, "points have not been sorted yet");
   }
   if ((point < 0) || (point > PointCount)) {
      buildmap_fatal (0, "invalid point index %d", point);
   }

   return buildmap_point_get(SortedPoint[point])->square;
}

/**
 * @brief get the longitude of a sorted point
 * @param point the point to query
 * @return longitude
 */
int  buildmap_point_get_longitude_sorted (int point) {

   if (SortedPoint == NULL) {
      buildmap_fatal (0, "points have not been sorted yet");
   }
   if ((point < 0) || (point > PointCount)) {
      buildmap_fatal (0, "invalid point index %d", point);
   }

   return buildmap_point_get(SortedPoint[point])->longitude;
}

/**
 * @brief get the latitude of a sorted point
 * @param point the point to query
 * @return latitude
 */
int  buildmap_point_get_latitude_sorted  (int point) {

   if (SortedPoint == NULL) {
      buildmap_fatal (0, "points have not been sorted yet");
   }
   if ((point < 0) || (point > PointCount)) {
      buildmap_fatal (0, "invalid point index %d", point);
   }

   return buildmap_point_get(SortedPoint[point])->latitude;
}

/**
 * @brief compare two points
 * @param r1 the first point
 * @param r2 the second point
 * @return 0 if equal
 */
static int buildmap_point_compare (const void *r1, const void *r2) {

   int result;
   int index1 = *((int *)r1);
   int index2 = *((int *)r2);

   BuildMapPoint *record1;
   BuildMapPoint *record2;

   record1 = Point[index1/BUILDMAP_BLOCK] + (index1 % BUILDMAP_BLOCK);
   record2 = Point[index2/BUILDMAP_BLOCK] + (index2 % BUILDMAP_BLOCK);


   /* group together the points that are in the same square. */

   if (record1->square != record2->square) {
      return record1->square - record2->square;
   }

   /* The two points are inside the same square: compare exact location. */

   result = record1->longitude - record2->longitude;

   if (result != 0) {
      return result;
   }

   return record1->latitude - record2->latitude;
}

/**
 * @brief sort the points
 */
void buildmap_point_sort (void) {

   int i;
   int j;
   BuildMapPoint *record;

   if (PointCount == 0) return;

   if (SortedPoint != NULL) return; /* Sort was already performed. */

   buildmap_info ("generating squares...");

   buildmap_square_initialize ();

   for (i = PointCount - 1; i >= 0; i--) {
      record = Point[i / BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);
      record->squareid =
         buildmap_square_add (record->longitude, record->latitude);
   }

   buildmap_square_sort ();

   for (i = PointCount - 1; i >= 0; i--) {
      record = Point[i / BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);
      record->square = buildmap_square_get_sorted (record->squareid);
   }


   buildmap_info ("sorting points...");

   SortedPoint = malloc (PointCount * sizeof(int));
   if (SortedPoint == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   for (i = 0; i < PointCount; i++) {
      SortedPoint[i] = i;
   }

   qsort (SortedPoint, PointCount, sizeof(int), buildmap_point_compare);

   for (i = 0; i < PointCount; i++) {
      j = SortedPoint[i];
      record = Point[j / BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);
      record->sorted = i;
   }
}

/**
 * @brief save points into the database file
 */
static void buildmap_point_save (void) {

   int i;
   int j;

   int square_count;
   int last_square = -1;
   int reference_longitude;
   int reference_latitude;

   BuildMapPoint *one_point;
   RoadMapPoint  *db_points;
   RoadMapPointBySquare *db_bysquare;

   
   buildmap_db *root;
   buildmap_db *table_data;
   buildmap_db *table_bysquare;

   if (!PointCount) return;

   buildmap_info ("saving %d points...", PointCount);

   square_count = buildmap_square_get_count();

   root = buildmap_db_add_section (NULL, "point");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_data = buildmap_db_add_section (root, "data");
   if (table_data == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_data, PointCount, sizeof(RoadMapPoint));

   table_bysquare = buildmap_db_add_section (root, "bysquare");
   if (table_bysquare == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data
      (table_bysquare, square_count, sizeof(RoadMapPointBySquare));

   db_points   = (RoadMapPoint *) buildmap_db_get_data (table_data);
   db_bysquare = (RoadMapPointBySquare *) buildmap_db_get_data (table_bysquare);


   for (i = 0; i < PointCount; i++) {

      j = SortedPoint[i];

      one_point = Point[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      if (one_point->square != last_square) {
         if (one_point->square != last_square + 1) {
            buildmap_fatal (0, "decreasing square order in point table");
         }
         last_square = one_point->square;
         db_bysquare[last_square].first = i;
         db_bysquare[last_square].count = 0;

         buildmap_square_get_reference_sorted
              (last_square, &reference_longitude, &reference_latitude);
      }
      db_bysquare[last_square].count += 1;

      db_points[i].longitude =
         (unsigned short) (one_point->longitude - reference_longitude);
      db_points[i].latitude =
         (unsigned short) (one_point->latitude - reference_latitude);
   }
}

/**
 * @brief print a summary for this module
 */
static void buildmap_point_summary (void) {

   fprintf (stderr, "-- point table statistics: %d points, %d bytes used\n",
                    PointCount, (int)(PointCount * sizeof(RoadMapPoint)));
}

/**
 * @brief remove all points
 */
static void buildmap_point_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; i++) {
      if (Point[i] != NULL) {
         free (Point[i]);
         Point[i] = NULL;
      }
   }

   if (SortedPoint != NULL) {
      free (SortedPoint);
      SortedPoint = NULL;
   }

   PointCount = 0;

   roadmap_hash_delete (PointByPosition);
   PointByPosition = NULL;

}

/**
 * @brief structure to register the point module
 */
static buildmap_db_module BuildMapPointModule = {
   "point",
   buildmap_point_sort,
   buildmap_point_save,
   buildmap_point_summary,
   buildmap_point_reset
};

/**
 * @brief register the point module with the buildmap application
 */
static void buildmap_point_register (void) {
   buildmap_db_register (&BuildMapPointModule);
}
