/* rdmxchange_shape.c - export the map shape points.
 *
 * LICENSE:
 *
 *   Copyright 2006 Pascal F. Martin
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

#include "rdmxchange.h"

#include "roadmap_db_shape.h"


static char *RoadMapShapeType = "RoadMapShapeContext";

typedef struct {

   char *type;

   RoadMapShape *Shape;
   int           ShapeCount;

   RoadMapShapeByLine *ShapeByLine;
   int                 ShapeByLineCount;

   RoadMapShapeBySquare *ShapeBySquare;
   int                   ShapeBySquareCount;

} RoadMapShapeContext;

static RoadMapShapeContext *RoadMapShapeActive = NULL;


static void rdmxchange_shape_register_export (void); 


static void *rdmxchange_shape_map (roadmap_db *root) {

   RoadMapShapeContext *context;

   roadmap_db *shape_table;
   roadmap_db *line_table;
   roadmap_db *square_table;


   context = malloc(sizeof(RoadMapShapeContext));
   roadmap_check_allocated(context);

   context->type = RoadMapShapeType;

   shape_table  = roadmap_db_get_subsection (root, "data");
   line_table   = roadmap_db_get_subsection (root, "byline");
   square_table = roadmap_db_get_subsection (root, "bysquare");

   context->Shape = (RoadMapShape *) roadmap_db_get_data (shape_table);
   context->ShapeCount = roadmap_db_get_count (shape_table);

   if (roadmap_db_get_size (shape_table) !=
       context->ShapeCount * sizeof(RoadMapShape)) {
      roadmap_log (ROADMAP_FATAL, "invalid shape/data structure");
   }

   context->ShapeByLine =
      (RoadMapShapeByLine *) roadmap_db_get_data (line_table);
   context->ShapeByLineCount = roadmap_db_get_count (line_table);

   if (roadmap_db_get_size (line_table) !=
       context->ShapeByLineCount * sizeof(RoadMapShapeByLine)) {
      roadmap_log (ROADMAP_FATAL, "invalid shape/byline structure");
   }

   context->ShapeBySquare =
      (RoadMapShapeBySquare *) roadmap_db_get_data (square_table);
   context->ShapeBySquareCount = roadmap_db_get_count (square_table);

   if (roadmap_db_get_size (square_table) !=
       context->ShapeBySquareCount * sizeof(RoadMapShapeBySquare)) {
      roadmap_log (ROADMAP_FATAL, "invalid shape/bysquare structure");
   }

   rdmxchange_shape_register_export();

   return context;
}

static void rdmxchange_shape_activate (void *context) {

   RoadMapShapeContext *shape_context = (RoadMapShapeContext *) context;

   if (shape_context != NULL) {

      if (shape_context->type != RoadMapShapeType) {
         roadmap_log (ROADMAP_FATAL, "cannot activate shape (bad type)");
      }
   }

   RoadMapShapeActive = shape_context;
}

static void rdmxchange_shape_unmap (void *context) {

   RoadMapShapeContext *shape_context = (RoadMapShapeContext *) context;

   if (shape_context->type != RoadMapShapeType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate shape (bad type)");
   }
   if (RoadMapShapeActive == shape_context) {
      RoadMapShapeActive = NULL;
   }
   free(shape_context);
}

roadmap_db_handler RoadMapShapeExport = {
   "shape",
   rdmxchange_shape_map,
   rdmxchange_shape_activate,
   rdmxchange_shape_unmap
};


static void rdmxchange_shape_export_head (FILE *file) {

   fprintf (file, "table shape/bysquare %d first count\n",
                  RoadMapShapeActive->ShapeBySquareCount);

   fprintf (file, "table shape/byline %d line first count\n",
                  RoadMapShapeActive->ShapeByLineCount);

   fprintf (file, "table shape/data %d longitude latitude\n",
                  RoadMapShapeActive->ShapeCount);
}


static void rdmxchange_shape_export_data (FILE *file) {

   int i;
   RoadMapShape *point;
   RoadMapShapeByLine *byline;
   RoadMapShapeBySquare *bysquare;


   fprintf (file, "table shape/bysquare\n");
   bysquare = RoadMapShapeActive->ShapeBySquare;

   for (i = 0; i < RoadMapShapeActive->ShapeBySquareCount; ++i, ++bysquare) {
      fprintf (file, "%d,%d\n", bysquare->first, bysquare->count);
   }
   fprintf (file, "\n");


   fprintf (file, "table shape/byline\n");
   byline = RoadMapShapeActive->ShapeByLine;

   for (i = 0; i < RoadMapShapeActive->ShapeByLineCount; ++i, ++byline) {
      fprintf (file, "%d,%d,%d\n", byline->line, byline->first, byline->count);
   }
   fprintf (file, "\n");


   fprintf (file, "table shape/data\n");
   point = RoadMapShapeActive->Shape;

   for (i = 0; i < RoadMapShapeActive->ShapeCount; ++i, ++point) {
      fprintf (file, "%d,%d\n", point->delta_longitude, point->delta_latitude);
   }
   fprintf (file, "\n");
}


static RdmXchangeExport RdmXchangeShapeExport = {
   "shape",
   rdmxchange_shape_export_head,
   rdmxchange_shape_export_data
};

static void rdmxchange_shape_register_export (void) {
   rdmxchange_main_register_export (&RdmXchangeShapeExport);
}

