/* buildmap_point.c - Build a table of all points referenced in lines.
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
 *   void buildmap_point_initialize (void);
 *   int  buildmap_point_add        (int longitude, int latitude);
 *
 *   void buildmap_point_sort (void);
 *   int  buildmap_point_get_square (int pointid);
 *   int  buildmap_point_get_longitude (int pointid);
 *   int  buildmap_point_get_latitude  (int pointid);
 *   int  buildmap_point_get_sorted (int pointid);
 *   int  buildmap_point_get_longitude_sorted (int point);
 *   int  buildmap_point_get_latitude_sorted  (int point);
 *   int  buildmap_point_get_square_sorted (int pointid);
 *   void buildmap_point_save    (void);
 *   void buildmap_point_summary (void);
 *   void buildmap_point_reset   (void);
 *
 * These functions are used to build a table of lines from
 * the Tiger maps. The objective is double: (1) reduce the size of
 * the Tiger data by sharing all duplicated information and
 * (2) produce the index data to serve as the basis for a fast
 * search mechanism for streets in roadmap.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_db_point.h"

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_square.h"


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

static int SortMaxLongitude = -0x7fffffff;
static int SortMinLongitude =  0x7fffffff;
static int SortMaxLatitude  = -0x7fffffff;
static int SortMinLatitude  =  0x7fffffff;


void buildmap_point_initialize (void) {

   PointByPosition =
      roadmap_hash_new ("PointByPosition", BUILDMAP_BLOCK);

   Point[0] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapPoint));
   if (Point[0] == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   PointCount = 0;

   SortMaxLongitude = -0x7fffffff;
   SortMinLongitude =  0x7fffffff;
   SortMaxLatitude  = -0x7fffffff;
   SortMinLatitude  =  0x7fffffff;
}


int buildmap_point_add (int longitude, int latitude) {

   int i;
   int block;
   int offset;
   BuildMapPoint *this_point;


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

   if (longitude < SortMinLongitude) {
      SortMinLongitude = longitude;
   }
   if (longitude > SortMaxLongitude) {
      SortMaxLongitude = longitude;
   }

   if (latitude < SortMinLatitude) {
      SortMinLatitude = latitude;
   }
   if (latitude > SortMaxLatitude) {
      SortMaxLatitude = latitude;
   }

   return PointCount++;
}


static BuildMapPoint *buildmap_point_get (int pointid) {

   BuildMapPoint *this_point;

   if ((pointid < 0) || (pointid > PointCount)) {
      buildmap_fatal (0, "invalid point index %d", pointid);
   }

   this_point = Point[pointid/BUILDMAP_BLOCK] + (pointid % BUILDMAP_BLOCK);

   return this_point;
}


int buildmap_point_get_square (int pointid) {

   return buildmap_point_get(pointid)->square;
}


int buildmap_point_get_longitude (int pointid) {

   return buildmap_point_get(pointid)->longitude;
}


int buildmap_point_get_latitude  (int pointid) {

   return buildmap_point_get(pointid)->latitude;
}


int buildmap_point_get_sorted (int pointid) {

   if (SortedPoint == NULL) {
      buildmap_fatal (0, "points have not been sorted yet");
   }

   return buildmap_point_get(pointid)->sorted;
}


int buildmap_point_get_square_sorted (int point) {

   if (SortedPoint == NULL) {
      buildmap_fatal (0, "points have not been sorted yet");
   }
   if ((point < 0) || (point > PointCount)) {
      buildmap_fatal (0, "invalid point index %d", point);
   }

   return buildmap_point_get(SortedPoint[point])->square;
}


int  buildmap_point_get_longitude_sorted (int point) {

   if (SortedPoint == NULL) {
      buildmap_fatal (0, "points have not been sorted yet");
   }
   if ((point < 0) || (point > PointCount)) {
      buildmap_fatal (0, "invalid point index %d", point);
   }

   return buildmap_point_get(SortedPoint[point])->longitude;
}


int  buildmap_point_get_latitude_sorted  (int point) {

   if (SortedPoint == NULL) {
      buildmap_fatal (0, "points have not been sorted yet");
   }
   if ((point < 0) || (point > PointCount)) {
      buildmap_fatal (0, "invalid point index %d", point);
   }

   return buildmap_point_get(SortedPoint[point])->latitude;
}


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

void buildmap_point_sort (void) {

   int i;
   int j;
   BuildMapPoint *record;

   if (SortedPoint != NULL) return; /* Sort was already performed. */

   buildmap_info ("generating squares...");

   buildmap_square_initialize (SortMinLongitude, SortMaxLongitude,
                               SortMinLatitude,  SortMaxLatitude);

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


void buildmap_point_save (void) {

   int i;
   int j;

   int last_square = -1;
   int reference_longitude;
   int reference_latitude;

   BuildMapPoint *one_point;
   RoadMapPoint  *db_points;
   RoadMapSortedList *db_bysquare;

   
   buildmap_db *root;
   buildmap_db *table_data;
   buildmap_db *table_bysquare;


   buildmap_info ("saving points...");

   root = buildmap_db_add_section (NULL, "point");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_data = buildmap_db_add_child
                  (root, "data", PointCount, sizeof(RoadMapPoint));

   table_bysquare = buildmap_db_add_child
                     (root,
                      "bysquare",
                      buildmap_square_get_count(),
                      sizeof(RoadMapSortedList));

   db_points   = (RoadMapPoint *) buildmap_db_get_data (table_data);
   db_bysquare = (RoadMapSortedList *) buildmap_db_get_data (table_bysquare);


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


void buildmap_point_summary (void) {

   fprintf (stderr, "-- point table statistics: %d points, %d bytes used\n",
                    PointCount, PointCount * sizeof(RoadMapPoint));
}


void buildmap_point_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; i++) {
      if (Point[i] != NULL) {
         free (Point[i]);
         Point[i] = NULL;
      }
   }

   free (SortedPoint);
   SortedPoint = NULL;

   PointCount = 0;

   PointByPosition = NULL;

   SortMaxLongitude = -0x7fffffff;
   SortMinLongitude =  0x7fffffff;
   SortMaxLatitude  = -0x7fffffff;
   SortMinLatitude  =  0x7fffffff;
}

