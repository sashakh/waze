/*
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright (c) 2008, 2009 by Danny Backx.
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

/**
 * @file 
 * @brief Navigate plugin, main plugin file
 */

/**
 * @defgroup NavigatePlugin Navigate Plugin
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
#include "roadmap_tripdb.h"
#include "roadmap_navigate.h"
#include "roadmap_screen.h"
// #include "roadmap_line_route.h"
#include "roadmap_math.h"
#include "roadmap_point.h"
#include "roadmap_layer.h"
#include "roadmap_adjust.h"
#include "roadmap_lang.h"
#include "roadmap_address.h"
#include "roadmap_sound.h"
#include "roadmap_locator.h"
#include "roadmap_config.h"
#include "roadmap_skin.h"
#include "roadmap_dialog.h"
#include "roadmap_main.h"

#include "navigate_plugin.h"
#include "navigate_bar.h"
#include "navigate_cost.h"
#include "navigate_route.h"
#include "navigate.h"

#define ROUTE_PEN_WIDTH 4

static RoadMapConfigDescriptor NavigateConfigRouteColor =
                    ROADMAP_CONFIG_ITEM("Navigation", "RouteColor");

static RoadMapConfigDescriptor NavigateConfigPossibleRouteColor =
                    ROADMAP_CONFIG_ITEM("Navigation", "PossibleRouteColor");

RoadMapConfigDescriptor NavigateConfigAutoZoom =
                  ROADMAP_CONFIG_ITEM("Routing", "Auto zoom");

RoadMapConfigDescriptor NavigateConfigLastPos =
                  ROADMAP_CONFIG_ITEM("Navigation", "Last position");

RoadMapConfigDescriptor NavigateConfigNavigating =
                  ROADMAP_CONFIG_ITEM("Navigation", "Is navigating");

int NavigateEnabled = 0;
int NavigatePluginID = -1;
static int NavigateTrackEnabled = 0;
static int NavigateTrackFollowGPS = 0;
static RoadMapPen NavigatePen[2];
static RoadMapPen NavigatePenEst[2];

static void navigate_update (RoadMapPosition *position, PluginLine *current);
static void navigate_get_next_line
          (PluginLine *current, int direction, PluginLine *next);

static RoadMapCallback NextMessageUpdate;

static int NavigateDistanceToDest;
static int NavigateETA;
static int NavigateDistanceToTurn;
static int NavigateETAToTurn;
static int NavigateFlags;

static NavigateSegment NavigateSegments[MAX_NAV_SEGMENTS];
static int NavigateNumSegments = 0;
static int NavigateCurrentSegment = 0;
static PluginLine NavigateDestination = PLUGIN_LINE_NULL;
static int NavigateDestPoint;
static RoadMapPosition NavigateDestPos;
static RoadMapPosition NavigateSrcPos;
static int NavigateNextAnnounce;

/**
 * @brief
 * @return
 */
static int navigate_recalc_route ()
{
#if 0
   int track_time;
   PluginLine from_line;
   int from_point;
   int flags;

   NavigateNumSegments = MAX_NAV_SEGMENTS;

   if (navigate_route_load_data () < 0) {
      return -1;
   }

   if (navigate_find_track_points
         (&from_line, &from_point,
          &NavigateDestination, &NavigateDestPoint) < 0) {

      return -1;
   }

   roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);

   flags = RECALC_ROUTE;
   navigate_cost_reset ();
   track_time =
      navigate_route_get_segments
            (&from_line, from_point, &NavigateDestination, NavigateDestPoint,
             NavigateSegments, &NavigateNumSegments,
             &flags);

   roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);

   if (track_time <= 0) {
      return -1;
   }

   NavigateFlags = flags;

   navigate_instr_prepare_segments (NavigateSegments, NavigateNumSegments,
                                   &NavigateSrcPos, &NavigateDestPos);
   NavigateTrackEnabled = 1;
   navigate_bar_set_mode (NavigateTrackEnabled);
   NavigateCurrentSegment = 0;
   roadmap_navigate_enable ();
