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
#include "roadmap_display.h"
#include "roadmap_messagebox.h"
#include "roadmap_canvas.h"
#include "roadmap_street.h"
#include "roadmap_trip.h"
#include "roadmap_navigate.h"
#include "roadmap_screen.h"
#include "roadmap_line_route.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "roadmap_point.h"

#include "navigate_plugin.h"
#include "navigate_graph.h"
#include "roadmap_layer.h"
#include "navigate_main.h"

#define LINE_START 0
#define LINE_END   1

#define ROUTE_PEN_WIDTH 4

int NavigateEnabled = 0;
int NavigatePluginID = -1;
static int NavigateTrackEnabled = 0;
static RoadMapPen NavigatePen;

static void navigate_update (RoadMapPosition *position, PluginLine *current);
static void navigate_get_next_line
          (PluginLine *current, int direction, PluginLine *next);

RoadMapNavigateRouteCB NavigateCallbacks = {
   &navigate_update,
   &navigate_get_next_line
};


#define MAX_NAV_SEGEMENTS 100

static NavigateSegment NavigateSegments[MAX_NAV_SEGEMENTS];
static int NavigateNumSegments = 0;
static int NavigateCurrentSegment = 0;
static PluginLine NavigateDestination = PLUGIN_LINE_NULL;
static int NavigateDestPoint;


static int navigate_find_track_points (PluginLine *from_line, int *from_point,
                                       PluginLine *to_line, int *to_point,
                                       int use_gps_location) {

   const RoadMapPosition *position;
   RoadMapPosition from_position;
   RoadMapPosition to_position;
   PluginLine line;
   int distance;
   int from_tmp;
   int to_tmp;
   int direction = ROUTE_DIRECTION_NONE;

   *from_point = -1;

   if (use_gps_location) {

      RoadMapPosition pos;

      if (roadmap_navigate_get_current (&pos, &line, &direction) != -1) {

         roadmap_line_points (line.line_id, &from_tmp, &to_tmp);

         if (direction == ROUTE_DIRECTION_WITH_LINE) {

            *from_point = to_tmp;
         } else {

            *from_point = from_tmp;
         }

      } else {
         position = roadmap_trip_get_position ("GPS");
      }

   } else {

      position = roadmap_trip_get_position ("Departure");
   }

   if (*from_point == -1) {

      if (!position) return -1;

      if ((roadmap_navigate_retrieve_line
               (position, 20, &line, &distance) == -1) ||
            (roadmap_plugin_get_id (&line) != ROADMAP_PLUGIN_ID)) {

         roadmap_messagebox
            ("Error", "Can't find a road near departure point.");

         return -1;
      }

   }

   *from_line = line;

   if (direction == ROUTE_DIRECTION_NONE) {

      switch (roadmap_plugin_get_direction (from_line, ROUTE_CAR_ALLOWED)) {
         case ROUTE_DIRECTION_ANY:
         case ROUTE_DIRECTION_NONE:
            roadmap_line_points (from_line->line_id, &from_tmp, &to_tmp);
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
            roadmap_line_points (from_line->line_id, &from_tmp, from_point);
            break;
         case ROUTE_DIRECTION_AGAINST_LINE:
            roadmap_line_points (from_line->line_id, from_point, &from_tmp);
            break;
         default:
            roadmap_line_points (from_line->line_id, &from_tmp, to_point);
      }
   }

   
   if (to_line->plugin_id != INVALID_PLUGIN_ID) {
      /* we already calculated the destination point */
      return 0;
   }

   position = roadmap_trip_get_position ("Destination");
   if (!position) return -1;

   if ((roadmap_navigate_retrieve_line
            (position, 20, &line, &distance) == -1) ||
         (roadmap_plugin_get_id (&line) != ROADMAP_PLUGIN_ID)) {

      roadmap_messagebox ("Error", "Can't find a road near destination point.");
      return -1;
   }

   *to_line = line;

   switch (roadmap_plugin_get_direction (to_line, ROUTE_CAR_ALLOWED)) {
      case ROUTE_DIRECTION_ANY:
      case ROUTE_DIRECTION_NONE:
         roadmap_line_points (to_line->line_id, &from_tmp, &to_tmp);
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
         roadmap_line_points (to_line->line_id, &to_tmp, to_point);
         break;
      case ROUTE_DIRECTION_AGAINST_LINE:
         roadmap_line_points (to_line->line_id, to_point, &to_tmp);
         break;
      default:
         roadmap_line_points (to_line->line_id, &to_tmp, to_point);
   }

   return 0;
}


