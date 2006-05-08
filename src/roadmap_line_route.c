/* roadmap_line_route.c - Manage line route data
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
#include "roadmap_line_route.h"

static char *RoadMapLineRouteType = "RoadMapLineRouteContext";

typedef struct {

   char *type;

   RoadMapLineRoute *LineRoute;
   int               LineRouteCount;

} RoadMapLineRouteContext;

static RoadMapLineRouteContext *RoadMapLineRouteActive = NULL;


static void *roadmap_line_route_map (roadmap_db *root) {

   RoadMapLineRouteContext *context;

   roadmap_db *data_table;

   context =
      (RoadMapLineRouteContext *) malloc (sizeof(RoadMapLineRouteContext));

   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }

   context->type = RoadMapLineRouteType;

   data_table    = roadmap_db_get_subsection (root, "data");

   context->LineRoute =
      (RoadMapLineRoute *) roadmap_db_get_data (data_table);
   context->LineRouteCount = roadmap_db_get_count (data_table);

   if (roadmap_db_get_size (data_table) !=
       context->LineRouteCount * sizeof(RoadMapLineRoute)) {
      roadmap_log (ROADMAP_ERROR, "invalid line route data structure");
      free(context);
      return NULL;
   }

   return context;
}

static void roadmap_line_route_activate (void *context) {

   RoadMapLineRouteContext *line_route_context =
      (RoadMapLineRouteContext *) context;

   if ((line_route_context != NULL) &&
       (line_route_context->type != RoadMapLineRouteType)) {
      roadmap_log (ROADMAP_FATAL, "invalid line route context activated");
   }
   RoadMapLineRouteActive = line_route_context;
}

static void roadmap_line_route_unmap (void *context) {

   RoadMapLineRouteContext *line_route_context =
      (RoadMapLineRouteContext *) context;

   if (line_route_context->type != RoadMapLineRouteType) {
      roadmap_log (ROADMAP_FATAL, "unmapping invalid line context");
   }
   free (line_route_context);
}

roadmap_db_handler RoadMapLineRouteHandler = {
   "line_route",
   roadmap_line_route_map,
   roadmap_line_route_activate,
   roadmap_line_route_unmap
};


int roadmap_line_route_get_direction (int line, int who) {

   RoadMapLineRoute *route;
   if (RoadMapLineRouteActive == NULL) return 0; /* No data. */
   if (RoadMapLineRouteActive->LineRouteCount <= line) return 0;

   route = &RoadMapLineRouteActive->LineRoute[line];

   if ((route->from_flags & who) && (route->to_flags & who)) {

      return ROUTE_DIRECTION_ANY;
   } else if (!(route->from_flags & who) && !(route->to_flags & who)) {
      return ROUTE_DIRECTION_NONE;
   } else if (route->from_flags & who) {

      return ROUTE_DIRECTION_WITH_LINE;
   } else {

      return ROUTE_DIRECTION_AGAINST_LINE;
   }
}


int roadmap_line_route_get_flags (int line,
                                   LineRouteFlag *from,
                                   LineRouteFlag *to) {

   RoadMapLineRoute *route;
   if (RoadMapLineRouteActive == NULL) return -1; /* No data. */
   if (RoadMapLineRouteActive->LineRouteCount <= line) return -1;

   route = &RoadMapLineRouteActive->LineRoute[line];

   *from = route->from_flags;
   *to = route->to_flags;

   return 0;
}


int roadmap_line_route_get_speed_limit (int line,
                                        LineRouteMax *from,
                                        LineRouteMax *to) {

   RoadMapLineRoute *route;
   if (RoadMapLineRouteActive == NULL) return -1; /* No data. */
   if (RoadMapLineRouteActive->LineRouteCount <= line) return -1;

   route = &RoadMapLineRouteActive->LineRoute[line];

   *from = route->from_max_speed;
   *to = route->to_max_speed;

   return 0;
}


int roadmap_line_route_get_cross_times (int line,
                                         LineRouteTime *from,
                                         LineRouteTime *to) {

   RoadMapLineRoute *route;
   if (RoadMapLineRouteActive == NULL) return -1; /* No data. */
   if (RoadMapLineRouteActive->LineRouteCount <= line) return -1;

   route = &RoadMapLineRouteActive->LineRoute[line];

   *from = route->from_cross_time;
   *to = route->to_cross_time;

   return 0;
}

