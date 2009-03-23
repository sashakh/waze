/*
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright (c) 2008, Danny Backx.
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
 * @brief implement plugin interfaces
 */

#include <stdlib.h>

#include "roadmap_plugin.h"
// #include "roadmap_trip.h"
#include "roadmap_factory.h"
#include "roadmap_display.h"
#include "navigate.h"
// #include "navigate_traffic.h"
#include "navigate_plugin.h"
#include "navigate_bar.h"
#include "navigate_cost.h"
#include "navigate_visual.h"

#include "roadmap_navigate.h"
#include "roadmap_trip.h"

/**
 * @brief for debugging
 */
static void navigate_debug_be_trip(void)
{
/* 11/3 - new departure : 50.8979914" lon="4.6839269 */
	roadmap_trip_set_selection(4683927, 50897991);
	roadmap_trip_set_selection_as ("Departure");
#if 0
/* old departure : 4683870,50898550 */
	roadmap_trip_set_selection(4683870, 50898550);
	roadmap_trip_set_selection_as ("Departure");
#endif
#if 0
/* destination : 4700775, 50903168 */
	roadmap_trip_set_selection(4700775, 50903168);
	roadmap_trip_set_selection_as ("Destination");
#endif
/* Sportschuur
 * <node id='325630180' timestamp='2009-01-04T09:55:50+00:00'
 * user='Danny Backx' visible='true' lat='50.9029438' lon='4.7012746' />
 */
	roadmap_trip_set_selection(4701274, 50902943);
	roadmap_trip_set_selection_as ("Destination");
}

/**
 * @brief for debugging
 */
static void navigate_debug_sf_trip(void)
{
	roadmap_trip_set_selection(-122394181, 37794928);
	roadmap_trip_set_selection_as ("Departure");
	roadmap_trip_set_selection(-122423168, 37788848);
	roadmap_trip_set_selection_as ("Destination");
}

static void navigate_start_navigate (void)
{
	roadmap_log (ROADMAP_DEBUG, "navigate_start_navigate");
	navigate_calc_route ();
}

static void roadmap_start_set_destination (void)
{
	roadmap_log (ROADMAP_DEBUG, "roadmap_start_set_destination");
	roadmap_trip_set_selection_as ("Destination");
	roadmap_screen_refresh();
}

static void roadmap_start_set_departure (void)
{
	roadmap_log (ROADMAP_DEBUG, "roadmap_start_set_departure");
	roadmap_trip_set_selection_as ("Departure");
	roadmap_screen_refresh();
}

#if 0
/* Not used */
static void roadmap_start_set_waypoint (void)
{
	const char *id = roadmap_display_get_id ("Selected Street");
	roadmap_log (ROADMAP_DEBUG, "roadmap_start_set_waypoint");
	if (id != NULL) {
		roadmap_trip_set_selection_as (id);
		roadmap_screen_refresh();
	}
}

static void roadmap_start_delete_waypoint (void)
{
	roadmap_log (ROADMAP_DEBUG, "roadmap_start_delete_waypoint");
	roadmap_tripdb_remove_point (NULL);
}
#endif

static RoadMapAction NavigateActions[] = {

   {"setasdestination", "Set as Destination", NULL, NULL,
     "Set the selected street block as the trip's destination", NULL,
     roadmap_start_set_destination},

   {"navigate", "Navigate", NULL, NULL,
     "Calculate route", NULL, navigate_start_navigate},
#if 0
   {"deletewaypoints", "Delete Waypoints...", "Delete...", NULL,
     "Delete selected waypoints", NULL, roadmap_start_delete_waypoint},
#endif
   {"setasdeparture", "Set as Departure", NULL, NULL,
     "Set the selected street block as the trip's departure", NULL,
	roadmap_start_set_departure},

   {"traffic", "Routing preferences", NULL, NULL,
      "Change routing preferences", NULL,
      navigate_cost_preferences},

   {"navigate-be-debug", "Belgium Debug trip for navigation", NULL, NULL,
	   "Debug trip for navigation", NULL, navigate_debug_be_trip},
   {"navigate-sf-debug", "San Francisco trip for navigation", NULL, NULL,
	   "Debug trip for navigation", NULL, navigate_debug_sf_trip},

   {"navigate-enable", "Enable navigation", NULL, NULL,
	   "Enable navigation", NULL, roadmap_navigate_enable},

   {"navigate-disable", "Disable navigation", NULL, NULL,
	   "Disable navigation", NULL, roadmap_navigate_disable},

   {NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

static const char *NavigateMenu[] = {
	ROADMAP_MENU "Navigate",
//		RoadMapFactorySeparator,
		"setasdestination",
		"setasdeparture",
//		"addaswaypoint",	cannot be here, is a RoadMap action
//		"deletewaypoints",
		"navigate-be-debug",
		"navigate-sf-debug",
		"navigate",
		RoadMapFactorySeparator,
		"traffic",
		"navigate-enable",
		"navigate-disable",

	NULL
};

/**
 * @brief
 */
static void navigate_plugin_initialize (void) {
		navigate_initialize();
}

/**
 * @brief
 */
static RoadMapPluginHooks navigate_plugin_hooks = {
  /* name */			"navigate plugin",
  /* size */			sizeof(navigate_plugin_hooks),
  /* line_from */		NULL,
  /* line_to */			NULL,
  /* activate_db */		NULL,
  /* get_distance */		NULL,
  /* override_line */		NULL,
  /* override_pen */		&navigate_visual_override_pen,
  /* screen_repaint */		NULL /* &navigate_screen_repaint */,
  /* get_street */		NULL,
  /* get_street_full_name */	NULL,
  /* get_street_properties */	NULL,
  /* find_connected_lines */	NULL,
  /* adjust_layer */		NULL /* &navigate_traffic_adjust_layer */,
  /* get_closest */		NULL,
  /* route_direction */		NULL,
  /* shutdown */		NULL,
  /* initialize */		&navigate_plugin_initialize,
  /* actions */			&NavigateActions[0],
  /* menu */			(char **)NavigateMenu,
  /* after_refresh */		&navigate_bar_after_refresh,
  /* format messages */		&navigate_format_messages,
  /* route_clear */		navigate_visual_route_clear,
  /* route_add */		navigate_visual_route_add
};

/**
 * @brief
 * @return
 */
int navigate_plugin_register (void) {
   return roadmap_plugin_register (&navigate_plugin_hooks);
}

/**
 * @brief
 * @param plugin_id
 */
void navigate_plugin_unregister (int plugin_id) {
   roadmap_plugin_unregister (plugin_id);
}
