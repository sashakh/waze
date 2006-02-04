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

   fprintf (file, "shape/bysquare,%d\n",
                  RoadMapShapeActive->ShapeBySquareCount);

   fprintf (file, "shape/byline,%d\n",
                  RoadMapShapeActive->ShapeByLineCount);

   fprintf (file, "shape/data,%d\n",
                  RoadMapShapeActive->ShapeCount);
}


static void rdmxchange_shape_export_data (FILE *file) {

   int i;
   RoadMapShape *point;
   RoadMapShapeByLine *byline;
   RoadMapShapeBySquare *bysquare;


   fprintf (file, "shape/bysquare\n"
                  "first,count\n");
   bysquare = RoadMapShapeActive->ShapeBySquare;

   for (i = 0; i < RoadMapShapeActive->ShapeBySquareCount; ++i, ++bysquare) {
      fprintf (file, "%.0d,%.0d\n", bysquare->first, bysquare->count);
   }
   fprintf (file, "\n");


   fprintf (file, "shape/byline\n"
                  "line,first,count\n");
   byline = RoadMapShapeActive->ShapeByLine;

   for (i = 0; i < RoadMapShapeActive->ShapeByLineCount; ++i, ++byline) {
      fprintf (file, "%.0d,%.0d,%.0d\n", byline->line,
                                         byline->first,
                                         byline->count);
   }
   fprintf (file, "\n");


   fprintf (file, "shape/data\n"
                  "longitude,latitude\n");
   point = RoadMapShapeActive->Shape;

   for (i = 0; i < RoadMapShapeActive->ShapeCount; ++i, ++point) {
      fprintf (file, "%.0d,%.0d\n", point->delta_longitude,
                                    point->delta_latitude);
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


/* The import side. ----------------------------------------------------- */

static RoadMapShape *Shape = NULL;
static RoadMapShapeByLine *ShapeByLine = NULL;
static RoadMapShapeBySquare *ShapeBySquare = NULL;
static int ShapeCount = 0;
static int ShapeByLineCount = 0;
static int ShapeBySquareCount = 0;
static int ShapeCursor = 0;


static void rdmxchange_shape_save (void) {

   int i;
   RoadMapShape *db_shape;
   RoadMapShapeByLine *db_byline;
   RoadMapShapeBySquare *db_bysquare;

   buildmap_db *root;
   buildmap_db *table_square;
   buildmap_db *table_line;
   buildmap_db *table_data;


   root  = buildmap_db_add_section (NULL, "shape");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_square = buildmap_db_add_section (root, "bysquare");
   if (table_square == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_square,
         ShapeBySquareCount, sizeof(RoadMapShapeBySquare));

   table_line = buildmap_db_add_section (root, "byline");
   if (table_line == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_line,
         ShapeByLineCount, sizeof(RoadMapShapeByLine));

   table_data = buildmap_db_add_section (root, "data");
   if (table_data == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_data, ShapeCount, sizeof(RoadMapShape));

   db_bysquare = (RoadMapShapeBySquare *) buildmap_db_get_data (table_square);
   db_byline = (RoadMapShapeByLine *) buildmap_db_get_data (table_line);
   db_shape  = (RoadMapShape *) buildmap_db_get_data (table_data);


   for (i = ShapeBySquareCount-1; i >= 0; --i) {
      db_bysquare[i] = ShapeBySquare[i];
   }

   for (i = ShapeByLineCount-1; i >= 0; --i) {
      db_byline[i] = ShapeByLine[i];
   }

   for (i = ShapeCount-1; i >= 0; --i) {
      db_shape[i] = Shape[i];
   }

   /* Do not save this data ever again. */

   free (ShapeBySquare);
   ShapeBySquare = NULL;
   ShapeBySquareCount = 0;

   free (ShapeByLine);
   ShapeByLine = NULL;
   ShapeByLineCount = 0;

   free (Shape);
   Shape = NULL;
   ShapeCount = 0;
}


static buildmap_db_module RdmXchangeShapeModule = {
   "shape",
   NULL,
   rdmxchange_shape_save,
   NULL,
   NULL
};


static void rdmxchange_shape_import_table (const char *name, int count) {

   if (strcmp (name, "shape/bysquare") == 0) {

      if (ShapeBySquare != NULL) free (ShapeBySquare);

      ShapeBySquare = calloc (count, sizeof(RoadMapShapeBySquare));
      ShapeBySquareCount = count;

   } else if (strcmp (name, "shape/byline") == 0) {

      if (ShapeByLine != NULL) free (ShapeByLine);

      ShapeByLine = calloc (count, sizeof(RoadMapShapeByLine));
      ShapeByLineCount = count;

   } else if (strcmp (name, "shape/data") == 0) {

      if (Shape != NULL) free (Shape);

      Shape = calloc (count, sizeof(RoadMapShape));
      ShapeCount = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }

   buildmap_db_register (&RdmXchangeShapeModule);
}


static int rdmxchange_shape_import_schema (const char *table,
                                                 char *fields[], int count) {

   ShapeCursor = 0;

   if (strcmp (table, "shape/bysquare") == 0) {

      if ((ShapeBySquare == NULL) ||
          (count != 2) ||
          (strcmp (fields[0], "first") != 0) ||
          (strcmp (fields[1], "count") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s\n", table);
      }
      return 1;

   } else if (strcmp (table, "shape/byline") == 0) {

      if ((ShapeByLine == NULL) ||
          (count != 3) ||
          (strcmp (fields[0], "line") != 0) ||
          (strcmp (fields[1], "first") != 0) ||
          (strcmp (fields[2], "count") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s\n", table);
      }
      return 2;

   } else if (strcmp (table, "shape/data") == 0) {

      if ((Shape == NULL) ||
          (count != 2) ||
          (strcmp (fields[0], "longitude") != 0) ||
          (strcmp (fields[1], "latitude") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s\n", table);
      }
      return 3;

   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}


static void rdmxchange_shape_import_data (int table,
                                              char *fields[], int count) {

   RoadMapShapeBySquare *bysquare;
   RoadMapShapeByLine   *byline;
   RoadMapShape         *shape;


   switch (table) {

      case 1:

         if (count != 2) {
            buildmap_fatal (count, "invalid shape/bysquare record");
         }
         bysquare = &(ShapeBySquare[ShapeCursor]);

         bysquare->first = rdmxchange_import_int (fields[0]);
         bysquare->count = rdmxchange_import_int (fields[1]);

         break;

      case 2:

         if (count != 3) {
            buildmap_fatal (count, "invalid shape/byline record");
         }
         byline = &(ShapeByLine[ShapeCursor]);

         byline->line  = rdmxchange_import_int (fields[0]);
         byline->first = rdmxchange_import_int (fields[1]);
         byline->count = rdmxchange_import_int (fields[2]);
         break;

      case 3:

         if (count != 2) {
            buildmap_fatal (count, "invalid shape/data record");
         }
         shape = &(Shape[ShapeCursor]);

         shape->delta_longitude = (short) rdmxchange_import_int (fields[0]);
         shape->delta_latitude  = (short) rdmxchange_import_int (fields[1]);

         break;
   }

   ShapeCursor += 1;
}


RdmXchangeImport RdmXchangeShapeImport = {
   "shape",
   rdmxchange_shape_import_table,
   rdmxchange_shape_import_schema,
   rdmxchange_shape_import_data
};
