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
   "dictionary",
   roadmap_line_map,
   roadmap_line_activate,
   roadmap_line_unmap
};


static void rdmxchange_line_export_head (FILE *file) {

   fprintf (file, "table line/data %d from to\n",
         RoadMapLineActive->LineCount);

   fprintf (file, "table line/bysquare1 %d first[%d] last\n",
                  RoadMapLineActive->LineBySquare1Count,
                  ROADMAP_CATEGORY_RANGE);

   fprintf (file, "table line/index2 %d index\n",
         RoadMapLineActive->LineIndex2Count);

   fprintf (file, "table line/bysquare2 %d first[%d] last\n",
                  RoadMapLineActive->LineBySquare2Count,
                  ROADMAP_CATEGORY_RANGE);
}


static void rdmxchange_line_export_data (FILE *file) {

   int i;
   int j;
   int *index2;
   RoadMapLine *line;
   RoadMapLineBySquare *bysquare;


   fprintf (file, "table line/data\n");
   line = RoadMapLineActive->Line;

   for (i = 0; i < RoadMapLineActive->LineCount; ++i, ++line) {
      fprintf (file, "%d,%d\n", line->from, line->to);
   }
   fprintf (file, "\n");


   fprintf (file, "table line/bysquare1\n");
   bysquare = RoadMapLineActive->LineBySquare1;

   for (i = 0; i < RoadMapLineActive->LineBySquare1Count; ++i, ++bysquare) {
      for (j = 0; j < ROADMAP_CATEGORY_RANGE; ++j) {
         fprintf (file, "%d,", bysquare->first[j]);
      }
      fprintf (file, "%d\n", bysquare->last);
   }
   fprintf (file, "\n");


   fprintf (file, "table line/index2\n");
   index2 = RoadMapLineActive->LineIndex2;

   for (i = 0; i < RoadMapLineActive->LineIndex2Count; ++i, ++index2) {
      fprintf (file, "%d\n", *index2);
   }
   fprintf (file, "\n");


   fprintf (file, "table line/bysquare2\n");
   bysquare = RoadMapLineActive->LineBySquare2;

   for (i = 0; i < RoadMapLineActive->LineBySquare2Count; ++i, ++bysquare) {
      for (j = 0; j < ROADMAP_CATEGORY_RANGE; ++j) {
         fprintf (file, "%d,", bysquare->first[j]);
      }
      fprintf (file, "%d\n", bysquare->last);
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

