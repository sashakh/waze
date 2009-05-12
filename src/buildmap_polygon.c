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
 * @brief Build a line table & index for RoadMap.
 *
 * This module considers a single area to be described by a unique landmark
 * ID. Each area can be made of multiple polygons, but the name of the area
 * as well as the layer to be used is the same for all the polygons and
 * is defined with the landmark.
 *
 * Thus buildmap_polygon_add_landmark() should be called before
 * buildmap_polygon_add(), which should be called before
 * buildmap_polygon_add_line().
 *
 * The contour of each polygon is defined by a succession of lines. The
 * map file will be filled with the lines in an order that is convenient
 * for drawing. Imagine you would walk around the area following the
 * polygon countour: you would then walk over all lines in a certain
 * order: this is the sorting order there.
 *
 * Of course this order can lead to two solutions: either you walk clockwise
 * or counter clockwise. The code here choose on way arbitrarily. However
 * you can provide the lines in direction, but you must tell which one it is.
 * The direction is defined by the order of the "from" and "to" points: if
 * you were to walk from "from" to "to", would you have the area's polygon
 * on you right or on your left? That information is the purpose of the "side"
 * parameter.
 *
 * Note that polygons must be loaded before the lines. This is because a
 * line must be loaded if it is part of a polygon, even if its layer is
 * normally ignored (a line may not belong to the same layer as the polygon:
 * think of a city park which half bounding lines are streets and the other
 * half are property lines--RoadMap would normally not load property lines).
 *
 * The function buildmap_polygon_use_line() tells the line loading logic
 * which layer to use for this line (which is used only if the line was not
 * going to be loaded according to its own native layer).
 *
 * The other functions are generic functions, same as for the other table
 * modules.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_db_polygon.h"

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_point.h"
#include "buildmap_square.h"
#include "buildmap_line.h"
#include "buildmap_polygon.h"


typedef struct {

   int   landid;
   short name;
   char  cfcc;

} BuildMapLandmark;

typedef struct {

   RoadMapString name;
   RoadMapString cenid;
   int           polyid;

   char  cfcc;
   short square[4];

   int count;
   int sorted;

} BuildMapPolygon;

typedef struct {

   BuildMapPolygon *polygon;

   int   tlid;
   int   line;
   char  side;   /* left or right. */
   short square;

} BuildMapPolygonLine;


static int PolygonCount = 0;
static int PolygonLineCount = 0;
static int LandmarkCount = 0;

static BuildMapLandmark    *Landmark[BUILDMAP_BLOCK] = {NULL};
static BuildMapPolygon     *Polygon[BUILDMAP_BLOCK] = {NULL};
static BuildMapPolygonLine *PolygonLine[BUILDMAP_BLOCK] = {NULL};

static RoadMapHash *LandmarkById = NULL;
static RoadMapHash *PolygonByPolyid = NULL;
static RoadMapHash *PolygonLineById = NULL;

static int *SortedPolygon = NULL;
static int *SortedPolygonLine = NULL;


static void buildmap_polygon_register (void);


static BuildMapLandmark *buildmap_polygon_search_landmark (int landid) {

   int index;
   BuildMapLandmark *this_landmark;

   for (index = roadmap_hash_get_first (LandmarkById, landid);
        index >= 0;
        index = roadmap_hash_get_next (LandmarkById, index)) {

      this_landmark = Landmark[index / BUILDMAP_BLOCK]
                            + (index % BUILDMAP_BLOCK);

      if (this_landmark->landid == landid) {

         return this_landmark;
      }
   }

   return NULL;
}


static BuildMapPolygon *buildmap_polygon_search
                           (RoadMapString cenid, int polyid) {

   int index;
   BuildMapPolygon *this_polygon;

   for (index = roadmap_hash_get_first (PolygonByPolyid, polyid);
        index >= 0;
        index = roadmap_hash_get_next (PolygonByPolyid, index)) {

      this_polygon = Polygon[index / BUILDMAP_BLOCK]
                          + (index % BUILDMAP_BLOCK);

      if ((this_polygon->polyid == polyid) &&
          (this_polygon->cenid  == cenid)) {

         return this_polygon;
      }
   }

   return NULL;
}


