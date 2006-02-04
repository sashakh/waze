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

   fprintf (file, "point/data,%d\n",
                  RoadMapPointActive->PointCount);

   fprintf (file, "point/bysquare,%d\n",
                  RoadMapPointActive->BySquareCount);
}


static void rdmxchange_point_export_data (FILE *file) {

   int i;
   RoadMapPoint *point;
   RoadMapPointBySquare *bysquare;


   fprintf (file, "point/data\n"
                  "longitude,latitude\n");
   point = RoadMapPointActive->Point;

   for (i = 0; i < RoadMapPointActive->PointCount; ++i, ++point) {
      fprintf (file, "%.0d,%.0d\n", point->longitude, point->latitude);
   }
   fprintf (file, "\n");


   fprintf (file, "point/bysquare\n"
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


/* The import side. ----------------------------------------------------- */

static RoadMapPoint *Point = NULL;
static RoadMapPointBySquare *PointBySquare = NULL;
static int  PointCount = 0;
static int  PointBySquareCount = 0;
static int  PointCursor = 0;


static void rdmxchange_point_save (void) {

   int i;
   RoadMapPoint  *db_points;
   RoadMapPointBySquare *db_bysquare;

   buildmap_db *root;
   buildmap_db *table_data;
   buildmap_db *table_bysquare;


   /* Create the tables. */

   root  = buildmap_db_add_section (NULL, "point");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_data = buildmap_db_add_section (root, "data");
   if (table_data == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_data, PointCount, sizeof(RoadMapPoint));

   table_bysquare = buildmap_db_add_section (root, "bysquare");
   if (table_bysquare == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data
      (table_bysquare, PointBySquareCount, sizeof(RoadMapPointBySquare));

   db_points   = (RoadMapPoint *) buildmap_db_get_data (table_data);
   db_bysquare = (RoadMapPointBySquare *) buildmap_db_get_data (table_bysquare);

   /* Fill the data in. */

   for (i = 0; i < PointCount; ++i) {
      db_points[i] = Point[i];
   }

   for (i = 0; i < PointBySquareCount; ++i) {
      db_bysquare[i] = PointBySquare[i];
   }

   /* Do not save this data ever again. */
   free (Point);
   Point = NULL;
   PointCount = 0;

   free (PointBySquare);
   PointBySquare = NULL;
   PointBySquareCount = 0;
}


static buildmap_db_module RdmXchangePointModule = {
   "point",
   NULL,
   rdmxchange_point_save,
   NULL,
   NULL
};


static void rdmxchange_point_import_table (const char *name, int count) {

   buildmap_db_register (&RdmXchangePointModule);

   if (strcmp (name, "point/data") == 0) {

      if (Point != NULL) free (Point);

      Point = calloc (count, sizeof(RoadMapPoint));
      PointCount = count;

   } else if (strcmp (name, "point/bysquare") == 0) {

      if (PointBySquare != NULL) free (PointBySquare);

      PointBySquare = calloc (count, sizeof(RoadMapPointBySquare));
      PointBySquareCount = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }
}


static int rdmxchange_point_import_schema (const char *table,
                                         char *fields[], int count) {

   if (strcmp (table, "point/data") == 0) {

      if (Point == NULL ||
            count != 2 ||
            strcmp(fields[0], "longitude") != 0 ||
            strcmp(fields[1], "latitude") != 0) {
         buildmap_fatal (1, "invalid schema for table point/data");
      }
      PointCursor = 0;

      return 1;

   } else if (strcmp (table, "point/bysquare") == 0) {

      if (PointBySquare == NULL ||
            count != 2 ||
            strcmp(fields[0], "first") != 0 ||
            strcmp(fields[1], "count") != 0) {
         buildmap_fatal (1, "invalid schema for table point/bysquare");
      }
      PointCursor = 0;

      return 2;

   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}

static void rdmxchange_point_import_data (int table,
                                        char *fields[], int count) {

   switch (table) {

      case 1:

         if (count != 2) {
            buildmap_fatal (count, "invalid point/data record");
         }
         Point[PointCursor].longitude =
            (unsigned short) rdmxchange_import_int (fields[0]);
         Point[PointCursor].latitude =
            (unsigned short) rdmxchange_import_int (fields[1]);
         break;

      case 2:

         if (count != 2) {
            buildmap_fatal (count, "invalid point/bysquare record");
         }
         PointBySquare[PointCursor].first = rdmxchange_import_int (fields[0]);
         PointBySquare[PointCursor].count = rdmxchange_import_int (fields[1]);
         break;

      default:

          buildmap_fatal (1, "invalid table");
   }

   PointCursor += 1;
}


RdmXchangeImport RdmXchangePointImport = {
   "point",
   rdmxchange_point_import_table,
   rdmxchange_point_import_schema,
   rdmxchange_point_import_data
};
