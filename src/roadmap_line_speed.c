/* roadmap_line_speed.c - Manage line speed data
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_line_speed.h"

static char *RoadMapLineSpeedType = "RoadMapLineSpeedContext";

typedef struct {

   char *type;

   RoadMapLineSpeed *LineSpeedSlots;
   int               LineSpeedSlotsCount;

   int              *LineSpeedIndex;
   int               LineSpeedIndexCount;

} RoadMapLineSpeedContext;

static RoadMapLineSpeedContext *RoadMapLineSpeedActive = NULL;


static void *roadmap_line_speed_map (roadmap_db *root) {

   RoadMapLineSpeedContext *context;

   roadmap_db *index_table;
   roadmap_db *data_table;

   context =
      (RoadMapLineSpeedContext *) malloc (sizeof(RoadMapLineSpeedContext));

   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }

   context->type = RoadMapLineSpeedType;

   index_table   = roadmap_db_get_subsection (root, "index");
   data_table    = roadmap_db_get_subsection (root, "data");

   context->LineSpeedIndex = (int *) roadmap_db_get_data (index_table);
   context->LineSpeedIndexCount = roadmap_db_get_count (index_table);

   context->LineSpeedSlots = (RoadMapLineSpeed *)
                              roadmap_db_get_data (data_table);
   context->LineSpeedSlotsCount = roadmap_db_get_count (data_table);

   if (roadmap_db_get_size (data_table) !=
       context->LineSpeedSlotsCount * sizeof(RoadMapLineSpeed)) {
      roadmap_log (ROADMAP_ERROR, "invalid line speed data structure");
      free(context);
      return NULL;
   }

   return context;
}

static void roadmap_line_speed_activate (void *context) {

   RoadMapLineSpeedContext *line_speed_context =
      (RoadMapLineSpeedContext *) context;

   if ((line_speed_context != NULL) &&
       (line_speed_context->type != RoadMapLineSpeedType)) {
      roadmap_log (ROADMAP_FATAL, "invalid line speed context activated");
   }
   RoadMapLineSpeedActive = line_speed_context;
}

static void roadmap_line_speed_unmap (void *context) {

   RoadMapLineSpeedContext *line_speed_context =
      (RoadMapLineSpeedContext *) context;

   if (line_speed_context->type != RoadMapLineSpeedType) {
      roadmap_log (ROADMAP_FATAL, "unmapping invalid line speed context");
   }
   free (line_speed_context);
}

roadmap_db_handler RoadMapLineSpeedHandler = {
   "line_speed",
   roadmap_line_speed_map,
   roadmap_line_speed_activate,
   roadmap_line_speed_unmap
};


int roadmap_line_speed_get_avg (int speed_ref) {

   RoadMapLineSpeed *speed;
   int index;
   int count = 0;
   int total = 0;

   if (RoadMapLineSpeedActive == NULL) return 0; /* No data. */
   if (RoadMapLineSpeedActive->LineSpeedIndexCount <= speed_ref) {
      roadmap_log (ROADMAP_ERROR, "Invalid speed_ref index:%d", speed_ref);
      return 0;
   }

   index = RoadMapLineSpeedActive->LineSpeedIndex[speed_ref];
   speed = &RoadMapLineSpeedActive->LineSpeedSlots[index];

   while (1) {
      total += speed->speed;
      count++;

      if (speed->time_slot & SPEED_EOL) break;

      speed++;
   }

   return total / count;
}


int roadmap_line_speed_get (int speed_ref, int time_slot) {

   RoadMapLineSpeed *speed;
   int index;

   if (RoadMapLineSpeedActive == NULL) return 0; /* No data. */
   if (RoadMapLineSpeedActive->LineSpeedIndexCount <= speed_ref) {
      roadmap_log (ROADMAP_ERROR, "Invalid speed_ref index:%d", speed_ref);
      return 0;
   }

   index = RoadMapLineSpeedActive->LineSpeedIndex[speed_ref];
   speed = &RoadMapLineSpeedActive->LineSpeedSlots[index];

   while (!(speed->time_slot & SPEED_EOL) &&
      (((speed+1)->time_slot & ~SPEED_EOL) <= time_slot)) {

      speed++;
   }

   return speed->speed;
}

