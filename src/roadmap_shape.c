/* roadmap_shape.c - Manage the tiger shape points.
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
 *   int  roadmap_shape_in_square (int square, int *first, int *last);
 *   int  roadmap_shape_of_line   (int line, int begin, int end,
 *                                 int *first, int *last);
 *   void roadmap_shape_get_position (int shape, RoadMapPosition *position);
 *
 * These functions are used to retrieve the shape points that belong to a line.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_db_shape.h"

#include "roadmap_line.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"


static char *RoadMapShapeType = "RoadMapShapeContext";

typedef struct {

   char *type;

   RoadMapShape *Shape;
   int           ShapeCount;

   RoadMapShapeByLine *ShapeByLine;
   int                 ShapeByLineCount;

   RoadMapShapeBySquare *ShapeBySquare;
   int                   ShapeBySquareCount;

   int *shape_cache;
   int  shape_cache_size;  /* This is the size in bits ! */

} RoadMapShapeContext;

static RoadMapShapeContext *RoadMapShapeActive = NULL;

static int RoadMapShape2Mask[8*sizeof(int)] = {0};


static void *roadmap_shape_map (roadmap_db *root) {

   int i;
   RoadMapShapeContext *context;

   roadmap_db *shape_table;
   roadmap_db *line_table;
   roadmap_db *square_table;


   for (i = 0; i < 8*sizeof(int); i++) {
      RoadMapShape2Mask[i] = 1 << i;
   }


   context = malloc(sizeof(RoadMapShapeContext));
   roadmap_check_allocated(context);

   context->type = RoadMapShapeType;

   shape_table  = roadmap_db_get_subsection (root, "data");
   line_table   = roadmap_db_get_subsection (root, "byline");
   square_table = roadmap_db_get_subsection (root, "bysquare");

   context->Shape = (RoadMapShape *) roadmap_db_get_data (shape_table);
   context->ShapeCount = roadmap_db_get_count (shape_table);

   if (context->ShapeCount !=
       roadmap_db_get_size (shape_table) / sizeof(RoadMapShape)) {
      roadmap_log (ROADMAP_FATAL, "invalid shape/data structure");
   }

   context->ShapeByLine =
      (RoadMapShapeByLine *) roadmap_db_get_data (line_table);
   context->ShapeByLineCount = roadmap_db_get_count (line_table);

   if (context->ShapeByLineCount !=
       roadmap_db_get_size (line_table) / sizeof(RoadMapShapeByLine)) {
      roadmap_log (ROADMAP_FATAL, "invalid shape/byline structure");
   }

   context->ShapeBySquare =
      (RoadMapShapeBySquare *) roadmap_db_get_data (square_table);
   context->ShapeBySquareCount = roadmap_db_get_count (square_table);

   if (context->ShapeBySquareCount !=
       roadmap_db_get_size (square_table) / sizeof(RoadMapShapeBySquare)) {
      roadmap_log (ROADMAP_FATAL, "invalid shape/bysquare structure");
   }

   context->shape_cache = NULL;
   context->shape_cache_size = 0;

   return context;
}

static void roadmap_shape_activate (void *context) {

   RoadMapShapeContext *shape_context = (RoadMapShapeContext *) context;

   if (shape_context->type != RoadMapShapeType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate shape (bad type)");
   }
   RoadMapShapeActive = shape_context;

   if (shape_context->shape_cache == NULL) {

      shape_context->shape_cache_size = roadmap_line_count();
      shape_context->shape_cache =
         calloc ((shape_context->shape_cache_size / (8 * sizeof(int))) + 1,
                 sizeof(int));
      roadmap_check_allocated(shape_context->shape_cache);
   }
}

static void roadmap_shape_unmap (void *context) {

   RoadMapShapeContext *shape_context = (RoadMapShapeContext *) context;

   if (shape_context->type != RoadMapShapeType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate shape (bad type)");
   }
   if (RoadMapShapeActive == shape_context) {
      RoadMapShapeActive = NULL;
   }
   if (shape_context->shape_cache != NULL) {
      free (shape_context->shape_cache);
   }
   free(shape_context);
}

roadmap_db_handler RoadMapShapeHandler = {
   "shape",
   roadmap_shape_map,
   roadmap_shape_activate,
   roadmap_shape_unmap
};



int  roadmap_shape_in_square (int square, int *first, int *last) {

   RoadMapShapeBySquare *ShapeBySquare;

   square = roadmap_square_index(square);

   if (square >= 0 && square < RoadMapShapeActive->ShapeBySquareCount) {

      ShapeBySquare = RoadMapShapeActive->ShapeBySquare;

      *first = ShapeBySquare[square].first;
      *last  = ShapeBySquare[square].first + ShapeBySquare[square].count - 1;

      return ShapeBySquare[square].count;
   }

   return 0;
}


int  roadmap_shape_of_line (int line, int begin, int end,
                                      int *first, int *last) {

   int middle = 0;
   RoadMapShapeByLine *shape_by_line;

   if (line >= 0 && line < RoadMapShapeActive->shape_cache_size) {

      int mask = RoadMapShapeActive->shape_cache[line / (8 * sizeof(int))];

      if (mask & RoadMapShape2Mask[line & ((8*sizeof(int))-1)]) {
         return 0;
      }
   }

   shape_by_line = RoadMapShapeActive->ShapeByLine;

   while (begin < end) {

      middle = (begin + end) / 2;

      if (line < shape_by_line[middle].line) {

         if (middle == end) break;
         end = middle;

      } else if (line > shape_by_line[middle].line) {

         if (middle == begin) break;
         begin = middle;

      } else {

         break;
      }
   }

   if (shape_by_line[middle].line == line) {

      *first = shape_by_line[middle].first;
      *last  = shape_by_line[middle].first + shape_by_line[middle].count - 1;

      return shape_by_line[middle].count;
   }

   RoadMapShapeActive->shape_cache[line / (8 * sizeof(int))] |=
      RoadMapShape2Mask[line & ((8*sizeof(int))-1)];

   return 0;
}


void roadmap_shape_get_position (int shape, RoadMapPosition *position) {

   position->longitude += RoadMapShapeActive->Shape[shape].delta_longitude;
   position->latitude  += RoadMapShapeActive->Shape[shape].delta_latitude;
}

