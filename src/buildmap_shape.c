/*
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
 */

/**
 * @file
 * @brief Build a shape table & index for RoadMap.
 *
 * Shapes are definitions of the points that make up a non-straight line.
 *
 * Remember : a line is a non-interrupted thing (e.g. no intersections in between),
 * but this makes it possible for it to have another shape than straight.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_db_shape.h"

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_line.h"
#include "buildmap_shape.h"
#include "buildmap_square.h"

/**
 * @brief
 */
typedef struct {

   int line;
   unsigned int sequence;

   int longitude;
   int latitude;

} BuildMapShape;

static int ShapeCount = 0;
static int ShapeLineCount = 0;

static int ShapeMaxLine = 0;
static int ShapeMaxSequence = 0;

static BuildMapShape *Shape[BUILDMAP_BLOCK] = {NULL};

static RoadMapHash *ShapeByLine   = NULL;

static int ShapeAddCount = 0;

static int *SortedShape = NULL;


static void buildmap_shape_register (void);

/**
 * @brief
 */
static void buildmap_shape_initialize (void) {

   ShapeByLine = roadmap_hash_new ("ShapeByLine", BUILDMAP_BLOCK);

   ShapeMaxLine = 0;

   ShapeAddCount = 0;
   ShapeCount = 0;
   ShapeLineCount = 0;
   ShapeMaxSequence = 0;

   buildmap_shape_register();
}

/**
 * @brief Add a shape entry
 * @param line
 * @param irec
 * @param uid
 * @param sequence
 * @param longitude
 * @param latitude
 * @return
 */