static void buildmap_polygon_eliminate_zero_length_lines (void) {

   int i;
   int from;
   int to;
   BuildMapPolygonLine *this_line;

   for (i = 0; i < PolygonLineCount; i++) {

      this_line = PolygonLine[i/BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);

      buildmap_line_get_points_sorted (this_line->line, &from, &to);

      if (from == to) {
         this_line->polygon->count -= 1;
         this_line->polygon = NULL;
      }
   }
}


static void buildmap_polygon_explain_the_line_order_problem
               (void *table, int first, int end,
                BuildMapPolygonLine *problem_line, char *text) {

   char *side2text[2];
   char description[256];

   int from;
   int to;
   int index;
   int line;
   BuildMapPolygonLine *this_line;

   if (problem_line != NULL) {
      fprintf (stderr, "%s at line %d:\n", text, problem_line->tlid);
   }

   fprintf (stderr, "%-6s %11s %6s      %-24s %-24s\n",
            "SIDE", "TLID", "POLYID", "FROM", "TO");

   side2text[POLYGON_SIDE_LEFT] = "left";
   side2text[POLYGON_SIDE_RIGHT] = "right";

   for (line = first; line <= end; line++) {

      index = SortedPolygonLine[line];
      this_line = PolygonLine[index/BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      buildmap_line_get_points_sorted (this_line->line, &from, &to);

      snprintf (description, sizeof(description),
                "%-6s %11d %6d %12d %11d %12d %11d",
                side2text[(int)this_line->side],
                this_line->tlid,
                this_line->polygon->polyid,
                buildmap_point_get_longitude_sorted (from),
                buildmap_point_get_latitude_sorted (from),
                buildmap_point_get_longitude_sorted (to),
                buildmap_point_get_latitude_sorted (to));

      if (this_line == problem_line) {
         fprintf (stderr, "%s  <-- %s\n", description, text);
      } else {
         fprintf (stderr, "%s\n", description);
      }
   }
}

/* FIXME.  this is called with the endpoints of every line.  it
 * misses all of the shape points for the line, so the bounding
 * box is a poor approximation, at best.  "long lines" have the
 * same problem (but in practice, not as bad -- polygons tend to
 * have bigger bounding boxes than lines, even long ones).
 */
static void buildmap_polygon_adjust_bbox
        (RoadMapPolygon *polygon, int from, int to) {

      int x1, x2, y1, y2;

      x1 = buildmap_point_get_longitude_sorted (from);
      y1 = buildmap_point_get_latitude_sorted  (from);
      x2 = buildmap_point_get_longitude_sorted (to);
      y2 = buildmap_point_get_latitude_sorted  (to);

      if (x1 < polygon->west) {
         polygon->west = x1;
      } else if (x1 > polygon->east) {
         polygon->east = x1;
      }

      if (x2 < polygon->west) {
         polygon->west = x2;
      } else if (x2 > polygon->east) {
         polygon->east = x2;
      }

      if (y1 < polygon->south) {
         polygon->south = y1;
      } else if (y1 > polygon->north) {
         polygon->north = y1;
      }

      if (y2 < polygon->south) {
         polygon->south = y2;
      } else if (y2 > polygon->north) {
         polygon->north = y2;
      }
}

static void buildmap_polygon_fill_in_drawing_order
               (RoadMapPolygon *polygon, RoadMapPolygonLine *polylines) {

   int i;
   int line, end;
   int from, to;
   int index;
   int first, count;
   int start_point = -1;
   int next_point = -1;
   int match_point;
   BuildMapPolygonLine *this_line;
   BuildMapPolygonLine *other_line;

   first = roadmap_polygon_get_first(polygon);
   count = roadmap_polygon_get_count(polygon);

   polygon->west = polygon->south = 180000000;
   polygon->east = polygon->north = -180000000;

   end = first + count - 1;

   for (line = first; line < end; line++) {

      index = SortedPolygonLine[line];
      this_line = PolygonLine[index/BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      buildmap_line_get_points_sorted (this_line->line, &from, &to);

      buildmap_polygon_adjust_bbox(polygon, from, to);

      if (this_line->side == POLYGON_SIDE_LEFT) {
         /* draw from 'to' to 'from' */
         polylines[line] = -this_line->line;

         if (line == first) {
            start_point = to;
         } else if (next_point != to) {
            buildmap_polygon_explain_the_line_order_problem
               (polylines, first, end, this_line, "incoherent drawing order");

            buildmap_fatal (0, "incoherent drawing order at line %d",
                            this_line->tlid);
         }
         next_point = from;

      } else {
         /* draw from 'from' to 'to' */
         polylines[line] = this_line->line;

         if (line == first) {
            start_point = from;
         } else if (next_point != from) {
            buildmap_polygon_explain_the_line_order_problem
               (polylines, first, end, this_line, "incoherent drawing order");

            buildmap_fatal (0, "incoherent drawing order at line %d",
                            this_line->tlid);
         }
         next_point = to;
      }

      /* scan the rest of the lines, looking for the one that's next
       * in polygon order.
       */
      for (i = line+1; i <= end; i++) {

         index = SortedPolygonLine[i];
         other_line = PolygonLine[index / BUILDMAP_BLOCK]
                               + (index % BUILDMAP_BLOCK);

         buildmap_line_get_points_sorted (other_line->line, &from, &to);

         if (other_line->side == POLYGON_SIDE_LEFT) {
            match_point = to;
         } else {
            match_point = from;
         }

         if (match_point == next_point) {

            SortedPolygonLine[i] = SortedPolygonLine[line+1];
            SortedPolygonLine[line+1] = index;
            break;
         }
      }

      if (i > end) {
         /* FIXME: it seems the Tiger database's CENID/POLYID is not
          * really unique: the database contains sometime several polygons
          * with the same ID. Or is it a special kind of complicated polygon ?
          * (this happens with Forrest Lawn areas, for example).
          * In the mean time, the solution is to check if we did not completed
          * one polygon, then forget about the rest.
          */
         if (((this_line->side == POLYGON_SIDE_LEFT) &&
              (start_point != next_point)) ||
             ((this_line->side == POLYGON_SIDE_RIGHT) &&
              (start_point != next_point))) {

            buildmap_polygon_explain_the_line_order_problem
               (polylines, first, end, this_line, "cannot find the next line");

            buildmap_fatal (0, "cannot find the next line at line %d",
                            this_line->tlid);
         }
         buildmap_info("skipping disconnected polygon lines");
         end = line;
         this_line->polygon->count = end - first + 1;
         break;
      }
   }

   index = SortedPolygonLine[end];
   this_line = PolygonLine[index/BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

   buildmap_line_get_points_sorted (this_line->line, &from, &to);
   buildmap_polygon_adjust_bbox(polygon, from, to);

   /* check that the last line connects with the first */
   if (this_line->side == POLYGON_SIDE_LEFT) {
      if (start_point != from) {
         buildmap_polygon_explain_the_line_order_problem
            (polylines, first, end, this_line, "open polygon");

         buildmap_fatal (0, "open polygon at line %d",
                         this_line->tlid);
      }
      polylines[end] = -this_line->line;
   } else {
      if (start_point != to) {
         buildmap_polygon_explain_the_line_order_problem
            (polylines, first, end, this_line, "open polygon");

         buildmap_fatal (0, "open polygon at line %d",
                         this_line->tlid);
      }
      polylines[end] = this_line->line;
   }
}


static void buildmap_polygon_initialize (void) {

   LandmarkById = roadmap_hash_new ("LandmarkById", BUILDMAP_BLOCK);
   PolygonByPolyid = roadmap_hash_new ("PolygonByPolyid", BUILDMAP_BLOCK);
   PolygonLineById = roadmap_hash_new ("PolygonLineById", BUILDMAP_BLOCK);

   Polygon[0] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapPolygon));
   if (Polygon[0] == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   PolygonLine[0] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapPolygonLine));
   if (PolygonLine[0] == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   Landmark[0] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapLandmark));
   if (Landmark[0] == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   PolygonCount = 0;
   PolygonLineCount = 0;
   LandmarkCount = 0;

   buildmap_polygon_register();
}


