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
#include <stdlib.h>
#include "type.h"
#include "graph.h"

#include "roadmap.h"
#include "roadmap_pointer.h"
#include "roadmap_plugin.h"
#include "roadmap_line.h"
#include "roadmap_display.h"
#include "roadmap_message.h"
#include "roadmap_voice.h"
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
#include "roadmap_layer.h"

#include "navigate_plugin.h"
#include "navigate_graph.h"
#include "navigate_bar.h"
#include "navigate_instr.h"
#include "navigate_main.h"

#define ROUTE_PEN_WIDTH 4

int NavigateEnabled = 0;
int NavigatePluginID = -1;
static int NavigateTrackEnabled = 0;
static RoadMapPen NavigatePen;

static void navigate_update (RoadMapPosition *position, PluginLine *current);
static void navigate_get_next_line
          (PluginLine *current, int direction, PluginLine *next);

static RoadMapCallback NextMessageUpdate;

static int NavigateDistanceToDest;
static int NavigateETA;
static int NavigateDistanceToTurn;
static int NavigateETAToTurn;

RoadMapNavigateRouteCB NavigateCallbacks = {
   &navigate_update,
   &navigate_get_next_line
};


#define MAX_NAV_SEGEMENTS 500

static NavigateSegment NavigateSegments[MAX_NAV_SEGEMENTS];
static int NavigateNumSegments = 0;
static int NavigateCurrentSegment = 0;
static PluginLine NavigateDestination = PLUGIN_LINE_NULL;
static int NavigateDestPoint;
static RoadMapPosition NavigateDestPos;
static RoadMapPosition NavigateSrcPos;
static int NavigateNextAnnounce;


static int navigate_find_track_points (PluginLine *from_line, int *from_point,
                                       PluginLine *to_line, int *to_point,
                                       int use_gps_location) {

   const RoadMapPosition *position = NULL;
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

         NavigateSrcPos = pos;
         roadmap_line_points (line.line_id, &from_tmp, &to_tmp);

         if (direction == ROUTE_DIRECTION_WITH_LINE) {

            *from_point = to_tmp;
         } else {

            *from_point = from_tmp;
         }

      } else {
         position = roadmap_trip_get_position ("GPS");
         NavigateSrcPos = *position;
      }

   } else {

      position = roadmap_trip_get_position ("Departure");
      NavigateSrcPos = *position;
   }

   if (*from_point == -1) {

      if (!position) return -1;

      if ((roadmap_navigate_retrieve_line
               (position, 20, &line, &distance, LAYER_ALL_ROADS) == -1) ||
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

   NavigateDestPos = *position;

   if ((roadmap_navigate_retrieve_line
            (position, 20, &line, &distance, LAYER_ALL_ROADS) == -1) ||
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
         roadmap_line_points (to_line->line_id, to_point, &to_tmp);
         break;
      case ROUTE_DIRECTION_AGAINST_LINE:
         roadmap_line_points (to_line->line_id, &to_tmp, to_point);
         break;
      default:
         roadmap_line_points (to_line->line_id, &to_tmp, to_point);
   }

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

   navigate_instr_prepare_segments (NavigateSegments, NavigateNumSegments,
                                   &NavigateSrcPos, &NavigateDestPos);
   NavigateTrackEnabled = 1;
   navigate_bar_set_mode (NavigateTrackEnabled);
   NavigateCurrentSegment = 0;
   roadmap_navigate_route (NavigateCallbacks);

   return 0;
}


static void navigate_main_format_messages (void) {

   int distance_to_destination;
   int distance_to_destination_far;
   int ETA;
   char str[100];

   (*NextMessageUpdate) ();

   if (!NavigateTrackEnabled) return;

   distance_to_destination = NavigateDistanceToDest + NavigateDistanceToTurn;
   ETA = NavigateETA + NavigateETAToTurn + 60;

   distance_to_destination_far =
      roadmap_math_to_trip_distance(distance_to_destination);

   if (distance_to_destination_far > 0) {
      roadmap_message_set ('D', "%d %s",
            distance_to_destination_far,
            roadmap_math_trip_unit());
   } else {
      roadmap_message_set ('D', "%d %s",
            distance_to_destination,
            roadmap_math_distance_unit());
   };

   sprintf (str, "%d:%02d", ETA / 3600, ETA / 60);
   roadmap_message_set ('T', str);
}


void navigate_update (RoadMapPosition *position, PluginLine *current) {

   int annouce = 0;
   const NavigateSegment *segment = NavigateSegments + NavigateCurrentSegment;
   int group_id = segment->group_id;
   const char *inst_text = "";

   if (!NavigateTrackEnabled) return;

   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      NavigateDistanceToTurn =
         navigate_instr_calc_length (position, segment, LINE_END);
   } else {

      NavigateDistanceToTurn =
         navigate_instr_calc_length (position, segment, LINE_START);
   }

   NavigateETAToTurn = (int) (1.0 * segment->cross_time * NavigateDistanceToTurn /
                             (segment->distance + 1));

   while (segment < (NavigateSegments + NavigateNumSegments - 1)) {
      if ((segment+1)->group_id != group_id) break;
      segment++;
      NavigateDistanceToTurn += segment->distance;
      NavigateETAToTurn += segment->cross_time;
   }

      
   navigate_bar_set_distance (NavigateDistanceToTurn);

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
      case APPROACHING_DESTINATION:
         inst_text = "Approaching destination";
         break;
      case CONTINUE:
         inst_text = "Continue straight";
         break;
      default:
         break;
   }

   if ((segment->instruction == APPROACHING_DESTINATION) &&
        NavigateDistanceToTurn <= 10) {

      NavigateTrackEnabled = 0;
      navigate_bar_set_mode (NavigateTrackEnabled);
      roadmap_navigate_end_route (NavigateCallbacks);
      return;
   }

   roadmap_message_set ('I', inst_text);

   if (NavigateNextAnnounce == -1) {

      NavigateNextAnnounce = 50;

      annouce = 1;
   }

   if (NavigateDistanceToTurn <= NavigateNextAnnounce) {
      NavigateNextAnnounce = 0;
      annouce = 1;
   }

   if (annouce) {
      PluginStreetProperties properties;

      if (segment < (NavigateSegments + NavigateNumSegments - 1)) {
         segment++;
      }

      roadmap_plugin_get_street_properties (&segment->line, &properties);

      roadmap_message_set ('#', properties.address);
      roadmap_message_set ('N', properties.street);
      roadmap_message_set ('T', properties.street_t2s);
      roadmap_message_set ('C', properties.city);

      if (NavigateDistanceToTurn <= 50) {
         roadmap_message_unset ('w');
      } else {

         int distance_far =
            roadmap_math_to_trip_distance(NavigateDistanceToTurn);

         if (distance_far > 0) {
            roadmap_message_set ('w', "%d %s",
                  distance_far, roadmap_math_trip_unit());
         } else {
            roadmap_message_set ('w', "%d %s",
                  NavigateDistanceToTurn, roadmap_math_distance_unit());
         };
      }

      roadmap_voice_announce ("Driving Instruction");
   }

}