#endif
   return 0;
}

/****** Route calculation progress dialog ******/
/**
 * @brief
 * @param name
 * @param data
 */
static void cancel_calc (const char *name, void *data) {
}

/**
 * @brief
 */
static void show_progress_dialog (void) {

   if (roadmap_dialog_activate ("Route calc", NULL)) {

      roadmap_dialog_new_label ("Calculating", "Calculting route, please wait...");
      roadmap_dialog_new_progress ("Calculating", "Progress");

      roadmap_dialog_add_button ("Cancel", cancel_calc);

      roadmap_dialog_complete (0);
   }

   roadmap_dialog_set_progress ("Calculating", "Progress", 0);

   roadmap_main_flush ();
}

/**
 * @brief
 * @param position
 * @param current
 */
static void navigate_update (RoadMapPosition *position, PluginLine *current)
{
#if 0
   int announce = 0;
   const NavigateSegment *segment = NavigateSegments + NavigateCurrentSegment;
   const NavigateSegment *next_turn_segment;
   int group_id = segment->group_id;
   const char *inst_text = "";
   const char *inst_voice = NULL;
   RoadMapSoundList sound_list;
   const int ANNOUNCES[] = { 800, 200, 50 };
   int announce_distance = 0;
   int distance_to_prev;
   int distance_to_next;

#define NAVIGATE_COMPENSATE 20


   if (!NavigateTrackEnabled) return;

   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      NavigateDistanceToTurn =
         navigate_instr_calc_length (position, segment, LINE_END);
   } else {

      NavigateDistanceToTurn =
         navigate_instr_calc_length (position, segment, LINE_START);
   }

   distance_to_prev = segment->distance - NavigateDistanceToTurn;

   NavigateETAToTurn = (int) (1.0 * segment->cross_time * NavigateDistanceToTurn /
                             (segment->distance + 1));

   while (segment < (NavigateSegments + NavigateNumSegments - 1)) {
      if ((segment+1)->group_id != group_id) break;
      segment++;
      NavigateDistanceToTurn += segment->distance;
      NavigateETAToTurn += segment->cross_time;
   }

   distance_to_next = 0;

   if (segment <  (NavigateSegments + NavigateNumSegments - 1)) {
      next_turn_segment = segment + 1;
      group_id = next_turn_segment->group_id;
      distance_to_next = next_turn_segment->distance;
      while (next_turn_segment < (NavigateSegments + NavigateNumSegments - 1)) {
         if ((next_turn_segment+1)->group_id != group_id) break;
         next_turn_segment++;
         distance_to_next += next_turn_segment->distance;
      }
   }

   if (roadmap_config_match(&NavigateConfigAutoZoom, "yes")) {
      const char *focus = roadmap_trip_get_focus_name ();

      if (focus && !strcmp (focus, "GPS")) {

         navigate_zoom_update (position, NavigateSegments,
                               NavigateCurrentSegment, segment,
                               distance_to_prev,
                               distance_to_next);
      }
   }
      
   navigate_bar_set_distance (NavigateDistanceToTurn);

   switch (segment->instruction) {

      case NAVIGATE_TURN_LEFT:
         inst_text = "Turn left";
         inst_voice = "TurnLeft";
         break;
      case NAVIGATE_KEEP_LEFT:
         inst_text = "Keep left";
         inst_voice = "KeepLeft";
         break;
      case NAVIGATE_TURN_RIGHT:
         inst_text = "Turn right";
         inst_voice = "TurnRight";
         break;
      case NAVIGATE_KEEP_RIGHT:
         inst_text = "Keep right";
         inst_voice = "KeepRight";
         break;
      case NAVIGATE_APPROACHING_DESTINATION:
         inst_text = "Approaching destination";
         break;
      case NAVIGATE_CONTINUE:
         inst_text = "Continue straight";
         inst_voice = "Straight";
         break;
      default:
         break;
   }

   if ((segment->instruction == NAVIGATE_APPROACHING_DESTINATION) &&
        NavigateDistanceToTurn <= 20) {

      sound_list = roadmap_sound_list_create (0);
      roadmap_sound_list_add (sound_list, "Arrive");
      roadmap_sound_play_list (sound_list);

      NavigateTrackEnabled = 0;
      navigate_bar_set_mode (NavigateTrackEnabled);

      if (roadmap_config_match(&NavigateConfigAutoZoom, "yes")) {
         const char *focus = roadmap_trip_get_focus_name ();
         /* We used auto zoom, so now we need to reset it */

         if (focus && !strcmp (focus, "GPS")) {

            roadmap_screen_zoom_reset ();
         }
      }

      roadmap_navigate_end_route (NavigateCallbacks);
      return;
   }

   roadmap_message_set ('I', inst_text);

   if (NavigateNextAnnounce == -1) {

      unsigned int i;

      for (i=0; i<sizeof(ANNOUNCES)/sizeof(ANNOUNCES[0]) - 1; i++) {
         
         if (NavigateDistanceToTurn > ANNOUNCES[i]) {
            NavigateNextAnnounce = ANNOUNCES[i];
            break;
         }
      }
         
      if (NavigateNextAnnounce == -1) {
         NavigateNextAnnounce =
            ANNOUNCES[sizeof(ANNOUNCES)/sizeof(ANNOUNCES[0]) - 1];
      }
   }

   if (NavigateNextAnnounce &&
      (NavigateDistanceToTurn <=
        (NavigateNextAnnounce + NAVIGATE_COMPENSATE))) {
      unsigned int i;

      announce_distance = NavigateNextAnnounce;
      NavigateNextAnnounce = 0;

      if (inst_voice) {
         announce = 1;
      }

      for (i=0; i<sizeof(ANNOUNCES)/sizeof(ANNOUNCES[0]); i++) {
         if ((ANNOUNCES[i] < announce_distance) &&
             (NavigateDistanceToTurn > ANNOUNCES[i])) {
            NavigateNextAnnounce = ANNOUNCES[i];
            break;
         }
      }
   }

   if (announce) {
      PluginStreetProperties properties;

      if (segment < (NavigateSegments + NavigateNumSegments - 1)) {
         segment++;
      }

      roadmap_plugin_get_street_properties (&segment->line, &properties);

      roadmap_message_set ('#', properties.address);
      roadmap_message_set ('N', properties.street);
#ifdef	HAVE_STREET_T2S
      roadmap_message_set ('T', properties.street_t2s);
#endif
      roadmap_message_set ('C', properties.city);

      sound_list = roadmap_sound_list_create (0);
      if (!NavigateNextAnnounce) {
         roadmap_message_unset ('w');
      } else {

         char distance_str[100];
         int distance_far =
            roadmap_math_to_trip_distance(announce_distance);

         roadmap_sound_list_add (sound_list, "within");

         if (distance_far > 0) {
            roadmap_message_set ('w', "%d %s",
                  distance_far, roadmap_math_trip_unit());

            sprintf(distance_str, "%d", distance_far);
            roadmap_sound_list_add (sound_list, distance_str);
            roadmap_sound_list_add (sound_list, roadmap_math_trip_unit());
         } else {
            roadmap_message_set ('w', "%d %s",
                  announce_distance, roadmap_math_distance_unit());

            sprintf(distance_str, "%d", announce_distance);
            roadmap_sound_list_add (sound_list, distance_str);
            roadmap_sound_list_add (sound_list, roadmap_math_distance_unit());
         };
      }

      roadmap_sound_list_add (sound_list, inst_voice);
      //roadmap_voice_announce ("Driving Instruction");
      roadmap_sound_play_list (sound_list);
   }
