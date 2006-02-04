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
 *   the Free Software Foundation, as of version 2 of the License.
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

   fprintf (file, "polygon/head,%d\n",
                  RoadMapPolygonActive->PolygonCount);

   fprintf (file, "polygon/point,%d\n",
                  RoadMapPolygonActive->PolygonPointCount);
}


static void rdmxchange_polygon_export_data (FILE *file) {

   int i;
   RoadMapPolygon *polygon;
   RoadMapPolygonPoint *point;


   fprintf (file, "polygon/head\n"
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


   fprintf (file, "polygon/point\n"
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


/* The import side. ----------------------------------------------------- */

static RoadMapPolygon      *Polygon = NULL;
static RoadMapPolygonPoint *PolygonPoint = NULL;
static int  PolygonCount = 0;
static int  PolygonPointCount = 0;
static int  PolygonCursor = 0;


static void rdmxchange_polygon_save (void) {

   int i;

   RoadMapPolygon *db_head;
   RoadMapPolygonPoint *db_point;

   buildmap_db *root;
   buildmap_db *head_table;
   buildmap_db *point_table;


   root = buildmap_db_add_section (NULL, "polygon");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   head_table = buildmap_db_add_section (root, "head");
   if (head_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (head_table, PolygonCount, sizeof(RoadMapPolygon));

   point_table = buildmap_db_add_section (root, "point");
   if (point_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (point_table,
         PolygonPointCount, sizeof(RoadMapPolygonPoint));

   db_head   = (RoadMapPolygon *)         buildmap_db_get_data (head_table);
   db_point  = (RoadMapPolygonPoint *)    buildmap_db_get_data (point_table);

   for (i = PolygonCount-1; i >= 0; --i) {
      db_head[i] = Polygon[i];
   }

   for (i = PolygonPointCount-1; i >= 0; --i) {
      db_point[i] = PolygonPoint[i];
   }

   /* Do not save this data ever again. */
   free (Polygon);
   Polygon = NULL;
   PolygonCount = 0;

   free (PolygonPoint);
   PolygonPoint = NULL;
   PolygonPointCount = 0;
}


static buildmap_db_module RdmXchangePolygonModule = {
   "polygon",
   NULL,
   rdmxchange_polygon_save,
   NULL,
   NULL
};


static void rdmxchange_polygon_import_table (const char *name, int count) {

   if (strcmp (name, "polygon/head") == 0) {

      if (Polygon != NULL) free (Polygon);

      Polygon = calloc (count, sizeof(RoadMapPolygon));
      PolygonCount = count;

   } else if (strcmp (name, "polygon/point") == 0) {

      if (PolygonPoint != NULL) free (PolygonPoint);

      PolygonPoint = calloc (count, sizeof(RoadMapPolygonPoint));
      PolygonPointCount = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }

   buildmap_db_register (&RdmXchangePolygonModule);
}


static int rdmxchange_polygon_import_schema (const char *table,
                                                  char *fields[], int count) {

   PolygonCursor = 0;

   if (strcmp (table, "polygon/head") == 0) {

      if ((Polygon == NULL) ||
          (count != 8) ||
          (strcmp (fields[0], "points.first") != 0) ||
          (strcmp (fields[1], "points.count") != 0) ||
          (strcmp (fields[2], "name") != 0) ||
          (strcmp (fields[3], "layer") != 0) ||
          (strcmp (fields[4], "edges.east") != 0) ||
          (strcmp (fields[5], "edges.north") != 0) ||
          (strcmp (fields[6], "edges.west") != 0) ||
          (strcmp (fields[7], "edges.south") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s\n", table);
      }
      return 1;

   } else if (strcmp (table, "polygon/point") == 0) {

      if ((PolygonPoint == NULL) ||
            (count != 1) || (strcmp (fields[0], "index") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s\n", table);
      }
      return 2;
   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}


static void rdmxchange_polygon_import_data (int table,
                                              char *fields[], int count) {

   RoadMapPolygon *polygon;

   switch (table) {
      case 1:

         if (count != 8) {
            buildmap_fatal (count, "invalid polygon/head record");
         }
         polygon = &(Polygon[PolygonCursor++]);

         polygon->first = rdmxchange_import_int (fields[0]);
         polygon->count = rdmxchange_import_int (fields[1]);
         polygon->name  = (RoadMapString) rdmxchange_import_int (fields[2]);
         polygon->cfcc  = (char) rdmxchange_import_int (fields[3]);
         polygon->east  = rdmxchange_import_int (fields[4]);
         polygon->north = rdmxchange_import_int (fields[5]);
         polygon->west  = rdmxchange_import_int (fields[6]);
         polygon->south = rdmxchange_import_int (fields[7]);

         break;

      case 2:

         if (count != 1) {
            buildmap_fatal (count, "invalid polygon/point record");
         }
         PolygonPoint[PolygonCursor++].point =
                           rdmxchange_import_int (fields[0]);
         break;
   }
}


RdmXchangeImport RdmXchangePolygonImport = {
   "polygon",
   rdmxchange_polygon_import_table,
   rdmxchange_polygon_import_schema,
   rdmxchange_polygon_import_data
};

