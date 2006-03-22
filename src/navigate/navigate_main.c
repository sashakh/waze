/* navigate_main.c - main navigate plugin file
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
 *
 *   See navigate_main.h
 */

#include <stdio.h>
#include <string.h>
#include "type.h"
#include "graph.h"

#include "roadmap.h"
#include "roadmap_pointer.h"
#include "roadmap_plugin.h"
#include "roadmap_line.h"
#include "roadmap_messagebox.h"
#include "roadmap_canvas.h"
#include "roadmap_street.h"
#include "roadmap_trip.h"
#include "roadmap_navigate.h"
#include "roadmap_screen.h"
#include "roadmap_line_route.h"
#include "roadmap_math.h"
#include "roadmap_point.h"

#include "navigate_plugin.h"
#include "navigate_graph.h"
#include "navigate_main.h"

int NavigateEnabled = 0;
int NavigatePluginID = -1;
static int NavigateTrackEnabled = 0;
static RoadMapPen NavigatePen;

#define MAX_NAV_SEGEMENTS 100

int NavigateSegments[MAX_NAV_SEGEMENTS];
int NavigateNumSegments = 0;


static int navigate_find_track_points (int *from_line, int *from_point,
                                       int *to_line, int *to_point) {
   const RoadMapPosition *position;
   RoadMapPosition from_position;
   RoadMapPosition to_position;
   PluginLine line;
   int distance;
   int from_tmp;
   int to_tmp;

   position = roadmap_trip_get_position ("Departure");
   if (!position) return -1;

   if ((roadmap_navigate_retrieve_line
         (position, 20, &line, &distance) == -1) ||
       (roadmap_plugin_get_id (&line) != ROADMAP_PLUGIN_ID)) {

      roadmap_messagebox ("Error", "Can't find a road near departure point.");
      return -1;
   }

   *from_line = roadmap_plugin_get_line_id (&line);

   switch (roadmap_line_route_get_direction (*from_line, ROUTE_CAR_ALLOWED)) {
      case ROUTE_DIRECTION_ANY:
      case ROUTE_DIRECTION_NONE:
         roadmap_line_points (*from_line, &from_tmp, &to_tmp);
         roadmap_point_position (from_tmp, &from_position);
         roadmap_point_position (to_tmp, &to_position);

         if (roadmap_math_distance (position, &from_position) <
             roadmap_math_distance (position, &to_position)) {
            *from_point = from_tmp;
         } else {
            *from_point = to_tmp;
         }
         break;
      case ROUTE_DIRECTION_WITH_LINE:
         roadmap_line_points (*from_line, &from_tmp, from_point);
         break;
      case ROUTE_DIRECTION_AGAINST_LINE:
         roadmap_line_points (*from_line, from_point, &from_tmp);
         break;
      default:
         roadmap_line_points (*from_line, &from_tmp, to_point);
   }

   position = roadmap_trip_get_position ("Destination");
   if (!position) return -1;

   if ((roadmap_navigate_retrieve_line
         (position, 20, &line, &distance) == -1) ||
       (roadmap_plugin_get_id (&line) != ROADMAP_PLUGIN_ID)) {

      roadmap_messagebox ("Error", "Can't find a road near destination point.");
      return -1;
   }

   *to_line = roadmap_plugin_get_line_id (&line);

   switch (roadmap_line_route_get_direction (*to_line, ROUTE_CAR_ALLOWED)) {
      case ROUTE_DIRECTION_ANY:
      case ROUTE_DIRECTION_NONE:
         roadmap_line_points (*to_line, &from_tmp, &to_tmp);
         roadmap_point_position (from_tmp, &from_position);
         roadmap_point_position (to_tmp, &to_position);

         if (roadmap_math_distance (position, &from_position) <
             roadmap_math_distance (position, &to_position)) {
            *to_point = from_tmp;
         } else {
            *to_point = to_tmp;
         }
         break;
      case ROUTE_DIRECTION_WITH_LINE:
         roadmap_line_points (*to_line, &to_tmp, to_point);
         break;
      case ROUTE_DIRECTION_AGAINST_LINE:
         roadmap_line_points (*to_line, to_point, &to_tmp);
         break;
      default:
         roadmap_line_points (*to_line, &to_tmp, to_point);
   }

   return 0;
}


int navigate_is_enabled (void) {
   return NavigateEnabled;
}


void navigate_main_initialize (void) {

   NavigatePen = roadmap_canvas_create_pen ("NavigatePen1");
   roadmap_canvas_set_foreground ("blue");
   roadmap_canvas_set_thickness (3);

   navigate_main_set (1);

   NavigatePluginID = navigate_plugin_register ();
}


void navigate_main_set (int status) {

   if (status && NavigateEnabled) {
      return;
   } else if (!status && !NavigateEnabled) {
      return;
   }

   NavigateEnabled = status;
}


void navigate_main_calc_route () {

   int track_time;
   int from_line;
   int to_line;
   int from_point;
   int to_point;
   NavigateNumSegments = MAX_NAV_SEGEMENTS - 2;

   if (navigate_load_data () < 0) return;

   if (navigate_find_track_points
         (&from_line, &from_point, &to_line, &to_point) < 0) {

      return;
   }

   track_time =
      navigate_get_route_segments
            (from_point, to_point, NavigateSegments, &NavigateNumSegments);

   if (track_time <= 0) {
      NavigateTrackEnabled = 0;
      if (track_time < 0) {
         roadmap_messagebox("Error", "Error calculating route.");
      } else {
         roadmap_messagebox("Error", "Can't find a route.");
      }
   } else {
      char msg[200];
      int i;
      int length = 0;

      if (NavigateSegments[0] != from_line) {
         memmove (&NavigateSegments[1], &NavigateSegments[0],
               NavigateNumSegments * sizeof(NavigateSegments[0]));
         NavigateNumSegments++;
         NavigateSegments[0] = from_line;
      }

      if (NavigateSegments[NavigateNumSegments-1] != to_line) {
         NavigateSegments[NavigateNumSegments++] = to_line;
      }

      for (i=0; i<NavigateNumSegments; i++) {
         length += roadmap_line_length (NavigateSegments[i]);
      }

      snprintf(msg, sizeof(msg), "Length: %.1f km\nTime: %.1f minutes", length/1000.0, track_time/60.0);
      NavigateTrackEnabled = 1;
      roadmap_screen_redraw ();
      roadmap_messagebox ("Route found", msg);
   }
}


int navigate_main_override_pen (int line,
                                int cfcc,
                                int fips,
                                int pen_type,
                                RoadMapPen *override_pen) {

   int j;

   for (j=0; j<NavigateNumSegments; j++) {

      if (line == NavigateSegments[j]) {
         *override_pen = NavigatePen;
         return 1;
      }
   }

   *override_pen = NULL;
   return 0;
}


