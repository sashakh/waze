/* rdmxchange_square.c - Export a map area, divided in small squares.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"

#include "rdmxchange.h"

#include "roadmap_db_square.h"

static char *RoadMapSquareType = "RoadMapSquareContext";


typedef struct {

   char *type;

   RoadMapGlobal *SquareGlobal;
   RoadMapSquare *Square;

   int SquareCount;

} RoadMapSquareContext;

static RoadMapSquareContext *RoadMapSquareActive = NULL;

static void rdmxchange_square_register_export (void);


static void *rdmxchange_square_map (roadmap_db *root) {

   RoadMapSquareContext *context;

   roadmap_db *global_table;
   roadmap_db *square_table;


   context = malloc(sizeof(RoadMapSquareContext));
   roadmap_check_allocated(context);

   context->type = RoadMapSquareType;

   global_table  = roadmap_db_get_subsection (root, "global");
   square_table = roadmap_db_get_subsection (root, "data");

   context->SquareGlobal = (RoadMapGlobal *) roadmap_db_get_data (global_table);
   context->Square       = (RoadMapSquare *) roadmap_db_get_data (square_table);

   context->SquareCount  = roadmap_db_get_count (square_table);

   rdmxchange_square_register_export();

   return context;
}

static void rdmxchange_square_activate (void *context) {

   RoadMapSquareContext *square_context = (RoadMapSquareContext *) context;

   if ((square_context != NULL) &&
       (square_context->type != RoadMapSquareType)) {
      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   RoadMapSquareActive = square_context;
}

static void rdmxchange_square_unmap (void *context) {

   RoadMapSquareContext *square_context = (RoadMapSquareContext *) context;

   if (square_context->type != RoadMapSquareType) {
      roadmap_log(ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapSquareActive == square_context) {
      RoadMapSquareActive = NULL;
   }
   free(square_context);
}

roadmap_db_handler RoadMapSquareExport = {
   "square",
   rdmxchange_square_map,
   rdmxchange_square_activate,
   rdmxchange_square_unmap
};


static void rdmxchange_square_export_head (FILE *file) {

   fprintf (file, "table square/global 1\n");

   fprintf (file, "table square/data %d\n",
                  RoadMapSquareActive->SquareCount);
}


static void rdmxchange_square_export_data (FILE *file) {

   int i;
   RoadMapSquare *square;

   fprintf (file, "table square/global\n"
                  "edges.east,"
                  "edges.north,"
                  "edges.west,"
                  "edges.south,"
                  "step.longitude,"
                  "step.latitude,"
                  "squares.count\n");

   fprintf (file, "%.0d,%.0d,%.0d,%.0d,",
                   RoadMapSquareActive->SquareGlobal->edges.east,
                   RoadMapSquareActive->SquareGlobal->edges.north,
                   RoadMapSquareActive->SquareGlobal->edges.west,
                   RoadMapSquareActive->SquareGlobal->edges.south);

   fprintf (file, "%.0d,%.0d,%.0d,%.0d,%.0d\n\n",
                  RoadMapSquareActive->SquareGlobal->step_longitude,
                  RoadMapSquareActive->SquareGlobal->step_latitude,
                  RoadMapSquareActive->SquareGlobal->count_longitude,
                  RoadMapSquareActive->SquareGlobal->count_latitude,
                  RoadMapSquareActive->SquareGlobal->count_squares);


   fprintf (file, "table square/data\n"
                  "edges.east,"
                  "edges.north,"
                  "edges.west,"
                  "edges.south,"
                  "points.count,"
                  "position\n");
   square = RoadMapSquareActive->Square;

   for (i = 0; i < RoadMapSquareActive->SquareCount; ++i, ++square) {

      fprintf (file, "%.0d,%.0d,%.0d,%.0d,", square->edges.east,
                                             square->edges.north,
                                             square->edges.west,
                                             square->edges.south);

      fprintf (file, "%.0d,%.0d\n", square->count_points, square->position);
   }
   fprintf (file, "\n");
}


static RdmXchangeExport RdmXchangeSquareExport = {
   "square",
   rdmxchange_square_export_head,
   rdmxchange_square_export_data
};

static void rdmxchange_square_register_export (void) {

   rdmxchange_main_register_export (&RdmXchangeSquareExport);
}

