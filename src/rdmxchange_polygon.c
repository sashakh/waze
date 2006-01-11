/* rdmxchange_polygon.c - Export the map polygons.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"

#include "rdmxchange.h"

#include "roadmap_db_polygon.h"


static char *RoadMapPolygonType = "RoadMapPolygonContext";

typedef struct {

   char *type;

   RoadMapPolygon *Polygon;
   int             PolygonCount;

   RoadMapPolygonPoint *PolygonPoint;
   int                  PolygonPointCount;

} RoadMapPolygonContext;

static RoadMapPolygonContext *RoadMapPolygonActive = NULL;


static void rdmxchange_polygon_register_export (void);


static void *rdmxchange_polygon_map (roadmap_db *root) {

   RoadMapPolygonContext *context;

   roadmap_db *head_table;
   roadmap_db *point_table;


   context = malloc (sizeof(RoadMapPolygonContext));
   roadmap_check_allocated(context);

   context->type = RoadMapPolygonType;

   head_table  = roadmap_db_get_subsection (root, "head");
   point_table = roadmap_db_get_subsection (root, "point");

   context->Polygon = (RoadMapPolygon *) roadmap_db_get_data (head_table);
   context->PolygonCount = roadmap_db_get_count (head_table);

   if (roadmap_db_get_size (head_table) !=
       context->PolygonCount * sizeof(RoadMapPolygon)) {
      roadmap_log (ROADMAP_FATAL, "invalid polygon/head structure");
   }

   context->PolygonPoint =
      (RoadMapPolygonPoint *) roadmap_db_get_data (point_table);
   context->PolygonPointCount = roadmap_db_get_count (point_table);

   if (roadmap_db_get_size (point_table) !=
       context->PolygonPointCount * sizeof(RoadMapPolygonPoint)) {
      roadmap_log (ROADMAP_FATAL, "invalid polygon/point structure");
   }

   rdmxchange_polygon_register_export();

   return context;
}

static void rdmxchange_polygon_activate (void *context) {

   RoadMapPolygonContext *polygon_context = (RoadMapPolygonContext *) context;

   if ((polygon_context != NULL) &&
       (polygon_context->type != RoadMapPolygonType)) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (invalid context type)");
   }
   RoadMapPolygonActive = polygon_context;
}

static void rdmxchange_polygon_unmap (void *context) {

   RoadMapPolygonContext *polygon_context = (RoadMapPolygonContext *) context;

   if (polygon_context->type != RoadMapPolygonType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (invalid context type)");
   }
   if (RoadMapPolygonActive == polygon_context) {
      RoadMapPolygonActive = NULL;
   }
   free (polygon_context);
}

roadmap_db_handler RoadMapPolygonExport = {
   "polygon",
   rdmxchange_polygon_map,
   rdmxchange_polygon_activate,
   rdmxchange_polygon_unmap
};


static void rdmxchange_polygon_export_head (FILE *file) {

   fprintf (file, "table polygon/head %d\n",
                  RoadMapPolygonActive->PolygonCount);

   fprintf (file, "table polygon/points %d\n",
                  RoadMapPolygonActive->PolygonPointCount);
}


static void rdmxchange_polygon_export_data (FILE *file) {

   int i;
   RoadMapPolygon *polygon;
   RoadMapPolygonPoint *point;


   fprintf (file, "table polygon/head\n"
                  "points.first,"
                  "points.count,"
                  "name,"
                  "layer,"
                  "edges.east,"
                  "edges.north,"
                  "edges.west,"
                  "edges.south\n");
   polygon = RoadMapPolygonActive->Polygon;

   for (i = 0; i < RoadMapPolygonActive->PolygonCount; ++i, ++polygon) {
      fprintf (file, "%.0d,%.0d,", polygon->first, polygon->count);
      fprintf (file, "%.0d,%.0d,", polygon->name, polygon->cfcc);
      fprintf (file, "%.0d,%.0d,%.0d,%.0d\n", polygon->east,
                                              polygon->north,
                                              polygon->west,
                                              polygon->south);
   }
   fprintf (file, "\n");


   fprintf (file, "table polygon/point\n"
                  "index\n");
   point = RoadMapPolygonActive->PolygonPoint;

   for (i = 0; i < RoadMapPolygonActive->PolygonPointCount; ++i, ++point) {
      fprintf (file, "%d\n", point->point);
   }
   fprintf (file, "\n");
}


static RdmXchangeExport RdmXchangePolygonExport = {
   "point",
   rdmxchange_polygon_export_head,
   rdmxchange_polygon_export_data
};

static void rdmxchange_polygon_register_export (void) {
   rdmxchange_main_register_export (&RdmXchangePolygonExport);
}
