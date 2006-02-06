/* roadmap_line.c - Manage the tiger lines.
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

#include "roadmap_db_line.h"


static char *RoadMapLineType = "RoadMapLineContext";

typedef struct {

   char *type;

   RoadMapLine *Line;
   int          LineCount;

   RoadMapLineBySquare *LineBySquare1;
   int                  LineBySquare1Count;

   int *LineByLayer1;
   int  LineByLayer1Count;

   int *LineIndex2;
   int  LineIndex2Count;

   int *LineByLayer2;
   int  LineByLayer2Count;

   RoadMapLineBySquare *LineBySquare2;
   int                  LineBySquare2Count;

} RoadMapLineContext;

static RoadMapLineContext *RoadMapLineActive = NULL;

static void rdmxchange_line_register_export (void);


static void *roadmap_line_map (roadmap_db *root) {

   RoadMapLineContext *context;

   roadmap_db *line_table;
   roadmap_db *index2_table;
   roadmap_db *square1_table;
   roadmap_db *square2_table;
   roadmap_db *layer1_table;
   roadmap_db *layer2_table;


   context = (RoadMapLineContext *) malloc (sizeof(RoadMapLineContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }
   context->type = RoadMapLineType;

   line_table    = roadmap_db_get_subsection (root, "data");
   square1_table = roadmap_db_get_subsection (root, "bysquare1");
   square2_table = roadmap_db_get_subsection (root, "bysquare2");
   layer1_table  = roadmap_db_get_subsection (root, "bylayer1");
   layer2_table  = roadmap_db_get_subsection (root, "bylayer2");
   index2_table  = roadmap_db_get_subsection (root, "index2");

   context->Line = (RoadMapLine *) roadmap_db_get_data (line_table);
   context->LineCount = roadmap_db_get_count (line_table);

   if (roadmap_db_get_size (line_table) !=
       context->LineCount * sizeof(RoadMapLine)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/data structure");
      goto roadmap_line_map_abort;
   }

   context->LineBySquare1 =
      (RoadMapLineBySquare *) roadmap_db_get_data (square1_table);
   context->LineBySquare1Count = roadmap_db_get_count (square1_table);

   if (roadmap_db_get_size (square1_table) !=
       context->LineBySquare1Count * sizeof(RoadMapLineBySquare)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/bysquare1 structure");
      goto roadmap_line_map_abort;
   }

   context->LineByLayer1 = (int *) roadmap_db_get_data (layer1_table);
   context->LineByLayer1Count = roadmap_db_get_count (layer1_table);

   if (roadmap_db_get_size (layer1_table) !=
       context->LineByLayer1Count * sizeof(int)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/bylayer1 structure");
      goto roadmap_line_map_abort;
   }

   context->LineIndex2 = (int *) roadmap_db_get_data (index2_table);
   context->LineIndex2Count = roadmap_db_get_count (index2_table);

   if (roadmap_db_get_size (index2_table) !=
       context->LineIndex2Count * sizeof(int)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/index2 structure");
      goto roadmap_line_map_abort;
   }

   context->LineBySquare2 =
      (RoadMapLineBySquare *) roadmap_db_get_data (square2_table);
   context->LineBySquare2Count = roadmap_db_get_count (square2_table);

   if (roadmap_db_get_size (square2_table) !=
       context->LineBySquare2Count * sizeof(RoadMapLineBySquare)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/bysquare2 structure");
      goto roadmap_line_map_abort;
   }

   context->LineByLayer2 = (int *) roadmap_db_get_data (layer2_table);
   context->LineByLayer2Count = roadmap_db_get_count (layer2_table);

   if (roadmap_db_get_size (layer2_table) !=
       context->LineByLayer2Count * sizeof(int)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/bylayer2 structure");
      goto roadmap_line_map_abort;
   }

   rdmxchange_line_register_export();

   return context;

roadmap_line_map_abort:

   free(context);
   return NULL;
}

static void roadmap_line_activate (void *context) {

   RoadMapLineContext *line_context = (RoadMapLineContext *) context;

   if ((line_context != NULL) &&
       (line_context->type != RoadMapLineType)) {
      roadmap_log (ROADMAP_FATAL, "invalid line context activated");
   }
   RoadMapLineActive = line_context;
}

static void roadmap_line_unmap (void *context) {

   RoadMapLineContext *line_context = (RoadMapLineContext *) context;

   if (line_context->type != RoadMapLineType) {
      roadmap_log (ROADMAP_FATAL, "unmapping invalid line context");
   }
   free (line_context);
}

roadmap_db_handler RoadMapLineExport = {
   "line",
   roadmap_line_map,
   roadmap_line_activate,
   roadmap_line_unmap
};


static void rdmxchange_line_export_head (FILE *file) {

   fprintf (file, "line/data,%d\n",
         RoadMapLineActive->LineCount);

   fprintf (file, "line/bysquare1,%d\n",
                  RoadMapLineActive->LineBySquare1Count);

   fprintf (file, "line/bylayer1,%d\n",
                  RoadMapLineActive->LineByLayer1Count);

   fprintf (file, "line/bysquare2,%d\n",
                  RoadMapLineActive->LineBySquare2Count);

   fprintf (file, "line/bylayer2,%d\n",
                  RoadMapLineActive->LineByLayer2Count);

   fprintf (file, "line/index2,%d\n",
         RoadMapLineActive->LineIndex2Count);
}


static void rdmxchange_line_export_data (FILE *file) {

   int i;
   int *index;
   RoadMapLine *line;
   RoadMapLineBySquare *bysquare;


   fprintf (file, "line/data\n"
                  "from,to\n");
   line = RoadMapLineActive->Line;

   for (i = 0; i < RoadMapLineActive->LineCount; ++i, ++line) {
      fprintf (file, "%.0d,%.0d\n", line->from, line->to);
   }
   fprintf (file, "\n");


   fprintf (file, "line/bysquare1\n"
                  "first,count\n");
   bysquare = RoadMapLineActive->LineBySquare1;

   for (i = 0; i < RoadMapLineActive->LineBySquare1Count; ++i, ++bysquare) {
      fprintf (file, "%.0d,%.0d\n", bysquare->first, bysquare->count);
   }
   fprintf (file, "\n");


   fprintf (file, "line/bylayer1\n"
                  "index\n");
   index = RoadMapLineActive->LineByLayer1;

   for (i = 0; i < RoadMapLineActive->LineByLayer1Count; ++i, ++index) {
      fprintf (file, "%d\n", *index);
   }
   fprintf (file, "\n");


   fprintf (file, "line/bysquare2\n"
                  "first,count\n");
   bysquare = RoadMapLineActive->LineBySquare2;

   for (i = 0; i < RoadMapLineActive->LineBySquare2Count; ++i, ++bysquare) {
      fprintf (file, "%.0d,%.0d\n", bysquare->first, bysquare->count);
   }
   fprintf (file, "\n");


   fprintf (file, "line/bylayer2\n"
                  "index\n");
   index = RoadMapLineActive->LineByLayer2;

   for (i = 0; i < RoadMapLineActive->LineByLayer2Count; ++i, ++index) {
      fprintf (file, "%d\n", *index);
   }
   fprintf (file, "\n");


   fprintf (file, "line/index2\n"
                  "index\n");
   index = RoadMapLineActive->LineIndex2;

   for (i = 0; i < RoadMapLineActive->LineIndex2Count; ++i, ++index) {
      fprintf (file, "%d\n", *index);
   }
   fprintf (file, "\n");
}


static RdmXchangeExport RdmXchangeLineExport = {
   "line",
   rdmxchange_line_export_head,
   rdmxchange_line_export_data
};

static void rdmxchange_line_register_export (void) {
   rdmxchange_main_register_export (&RdmXchangeLineExport);
}


/* The import side. ----------------------------------------------------- */