int  buildmap_polygon_add_landmark
        (int landid, int cfcc, RoadMapString name) {

   int block;
   int offset;
   BuildMapLandmark *this_landmark;


   if (LandmarkById == NULL) buildmap_polygon_initialize ();


   if (cfcc < 0 || cfcc > 255) {
      buildmap_fatal (0, "invalid cfcc");
   }

   block = LandmarkCount / BUILDMAP_BLOCK;
   offset = LandmarkCount % BUILDMAP_BLOCK;

   if (block >= BUILDMAP_BLOCK) {
      buildmap_fatal (0,
         "Underdimensioned landmark table (block %d, BUILDMAP_BLOCK %d)",
	 block, BUILDMAP_BLOCK);
   }

   if (Landmark[block] == NULL) {

      /* We need to add a new block to the table. */

      Landmark[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapLandmark));
      if (Landmark[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }

      roadmap_hash_resize (LandmarkById, (block+1) * BUILDMAP_BLOCK);
   }

   this_landmark = Landmark[block] + offset;

   this_landmark->landid = landid;
   this_landmark->cfcc   = cfcc;
   this_landmark->name   = name;

   roadmap_hash_add (LandmarkById, landid, LandmarkCount);

   return LandmarkCount++;
}


int  buildmap_polygon_add (int landid, RoadMapString cenid, int polyid) {

   int block;
   int offset;
   BuildMapPolygon *this_polygon;
   BuildMapLandmark *this_landmark;


   if (LandmarkById == NULL) buildmap_polygon_initialize ();


   this_landmark = buildmap_polygon_search_landmark (landid);
   if (this_landmark == NULL) {
      return -1;
   }

   block = PolygonCount / BUILDMAP_BLOCK;
   offset = PolygonCount % BUILDMAP_BLOCK;

   if (Polygon[block] == NULL) {

      /* We need to add a new block to the table. */

      Polygon[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapPolygon));
      if (Polygon[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }

      roadmap_hash_resize (PolygonByPolyid, (block+1) * BUILDMAP_BLOCK);
   }

   this_polygon = Polygon[block] + offset;

   this_polygon->cenid  = cenid;
   this_polygon->polyid = polyid;

   this_polygon->cfcc = this_landmark->cfcc;
   this_polygon->name = this_landmark->name;
   this_polygon->square[0] = -1;
   this_polygon->square[1] = -1;
   this_polygon->square[2] = -1;
   this_polygon->square[3] = -1;

   this_polygon->count = 0;

   roadmap_hash_add (PolygonByPolyid, polyid, PolygonCount);

   return PolygonCount++;
}


