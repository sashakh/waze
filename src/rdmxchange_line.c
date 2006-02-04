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

   int *LineIndex2;
   int  LineIndex2Count;

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


   context = (RoadMapLineContext *) malloc (sizeof(RoadMapLineContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }
   context->type = RoadMapLineType;

   line_table    = roadmap_db_get_subsection (root, "data");
   square1_table = roadmap_db_get_subsection (root, "bysquare1");
   square2_table = roadmap_db_get_subsection (root, "bysquare2");
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

   fprintf (file, "line/index2,%d\n",
         RoadMapLineActive->LineIndex2Count);

   fprintf (file, "line/bysquare2,%d\n",
                  RoadMapLineActive->LineBySquare2Count);
}


static void rdmxchange_line_export_data (FILE *file) {

   int i;
   int j;
   int *index2;
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
                  "first[%d],last\n", ROADMAP_CATEGORY_RANGE);
   bysquare = RoadMapLineActive->LineBySquare1;

   for (i = 0; i < RoadMapLineActive->LineBySquare1Count; ++i, ++bysquare) {
      for (j = 0; j < ROADMAP_CATEGORY_RANGE; ++j) {
         fprintf (file, "%.0d,", bysquare->first[j]);
      }
      fprintf (file, "%.0d\n", bysquare->last);
   }
   fprintf (file, "\n");


   fprintf (file, "line/index2\n"
                  "index\n");
   index2 = RoadMapLineActive->LineIndex2;

   for (i = 0; i < RoadMapLineActive->LineIndex2Count; ++i, ++index2) {
      fprintf (file, "%d\n", *index2);
   }
   fprintf (file, "\n");


   fprintf (file, "line/bysquare2\n"
                  "first[%d],last\n", ROADMAP_CATEGORY_RANGE);
   bysquare = RoadMapLineActive->LineBySquare2;

   for (i = 0; i < RoadMapLineActive->LineBySquare2Count; ++i, ++bysquare) {
      for (j = 0; j < ROADMAP_CATEGORY_RANGE; ++j) {
         fprintf (file, "%.0d,", bysquare->first[j]);
      }
      fprintf (file, "%.0d\n", bysquare->last);
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
static RoadMapLineBySquare *LineSquare1 = NULL;
static RoadMapLineBySquare *LineSquare2 = NULL;
static int  LineCount = 0;
static int  LineIndex2Count = 0;
static int  LineSquare1Count = 0;
static int  LineSquare2Count = 0;
static int  LineCursor = 0;


static void rdmxchange_line_save (void) {

   int i;
   int *db_index2;
   RoadMapLine *db_lines;
   RoadMapLineBySquare *db_square1;
   RoadMapLineBySquare *db_square2;

   buildmap_db *root;
   buildmap_db *data_table;
   buildmap_db *square1_table;
   buildmap_db *index2_table;
   buildmap_db *square2_table;


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

   index2_table = buildmap_db_add_section (root, "index2");
   if (index2_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (index2_table, LineIndex2Count, sizeof(int));

   square2_table = buildmap_db_add_section (root, "bysquare2");
   if (square2_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (square2_table,
                         LineSquare2Count, sizeof(RoadMapLineBySquare));

   db_lines   = (RoadMapLine *) buildmap_db_get_data (data_table);
   db_square1 = (RoadMapLineBySquare *) buildmap_db_get_data (square1_table);
   db_index2  = (int *) buildmap_db_get_data (index2_table);
   db_square2 = (RoadMapLineBySquare *) buildmap_db_get_data (square2_table);

   /* Fill in the data. */

   for (i = 0; i < LineCount; ++i) {
      db_lines[i] = Line[i];
   }

   for (i = 0; i < LineSquare1Count; ++i) {
      db_square1[i] = LineSquare1[i];
   }

   for (i = 0; i < LineIndex2Count; ++i) {
      db_index2[i] = LineIndex2[i];
   }

   for (i = 0; i < LineSquare2Count; ++i) {
      db_square2[i] = LineSquare2[i];
   }

   /* Do not save this data ever again. */
   free (Line);
   Line = NULL;
   LineCount = 0;

   free (LineSquare1);
   LineSquare1 = NULL;
   LineSquare1Count = 0;

   free (LineIndex2);
   LineIndex2 = NULL;
   LineIndex2Count = 0;

   free (LineSquare2);
   LineSquare2 = NULL;
   LineSquare2Count = 0;
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

   } else if (strcmp (name, "line/index2") == 0) {

      if (LineIndex2 != NULL) free (LineIndex2);

      LineIndex2 = calloc (count, sizeof(int));
      LineIndex2Count = count;

   } else if (strcmp (name, "line/bysquare2") == 0) {

      if (LineSquare2 != NULL) free (LineSquare2);

      LineSquare2 = calloc (count, sizeof(RoadMapLineBySquare));
      LineSquare2Count = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }
}


static int rdmxchange_line_bad_square (char *fields[], int count) {

   char first_name[64];

   snprintf (first_name, sizeof(first_name),
             "first[%d]", ROADMAP_CATEGORY_RANGE);

   if ((count != 2) ||
       (strcmp(fields[0], first_name) != 0) ||
       (strcmp(fields[1], "last") != 0)) {
      return 1;
   }

   return 0;
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

      if (LineSquare1 == NULL || rdmxchange_line_bad_square(fields, count)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 2;

   } else if (strcmp (table, "line/index2") == 0) {

      if ((LineIndex2 == NULL) ||
          (count != 1) || (strcmp (fields[0], "index") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 3;

   } else if (strcmp (table, "line/bysquare2") == 0) {

      if (LineSquare2 == NULL || rdmxchange_line_bad_square(fields, count)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 4;

   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}

static void rdmxchange_line_import_data (int table,
                                        char *fields[], int count) {

   int i;

   switch (table) {

      case 1:

         if (count != 2) {
            buildmap_fatal (count, "invalid line/data record");
         }
         Line[LineCursor].from = rdmxchange_import_int (fields[0]);
         Line[LineCursor].to   = rdmxchange_import_int (fields[1]);
         break;

      case 2:

         if (count != ROADMAP_CATEGORY_RANGE + 1) {
            buildmap_fatal (count, "invalid line/bysquare record");
         }
         for (i = 0; i < ROADMAP_CATEGORY_RANGE; ++i) {
            LineSquare1[LineCursor].first[i] =
               rdmxchange_import_int (fields[i]);
         }
         LineSquare1[LineCursor].last = rdmxchange_import_int (fields[i]);
         break;

      case 3:

         if (count != 1) {
            buildmap_fatal (count, "invalid line/index2 record");
         }
         LineIndex2[LineCursor] = rdmxchange_import_int (fields[0]);
         break;

      case 4:

         if (count != ROADMAP_CATEGORY_RANGE + 1) {
            buildmap_fatal (count, "invalid line/bysquare record");
         }
         for (i = 0; i < ROADMAP_CATEGORY_RANGE; ++i) {
            LineSquare2[LineCursor].first[i] =
               rdmxchange_import_int (fields[i]);
         }
         LineSquare2[LineCursor].last = rdmxchange_import_int (fields[i]);
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
