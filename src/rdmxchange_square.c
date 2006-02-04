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

   fprintf (file, "square/global,1\n");
   fprintf (file, "square/data,%d\n", RoadMapSquareActive->SquareCount);
}


static void rdmxchange_square_export_data (FILE *file) {

   int i;
   RoadMapSquare *square;

   fprintf (file, "square/global\n"
                  "edges.east,"
                  "edges.north,"
                  "edges.west,"
                  "edges.south,"
                  "longitude.step,"
                  "longitude.count,"
                  "latitude.step,"
                  "latitude.count,"
                  "squares.count\n");

   fprintf (file, "%.0d,%.0d,%.0d,%.0d,",
                   RoadMapSquareActive->SquareGlobal->edges.east,
                   RoadMapSquareActive->SquareGlobal->edges.north,
                   RoadMapSquareActive->SquareGlobal->edges.west,
                   RoadMapSquareActive->SquareGlobal->edges.south);

   fprintf (file, "%.0d,%.0d,%.0d,%.0d,%.0d\n\n",
                  RoadMapSquareActive->SquareGlobal->step_longitude,
                  RoadMapSquareActive->SquareGlobal->count_longitude,
                  RoadMapSquareActive->SquareGlobal->step_latitude,
                  RoadMapSquareActive->SquareGlobal->count_latitude,
                  RoadMapSquareActive->SquareGlobal->count_squares);


   fprintf (file, "square/data\n"
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


/* The import side. ----------------------------------------------------- */

static RoadMapGlobal SquareGlobal;
static RoadMapSquare *Square = NULL;
static int  SquareCount = 0;
static int  SquareCursor = 0;


static void rdmxchange_square_save (void) {

   int i;

   RoadMapGlobal  *db_global;
   RoadMapSquare  *db_square;

   buildmap_db *root;
   buildmap_db *table_data;
   buildmap_db *table_global;

   /* Create the tables. */

   root  = buildmap_db_add_section (NULL, "square");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_global = buildmap_db_add_section (root, "global");
   if (table_global == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_global, 1, sizeof(RoadMapGlobal));

   table_data = buildmap_db_add_section (root, "data");
   if (table_data == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_data, SquareCount, sizeof(RoadMapSquare));

   db_global = (RoadMapGlobal *) buildmap_db_get_data (table_global);
   db_square = (RoadMapSquare *) buildmap_db_get_data (table_data);

   /* Fill the data in. */

   for (i = 0; i < SquareCount; ++i) {
      db_square[i] = Square[i];
   }
   *db_global = SquareGlobal;

   /* Do not save this data ever again. */
   free (Square);
   Square = NULL;
   SquareCount = 0;
}


static buildmap_db_module RdmXchangeSquareModule = {
   "square",
   NULL,
   rdmxchange_square_save,
   NULL,
   NULL
};


static void rdmxchange_square_import_table (const char *name, int count) {

   buildmap_db_register (&RdmXchangeSquareModule);

   if (strcmp (name, "square/global") == 0) {

      if (count != 1) {
         buildmap_fatal (1, "invalid record count for table square/global");
      }

   } else if (strcmp (name, "square/data") == 0) {

      if (Square!= NULL) free (Square);

      Square = calloc (count, sizeof(RoadMapSquare));
      SquareCount = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }
}


static int rdmxchange_square_import_schema (const char *table,
                                            char *fields[], int count) {

   if (strcmp (table, "square/global") == 0) {

      if (count != 9 ||
            strcmp(fields[0], "edges.east") != 0 ||
            strcmp(fields[1], "edges.north") != 0 ||
            strcmp(fields[2], "edges.west") != 0 ||
            strcmp(fields[3], "edges.south") != 0 ||
            strcmp(fields[4], "longitude.step") != 0 ||
            strcmp(fields[5], "longitude.count") != 0 ||
            strcmp(fields[6], "latitude.step") != 0 ||
            strcmp(fields[7], "latitude.count") != 0 ||
            strcmp(fields[8], "squares.count") != 0) {
         buildmap_fatal (1, "invalid schema for table square/global");
      }
      SquareCursor = 0;

      return 1;

   } else if (strcmp (table, "square/data") == 0) {

      if (count != 6 ||
            strcmp(fields[0], "edges.east") != 0 ||
            strcmp(fields[1], "edges.north") != 0 ||
            strcmp(fields[2], "edges.west") != 0 ||
            strcmp(fields[3], "edges.south") != 0 ||
            strcmp(fields[4], "points.count") != 0 ||
            strcmp(fields[5], "position") != 0) {
         buildmap_fatal (1, "invalid schema for table square/data");
      }
      SquareCursor = 0;

      return 2;
   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}


static void rdmxchange_square_import_data (int table,
                                        char *fields[], int count) {

   switch (table) {

      case 1:

         if (count != 9) {
            buildmap_fatal (count, "invalid square/global record");
         }
         if (SquareCursor != 0) {
            buildmap_fatal
               (count, "invalid record count for table square/global");
         }
         SquareGlobal.edges.east      = rdmxchange_import_int (fields[0]);
         SquareGlobal.edges.north     = rdmxchange_import_int (fields[1]);
         SquareGlobal.edges.west      = rdmxchange_import_int (fields[2]);
         SquareGlobal.edges.south     = rdmxchange_import_int (fields[3]);
         SquareGlobal.step_longitude  = rdmxchange_import_int (fields[4]);
         SquareGlobal.count_longitude = rdmxchange_import_int (fields[5]);
         SquareGlobal.step_latitude   = rdmxchange_import_int (fields[6]);
         SquareGlobal.count_latitude  = rdmxchange_import_int (fields[7]);
         SquareGlobal.count_squares   = rdmxchange_import_int (fields[8]);
         break;

      case 2:

         if (count != 6) {
            buildmap_fatal (count, "invalid square/data record");
         }
         Square[SquareCursor].edges.east    = rdmxchange_import_int (fields[0]);
         Square[SquareCursor].edges.north   = rdmxchange_import_int (fields[1]);
         Square[SquareCursor].edges.west    = rdmxchange_import_int (fields[2]);
         Square[SquareCursor].edges.south   = rdmxchange_import_int (fields[3]);
         Square[SquareCursor].count_points  = rdmxchange_import_int (fields[4]);
         Square[SquareCursor].position      = rdmxchange_import_int (fields[5]);
         break;

      default:

          buildmap_fatal (1, "invalid table");
   }
   SquareCursor += 1;
}


RdmXchangeImport RdmXchangeSquareImport = {
   "square",
   rdmxchange_square_import_table,
   rdmxchange_square_import_schema,
   rdmxchange_square_import_data
};