int  buildmap_polygon_add_line
        (RoadMapString cenid, int polyid, int tlid, int side) {

   int block;
   int offset;
   BuildMapPolygon *this_polygon;
   BuildMapPolygonLine *this_line;


   if (LandmarkById == NULL) buildmap_polygon_initialize ();

   this_polygon = buildmap_polygon_search (cenid, polyid);
   if (this_polygon == NULL) {
      return -1;
   }

   block = PolygonLineCount / BUILDMAP_BLOCK;
   offset = PolygonLineCount % BUILDMAP_BLOCK;

   if (PolygonLine[block] == NULL) {

      /* We need to add a new block to the table. */

      PolygonLine[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapPolygonLine));
      if (PolygonLine[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }

      roadmap_hash_resize (PolygonLineById, (block+1) * BUILDMAP_BLOCK);
   }

   this_line = PolygonLine[block] + offset;

   this_line->polygon = this_polygon;
   this_line->tlid    = tlid;
   this_line->side    = side;

   roadmap_hash_add (PolygonLineById, tlid, PolygonLineCount);
   this_polygon->count += 1;

   return PolygonLineCount++;
}


static int buildmap_polygon_compare (const void *r1, const void *r2) {

   int i;
   int index1 = *((int *)r1);
   int index2 = *((int *)r2);

   BuildMapPolygon *record1;
   BuildMapPolygon *record2;

   record1 = Polygon[index1/BUILDMAP_BLOCK] + (index1 % BUILDMAP_BLOCK);
   record2 = Polygon[index2/BUILDMAP_BLOCK] + (index2 % BUILDMAP_BLOCK);


   /* Empty polygons are moved to the end, to be removed later. */
   if (record1->count <= 1) {
      if (record2->count > 1) {
         return 1;
      }
   } else if (record2->count <= 1) {
      return -1;
   }

   /* The polygons are first sorted by square.
    * Within a square, polygons are sorted by category.
    */

   if (record1->square[0] != record2->square[0]) {
      return record1->square[0] - record2->square[0];
   }

   if (record1->cfcc != record2->cfcc) {
      return record1->cfcc - record2->cfcc;
   }

   for (i = 1; i < 4; i++) {

      if (record1->square[i] != record2->square[i]) {
         return record1->square[i] - record2->square[i];
      }
      if (record1->square[i] < 0) break;
   }

   return 0;
}