#endif
}

/**
 * @brief gets called after every GPS input, choose when to do something
 */
void navigate_format_messages(void)
{
	static int counter = 0;

	counter++;

	/* Don't call this so often */
	if (counter < 10)
		return;

	counter = 0;
	roadmap_log (ROADMAP_WARNING, "navigate_format_messages -> navigate_update(NULL, NULL);");
	navigate_update(NULL, NULL);
}

/**
 * @brief
 * @param current
 * @param direction
 * @param next
 */
void navigate_get_next_line (PluginLine *current, int direction, PluginLine *next)
{
#if 0
   int new_instruction = 0;

   if (!NavigateTrackEnabled) {
      
      if (navigate_recalc_route () != -1) {

         roadmap_trip_route_stop ();
      }

      return;
   }

   /* Ugly hack as we don't support navigation through editor lines */
   if (roadmap_plugin_get_id (current) != ROADMAP_PLUGIN_ID) {
      *next = NavigateSegments[NavigateCurrentSegment+1].line;
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
      
      if (navigate_recalc_route () == -1) {

         roadmap_trip_route_start ();
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
#warning "roadmap_plugin_get_street_properties not right"
#if 0
      roadmap_plugin_get_street_properties (&segment->line, &properties, 0);
#endif
      navigate_bar_set_street (properties.street);

      NavigateNextAnnounce = -1;

      NavigateDistanceToDest = 0;
      NavigateETA = 0;

      if (segment->group_id != group_id) {

         /* Update distance to destination and ETA
          * excluding current group (computed in navigate_update)
          */

         while (segment < NavigateSegments + NavigateNumSegments) {

            NavigateDistanceToDest += segment->distance;
            NavigateETA            += segment->cross_time;
            segment++;
         }
      }
   }

   return;
#endif
}

/**
 * @brief
 * @return
 */
int navigate_is_enabled (void) {
   return NavigateEnabled;
}

/**
 * @brief
 */
static void navigate_init_pens (void)
{
	RoadMapPen pen;

	roadmap_log (ROADMAP_DEBUG, "navigate_init_pens");

	pen = roadmap_canvas_create_pen ("NavigatePen1");
	roadmap_canvas_set_foreground (roadmap_config_get (&NavigateConfigRouteColor));
	roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
	NavigatePen[0] = pen;

	pen = roadmap_canvas_create_pen ("NavigatePen2");
	roadmap_canvas_set_foreground (roadmap_config_get (&NavigateConfigRouteColor));
	roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
	NavigatePen[1] = pen;

	pen = roadmap_canvas_create_pen ("NavigatePenEst1");
	roadmap_canvas_set_foreground (roadmap_config_get (&NavigateConfigPossibleRouteColor));
	roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
	NavigatePenEst[0] = pen;

	pen = roadmap_canvas_create_pen ("NavigatePenEst2");
	roadmap_canvas_set_foreground (roadmap_config_get (&NavigateConfigPossibleRouteColor));
	roadmap_canvas_set_thickness (ROUTE_PEN_WIDTH);
	NavigatePenEst[1] = pen;
}

/**
 * @brief
 */
void navigate_shutdown (void)
{
	roadmap_config_set_integer (&NavigateConfigNavigating, 0);
}

/**
 * @brief
 */
void navigate_initialize (void)
{
	roadmap_log (ROADMAP_DEBUG, "navigate_initialize");
	roadmap_config_declare ("preferences", &NavigateConfigRouteColor,  "#00ff00a0");
	roadmap_config_declare ("preferences", &NavigateConfigPossibleRouteColor,  "#ff0000a0");
	roadmap_config_declare_enumeration
		("preferences", &NavigateConfigAutoZoom, "no", "yes", NULL);
	roadmap_config_declare ("session",  &NavigateConfigLastPos, "0, 0");
	roadmap_config_declare ("session",  &NavigateConfigNavigating, "0");

	navigate_init_pens ();
	navigate_cost_initialize ();
	navigate_bar_initialize ();
	navigate_visual_initialize ();
	navigate_simple_initialize ();

	navigate_set (1);

//	roadmap_address_register_nav (navigate_address_cb);
	roadmap_skin_register (navigate_init_pens);

#if 0
	/* FIX ME need to figure out why this causes roadmap to stop */
	if (roadmap_config_get_integer (&NavigateConfigNavigating)) {
		RoadMapPosition pos;
		roadmap_config_get_position (&NavigateConfigLastPos, &pos);
		roadmap_trip_set_focus ("GPS");
		roadmap_trip_set_point ("Destination", &pos);

		navigate_calc_route ();
	}
#endif
}

/**
 * @brief enable or disable navigation
 * @param status whether to set NavigateEnabled
 */
void navigate_set (int status)
{
	if (status && NavigateEnabled) {
		return;
	} else if (!status && !NavigateEnabled) {
		return;
	}

	NavigateEnabled = status;
}

/**
 * @brief Calculate an initial route between the GPS position and the destination
 * @return success
 */
int navigate_calc_route ()
{
	int			track_time;
	PluginLine		from_line;
	RoadMapPosition		from_pos, *fp, *dp;
	int			flags;
	NavigateStatus		status;
	NavigateIteration	*p;

	const char *focus = roadmap_trip_get_focus_name ();

	if (!(fp = roadmap_trip_get_position ("Departure"))) {
		roadmap_log (ROADMAP_DEBUG, "navigate_calc_route: no departure");
	}

	from_pos = *fp;
	if (!(dp = roadmap_trip_get_position ("Destination"))) {
		roadmap_log (ROADMAP_DEBUG, "navigate_calc_route: no destination");
	}

	roadmap_log(ROADMAP_DEBUG, "navigate_calc_route : start");
	NavigateDestPos = *dp;
	NavigateTrackEnabled = 0;
	navigate_bar_set_mode (NavigateTrackEnabled);

	NavigateNumSegments = MAX_NAV_SEGMENTS;

	show_progress_dialog ();

	navigate_cost_reset ();

	status = navigate_route_get_initial (&from_line, from_pos,
			&NavigateDestination, NavigateDestPos);

	roadmap_log(ROADMAP_DEBUG, "navigate_calc_route: time %d", track_time);

	if (status.current == 0 || status.current->cost.distance <= 0) {
		roadmap_dialog_hide ("Route calc");
		NavigateTrackEnabled = 0;
		navigate_bar_set_mode (NavigateTrackEnabled);
		if (track_time < 0) {
			roadmap_messagebox("Error", "Error calculating route.");
		} else {
			roadmap_messagebox("Error", "Can't find a route.");
		}

		return -1;
	} else {
		char msg[200] = {0};
		int i;
		int length = 0;

		length = status.current->cost.distance;
		track_time = status.current->cost.time;

		roadmap_dialog_hide ("Route calc");
		NavigateFlags = flags;

		if (flags & GRAPH_IGNORE_TURNS) {
			snprintf(msg, sizeof(msg), "%s\n",
			roadmap_lang_get ("The calculated route may have incorrect turn instructions."));
		}

		snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
				"%s: %.1f %s,\n%s: %.1f %s",
				roadmap_lang_get ("Length"),
				length/1000.0,
				roadmap_lang_get ("Km"),
				roadmap_lang_get ("Time"),
				track_time/60.0,
				roadmap_lang_get ("minutes"));

		NavigateTrackEnabled = 1;
		navigate_bar_set_mode (NavigateTrackEnabled);
#if 1
		roadmap_navigate_enable ();
#else
		if (NavigateTrackFollowGPS) {
			NavigateCurrentSegment = 0;
			roadmap_trip_route_stop ();
			roadmap_navigate_enable ();
		}
#endif

		roadmap_screen_repaint ();
		roadmap_config_set_position (&NavigateConfigLastPos, &NavigateDestPos);
		roadmap_config_set_integer (&NavigateConfigNavigating, 1);
		roadmap_config_save (0);
		roadmap_messagebox ("Route found", msg);

		/* Stuff this route in the trip plugin */
		roadmap_trip_new();
		roadmap_tripdb_empty_list();

		roadmap_trip_add_waypoint("",
				&status.first->segment->from_pos,
				TRIP_PLACE_ROUTE_MARK_START);

		for (i=0, p=status.first; p; p = p->next, i++) {
			roadmap_trip_add_point_way(
				p->segment->from_point,
				p->segment->to_point,
				p->segment->line,
				(i == NavigateNumSegments - 1)
					?  TRIP_APPROACHING_DESTINATION : TRIP_CONTINUE);

			roadmap_trip_add_waypoint("",
				&p->segment->to_pos,
				TRIP_PLACE_ROUTE_MARK_INSERT);
		}
		roadmap_trip_complete();

		roadmap_trip_refresh();
	}

	return 0;
}