static RoadMapLine *Line = NULL;
static int *LineIndex2 = NULL;
static int *LineLayer1 = NULL;
static int *LineLayer2 = NULL;
static RoadMapLineBySquare *LineSquare1 = NULL;
static RoadMapLineBySquare *LineSquare2 = NULL;
static int  LineCount = 0;
static int  LineIndex2Count = 0;
static int  LineLayer1Count = 0;
static int  LineLayer2Count = 0;
static int  LineSquare1Count = 0;
static int  LineSquare2Count = 0;
static int  LineCursor = 0;


static void rdmxchange_line_save (void) {

   int i;
   int *db_layer1;
   int *db_layer2;
   int *db_index2;
   RoadMapLine *db_lines;
   RoadMapLineBySquare *db_square1;
   RoadMapLineBySquare *db_square2;

   buildmap_db *root;
   buildmap_db *data_table;
   buildmap_db *square1_table;
   buildmap_db *layer1_table;
   buildmap_db *square2_table;
   buildmap_db *layer2_table;
   buildmap_db *index2_table;


   /* Create the tables. */

   root = buildmap_db_add_section (NULL, "line");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   data_table = buildmap_db_add_section (root, "data");
   if (data_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (data_table, LineCount, sizeof(RoadMapLine));

   square1_table = buildmap_db_add_section (root, "bysquare1");
   if (square1_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (square1_table,
                         LineSquare1Count, sizeof(RoadMapLineBySquare));

   layer1_table = buildmap_db_add_section (root, "bylayer1");
   if (layer1_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (layer1_table, LineLayer1Count, sizeof(int));

   square2_table = buildmap_db_add_section (root, "bysquare2");
   if (square2_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (square2_table,
                         LineSquare2Count, sizeof(RoadMapLineBySquare));

   layer2_table = buildmap_db_add_section (root, "bylayer2");
   if (layer2_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (layer2_table, LineLayer2Count, sizeof(int));

   index2_table = buildmap_db_add_section (root, "index2");
   if (index2_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (index2_table, LineIndex2Count, sizeof(int));

   db_lines   = (RoadMapLine *) buildmap_db_get_data (data_table);
   db_square1 = (RoadMapLineBySquare *) buildmap_db_get_data (square1_table);
   db_layer1  = (int *) buildmap_db_get_data (layer1_table);
   db_index2  = (int *) buildmap_db_get_data (index2_table);
   db_layer2  = (int *) buildmap_db_get_data (layer2_table);
   db_square2 = (RoadMapLineBySquare *) buildmap_db_get_data (square2_table);

   /* Fill in the data. */

   for (i = 0; i < LineCount; ++i) {
      db_lines[i] = Line[i];
   }

   for (i = 0; i < LineSquare1Count; ++i) {
      db_square1[i] = LineSquare1[i];
   }

   for (i = 0; i < LineLayer1Count; ++i) {
      db_layer1[i] = LineLayer1[i];
   }

   for (i = 0; i < LineIndex2Count; ++i) {
      db_index2[i] = LineIndex2[i];
   }

   for (i = 0; i < LineSquare2Count; ++i) {
      db_square2[i] = LineSquare2[i];
   }

   for (i = 0; i < LineLayer2Count; ++i) {
      db_layer2[i] = LineLayer2[i];
   }

   /* Do not save this data ever again. */
   free (Line);
   Line = NULL;
   LineCount = 0;

   free (LineSquare1);
   LineSquare1 = NULL;
   LineSquare1Count = 0;

   free (LineLayer1);
   LineLayer1 = NULL;
   LineLayer1Count = 0;

   free (LineIndex2);
   LineIndex2 = NULL;
   LineIndex2Count = 0;

   free (LineSquare2);
   LineSquare2 = NULL;
   LineSquare2Count = 0;

   free (LineLayer2);
   LineLayer2 = NULL;
   LineLayer2Count = 0;
}


static buildmap_db_module RdmXchangeLineModule = {
   "line",
   NULL,
   rdmxchange_line_save,
   NULL,
   NULL
};


static void rdmxchange_line_import_table (const char *name, int count) {

   buildmap_db_register (&RdmXchangeLineModule);

   if (strcmp (name, "line/data") == 0) {

      if (Line != NULL) free (Line);

      Line = calloc (count, sizeof(RoadMapLine));
      LineCount = count;

   } else if (strcmp (name, "line/bysquare1") == 0) {

      if (LineSquare1 != NULL) free (LineSquare1);

      LineSquare1 = calloc (count, sizeof(RoadMapLineBySquare));
      LineSquare1Count = count;

   } else if (strcmp (name, "line/bylayer1") == 0) {

      if (LineLayer1 != NULL) free (LineLayer1);

      LineLayer1 = calloc (count, sizeof(int));
      LineLayer1Count = count;

   } else if (strcmp (name, "line/bysquare2") == 0) {

      if (LineSquare2 != NULL) free (LineSquare2);

      LineSquare2 = calloc (count, sizeof(RoadMapLineBySquare));
      LineSquare2Count = count;

   } else if (strcmp (name, "line/index2") == 0) {

      if (LineIndex2 != NULL) free (LineIndex2);

      LineIndex2 = calloc (count, sizeof(int));
      LineIndex2Count = count;

   } else if (strcmp (name, "line/bylayer2") == 0) {

      if (LineLayer2 != NULL) free (LineLayer2);

      LineLayer2 = calloc (count, sizeof(int));
      LineLayer2Count = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }
}


static int rdmxchange_line_import_schema (const char *table,
                                          char *fields[], int count) {

   LineCursor = 0;

   if (strcmp (table, "line/data") == 0) {

      if (Line == NULL ||
            count != 2 ||
            (strcmp(fields[0], "from") != 0) ||
            (strcmp(fields[1], "to") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 1;

   } else if (strcmp (table, "line/bysquare1") == 0) {

      if ((LineSquare1 == NULL) ||
            (count != 2) ||
            (strcmp(fields[0], "first") != 0) ||
            (strcmp(fields[1], "count") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 2;

   } else if (strcmp (table, "line/bylayer1") == 0) {

      if ((LineLayer1 == NULL) ||
          (count != 1) || (strcmp (fields[0], "index") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 3;

   } else if (strcmp (table, "line/bysquare2") == 0) {

      if ((LineSquare2 == NULL) ||
            (count != 2) ||
            (strcmp(fields[0], "first") != 0) ||
            (strcmp(fields[1], "count") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 4;

   } else if (strcmp (table, "line/bylayer2") == 0) {

      if ((LineLayer2 == NULL) ||
          (count != 1) || (strcmp (fields[0], "index") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 5;

   } else if (strcmp (table, "line/index2") == 0) {

      if ((LineIndex2 == NULL) ||
          (count != 1) || (strcmp (fields[0], "index") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 6;

   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}

static void rdmxchange_line_import_data (int table,
                                        char *fields[], int count) {

   switch (table) {

      case 1:

         if (count != 2) {
            buildmap_fatal (count, "invalid line/data record");
         }
         Line[LineCursor].from = rdmxchange_import_int (fields[0]);
         Line[LineCursor].to   = rdmxchange_import_int (fields[1]);
         break;

      case 2:

         if (count != 2) {
            buildmap_fatal (count, "invalid line/bysquare1 record");
         }
         LineSquare1[LineCursor].first = rdmxchange_import_int (fields[0]);
         LineSquare1[LineCursor].count = rdmxchange_import_int (fields[1]);
         break;

      case 3:

         if (count != 1) {
            buildmap_fatal (count, "invalid line/bylayer1 record");
         }
         LineLayer1[LineCursor] = rdmxchange_import_int (fields[0]);
         break;

      case 4:

         if (count != 2) {
            buildmap_fatal (count, "invalid line/bysquare2 record");
         }
         LineSquare2[LineCursor].first = rdmxchange_import_int (fields[0]);
         LineSquare2[LineCursor].count = rdmxchange_import_int (fields[1]);
         break;

      case 5:

         if (count != 1) {
            buildmap_fatal (count, "invalid line/bylayer2 record");
         }
         LineLayer2[LineCursor] = rdmxchange_import_int (fields[0]);
         break;

      case 6:

         if (count != 1) {
            buildmap_fatal (count, "invalid line/index2 record");
         }
         LineIndex2[LineCursor] = rdmxchange_import_int (fields[0]);
         break;

      default:

          buildmap_fatal (1, "invalid table");
   }
   LineCursor += 1;
}


RdmXchangeImport RdmXchangeLineImport = {
   "line",
   rdmxchange_line_import_table,
   rdmxchange_line_import_schema,
   rdmxchange_line_import_data
};
