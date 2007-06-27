/* roadmap_polygon.c - Manage the tiger polygons.
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
 *   int  roadmap_polygon_count (void);
 *   int  roadmap_polygon_category (int polygon);
 *   void roadmap_polygon_edges (int polygon, RoadMapArea *edges);
 *   int  roadmap_polygon_points (int polygon, int *list, int size);
 *
 * These functions are used to retrieve the polygons to draw.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_db_polygon.h"

#include "roadmap_polygon.h"

static char *RoadMapPolygonType = "RoadMapPolygonContext";

typedef struct {

   char *type;

   RoadMapPolygon *Polygon;
   int             PolygonCount;

   RoadMapPolygonPoint *PolygonPoint;
   int                  PolygonPointCount;

} RoadMapPolygonContext;

static RoadMapPolygonContext *RoadMapPolygonActive = NULL;


static void *roadmap_polygon_map (roadmap_db *root) {

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

   return context;
}

static void roadmap_polygon_activate (void *context) {

   RoadMapPolygonContext *polygon_context = (RoadMapPolygonContext *) context;

   if ((polygon_context != NULL) &&
       (polygon_context->type != RoadMapPolygonType)) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (invalid context type)");
   }
   RoadMapPolygonActive = polygon_context;
}

static void roadmap_polygon_unmap (void *context) {

   RoadMapPolygonContext *polygon_context = (RoadMapPolygonContext *) context;

   if (polygon_context->type != RoadMapPolygonType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (invalid context type)");
   }
   if (RoadMapPolygonActive == polygon_context) {
      RoadMapPolygonActive = NULL;
   }
   free (polygon_context);
}

roadmap_db_handler RoadMapPolygonHandler = {
   "polygon",
   roadmap_polygon_map,
   roadmap_polygon_activate,
   roadmap_polygon_unmap
};



int  roadmap_polygon_count (void) {

   if (RoadMapPolygonActive == NULL) return 0;

   return RoadMapPolygonActive->PolygonCount;
}

int  roadmap_polygon_category (int polygon) {

   return RoadMapPolygonActive->Polygon[polygon].cfcc;
}


void roadmap_polygon_edges (int polygon, RoadMapArea *edges) {

   RoadMapPolygon *this_polygon = RoadMapPolygonActive->Polygon + polygon;

   edges->west = this_polygon->west;
   edges->east = this_polygon->east;
   edges->north = this_polygon->north;
   edges->south = this_polygon->south;
}


int  roadmap_polygon_points (int polygon, int *list, int size) {

   int i;
   RoadMapPolygon      *this_polygon;
   RoadMapPolygonPoint *this_point;

   int count;
   int first;

   this_polygon = RoadMapPolygonActive->Polygon + polygon;

   count = roadmap_polygon_get_count(this_polygon);
   first = roadmap_polygon_get_first(this_polygon);

   this_point   = RoadMapPolygonActive->PolygonPoint + first;

   if (size < count) {
      return -1; /* Not enough space. */
   }

   for (i = count; i > 0; --i, ++list, ++this_point) {
      *list = this_point->point;
   }

   return count;
}