/**
 * @brief
 * @return
 */
int navigate_reload_data (void)
{
//	navigate_traffic_refresh ();
	return navigate_route_reload_data ();
}

/**
 * @brief
 * @param max_pen
 */
void navigate_screen_repaint (int max_pen)
{
#if 0
   int i;
   int current_width = -1;
   int last_cfcc = -1;
   RoadMapPen *pens;
   int current_pen = 0;

   if (!NavigateTrackEnabled)
	   return;

   if (NavigateFlags & GRAPH_IGNORE_TURNS) {
      pens = NavigatePenEst;
   } else {
      pens = NavigatePen;
   }

   if (!NavigateTrackFollowGPS && !strcmp (roadmap_trip_get_focus_name (), "GPS")) {
      NavigateTrackFollowGPS = 1;

      roadmap_trip_route_stop ();

      if (roadmap_trip_get_position ("Departure")) {
         roadmap_tripdb_remove_point ("Departure");
      }
      roadmap_navigate_enable ();
   }

   for (i=0; i<NavigateNumSegments; i++) {
      NavigateSegment *segment = NavigateSegments + i;

      if (segment->line.layer != last_cfcc) {
         RoadMapPen layer_pen = roadmap_layer_get_pen (segment->line.layer, 0);
         int width;
         
         if (layer_pen)
		 width = roadmap_canvas_get_thickness (layer_pen) + 4;
         else
		 width = ROUTE_PEN_WIDTH;

         if (width < ROUTE_PEN_WIDTH) {
            width = ROUTE_PEN_WIDTH;
         }

         if (width != current_width) {
            RoadMapPen previous_pen;

            current_pen = (current_pen+1)%2;
            previous_pen = roadmap_canvas_select_pen (pens[current_pen]);
            roadmap_canvas_set_thickness (width);
            current_width = width;

            if (previous_pen) {
               roadmap_canvas_select_pen (previous_pen);
            }
         }

         last_cfcc = segment->line.layer;
      }

      roadmap_screen_draw_one_line_from_to (&segment->from_pos,
                                    &segment->to_pos,
                                    0,
                                    &segment->shape_initial_pos,
                                    segment->first_shape,
                                    segment->last_shape,
                                    segment->shape_itr,
                                    pens + current_pen,
                                    1,
                                    NULL,
                                    NULL,
                                    NULL);
   }
#endif
}

/**
 * @brief
 * @param
 */
void navigate_update_progress (int progress)
{
   progress = progress * 9 / 10;
   roadmap_dialog_set_progress ("Calculating", "Progress", progress);
   roadmap_log(ROADMAP_DEBUG, "update_progress(%d)", progress);

   roadmap_main_flush ();
}