static int navigate_main_calc_azymuth (NavigateSegment *seg, int type) {

   RoadMapPosition start;
   RoadMapPosition end;
   RoadMapPosition *shape_pos;
   int shape;

   start = seg->from_pos;
   end   = seg->to_pos;

   if (seg->first_shape > -1) {

      int last_shape;

      if (type == LINE_START) {

         last_shape = seg->first_shape;
         shape_pos  = &end;
         *shape_pos = seg->from_pos;
      } else {

         last_shape = seg->last_shape;
         shape_pos  = &start;
      }

      for (shape = seg->first_shape; shape <= last_shape; shape++) {

         seg->shape_itr (shape, shape_pos);
      }
   }

   return roadmap_math_azymuth (&start, &end);
}


static void navigate_main_set_road_instr (NavigateSegment *seg1,
                                          NavigateSegment *seg2) {

   int seg1_azymuth;
   int seg2_azymuth;
   int delta;

   if (seg1->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg1_azymuth  = navigate_main_calc_azymuth (seg1, LINE_END);
   } else {
      seg1_azymuth  = 180 + navigate_main_calc_azymuth (seg1, LINE_START);
   }


   if (seg2->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg2_azymuth  = navigate_main_calc_azymuth (seg2, LINE_START);
   } else {
      seg2_azymuth  = 180 + navigate_main_calc_azymuth (seg2, LINE_END);
   }

   delta = seg1_azymuth - seg2_azymuth;

   while (delta > 180)  delta -= 360;
   while (delta < -180) delta += 360;

   if (delta < -30) {

      if (delta > -45) {
         seg1->instruction = KEEP_RIGHT;
      } else {
         seg1->instruction = TURN_RIGHT;
      }

   } else if (delta > 30) {

      if (delta < 45) {
         seg1->instruction = KEEP_LEFT;
      } else {
         seg1->instruction = TURN_LEFT;
      }
      
   } else {

      seg1->instruction = CONTINUE;
   }
}


static int navigate_main_prepare_segments (NavigateSegment *segments,
                                           int count) {

   int i;

   for (i=0; i < count; i++) {

      roadmap_plugin_get_line_points (&segments[i].line,
                                      &segments[i].from_pos,
                                      &segments[i].to_pos,
                                      &segments[i].first_shape,
                                      &segments[i].last_shape,
                                      &segments[i].shape_itr);

      roadmap_plugin_get_street (&segments[i].line, &segments[i].street);
   }

   for (i=0; i < count - 1; i++) {

      navigate_main_set_road_instr (&segments[i], &segments[i+1]);
   }

   segments[i].instruction = APPROCHING_DESTINATION;

   return 0;
}


static int navigate_main_recalc_route () {

   int track_time;
   PluginLine from_line;
   int from_point;

   NavigateNumSegments = MAX_NAV_SEGEMENTS;

   if (navigate_load_data () < 0) {
      return -1;
   }

   if (navigate_find_track_points
         (&from_line, &from_point,
          &NavigateDestination, &NavigateDestPoint, 1) < 0) {

      return -1;
   }

   track_time =
      navigate_get_route_segments
            (&from_line, from_point, &NavigateDestination, NavigateDestPoint,
             NavigateSegments, &NavigateNumSegments);

   if (track_time <= 0) {
      return -1;
   }

   navigate_main_prepare_segments (NavigateSegments, NavigateNumSegments);
   NavigateTrackEnabled = 1;
   NavigateCurrentSegment = 0;
   roadmap_navigate_route (NavigateCallbacks);

   return 0;
}


void navigate_update (RoadMapPosition *position, PluginLine *current) {

   int distance_to_destination;
   int distance_to_destination_far;
   const NavigateSegment *segment = NavigateSegments + NavigateCurrentSegment;
   const char *inst_text = "";

   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      distance_to_destination =
         roadmap_math_distance (position, &segment->to_pos);
   } else {

      distance_to_destination =
         roadmap_math_distance (position, &segment->from_pos);
   }

   switch (segment->instruction) {

      case TURN_LEFT:
         inst_text = "Turn left";
         break;
      case KEEP_LEFT:
         inst_text = "Keep left";
         break;
      case TURN_RIGHT:
         inst_text = "Turn right";
         break;
      case KEEP_RIGHT:
         inst_text = "Keep right";
         break;
      case CONTINUE:
         inst_text = "Continue straight";
         break;
      case APPROCHING_DESTINATION:
         inst_text = "Approaching destination";
         break;
   }

   distance_to_destination_far =
      roadmap_math_to_trip_distance(distance_to_destination);

   if (distance_to_destination_far > 0) {

      roadmap_display_text ("Driving Instruction", "%s, %d %s",
                            inst_text,
                            distance_to_destination_far,
                            roadmap_math_trip_unit());
   } else {

      roadmap_display_text ("Driving Instruction", "%s, %d %s",
                            inst_text,
                            distance_to_destination,
                            roadmap_math_distance_unit());
   };

}