int buildmap_polygon_use_line (int tlid) {

   int index;
   BuildMapPolygonLine *this_line;


   if (LandmarkById == NULL) return 0;

   for (index = roadmap_hash_get_first (PolygonLineById, tlid);
        index >= 0;
        index = roadmap_hash_get_next (PolygonLineById, index)) {

      this_line = PolygonLine[index / BUILDMAP_BLOCK]
                           + (index % BUILDMAP_BLOCK);

      if (this_line->tlid == tlid) {
         return this_line->polygon->cfcc;
      }
   }

   return 0;
}


static int buildmap_polygon_compare_lines (const void *r1, const void *r2) {

   int index1 = *((int *)r1);
   int index2 = *((int *)r2);

   BuildMapPolygonLine *record1;
   BuildMapPolygonLine *record2;

   record1 = PolygonLine[index1/BUILDMAP_BLOCK] + (index1 % BUILDMAP_BLOCK);
   record2 = PolygonLine[index2/BUILDMAP_BLOCK] + (index2 % BUILDMAP_BLOCK);


   /* The lines are first sorted by polygons, then by square. */

   if (record1->polygon != record2->polygon) {

      if (record1->polygon == NULL) {
         return 1;
      }
      if (record2->polygon == NULL) {
         return -1;
      }
      return record1->polygon->sorted - record2->polygon->sorted;
   }

   if (record1->square != record2->square) {
      return record1->square - record2->square;
   }

   if (record1->polygon == NULL) {
      return 0;
   }
   if (record1->polygon->cfcc != record2->polygon->cfcc) {
      return record1->polygon->cfcc - record2->polygon->cfcc;
   }

   return 0;
}


