/* rdmxchange_point.c - Export the points that define tiger lines.
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

#include "roadmap_db_point.h"


typedef struct {

   char *type;

   RoadMapPoint *Point;
   int           PointCount;

   RoadMapPointBySquare *BySquare;
   int                   BySquareCount;

} RoadMapPointContext;

static RoadMapPointContext *RoadMapPointActive = NULL;

static char *RoadMapPointType = "RoadMapPointContext";


static void rdmxchange_point_register_export (void);


static void *rdmxchange_point_map (roadmap_db *root) {

   roadmap_db *point_table;
   roadmap_db *bysquare_table;

   RoadMapPointContext *context;

   context = malloc (sizeof(RoadMapPointContext));
   roadmap_check_allocated(context);
   context->type = RoadMapPointType;

   bysquare_table  = roadmap_db_get_subsection (root, "bysquare");
   point_table = roadmap_db_get_subsection (root, "data");

   context->BySquare =
      (RoadMapPointBySquare *) roadmap_db_get_data (bysquare_table);
   context->Point = (RoadMapPoint *) roadmap_db_get_data (point_table);

   context->BySquareCount = roadmap_db_get_count (bysquare_table);
   context->PointCount    = roadmap_db_get_count (point_table);

   if (roadmap_db_get_size(bysquare_table) !=
          context->BySquareCount * sizeof(RoadMapPointBySquare)) {
      roadmap_log (ROADMAP_FATAL, "invalid point/bysquare structure");
   }
   if (roadmap_db_get_size(point_table) !=
          context->PointCount * sizeof(RoadMapPoint)) {
      roadmap_log (ROADMAP_FATAL, "invalid point/data structure");
   }

   rdmxchange_point_register_export();

   return context;
}

static void rdmxchange_point_activate (void *context) {

   RoadMapPointContext *point_context = (RoadMapPointContext *) context;

   if ((point_context != NULL) &&
       (point_context->type != RoadMapPointType)) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (invalid context type)");
   }
   RoadMapPointActive = point_context;
}

static void rdmxchange_point_unmap (void *context) {

   RoadMapPointContext *point_context = (RoadMapPointContext *) context;

   if (point_context == RoadMapPointActive) {
      RoadMapPointActive = NULL;
   }
   free (point_context);
}

roadmap_db_handler RoadMapPointExport = {
   "point",
   rdmxchange_point_map,
   rdmxchange_point_activate,
   rdmxchange_point_unmap
};


static void rdmxchange_point_export_head (FILE *file) {

   fprintf (file, "table point/data %d\n",
                  RoadMapPointActive->PointCount);

   fprintf (file, "table point/bysquare %d\n",
                  RoadMapPointActive->BySquareCount);
}


static void rdmxchange_point_export_data (FILE *file) {

   int i;
   RoadMapPoint *point;
   RoadMapPointBySquare *bysquare;


   fprintf (file, "table point/data\n"
                  "longitude,latitude\n");
   point = RoadMapPointActive->Point;

   for (i = 0; i < RoadMapPointActive->PointCount; ++i, ++point) {
      fprintf (file, "%.0d,%.0d\n", point->longitude, point->latitude);
   }
   fprintf (file, "\n");


   fprintf (file, "table point/bysquare\n"
                  "first,count\n");
   bysquare = RoadMapPointActive->BySquare;

   for (i = 0; i < RoadMapPointActive->BySquareCount; ++i, ++bysquare) {
      fprintf (file, "%.0d,%.0d\n", bysquare->first, bysquare->count);
   }
   fprintf (file, "\n");
}


static RdmXchangeExport RdmXchangePointExport = {
   "point",
   rdmxchange_point_export_head,
   rdmxchange_point_export_data
};

static void rdmxchange_point_register_export (void) {
   rdmxchange_main_register_export (&RdmXchangePointExport);
}