void navigate_get_next_line
          (PluginLine *current, int direction, PluginLine *next) {

   int new_instruction = 0;

   if (!NavigateTrackEnabled) {
      
      if (navigate_main_recalc_route () != -1) {

         roadmap_trip_stop ();
      }

      return;
   }

   if (!roadmap_plugin_same_line
         (current, &NavigateSegments[NavigateCurrentSegment].line)) {

      int i;
      for (i=NavigateCurrentSegment+1; i < NavigateNumSegments; i++) {
         
         if (roadmap_plugin_same_line
            (current, &NavigateSegments[i].line)) {

            if (NavigateSegments[NavigateCurrentSegment].group_id !=
                  NavigateSegments[i].group_id) {

               new_instruction = 1;
            }

            NavigateCurrentSegment = i;
            break;
         }
      }
   }

   if ((NavigateCurrentSegment < NavigateNumSegments) &&
       !roadmap_plugin_same_line
         (current, &NavigateSegments[NavigateCurrentSegment].line)) {

      NavigateTrackEnabled = 0;
      navigate_bar_set_mode (NavigateTrackEnabled);
      
      if (navigate_main_recalc_route () == -1) {

         roadmap_trip_start ();
      }

      NavigateNextAnnounce = -1;

      return;
   }

   if ((NavigateCurrentSegment+1) >= NavigateNumSegments) {

      next->plugin_id = INVALID_PLUGIN_ID;
   } else {

      *next = NavigateSegments[NavigateCurrentSegment+1].line;
   }

   if (new_instruction || !NavigateCurrentSegment) {
      int group_id;

      /* new driving instruction */

      PluginStreetProperties properties;
      const NavigateSegment *segment =
               NavigateSegments + NavigateCurrentSegment;

      while (segment < NavigateSegments + NavigateNumSegments - 1) {
         if ((segment + 1)->group_id != segment->group_id) break;
         segment++;
      }

      navigate_bar_set_instruction (segment->instruction);

      group_id = segment->group_id;
      if (segment < NavigateSegments + NavigateNumSegments - 1) {
         /* we need the name of the next street */
         segment++;
      }
      roadmap_plugin_get_street_properties (&segment->line, &properties);
      navigate_bar_set_street (properties.street);

      NavigateNextAnnounce = -1;

      if (segment->group_id != group_id) {

         /* Update distance to destination and ETA
          * excluding current group (computed in navigate_update)
          */

         NavigateDistanceToDest = 0;
         NavigateETA = 0;

         while (segment < NavigateSegments + NavigateNumSegments) {

            NavigateDistanceToDest += segment->distance;
            NavigateETA            += segment->cross_time;
            segment++;
         }
      }
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

   navigate_bar_initialize ();
   navigate_main_set (1);

    NextMessageUpdate =
       roadmap_message_register (navigate_main_format_messages);

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

   NavigateDestination.plugin_id = INVALID_PLUGIN_ID;
   NavigateTrackEnabled = 0;
   navigate_bar_set_mode (NavigateTrackEnabled);

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
      navigate_bar_set_mode (NavigateTrackEnabled);
      if (track_time < 0) {
         roadmap_messagebox("Error", "Error calculating route.");
      } else {
         roadmap_messagebox("Error", "Can't find a route.");
      }
   } else {
      char msg[200];
      int i;
      int length = 0;

      navigate_instr_prepare_segments (NavigateSegments, NavigateNumSegments,
                                      &NavigateSrcPos, &NavigateDestPos);

      for (i=0; i<NavigateNumSegments; i++) {
         length += NavigateSegments[i].distance;
      }

      snprintf(msg, sizeof(msg), "Length: %.1f km\nTime: %.1f minutes",
            length/1000.0, track_time/60.0);

      NavigateTrackEnabled = 1;
      navigate_bar_set_mode (NavigateTrackEnabled);

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
                                    &segment->shape_inital_pos,
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