static void buildmap_polygon_sort (void) {

   int i;
   int j;
   int k;
   int first_empty_polygon;
   int first_unused_line;
   BuildMapPolygon *one_polygon;
   BuildMapPolygonLine *one_line;

   if (!PolygonLineCount || !PolygonCount) return;

   if (SortedPolygon != NULL) return; /* Sort was already performed. */

   buildmap_point_sort ();
   buildmap_line_sort ();


   buildmap_info ("retrieving lines and squares...");

   for (i = 0; i < PolygonLineCount; i++) {

      one_line = PolygonLine[i/BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);

      one_line->line = buildmap_line_find_sorted (one_line->tlid);
      if (one_line->line < 0) {
         buildmap_fatal
            (0, "cannot find any line with TLID = %d", one_line->tlid);
      }
   }

   buildmap_polygon_eliminate_zero_length_lines ();


   /* Retrieve which squares each polygon fits in. */

   for (i = 0; i < PolygonLineCount; i++) {

      one_line = PolygonLine[i/BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);

      /* this NULL may be the result of pruning that happened in
       * eliminate_zero_length_lines(), above.
       */
      if (one_line->polygon == NULL) continue;

      one_line->square = buildmap_line_get_square_sorted (one_line->line);
      one_polygon = one_line->polygon;

      for (j = 0; j < 4; j++) {

         if (one_polygon->square[j] == one_line->square) break;

         if (one_polygon->square[j] < 0) {
            one_polygon->square[j] = one_line->square;
            break;
         }
      }
   }


   /* Sort the list of squares in each polygon. */

   buildmap_info ("sorting polygons' squares...");

   for (i = 0; i < PolygonCount; i++) {

      one_polygon = Polygon[i/BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);

      for (j = 0; j < 3; j++) {

         for (k = j + 1; k < 4; k++) {

            if (one_polygon->square[j] > one_polygon->square[k]) {

               short temp = one_polygon->square[j];

               one_polygon->square[j] = one_polygon->square[k];
               one_polygon->square[k] = temp;
            }
         }
      }
   }

   buildmap_info ("sorting polygons...");

   SortedPolygon = malloc (PolygonCount * sizeof(int));
   if (SortedPolygon == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   for (i = 0; i < PolygonCount; i++) {
      SortedPolygon[i] = i;
   }

   qsort (SortedPolygon, PolygonCount, sizeof(int), buildmap_polygon_compare);

   first_empty_polygon = PolygonCount;

   for (i = 0; i < PolygonCount; i++) {

      j = SortedPolygon[i];
      one_polygon = Polygon[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);
      one_polygon->sorted = i;

      if (one_polygon->count <= 1) {
         first_empty_polygon = i;
         break;
      }
   }

   for (i = first_empty_polygon; i < PolygonCount; i++) {

      j = SortedPolygon[i];
      one_polygon = Polygon[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);
      one_polygon->sorted = i;

      if (one_polygon->count > 1) {
         buildmap_fatal (0, "invalid polygon sort");
      }
   }

   PolygonCount = first_empty_polygon;


   buildmap_info ("sorting polygon lines...");

   SortedPolygonLine = malloc (PolygonLineCount * sizeof(int));
   if (SortedPolygonLine == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   for (i = 0; i < PolygonLineCount; i++) {
      SortedPolygonLine[i] = i;
   }

   qsort (SortedPolygonLine, PolygonLineCount,
          sizeof(int), buildmap_polygon_compare_lines);

   first_unused_line = PolygonLineCount;

   for (i = 0; i < PolygonLineCount; i++) {

      j = SortedPolygonLine[i];
      one_line = PolygonLine[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      if (one_line->polygon == NULL) {
         first_unused_line = i;
         break;
      }
      if (one_line->polygon->sorted >= first_empty_polygon) {
         first_unused_line = i;
         break;
      }
   }

   for (i = first_unused_line; i < PolygonLineCount; i++) {

      j = SortedPolygonLine[i];
      one_line = PolygonLine[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      if (one_line->polygon == NULL) continue;
      if (one_line->polygon->sorted >= first_empty_polygon) continue;

      buildmap_fatal (0, "invalid polygon line sort");
   }
   PolygonLineCount = first_unused_line;
}


static void buildmap_polygon_save (void) {

   int i;
   int j;
   int square;
   int square_current;
   int polygon_current;

   BuildMapPolygon *one_polygon;
   BuildMapPolygonLine *one_line;

   RoadMapPolygon *db_head, *db_poly;
   RoadMapPolygonLine *db_line;

   buildmap_db *root;
   buildmap_db *head_table;
   buildmap_db *point_table;
   buildmap_db *line_table;


   buildmap_info ("saving polygons...");

   /* Create empty old-style "polygon" tables, to satisfy old
    * RoadMap code that might be asked to use this db.  (without this,
    * old code will segv.)
    */
   root = buildmap_db_add_section (NULL, "polygon");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   head_table = buildmap_db_add_section (root, "head");
   if (head_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (head_table, 0, sizeof(RoadMapPolygon));

   point_table = buildmap_db_add_section (root, "point");
   if (point_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (point_table, 0, sizeof(RoadMapPolygonPoint));


   /* Create the new-style "polygons" (note new name) tables,
    * based on lines, instead of points).
    */
   if (PolygonLineCount > 0xffff) {
      buildmap_fatal (0, "too many polygon lines");
   }

   root = buildmap_db_add_section (NULL, "polygons");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   head_table = buildmap_db_add_section (root, "head");
   if (head_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (head_table, PolygonCount, sizeof(RoadMapPolygon));

   line_table = buildmap_db_add_section (root, "line");
   if (line_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (line_table,
                         PolygonLineCount, sizeof(RoadMapPolygonLine));

   db_head   = (RoadMapPolygon *)     buildmap_db_get_data (head_table);
   db_line   = (RoadMapPolygonLine *) buildmap_db_get_data (line_table);

   square_current = -1;
   polygon_current = -1;
   db_poly = NULL;

   for (i = 0; i < PolygonLineCount; i++) {

      j = SortedPolygonLine[i];

      one_line = PolygonLine[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);
      one_polygon = one_line->polygon;

      if (one_polygon == NULL) {
         buildmap_fatal (0, "invalid line was not removed");
      }

      if (one_polygon->sorted != polygon_current) {

         if (one_polygon->sorted < polygon_current) {
            buildmap_fatal (0, "abnormal polygon order: %d following %d",
                               one_polygon->sorted, polygon_current);
         }

         if (polygon_current >= 0) {

            if (roadmap_polygon_get_count(db_poly) <= 1) {
               buildmap_fatal (0, "empty polygon");
            }

            buildmap_polygon_fill_in_drawing_order (db_poly, db_line);

         }

         polygon_current = one_polygon->sorted;
         db_poly = &db_head[polygon_current];

         db_poly->name  = one_polygon->name;
         buildmap_polygon_set_first(db_poly, i);
         db_poly->cfcc  = one_polygon->cfcc;

         if (one_polygon->count > 0xfffff) {
            buildmap_fatal (0, "too many polygon lines");
         }
         buildmap_polygon_set_count(db_poly, one_polygon->count);

         square = one_polygon->square[0];

         if (square != square_current) {

            if (square < square_current) {
               buildmap_fatal (0, "abnormal square order: %d following %d",
                                  square, square_current);
            }
            square_current = square;
         }
      }
   }

   if (polygon_current >= 0) {

      buildmap_polygon_fill_in_drawing_order (db_poly, db_line);

   }
}


static void buildmap_polygon_summary (void) {

   fprintf (stderr,
            "-- polygon table statistics: %d polygons, %d lines\n",
            PolygonCount, PolygonLineCount);
}


static void buildmap_polygon_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; i++) {

      if (PolygonLine[i] != NULL) {
         free(PolygonLine[i]);
         PolygonLine[i] = NULL;
      }
      if (Polygon[i] != NULL) {
         free(Polygon[i]);
         Polygon[i] = NULL;
      }
      if (Landmark[i] != NULL) {
         free(Landmark[i]);
         Landmark[i] = NULL;
      }
   }

   free (SortedPolygonLine);
   SortedPolygonLine = NULL;

   free (SortedPolygon);
   SortedPolygon = NULL;

   LandmarkCount = 0;
   PolygonCount  = 0;
   PolygonLineCount = 0;

   roadmap_hash_delete (LandmarkById);
   LandmarkById = NULL;

   roadmap_hash_delete (PolygonByPolyid);
   PolygonByPolyid = NULL;

   roadmap_hash_delete (PolygonLineById);
   PolygonLineById = NULL;
}


static buildmap_db_module BuildMapPolygonModule = {
   "polygons",
   buildmap_polygon_sort,
   buildmap_polygon_save,
   buildmap_polygon_summary,
   buildmap_polygon_reset
}; 
      
         
static void buildmap_polygon_register (void) {
   buildmap_db_register (&BuildMapPolygonModule);
}