void navigate_get_next_line
          (PluginLine *current, int direction, PluginLine *next) {

   if (!NavigateTrackEnabled) {
      
      if (navigate_main_recalc_route () != -1) {

         roadmap_trip_stop ();
      }

      return;
   }

   if (!roadmap_plugin_same_line
         (current, &NavigateSegments[NavigateCurrentSegment].line)) {
      NavigateCurrentSegment++;
   }

   if ((NavigateCurrentSegment < NavigateNumSegments) &&
       !roadmap_plugin_same_line
         (current, &NavigateSegments[NavigateCurrentSegment].line)) {

      NavigateTrackEnabled = 0;
      //roadmap_navigate_end_route (NavigateCallbacks);
      
      if (navigate_main_recalc_route () == -1) {

         roadmap_trip_start ();
      }
      //roadmap_messagebox("Error", "Lost route.");

      return;
   }

   if ((NavigateCurrentSegment+1) >= NavigateNumSegments) {

      next->plugin_id = INVALID_PLUGIN_ID;
   } else {
      *next = NavigateSegments[NavigateCurrentSegment].line;
   }

   return;
}


int navigate_is_enabled (void) {
   return NavigateEnabled;
}


void navigate_main_initialize (void) {

   NavigatePen = roadmap_canvas_create_pen ("NavigatePen1");
   roadmap_canvas_set_foreground ("blue");
   roadmap_canvas_set_opacity (160);
   roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);

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
   PluginLine from_line;
   int from_point;
   int use_gps_location = (strcmp (roadmap_trip_get_focus_name (), "GPS") == 0);
   use_gps_location = 0;

   NavigateDestination.plugin_id = INVALID_PLUGIN_ID;
   NavigateTrackEnabled = 0;

   NavigateNumSegments = MAX_NAV_SEGEMENTS;

   if (navigate_load_data () < 0) return;

   if (navigate_find_track_points
         (&from_line, &from_point, &NavigateDestination, &NavigateDestPoint,
          use_gps_location) < 0) {

      return;
   }

   track_time =
      navigate_get_route_segments
            (&from_line, from_point, &NavigateDestination, NavigateDestPoint,
             NavigateSegments, &NavigateNumSegments);

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

      navigate_main_prepare_segments (NavigateSegments, NavigateNumSegments);

      for (i=0; i<NavigateNumSegments; i++) {
         if (NavigateSegments[i].line.plugin_id == ROADMAP_PLUGIN_ID) {
            length += roadmap_line_length (NavigateSegments[i].line.line_id);
         }
      }

      snprintf(msg, sizeof(msg), "Length: %.1f km\nTime: %.1f minutes",
            length/1000.0, track_time/60.0);

      NavigateTrackEnabled = 1;

      if (use_gps_location) {
         NavigateCurrentSegment = 0;

         roadmap_trip_stop ();
         roadmap_navigate_route (NavigateCallbacks);
      }

      roadmap_screen_redraw ();
      roadmap_messagebox ("Route found", msg);
   }
}


void navigate_main_screen_repaint (int max_pen) {
   int i;
   int current_width = -1;
   int last_cfcc = -1;

   if (!NavigateTrackEnabled) return;

   for (i=0; i<NavigateNumSegments; i++) {

      NavigateSegment *segment = NavigateSegments + i;

      if (segment->line.cfcc != last_cfcc) {
         RoadMapPen pen = roadmap_layer_get_pen (segment->line.cfcc, 0);
         int width = roadmap_canvas_get_thickness (pen);

         if (width < ROUTE_PEN_WIDTH) {
            width = ROUTE_PEN_WIDTH;
         }

         if (width != current_width) {

            RoadMapPen previous_pen;
            previous_pen = roadmap_canvas_select_pen (NavigatePen);
            roadmap_canvas_set_thickness (width);
            current_width = width;

            if (previous_pen) {
               roadmap_canvas_select_pen (previous_pen);
            }
         }

         last_cfcc = segment->line.cfcc;
      }

      roadmap_screen_draw_one_line (&segment->from_pos,
                                    &segment->to_pos,
                                    0,
                                    &segment->from_pos,
                                    segment->first_shape,
                                    segment->last_shape,
                                    segment->shape_itr,
                                    NavigatePen,
                                    NULL,
                                    NULL,
                                    NULL);

   }
}


int navigate_main_override_pen (int line,
                                int cfcc,
                                int fips,
                                int pen_type,
                                RoadMapPen *override_pen) {

   return 0;

#if 0
   int j;

   for (j=0; j<NavigateNumSegments; j++) {

      if (line == NavigateSegments[j]) {
         *override_pen = NavigatePen;
         return 1;
      }
   }

   *override_pen = NULL;
   return 0;
#endif
}