int buildmap_shape_add (int line, int irec, int uid, int sequence, int longitude, int latitude)
{
   int index;
   int line_exists;
   int block;
   int offset;
   BuildMapShape *this_shape;

   if (ShapeByLine == NULL) buildmap_shape_initialize ();

   ShapeAddCount += 1;

   /* First search if that shape is not known yet. */

   line_exists = 0;

   for (index = roadmap_hash_get_first (ShapeByLine, line);
        index >= 0;
        index = roadmap_hash_get_next (ShapeByLine, index)) {

      this_shape = Shape[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      if (this_shape->line == line) {

         if (this_shape->sequence == (unsigned int)sequence) {

            if ((this_shape->longitude != longitude) ||
                (this_shape->latitude  != latitude )) {
               buildmap_error
                (0, "duplicated sequence number %d, irec %d, uid %d, %d/%d",
                        sequence, irec, uid, longitude, latitude);
            }

            return index;
         }
         line_exists = 1;
      }
   }

   buildmap_line_test_long (line, longitude, latitude);

   /* This shape was not known yet: create a new one. */

   block = ShapeCount / BUILDMAP_BLOCK;
   offset = ShapeCount % BUILDMAP_BLOCK;

   if (block >= BUILDMAP_BLOCK) {
      buildmap_fatal (0, "too many shape records");
   }

   if (Shape[block] == NULL) {

      /* We need to add a new block to the table. */

      Shape[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapShape));
      if (Shape[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
      roadmap_hash_resize (ShapeByLine, (block+1) * BUILDMAP_BLOCK);
   }

   this_shape = Shape[block] + offset;

   this_shape->line = line;
   this_shape->sequence = (unsigned int)sequence;
   this_shape->longitude = longitude;
   this_shape->latitude  = latitude;

   if (! line_exists) {

      ShapeLineCount += 1;

      if (line > ShapeMaxLine) {
         ShapeMaxLine = line;
      }

      if (line < 0) {
         buildmap_fatal (0, "negative line index");
      }
   }

   roadmap_hash_add (ShapeByLine, line, ShapeCount);

   if (sequence > ShapeMaxSequence) {

      ShapeMaxSequence = sequence;

   } else if (sequence < 0) {

      buildmap_fatal (0, "negative sequence index");
   }

   return ShapeCount++;
}

/**
 * @brief
 * @param r1
 * @param r2
 * @return
 */
static int buildmap_shape_compare (const void *r1, const void *r2) {

   int index1 = *((int *)r1);
   int index2 = *((int *)r2);

   BuildMapShape *record1;
   BuildMapShape *record2;

   record1 = Shape[index1/BUILDMAP_BLOCK] + (index1 % BUILDMAP_BLOCK);
   record2 = Shape[index2/BUILDMAP_BLOCK] + (index2 % BUILDMAP_BLOCK);

   if (record1->line != record2->line) {
      return record1->line - record2->line;
   }

   return record1->sequence - record2->sequence;
}

/**
 * @brief
 */
static void buildmap_shape_sort (void) {

   int i;

   if (ShapeCount == 0) return;

   if (SortedShape != NULL) return; /* Sort was already performed. */

   buildmap_info ("sorting shapes...");

   SortedShape = malloc (ShapeCount * sizeof(int));
   if (SortedShape == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   for (i = 0; i < ShapeCount; i++) {
      SortedShape[i] = i;
   }

   qsort (SortedShape, ShapeCount, sizeof(int), buildmap_shape_compare);
}

/**
 * @brief
 */
static void buildmap_shape_save (void) {

   int i;
   int j;
   int square;
   int last_line = -1;
   int line_index = -1;
   int shape_index;
   int last_square = -1;
   int square_count;
   int shape_count;

   int longitude = 0;
   int latitude = 0;

   RoadMapShape *db_shape;
   BuildMapShape *one_shape;
   RoadMapShapeByLine *db_byline;
   RoadMapShapeBySquare *db_bysquare;

   buildmap_db *root;
   buildmap_db *table_square;
   buildmap_db *table_line;
   buildmap_db *table_data;


   if (!ShapeCount) return;

   buildmap_info ("saving %d shapes...", ShapeCount);

   square_count = buildmap_square_get_count();

   /* Evaluate the number of shape records we need, counting for
    * additional mid-points when the distance is too great for
    * the relative position's range.
    */
   shape_count = ShapeCount;

   for (i = 0; i < ShapeCount; i++) {

      j = SortedShape[i];

      one_shape = Shape[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      if (one_shape->line != last_line) {

         buildmap_line_get_position_sorted
            (one_shape->line, &longitude, &latitude);

         last_line = one_shape->line;
      }

      while ((abs(one_shape->longitude - longitude) > 0x7fff) ||
             (abs(one_shape->latitude - latitude) > 0x7fff)) {

         short delta_longitude;
         short delta_latitude;


         if (one_shape->longitude == longitude) {

            delta_longitude = 0;

            if (one_shape->latitude - latitude > 0x7fff) {
               delta_latitude = 0x7fff;
            } else {
               delta_latitude = 0 - 0x7fff;
            }

         } else {

            double a = (1.0 * (one_shape->latitude - latitude))
                                  / (one_shape->longitude - longitude);

            if ((a <= 1) && (a >= -1)) {

               if (one_shape->longitude - longitude > 0x7fff) {
                  delta_longitude = 0x7fff;
                  delta_latitude  = (a * 0x7fff);
               } else {
                  delta_longitude = 0 - 0x7fff;
                  delta_latitude  = 0 - (short) (a * 0x7fff);
               }

            } else {

               if (one_shape->latitude - latitude > 0x7fff) {
                  delta_latitude = 0x7fff;
                  delta_longitude = (short) (0x7fff / a);
               } else {
                  delta_latitude = 0 - 0x7fff;
                  delta_longitude = 0 - (short) (0x7fff / a);
               }
            }
         }

         latitude  += delta_latitude;
         longitude += delta_longitude;

         shape_count += 1;
      }

      longitude = one_shape->longitude;
      latitude  = one_shape->latitude;
   }


   /* Create the database space. */

   root  = buildmap_db_add_section (NULL, "shape");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_square = buildmap_db_add_section (root, "bysquare");
   if (table_square == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_square,
                         square_count, sizeof(RoadMapShapeBySquare));

   table_line = buildmap_db_add_section (root, "byline");
   if (table_line == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_line,
                         ShapeLineCount, sizeof(RoadMapShapeByLine));

   table_data = buildmap_db_add_section (root, "data");
   if (table_data == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_data, shape_count, sizeof(RoadMapShape));

   db_bysquare = (RoadMapShapeBySquare *) buildmap_db_get_data (table_square);
   db_byline = (RoadMapShapeByLine *) buildmap_db_get_data (table_line);
   db_shape  = (RoadMapShape *) buildmap_db_get_data (table_data);


   last_line = -1;

   for (i = 0, shape_index = 0; i < ShapeCount; i++, shape_index++) {

      j = SortedShape[i];

      one_shape = Shape[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      if (one_shape->line != last_line) {

         if (last_line > one_shape->line) {
            buildmap_fatal (0, "decreasing line order in shape table");
         }

         line_index += 1;
         db_byline[line_index].line = one_shape->line;
         db_byline[line_index].first = shape_index;
         db_byline[line_index].count = 0;

         buildmap_line_get_position_sorted
            (one_shape->line, &longitude, &latitude);

         last_line = one_shape->line;


         square = buildmap_line_get_square_sorted (one_shape->line);

         if (square != last_square) {

            if (square < last_square) {
               buildmap_fatal (0, "decreasing square order in shape table");
            }
            while (last_square < square) {
               last_square += 1;
               db_bysquare[last_square].first = line_index;
               db_bysquare[last_square].count = 0;
            }
         }
         db_bysquare[last_square].count += 1;
      }

      while ((abs(one_shape->longitude - longitude) > 0x7fff) ||
             (abs(one_shape->latitude - latitude) > 0x7fff)) {

         short delta_longitude;
         short delta_latitude;


         if (one_shape->longitude == longitude) {

            delta_longitude = 0;

            if (one_shape->latitude - latitude > 0x7fff) {
               delta_latitude = 0x7fff;
            } else {
               delta_latitude = 0 - 0x7fff;
            }

         } else {

            double a = (1.0 * (one_shape->latitude - latitude))
                                  / (one_shape->longitude - longitude);

            if ((a <= 1) && (a >= -1)) {

               if (one_shape->longitude - longitude > 0x7fff) {
                  delta_longitude = 0x7fff;
                  delta_latitude  = (a * 0x7fff);
               } else {
                  delta_longitude = 0 - 0x7fff;
                  delta_latitude  = 0 - (short) (a * 0x7fff);
               }

            } else {

               if (one_shape->latitude - latitude > 0x7fff) {
                  delta_latitude = 0x7fff;
                  delta_longitude = (short) (0x7fff / a);
               } else {
                  delta_latitude = 0 - 0x7fff;
                  delta_longitude = 0 - (short) (0x7fff / a);
               }
            }
         }

         db_byline[line_index].count += 1;

         db_shape[shape_index].delta_latitude = delta_latitude;
         db_shape[shape_index].delta_longitude = delta_longitude;

         latitude  += delta_latitude;
         longitude += delta_longitude;

         shape_index += 1;
      }

      db_byline[line_index].count += 1;

      db_shape[shape_index].delta_longitude =
          (short) (one_shape->longitude - longitude);
      db_shape[shape_index].delta_latitude =
          (short) (one_shape->latitude  - latitude);

      longitude = one_shape->longitude;
      latitude  = one_shape->latitude;
   }

   if (shape_index != shape_count) {
      buildmap_fatal (0, "inconsistent count of shapes: "
                            "total = %d, saved = %d",
                         shape_count, shape_index+1);
   }

   if (last_square >= square_count) {
      buildmap_fatal (0, "inconsistent count of squares: "
                            "total = %d, saved = %d",
                         square_count, last_square+1);
   }

   for (last_square += 1; last_square < square_count; last_square += 1) {
      db_bysquare[last_square].first = line_index;
      db_bysquare[last_square].count = 0;
   }

   if (line_index+1 != ShapeLineCount) {
      buildmap_fatal (0, "inconsistent count of lines: "
                            "total = %d, saved = %d",
                         ShapeLineCount, line_index+1);
   }
}

/**
 * @brief
 */
static void buildmap_shape_summary (void) {

   fprintf (stderr,
            "-- shape table: %d items, %d add, %d bytes used\n"
            "                %d lines (range %d), max %d points per line\n",
            ShapeCount, ShapeAddCount,
            (int) (ShapeCount * sizeof(RoadMapShape)
               + (ShapeMaxLine + 1) * sizeof(RoadMapShapeByLine)),
            ShapeLineCount, ShapeMaxLine, ShapeMaxSequence);
}

/**
 * @brief
 */
static void  buildmap_shape_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; i++) {
      if (Shape[i] != NULL) {
         free(Shape[i]);
         Shape[i] = NULL;
      }
   }

   free (SortedShape);
   SortedShape = NULL;

   ShapeCount = 0;
   ShapeLineCount = 0;

   ShapeMaxLine = 0;
   ShapeMaxSequence = 0;

   roadmap_hash_delete (ShapeByLine);
   ShapeByLine = NULL;

   ShapeAddCount = 0;
}

/**
 * @brief
 */
static buildmap_db_module BuildMapShapeModule = {
   "shape",
   buildmap_shape_sort,
   buildmap_shape_save,
   buildmap_shape_summary,
   buildmap_shape_reset
};

/**
 * @brief
 */
static void buildmap_shape_register (void) {
   buildmap_db_register (&BuildMapShapeModule);
}

