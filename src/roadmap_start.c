/*
 * LICENSE:
 *
 *   (c) Copyright 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009
 *	 Pascal F. Martin, Paul Fox, Ehud Shabtai, Danny Backx,
 *	 and others.
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
 * @brief The main function of the RoadMap application.
 */

#include <stdlib.h>
#include <string.h>

#ifdef ROADMAP_MTRACE
#include <mcheck.h>
#endif

#include "roadmap.h"
#include "roadmap_copyright.h"
#include "roadmap_dbread.h"
#include "roadmap_math.h"
#include "roadmap_string.h"
#include "roadmap_config.h"
#include "roadmap_history.h"

#include "roadmap_spawn.h"
#include "roadmap_path.h"
#include "roadmap_io.h"

#include "roadmap_object.h"
#include "roadmap_voice.h"
#include "roadmap_gps.h"

#include "roadmap_canvas.h"
#include "roadmap_time.h"
#include "roadmap_preferences.h"
#include "roadmap_address.h"
#include "roadmap_coord.h"
#include "roadmap_crossing.h"
#include "roadmap_screen_obj.h"
#include "roadmap_sprite.h"
#include "roadmap_gpx.h"
#include "roadmap_trip.h"
#include "roadmap_tripdb.h"
#include "roadmap_track.h"
#include "roadmap_landmark.h"
#include "roadmap_features.h"
#include "roadmap_adjust.h"
#include "roadmap_screen.h"
#include "roadgps_screen.h"
#include "roadgps_logger.h"
#include "roadmap_fuzzy.h"
#include "roadmap_navigate.h"
#include "roadmap_label.h"
#include "roadmap_display.h"
#include "roadmap_locator.h"
#include "roadmap_copy.h"
#include "roadmap_httpcopy.h"
#include "roadmap_download.h"
#include "roadmap_driver.h"
#include "roadmap_factory.h"
#include "roadmap_main.h"
#include "roadmap_message.h"
#include "roadmap_messagebox.h"
#include "roadmap_osm.h"
#include "roadmap_help.h"
#include "roadmap_pointer.h"
#include "roadmap_layer.h"
#include "roadmap_sunrise.h"
#include "roadmap_lang.h"

#include "roadmap_start.h"

/* Only for roadmap_plugin_initialize */
#include "roadmap_library.h"

#ifdef HAVE_TRIP_PLUGIN
#include "trip/trip.h"
#include "trip/trip_plugin.h"
#endif
#ifdef HAVE_NAVIGATE_PLUGIN
#include "navigate/navigate_plugin.h"
#endif

#ifdef _WIN32
extern const char *roadmap_build(void);
#endif

static const char *RoadMapMainTitle = "RoadMap";

static int RoadMapStartFrozen = 0;

static RoadMapDynamicString RoadMapStartGpsID;

static RoadMapConfigDescriptor RoadMapConfigGeneralUnit =
                        ROADMAP_CONFIG_ITEM("General", "Unit");

static RoadMapConfigDescriptor RoadMapConfigGeneralKeyboard =
                        ROADMAP_CONFIG_ITEM("General", "Keyboard");

static RoadMapConfigDescriptor RoadMapConfigGeneralShowErrors =
                        ROADMAP_CONFIG_ITEM("General", "Show Errors");

static RoadMapConfigDescriptor RoadMapConfigGeometryMain =
                        ROADMAP_CONFIG_ITEM("Geometry", "Main");

static RoadMapConfigDescriptor RoadMapConfigGeometryDefault =
                        ROADMAP_CONFIG_ITEM("Geometry", "Default");

static RoadMapConfigDescriptor RoadMapConfigMapPath =
                        ROADMAP_CONFIG_ITEM("Map", "Path");

static RoadMapConfigDescriptor RoadMapConfigPathIcons =
                        ROADMAP_CONFIG_ITEM("General", "IconPath");

static RoadMapConfigDescriptor RoadMapConfigDisplayRefresh =
                        ROADMAP_CONFIG_ITEM("Display", "Refresh Period");


#define ROADMAP_DEFAULT_REFRESH_INTERVAL 200


#ifndef ROADMAP_USES_EXPAT
/* if there's no libexpat to build against, the loading/saving
 * code all goes away.
 */
#define roadmap_trip_load_ask       roadmap_start_no_expat
#define roadmap_trip_merge_ask      roadmap_start_no_expat
#define roadmap_trip_save_manual    roadmap_start_no_expat
#define roadmap_trip_save_as        roadmap_start_no_expat
#define roadmap_track_save          roadmap_start_no_expat
#define roadmap_landmark_merge      roadmap_start_no_expat

/**
 * @brief warn the user that features using expat are not available
 */
static void roadmap_start_no_expat (void)
{
    roadmap_log (ROADMAP_ERROR,
                 "This feature is not available (no expat library)");
}

#endif // ROADMAP_USES_EXPAT


/* The menu and toolbar callbacks: --------------------------------------- */

static void roadmap_start_cancel (void) {
    /* null action -- for menus, mainly popups, where the mouse
     * can easily cause an unwanted action -- putting this at the
     * top of the menu helps prevent that */
}


static void roadmap_start_periodic (void);

static void roadmap_start_purge (void) {
   roadmap_history_purge (10);
}

static void roadmap_start_show_start (void)
{
#ifdef	HAVE_TRIP_PLUGIN
    roadmap_trip_set_focus ("Start"); /* trip_set_focus ("Start"); */
#else
    roadmap_trip_set_focus ("Start");
#endif
    roadmap_screen_refresh ();
}

static void roadmap_start_show_departure (void)
{
#ifdef	HAVE_TRIP_PLUGIN
    roadmap_trip_set_focus ("Departure"); /* trip_set_focus ("Departure"); */
#else
    roadmap_trip_set_focus ("Departure");
#endif
    roadmap_screen_refresh ();
}

static void roadmap_start_show_destination (void)
{
#ifdef	HAVE_TRIP_PLUGIN
    roadmap_trip_set_focus ("Destination"); /* trip_set_focus ("Destination"); */
#else
    roadmap_trip_set_focus ("Destination");
#endif
    roadmap_screen_refresh ();
}

static void roadmap_start_show_location (void)
{
#ifdef HAVE_TRIP_PLUGIN
    roadmap_trip_set_focus ("Address"); /* trip_set_focus ("Address"); */
#else
    roadmap_trip_set_focus ("Address");
#endif
    roadmap_screen_refresh ();
}

static void roadmap_start_show_gps (void)
{
#ifdef HAVE_TRIP_PLUGIN
    roadmap_trip_set_focus ("GPS"); /* trip_set_focus ("GPS"); */
#endif
    roadmap_trip_set_focus ("GPS");
    roadmap_screen_refresh ();
}

static void roadmap_start_hold_map (void) {
   roadmap_start_periodic (); /* To make sure the map is current. */
   roadmap_screen_hold ();
}


static void roadmap_start_rotate (void) {
    roadmap_screen_rotate (10);
}

static void roadmap_start_counter_rotate (void) {
    roadmap_screen_rotate (-10);
}

#if 1

#define	CRLF	"\n"

#else
#ifdef _WIN32
#define	CRLF	"\r\n"
#else
#define	CRLF	"\n"
#endif
#endif

static void roadmap_start_about (void) {

   roadmap_messagebox ( "About",
       "RoadMap " ROADMAP_VERSION CRLF
       "Copyright " ROADMAP_YEAR " Pascal Martin," CRLF
       "Paul Fox, Ehud Shabtai, Danny Backx and many others" CRLF
       CRLF
       "RoadMap is free software; you can" CRLF
       "redistribute it and/or modify it under the" CRLF
       "terms of version 2 (or later) of the GNU" CRLF
       "General Public License, as published by the" CRLF
       "Free Software Foundation.  You should have" CRLF
       "received a copy of the GPL license along" CRLF
       "with this program." CRLF
   );

}

/**
 * @brief show a list of all active plugins.
 * This needs to be here, moving the roadmap_messagebox() call into roadmap_plugin.c would
 * break the build.
 */
static void roadmap_start_list_all_plugins(void)
{
	char *r = roadmap_plugin_list_all_plugins();
	roadmap_messagebox("Plugin list", r);
	free(r);
}


static void roadmap_start_mapinfo (void) {

   char map_info[512];
   RoadMapPosition pos;
   char lon[32], lat[32];

   roadmap_math_get_context (&pos, NULL, NULL);

   snprintf(map_info, sizeof(map_info),
            "Map view area: %s by %s\n"
            "Map center: %s, %s",
             roadmap_message_get('x'),
             roadmap_message_get('y'),
             roadmap_math_to_floatstring(lon, pos.longitude, MILLIONTHS),
             roadmap_math_to_floatstring(lat, pos.latitude, MILLIONTHS));
   roadmap_messagebox_wait ("Map Parameters", map_info);
}

static void roadmap_start_create_waypoint (void)
{
#ifdef HAVE_TRIP_PLUGIN
    roadmap_trip_create_selection_waypoint (); /* trip_create_selection_waypoint (); */
#else
    roadmap_trip_create_selection_waypoint ();
#endif
    roadmap_screen_refresh();
}

static void roadmap_start_create_gps_waypoint (void)
{
#ifdef HAVE_TRIP_PLUGIN
    roadmap_trip_create_gps_waypoint (); /* trip_create_gps_waypoint (); */
#else
    roadmap_trip_create_gps_waypoint ();
#endif
    roadmap_screen_refresh();
}

static int roadmap_start_no_download (int fips) {

   if (! roadmap_download_blocked (fips)) {
      if (fips > 0) {
        /* only report on maps that were indexed.  the names of
         * geographic maps (e.g., OSM quadtiles) are calculated, not
         * looked up, so warning about them is overload.
         */
        roadmap_log (ROADMAP_WARNING, "cannot open map database %s",
                roadmap_locator_filename(NULL, fips));
      }
      roadmap_download_block (fips);
   }
   return 0;
}

void roadmap_start_download_done_callback(void) {
   roadmap_start_request_repaint_map (REPAINT_NOW);
}

static void roadmap_start_toggle_download (void) {

   if (roadmap_download_enabled()) {

      roadmap_download_subscribe_when_done (NULL);
      roadmap_locator_declare_downloader (&roadmap_start_no_download);

   } else {

      static int ProtocolInitialized = 0;

      if (! ProtocolInitialized) {

         /* PLUGINS NOT SUPPORTED YET.
          * roadmap_plugin_load_all
          *      ("download", roadmap_download_subscribe_protocol);
          */

         roadmap_copy_init (roadmap_download_subscribe_protocol);
         roadmap_httpcopy_init (roadmap_download_subscribe_protocol);

         ProtocolInitialized = 1;
      }

      roadmap_download_subscribe_when_done
            (roadmap_start_download_done_callback);
      roadmap_locator_declare_downloader (roadmap_download_get_county);
      roadmap_download_unblock_all ();
   }

   roadmap_start_request_repaint_map (REPAINT_NOW);
}

#ifdef _WIN32
static void roadmap_start_detect_receiver (void)
{
    roadmap_main_remove_periodic (roadmap_gps_open);
    roadmap_gps_shutdown ();
    roadmap_gps_detect_receiver ();
}
#endif

static int RoadMapStartScreenActive = ROADMAP_MAP;

int roadmap_start_map_active(void) {
    return (RoadMapStartScreenActive == ROADMAP_MAP);
}

int roadmap_start_return_to_map(void) {
    if (RoadMapStartScreenActive != ROADMAP_MAP) {
        RoadMapStartScreenActive = ROADMAP_MAP;
        roadmap_start_request_repaint(ROADMAP_MAP, REPAINT_NOW);
        return 1;
    }
    return 0;
}

void roadmap_start_gps_console(void) {
    RoadMapStartScreenActive = ROADMAP_GPS;
    roadmap_start_request_repaint(ROADMAP_GPS, REPAINT_NOW);
}

void roadmap_start_do_callback(RoadMapCallback callback) {

    if (roadmap_start_return_to_map()) return;

    callback();
}

static void roadmap_start_external_gps_console (void) {

   const char *url = roadmap_gps_source();
   const char *config = roadmap_extra_config();
   const char *userpath = roadmap_userpath();
   const char *icons = roadmap_icon_path();
   char arguments[1024];
   char *s = arguments;
   int n, l;
   
   l = sizeof(arguments);
   *s = '\0';

   if (url) {
      n = snprintf (s, l, "--gps=%s", url);
      s += n;
      l -= n;
   }
   if (userpath) {
      n = snprintf (s, l, " --userpath=%s", userpath);
      s += n;
      l -= n;
   }
   if (config) {
      n = snprintf (s, l, " --config=%s", config);
      s += n;
      l -= n;
   }
   if (icons) {
      n = snprintf (s, l, " --icons=%s", icons);
      s += n;
      l -= n;
   }
   roadmap_spawn ("roadgps", arguments);
}


static int RoadMapStartRepaintNeeded = REPAINT_NOT_NEEDED;
static int RoadMapStartIdleInstalled;

int roadmap_start_repaint_scheduled(void) {

   return RoadMapStartRepaintNeeded;
}

static void roadmap_start_repaint_if_requested(void) {

   while (RoadMapStartRepaintNeeded != REPAINT_NOT_NEEDED) {

      RoadMapStartRepaintNeeded = REPAINT_NOT_NEEDED;

      if (RoadMapStartScreenActive == ROADMAP_MAP) {
         roadmap_screen_repaint();
      } else {
         roadgps_screen_draw();
      }
   }

   roadmap_main_remove_idle_function();
   RoadMapStartIdleInstalled = 0;
}

/* The "which_screen" parameter tells us whether we're being
 * asked to upgrade the regular map screen or the gps console
 * screen.  There's no point in rendering a hidden screen.  The
 * "priority" parameter tells us (usually) whether the requested
 * repaint was user-initiated or not.  The difference is in
 * whether we interrupt a repaint already in progress or not:  we
 * assume the user wants instant gratification, and so we'll
 * interrupt an in-progress repaint.  A repaint requested because
 * the GPS moved, or because the time changed, is lower priority,
 * and won't happen unless there's nothing else to do.  That's
 * the theory anyway.
 */
void roadmap_start_request_repaint (int which_screen, int priority) {

   /* The repaint is actually invoked via the gui's mainloop
    * idle routine.  We need to install and remove this routine
    * on an as-needed basis, because otherwise installing something
    * in the gui's idle loop causes it to eat CPU (i.e., it can't
    * sleep -- it has to spin.)
    */
   if (RoadMapStartScreenActive == which_screen) {
      if (!RoadMapStartIdleInstalled) {
         roadmap_main_set_idle_function(roadmap_start_repaint_if_requested);
         RoadMapStartIdleInstalled = 1;
      }
      RoadMapStartRepaintNeeded = priority;
   }
}

void roadmap_start_request_repaint_map (int priority) {

    roadmap_start_request_repaint (ROADMAP_MAP, priority);
}

/* The RoadMap menu and toolbar items: ----------------------------------- */

/* This table lists all the RoadMap actions that can be initiated
 * fom the user interface (a sort of symbol table).
 * Any other part of the user interface (menu, toolbar, etc..)
 * will reference an action.
 */
static RoadMapAction RoadMapStartActions[] = {

   {"cancel", "", NULL, NULL,
      "Do nothing", NULL, roadmap_start_cancel},

   {"preferences", "Preferences", "Preferences", "P",
      "Open the preferences editor", NULL, roadmap_preferences_edit},

   {"gpsconsole", "GPS Console", "Console", "C",
      "Show the GPS console", NULL, roadmap_start_gps_console},

   {"roadgps", "Run RoadGps", "RoadGps", NULL,
      "Start the external GPS console program", NULL,
       roadmap_start_external_gps_console},

   {"mutevoice", "Mute Voice", "Mute", NULL,
      "Mute all voice annoucements", NULL, roadmap_voice_mute},

   {"enablevoice", "Enable Voice", "Mute Off", NULL,
      "Enable all voice annoucements", NULL, roadmap_voice_enable},

   {"nonavigation", "Disable Navigation", "Nav Off", NULL,
      "Disable all navigation feedback", NULL, roadmap_navigate_disable},

   {"navigation", "Enable Navigation", "Nav On", NULL,
      "Enable navigation feedback", NULL, roadmap_navigate_enable},

   {"logtofile", "Log to File", "Log", NULL,
      "Save future log messages to the postmortem file", NULL,
      roadmap_log_save_all},

   {"nolog", "Disable Log", "No Log", NULL,
      "Do not save future log messages to the postmortem file", NULL,
      roadmap_log_save_all},

   {"gpslog", "Start GPS Logging", "GPS Log", NULL,
      "Start logging GPS messages", NULL, roadgps_logger_start},

   {"gpslogstop", "Stop GPS Logging", "No GPS Log", NULL,
      "Stop logging GPS messages", NULL, roadgps_logger_stop},

   {"purgelogfile", "Purge Log File", "Purge", NULL,
      "Delete the current postmortem log file", NULL, roadmap_log_purge},

   {"purgehistory", "Purge History", "Forget", NULL,
      "Remove all but the 10 most recent searches", NULL, roadmap_start_purge},

   {"quit", "Quit", NULL, NULL,
      "Quit RoadMap", NULL, roadmap_main_exit},

   {"zoomin", "Zoom In", "+", NULL,
      "Enlarge the central part of the map", NULL, roadmap_screen_zoom_in},

   {"zoomout", "Zoom Out", "-", NULL,
      "Show a larger area", NULL, roadmap_screen_zoom_out},

   {"zoom1", "Normal Size", ":1", NULL,
      "Set the map back to the default zoom level", NULL,
      roadmap_screen_zoom_reset},

   {"up", "Up", "N", NULL,
      "Move the map view upward", NULL, roadmap_screen_move_up},

   {"left", "Left", "W", NULL,
      "Move the map view to the left", NULL, roadmap_screen_move_left},

   {"right", "Right", "E", NULL,
      "Move the map view to the right", NULL, roadmap_screen_move_right},

   {"down", "Down", "S", NULL,
      "Move the map view downward", NULL, roadmap_screen_move_down},

   {"clockwise", "Rotate Clockwise", "R+", NULL,
      "Rotate the map view clockwise", NULL, roadmap_start_rotate},

   {"counterclockwise", "Rotate Counter-Clockwise", "R-", NULL,
      "Rotate the map view counter-clockwise", NULL,
      roadmap_start_counter_rotate},

   {"hold", "Hold Map", "Hold", "H",
      "Hold the map view in its current position", NULL,
      roadmap_start_hold_map},

   {"address", "Address...", "Addr", "A",
      "Show a specified address", NULL, roadmap_address_location_by_city},

   {"intersection", "Intersection...", "X", NULL,
      "Show a specified street intersection", NULL, roadmap_crossing_dialog},

   {"position", "Position...", "P", NULL,
      "Show a position at the specified coordinates", NULL,
      roadmap_coord_dialog},

   {"startpoint", "Route Start", "Start", NULL,
      "Show the current route starting point", NULL, roadmap_start_show_start},

   {"nextpoint", "Next Route Point", "N", NULL,
      "Show the next route point", NULL, roadmap_trip_show_nextpoint},

   {"2ndpoint", "Second Next Route Point", "", NULL,
      "Show the route point after the next", NULL,
      roadmap_trip_show_2ndnextpoint},

   {"destination", "Destination", "D", NULL,
      "Show the current destination", NULL, roadmap_start_show_destination},

   {"departure", "Departure", "Dep", NULL,
      "Show the route's point of departure", NULL,
      roadmap_start_show_departure},

   {"gps", "GPS Position", "GPS", "G",
      "Show the current GPS position", NULL, roadmap_start_show_gps},

   {"location", "Location", "L", NULL,
      "Show the last selected address, crossroads, or position", NULL,
      roadmap_start_show_location},

   {"mapdownload", "Toggle Tiger Map Download", "Download", NULL,
      "Enable/Disable Tiger map download mode", NULL,
      roadmap_start_toggle_download},

#if SOON
   {"osmdownload", "Start an OpenStreetMap Download", "OSM Download", NULL,
      "Start OSM download", NULL,
      roadmap_osm_start_download},
#endif

   {"mapdiskspace", "Map Disk Space", "Disk", NULL,
      "Show the amount of disk space occupied by the maps", NULL,
      roadmap_download_show_space},

   {"deletemaps", "Delete Maps...", "Delete", "Del",
      "Delete maps that are currently visible", NULL, roadmap_download_delete},

   {"newtrip", "New Trip", "New", NULL,
      "Create a new trip", NULL, roadmap_trip_new},

   {"opentrip", "Open Trip", "Open", "O",
      "Open an existing trip", NULL, roadmap_trip_load_ask},

   {"mergetrip", "Merge Trip", "Merge", "M",
      "Merge a trip with the current trip", NULL, roadmap_trip_merge_ask},

   {"savetrip", "Save Trip", "Save", "S",
      "Save the current trip", NULL, roadmap_trip_save_manual},

   {"savescreenshot", "Map Screenshot", "Screenshot", "Y",
      "Make a screenshot of the current map under the trip name", NULL,
      roadmap_trip_save_screenshot},

   {"savetripas", "Save Trip As...", "Save As", "As",
      "Save the current trip under a different name", NULL,
      roadmap_trip_save_as},

   {"starttrip", "Start Route", "Start", NULL,
      "Start following the current route", NULL, roadmap_trip_route_start},

   {"stoptrip", "Stop Route", "Stop", NULL,
      "Stop following the current route", NULL, roadmap_trip_route_stop},

   {"toggleview", "2D/3D View", "M", NULL,
      "Toggle view mode 2D / 3D", NULL, roadmap_screen_toggle_view_mode},

   {"togglelabels", "Show/Hide Street Labels", "Labels", NULL,
      "Show or Hide the names of streets", NULL, roadmap_screen_toggle_labels},

   {"toggleorientation", "Dynamic/Fixed Orientation", "", NULL,
      "Toggle orientation mode dynamic / fixed", NULL,
      roadmap_screen_toggle_orientation_mode},

   {"increasehorizon", "Increase Horizon", "I", NULL,
      "Increase the 3D horizon", NULL, roadmap_screen_increase_horizon},

   {"decreasehorizon", "Decrease Horizon", "DI", NULL,
      "Decrease the 3D horizon", NULL, roadmap_screen_decrease_horizon},

   {"tracktoggle", "Show/Hide Current Track", "Track", NULL,
      "Show or Hide the GPS breadcrumb track", NULL,
      roadmap_track_toggle_display},

   {"tracksave", "Save Current Track", "Save Track", NULL,
      "Save the current GPS breadcrumb track", NULL, roadmap_track_save},

   {"trackreset", "Save and Reset Current Track", "Save/Reset Track", NULL,
      "Save the current GPS breadcrumb track, then clear it", NULL,
      roadmap_track_reset},

   {"backtrackroute", "Create Backtrack Route", "BackTrack", NULL,
      "Convert the current GPS breadcrumb track to a new route", NULL,
      roadmap_trip_currenttrack_to_route },

   {"tracktoroute", "Convert Track to Route", "RouteToTrack", NULL,
      "Create a new route from the currently selected track", NULL,
      roadmap_trip_track_to_route },

   {"addtrack", "Add Current Track to Trip", "AddTrack", NULL,
      "Add a copy of the current GPS breadcrumb track to the trip", NULL,
      roadmap_trip_currenttrack_to_track },

   {"resumeroute", "Resume Route", "Resume", NULL,
      "Resume following (resync with) the current route", NULL,
      roadmap_trip_route_resume},

   {"returnroute", "Return Route", "Return", NULL,
      "Start the route back to the departure point", NULL,
      roadmap_trip_route_return},

   {"reverseroute", "Reverse Route", "Reverse", NULL,
      "Reverse the current route", NULL,
       roadmap_trip_route_reverse},

   {"simplifyroute", "Simplify Route", NULL, NULL,
      "Create simplified version of current route", NULL,
      roadmap_trip_route_simplify },

   {"createroute", "New route using selection", NULL, NULL,
      "Start new route using last selected street or place", NULL,
      roadmap_trip_new_route},

#if WGET_GOOGLE_ROUTE
   {"getgoogleroute", "Fetch route from google", NULL, NULL,
      "Fetch google route for current route's start/dest", NULL,
        roadmap_trip_replace_with_google_route},
#endif

   {"setasdestination", "Goto selection", NULL, NULL,
      "Show distance and direction to the last selected street or place", NULL,
      roadmap_trip_set_as_destination},

   {"manageroutes", "Select Route...", "Select Route", NULL,
      "Select, rename, or delete routes", NULL,
      roadmap_trip_route_manage_dialog},

   {"listdeletedroutes", "Deleted Routes...", NULL, NULL,
      "List and restore deleted routes", NULL,
      roadmap_trip_lost_route_manage_dialog},

   {"allroutetoggle", "Show/Hide All Routes", "AllRoutes", NULL,
      "Show or Hide currently unselected routes", NULL,
      roadmap_trip_toggle_show_inactive_routes},

   {"alltracktoggle", "Show/Hide All Tracks", "AllTracks", NULL,
      "Show or Hide currently unselected tracks", NULL,
      roadmap_trip_toggle_show_inactive_tracks},

   {"gpsaswaypoint", "New place from GPS...", "GPS Place", "GW",
      "Create new place using current GPS position", NULL,
      roadmap_start_create_gps_waypoint},

   {"addaswaypoint", "New place from selected...", "Place", "W",
      "Create new place using last selected street or place", NULL,
      roadmap_start_create_waypoint},

   {"editroutepoints", "Route Waypoints...", NULL, NULL,
      "Edit current route's waypoints", NULL,
      roadmap_trip_route_waypoint_manage_dialog },

   {"listdeletedplaces", "Deleted Places...", NULL, NULL,
      "List deleted or moved places", NULL,
      roadmap_trip_lost_waypoint_manage_dialog },

   {"addroutepointnear", "Insert Route Waypoint", NULL, NULL,
      "Insert routepoint into nearest leg of the current route",  NULL,
      roadmap_trip_insert_routepoint_best },

   {"addroutepointend", "Append Past Route Destination", NULL, NULL,
      "Add routepoint past the end of current route",  NULL,
      roadmap_trip_insert_routepoint_dest },

   {"addroutepointstart", "Prepend Before Route Start", NULL, NULL,
      "Add routepoint before the start of current route",  NULL,
      roadmap_trip_insert_routepoint_start },

   {"addtriplandmark", "Add Trip Landmark", NULL, NULL,
      "Add selection to list of trip landmarks",  NULL,
      roadmap_trip_insert_trip_point },

   {"addpersonallandmark", "Add Personal Landmark", NULL, NULL,
      "Add selection to list of personal landmarks",  NULL,
      roadmap_trip_insert_personal_point },

   {"edittriplandmarks", "Trip Landmarks...", NULL, NULL,
      "Edit landmarks associated with this trip", NULL,
      roadmap_trip_trip_waypoint_manage_dialog },

   {"editpersonallandmarks", "Personal Landmarks...", NULL, NULL,
      "Edit personal landmarks", NULL,
      roadmap_trip_personal_waypoint_manage_dialog },

   {"mergepersonallandmarks", "Load more Personal Landmarks...", NULL, NULL,
      "Merge personal landmarks from file", NULL, roadmap_landmark_merge },

   {"lastplacedelete", "Delete place", "Delete Selected Place", NULL,
      "Delete the last selected place", NULL,
      roadmap_trip_delete_last_place },

   {"lastplaceedit", "Edit place", "Edit Selected Place", NULL,
      "Edit the last selected place", NULL, roadmap_trip_edit_last_place },

   {"lastplacemove", "Move place", NULL, NULL,
      "Relocate the last selected place to the popup location", NULL,
      roadmap_trip_move_last_place },

   {"routepointahead", "Reorder, move ahead", "Ahead", NULL,
      "Reorder the route, moving this point later in the route", NULL,
      roadmap_trip_move_routepoint_ahead },

   {"routepointback", "Reorder, move back", "Back", NULL,
      "Reorder the route, moving this point earlier in the route", NULL,
      roadmap_trip_move_routepoint_back },

   {"full", "Full Screen", "Full", "F",
      "Toggle the window full screen mode (if window manager permits)", NULL,
      roadmap_main_toggle_full_screen},

   {"about", "About", NULL, NULL,
      "Show information about RoadMap", NULL, roadmap_start_about},

   {"mapinfo", "Map Parameters", NULL, NULL,
      "Show parameters of the currently displayed map", NULL,
      roadmap_start_mapinfo},

   {"togglehelpsource", "Toggle Help Source", NULL, NULL,
      "Switch between local/web help", NULL, roadmap_help_toggle_source},

#ifdef _WIN32
   {"detectreceiver", "Detect GPS receiver", NULL, NULL,
      "Auto-detect GPS receiver", NULL, roadmap_start_detect_receiver},
#endif

   {"listplugins", "List plugins", NULL, NULL,
	   "List the RoadMap plugins", NULL, roadmap_start_list_all_plugins},

   {NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};


static const char *RoadMapStartMenu[] = {

   /* The "Menus..." popup isn't normally used, but can be useful
    * for binding to a mouse click or toolbar button, particularly
    * if the menubar has been suppressed.
    */
   ROADMAP_SUBMENU "Menus...",

      ROADMAP_INVOKE_SUBMENU "File",
      ROADMAP_INVOKE_SUBMENU "View",
      ROADMAP_INVOKE_SUBMENU "Find",
      ROADMAP_INVOKE_SUBMENU "Trip",
      ROADMAP_INVOKE_SUBMENU "Help",

   ROADMAP_SUBMENU "Announcements...",
      "mutevoice",
      "enablevoice",

      RoadMapFactorySeparator,

      "nonavigation",
      "navigation",

      RoadMapFactorySeparator,

      "logtofile",
      "nolog",
      "gpslog",
      "gpslogstop", 

   ROADMAP_SUBMENU "Map Download...",

      "mapdownload",
      "mapdiskspace",
      "deletemaps",

   ROADMAP_MENU "File",

      "preferences",
      "gpsconsole",
#ifdef _WIN32
      "detectreceiver",
#endif
#ifndef HAVE_TRIP_PLUGIN
      "savescreenshot",
#endif
      "listplugins",

      RoadMapFactorySeparator,

      ROADMAP_INVOKE_SUBMENU "Announcements...",

      RoadMapFactorySeparator,

      ROADMAP_INVOKE_SUBMENU "Map Download...",

      RoadMapFactorySeparator,

      "quit",


   ROADMAP_SUBMENU "Display modes...",

      "togglelabels",
      "toggleorientation",
      "toggleview",
      "increasehorizon",
      "decreasehorizon",
      "full",

   ROADMAP_MENU "View",

      "zoomin",
      "zoomout",
      "zoom1",

      RoadMapFactorySeparator,

      "up",
      "left",
      "right",
      "down",

      RoadMapFactorySeparator,

      "clockwise",
      "counterclockwise",

      RoadMapFactorySeparator,

      ROADMAP_INVOKE_SUBMENU "Display modes...",

      RoadMapFactorySeparator,

      "hold",


   ROADMAP_MENU "Find",

      "address",
      "intersection",
      "position",
      "purgehistory",

      RoadMapFactorySeparator,

      "startpoint",
      "destination",
      "departure",
      "gps",

#ifndef	HAVE_TRIP_PLUGIN
   ROADMAP_MENU "Trip",

      "newtrip",
      "opentrip",
      "mergetrip",
      "savetrip",
      "savetripas",

      RoadMapFactorySeparator,

      ROADMAP_INVOKE_SUBMENU "Routes...",
      ROADMAP_INVOKE_SUBMENU "Places...",
      ROADMAP_INVOKE_SUBMENU "Edit Places...",
      ROADMAP_INVOKE_SUBMENU "Tracks...",
#endif
#ifndef HAVE_TRIP_PLUGIN
   ROADMAP_SUBMENU "Routes...",

      "manageroutes",
      "listdeletedroutes",
      "starttrip",
      "stoptrip",
      "resumeroute",
      "reverseroute",
      "simplifyroute",
      "createroute",
      "setasdestination",
      "allroutetoggle",
#if WGET_GOOGLE_ROUTE
      "getgoogleroute",
#endif

   ROADMAP_SUBMENU "Places...",

      "addaswaypoint",
      "gpsaswaypoint",
      "editroutepoints",
      "edittriplandmarks",
      "editpersonallandmarks",
      "listdeletedplaces",
      "mergepersonallandmarks",

   ROADMAP_SUBMENU "Tracks...",

      "tracktoroute",

      RoadMapFactorySeparator,

      "tracktoggle",
      "alltracktoggle",

      RoadMapFactorySeparator,

      "backtrackroute",
      "addtrack",
      // "tracksave", // better to save and reset, to avoid
                        // multiple saves of same data
      "trackreset",

   ROADMAP_SUBMENU "Edit Places...",

      "lastplacedelete",
      "lastplaceedit",
      "lastplacemove",

      RoadMapFactorySeparator,

      "addroutepointnear",
      "addroutepointstart",
      "addroutepointend",

      RoadMapFactorySeparator,

      "addpersonallandmark",
      "addtriplandmark",

      RoadMapFactorySeparator,

      "routepointahead",
      "routepointback",
#endif

   ROADMAP_MENU "Help",

      "togglehelpsource",

      RoadMapFactoryHelpTopics,

      "mapinfo",

      RoadMapFactorySeparator,

      "about",

   NULL
};


static char const *RoadMapStartToolbar[] = {

   "destination",
   "location",
   "gps",
   "hold",

   RoadMapFactorySeparator,

   "counterclockwise",
   "clockwise",

   "zoomin",
   "zoomout",
   "zoom1",

   RoadMapFactorySeparator,

   "up",
   "left",
   "right",
   "down",

   RoadMapFactorySeparator,

   "full",
   "quit",

   NULL,
};


static char const *RoadMapStartKeyBinding[] = {

   "LeftArrow"     ROADMAP_MAPPED_TO "left",
   "RightArrow"    ROADMAP_MAPPED_TO "right",
   "UpArrow"       ROADMAP_MAPPED_TO "up",
   "DownArrow"     ROADMAP_MAPPED_TO "down",

   "Enter"           ROADMAP_MAPPED_TO "resumeroute",

   /* These binding are for the iPAQ buttons: */
   "Special-Menu"     ROADMAP_MAPPED_TO "zoom1",
   "Special-Contact"  ROADMAP_MAPPED_TO "zoomin",
   "Special-Calendar" ROADMAP_MAPPED_TO "zoomout",
   "Special-Start"    ROADMAP_MAPPED_TO "quit",

   /* These binding are for the OLPC XO laptop buttons: */
   "Special-PageUp"   ROADMAP_MAPPED_TO "zoomin",      // circle   
   "Special-PageDown" ROADMAP_MAPPED_TO "zoomout",     // square   
   "Special-Home"     ROADMAP_MAPPED_TO "resumeroute", // X        
   "Special-End"      ROADMAP_MAPPED_TO "destination", // checkmark

   /* These binding are for regular keyboards (case unsensitive !): */
   "+"               ROADMAP_MAPPED_TO "zoomin",
   "="               ROADMAP_MAPPED_TO "zoomin",
   "-"               ROADMAP_MAPPED_TO "zoomout",
   "A"               ROADMAP_MAPPED_TO "address",
   "B"               ROADMAP_MAPPED_TO "returnroute",
   "C"               ROADMAP_MAPPED_TO "gpsconsole",
   "D"               ROADMAP_MAPPED_TO "destination",
   "E"               ROADMAP_MAPPED_TO "deletemaps",
   "F"               ROADMAP_MAPPED_TO "full",
   "G"               ROADMAP_MAPPED_TO "gps",
   "H"               ROADMAP_MAPPED_TO "hold",
   "I"               ROADMAP_MAPPED_TO "intersection",
   "J"               ROADMAP_MAPPED_TO "counterclockwise",
   "K"               ROADMAP_MAPPED_TO "clockwise",
   "L"               ROADMAP_MAPPED_TO "location",
   "M"               ROADMAP_MAPPED_TO "mapdownload",
   "N"               ROADMAP_MAPPED_TO "newtrip",
   "O"               ROADMAP_MAPPED_TO "opentrip",
   "P"               ROADMAP_MAPPED_TO "stoptrip",
   "Q"               ROADMAP_MAPPED_TO "quit",
   "R"               ROADMAP_MAPPED_TO "zoom1",
   "S"               ROADMAP_MAPPED_TO "starttrip",
   "T"               ROADMAP_MAPPED_TO "tracktoggle",
   "U"               ROADMAP_MAPPED_TO "toggleorientation",
   "V"               ROADMAP_MAPPED_TO "gpsaswaypoint",
   "W"               ROADMAP_MAPPED_TO "addaswaypoint",
   "X"               ROADMAP_MAPPED_TO "intersection",
   "Y"               ROADMAP_MAPPED_TO "savesscreenshot",
   /* Z Unused. */
   "F11"             ROADMAP_MAPPED_TO "full",
   NULL
};


static void roadmap_start_set_unit (void) {

   const char *unit = roadmap_config_get (&RoadMapConfigGeneralUnit);

   if (strcmp (unit, "imperial") == 0) {

      roadmap_math_use_imperial();

   } else if (strcmp (unit, "metric") == 0) {

      roadmap_math_use_metric();

   } else {
      roadmap_log (ROADMAP_ERROR, "%s is not a supported unit", unit);
      roadmap_math_use_imperial();
   }
}

static void roadmap_gps_unset_messages() {

   roadmap_message_unset ('S');
   roadmap_message_unset ('H');
   roadmap_message_unset ('B');
   roadmap_message_unset ('M');
   roadmap_message_unset ('m');
   roadmap_message_unset ('E');
   roadmap_message_unset ('e');
}

static void roadmap_gps_set_messages(const RoadMapGpsPosition *gps) {

   char *timestr;
   time_t sun;
   time_t now = time (NULL);
   int before_sunset = 0;

   /* system time, not GPS time */
   roadmap_message_set ('S', "%3d %s",
                        roadmap_math_knots_to_speed_unit (gps->speed),
                        roadmap_math_speed_unit ());
   roadmap_message_set ('H', "%d %s",
                        gps->altitude, roadmap_math_distance_unit ());
   roadmap_message_set('B', "%d", gps->steering);

   sun = roadmap_sunset (gps);
   timestr = roadmap_time_get_hours_minutes (sun);
   roadmap_message_set ('e', timestr );
   if (sun > now) {
       roadmap_message_unset ('M');
       roadmap_message_set ('E', timestr );
       before_sunset = 1;
   }

   sun = roadmap_sunrise (gps);
   timestr = roadmap_time_get_hours_minutes (sun);
   roadmap_message_set ('m', timestr );
   if (!before_sunset) {
       roadmap_message_unset ('E');
       roadmap_message_set ('M', timestr);
   } 
}

static int RoadMapStartGpsRefresh = 0;

/**
 * @brief GPS listener : is called when new information is available
 * @param reception
 * @param gps_time
 * @param dilution
 * @param gps_position
 */
static void roadmap_start_gps_listen
               (int reception,
                int gps_time,
                const RoadMapGpsPrecision *dilution,
                const RoadMapGpsPosition  *gps_position) {

   static int RoadMapSynchronous = -1;

//   roadmap_log (ROADMAP_WARNING, "roadmap_start_gps_listen");

   if (RoadMapStartFrozen) {

      RoadMapStartGpsRefresh = 0;

   } else {
 
      if (reception <= GPS_RECEPTION_NONE) {

         roadmap_gps_unset_messages();

      } else {

         roadmap_object_move (RoadMapStartGpsID, gps_position);

#ifdef HAVE_TRIP_PLUGIN
	 /* Here Danny FIX ME FIXME */
         trip_set_gps (gps_time, gps_position);
         roadmap_trip_set_gps (gps_time, gps_position);
#else
         roadmap_trip_set_gps (gps_time, gps_position);
#endif
         roadmap_gps_set_messages(gps_position);

         roadmap_log_reset_stack ();

         roadmap_navigate_locate (gps_position);
         roadmap_log_reset_stack ();
      }

      if (RoadMapSynchronous) {

         if (RoadMapSynchronous < 0) {
            RoadMapSynchronous = roadmap_option_is_synchronous ();
         }

         RoadMapStartGpsRefresh = 0;

         roadmap_screen_refresh();
         roadmap_log_reset_stack ();

      } else {

         RoadMapStartGpsRefresh = 1;
      }
   }
}

static void roadmap_start_gps_monitor
               (int reception,
                const RoadMapGpsPrecision *precision,
                const RoadMapGpsSatellite *satellites,
                int activecount,
                int count) {
   roadmap_message_set('s', "%d", count);
   roadmap_message_set('v', "%d", activecount);
}


void roadmap_start_error (const char *text) {
   if (roadmap_config_match(&RoadMapConfigGeneralShowErrors, "yes")) {
      roadmap_messagebox ("Error", text);
   }
}

void roadmap_start_fatal (const char *text) {
   roadmap_messagebox_die ("Fatal Error", text);
}

static void roadmap_start_periodic (void) {

   roadmap_spawn_check ();

   if (RoadMapStartGpsRefresh) {
        RoadMapStartGpsRefresh = 0;
        roadmap_screen_refresh();
        roadmap_log_reset_stack ();
   }
}


static void roadmap_start_add_gps (RoadMapIO *io) {

   roadmap_main_set_input (io, roadmap_gps_input);
}

static void roadmap_start_remove_gps (RoadMapIO *io) {

   roadmap_main_remove_input(io);
}


static void roadmap_start_add_driver (RoadMapIO *io) {

   roadmap_main_set_input (io, roadmap_driver_input);
}

static void roadmap_start_remove_driver (RoadMapIO *io) {

   roadmap_main_remove_input(io);
}


static void roadmap_start_add_driver_server (RoadMapIO *io) {

   roadmap_main_set_input (io, roadmap_driver_accept);
}

static void roadmap_start_remove_driver_server (RoadMapIO *io) {

   roadmap_main_remove_input(io);
}


static void roadmap_start_set_timeout (RoadMapCallback callback) {

   /* NB -- if this isn't longer than the connect() timeout to a
    * network host, then if you configure to use a gpsd server on
    * a missing host, then roadmap will spend all its time
    * connecting, and get zero other work done.
    */
   roadmap_main_set_periodic (5000, callback);
}


static void roadmap_start_window (void) {

   roadmap_main_new (RoadMapMainTitle,
                     roadmap_option_width("Main"),
                     roadmap_option_height("Main"));

   roadmap_factory ("roadmap",
                    RoadMapStartActions,
                    RoadMapStartMenu,
                    RoadMapStartToolbar);

   roadmap_main_add_canvas ();

   roadmap_main_show ();

   roadmap_gps_register_link_control
      (roadmap_start_add_gps, roadmap_start_remove_gps);

   roadmap_gps_register_periodic_control
      (roadmap_start_set_timeout, roadmap_main_remove_periodic);

   roadmap_driver_register_link_control
      (roadmap_start_add_driver, roadmap_start_remove_driver);

   roadmap_driver_register_server_control
      (roadmap_start_add_driver_server, roadmap_start_remove_driver_server);
}


const char *roadmap_start_get_title (const char *name) {

   static char *RoadMapMainTitleBuffer = NULL;

   int length;


   if (name == NULL) {
      return RoadMapMainTitle;
   }

   length = strlen(RoadMapMainTitle) + strlen(name) + 4;

   if (RoadMapMainTitleBuffer != NULL) {
         free(RoadMapMainTitleBuffer);
   }
   RoadMapMainTitleBuffer = malloc (length);

   if (RoadMapMainTitleBuffer != NULL) {

      strcpy (RoadMapMainTitleBuffer, RoadMapMainTitle);
      strcat (RoadMapMainTitleBuffer, ": ");
      strcat (RoadMapMainTitleBuffer, name);
      return RoadMapMainTitleBuffer;
   }

   return name;
}


static void roadmap_start_usage (const char *section) {

   roadmap_factory_usage (section, RoadMapStartActions);
}


void roadmap_start_freeze (void) {

   RoadMapStartFrozen = 1;
   RoadMapStartGpsRefresh = 0;

   roadmap_screen_freeze ();
}

void roadmap_start_unfreeze (void) {

   RoadMapStartFrozen = 0;
   roadmap_screen_unfreeze ();
}

/**
 * @brief get a string indicating current time
 * @return the string, pointer to a static array (overwritten next call, don't free)
 */
char * roadmap_start_now() {

   return roadmap_time_get_hours_minutes (time(NULL));

}

static void roadmap_start_screen_configure (void) {
   roadgps_screen_configure();
   roadmap_screen_configure();
}

#ifdef HAVE_EDITOR_PLUGIN
int	editor_plugin_id = 0;
#endif
#ifdef HAVE_NAVIGATE_PLUGIN
int	navigate_plugin_id = 0;
#endif
#ifdef HAVE_TRIP_PLUGIN
int	trip_plugin_id = 0;
#endif

/**
 * @brief read a plugin configuration file, and dynamically load whatever it says.
 * Hardcoded this for now as there are issues with mutual requirements.
 */
void roadmap_start_plugin_register (void)
{
#if 1
	/*
	 * Workaround for now : register plugins here
	 */
#ifdef HAVE_NAVIGATE_PLUGIN
	navigate_plugin_id = navigate_plugin_register ();
#endif
#else
	RoadMapFile	plugin_file;
	char		*buffer;
	int		count, sz = 1024;
	char		*dllname, *funcname;
	RoadMapLibrary	lib;
	void (*func)(void) = NULL;

	roadmap_log (ROADMAP_WARNING, "roadmap_plugin_register()");

	/*
	 * This is not compatible with the current way of building RoadMap.
	 *
	 * At least on Linux/UNIX, dlopen() requires that the shared library
	 * is either self-contained or links other libraries.
	 * This would require e.g. roadmap_canvas_draw_image to be linked in
	 * for navigate/navigate_bar.c . But dlopen() doesn't know that this
	 * function is already in the RoadMap executable.
	 *
	 * So it is not as simple as creating a shared library per plugin :
	 * the bulk of RoadMap must also be in shared libraries so they're
	 * not loaded more than once.
	 */
	plugin_file = roadmap_file_open("plugins" _TXT, "r");
	if (plugin_file) {
		roadmap_log (ROADMAP_WARNING, "reading plugin file");
		buffer = malloc(sz);
		dllname = malloc(64);
		funcname = malloc(64);
		while (1) {
			count = roadmap_file_read (plugin_file, buffer, sz);
			if (count <= 0)
				break;
			/* Read two fields, separated by a comma. Ignore whitespace. */
			sscanf(buffer, "%[^,], %s\n", dllname, funcname);
			lib = roadmap_library_load(dllname);
			if (lib) {
#if 0
				func = roadmap_library_symbol(lib, funcname);
				roadmap_library_close(lib);
				if (func)
					(*func)();
#else
				RoadMapPluginHooks *plugin = roadmap_library_symbol(dllname, "nagivate_plugin_hooks");
				if (plugin) {
					roadmap_log(ROADMAP_WARNING, "Loaded plugin [%s] from %s",
							plugin->name,
							dllname);
				}
#endif
			}
		}
		free(buffer);
		roadmap_file_close (plugin_file);
	}
#endif
}

/**
 * @brief loop over the registered plugins and initialize them.
 */
void roadmap_start_initialize_plugins (void)
{
	roadmap_plugin_initialize_all_plugins();
}

void roadmap_start (int argc, char **argv) {

   int period;

#ifdef ROADMAP_MTRACE
   /* Do not forget to set the trace file using the env. variable MALLOC_TRACE,
    * then use the mtrace tool to analyze the output.
    */
   mtrace();
#endif

   roadmap_log_redirect (ROADMAP_MESSAGE_ERROR, roadmap_start_error);
   roadmap_log_redirect (ROADMAP_MESSAGE_FATAL, roadmap_start_fatal);

   roadmap_config_initialize ();

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralUnit, "imperial", "metric", NULL);
   
   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralKeyboard, "yes", "no", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralShowErrors, "yes", "no", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigGeometryMain, "800x600");

   roadmap_config_declare
      ("preferences", &RoadMapConfigGeometryDefault, "300x200");

   roadmap_config_declare
      ("preferences", &RoadMapConfigDisplayRefresh, "");

   roadmap_config_declare
      ("preferences", &RoadMapConfigMapPath, "");

   roadmap_config_declare
      ("preferences", &RoadMapConfigPathIcons, "");

   roadmap_config_load ();

   roadmap_option_initialize   ();
   roadmap_math_initialize     ();
   roadmap_trip_initialize     ();
   roadmap_tripdb_initialize   ();
   roadmap_track_initialize    ();
   roadmap_landmark_initialize ();
   roadmap_features_initialize ();
   roadmap_pointer_initialize  ();
   roadmap_screen_initialize   ();
   roadmap_fuzzy_initialize    ();
   roadmap_navigate_initialize ();
   roadmap_label_initialize    ();
   roadmap_display_initialize  ();
   roadmap_voice_initialize    ();
   roadmap_gps_initialize      ();
   roadgps_screen_initialize ();
   roadmap_canvas_register_configure_handler (roadmap_start_screen_configure);
   roadgps_logger_initialize ();
   roadmap_history_initialize  ();
   roadmap_download_initialize ();
   roadmap_adjust_initialize   ();
   roadmap_driver_initialize   ();
   roadmap_layer_initialize    ();
   roadmap_lang_initialize     ();

   roadmap_gps_register_listener (&roadmap_start_gps_listen);
   roadmap_gps_register_monitor (&roadmap_start_gps_monitor);

   RoadMapStartGpsID = roadmap_string_new("GPS");

   roadmap_object_add_sprite (roadmap_string_new("RoadMap"),
                              RoadMapStartGpsID,
                              NULL,
                              NULL,
                              NULL);

   if (roadmap_config_get(&RoadMapConfigMapPath)[0] != 0) {
      roadmap_path_set("maps", roadmap_config_get(&RoadMapConfigMapPath));
   }

   if (roadmap_config_get(&RoadMapConfigPathIcons)[0] != 0) {
      roadmap_path_set("icons", roadmap_config_get(&RoadMapConfigPathIcons));
   }


   roadmap_osm_initialize();

   roadmap_factory_keymap (RoadMapStartActions, RoadMapStartKeyBinding);

   roadmap_option (argc, argv, 1, roadmap_start_usage);

#ifdef _WIN32
   roadmap_log (ROADMAP_WARNING,
		   "RoadMap build %s starting, time %s",
		   roadmap_build(), roadmap_start_now());
#else
   roadmap_log (ROADMAP_WARNING,
		   "RoadMap starting, time %s",
		   roadmap_start_now());
#endif

   roadmap_start_set_unit ();
   
   roadmap_math_restore_zoom ();
   roadmap_start_plugin_register   ();
   roadmap_start_window      ();
   roadmap_label_activate    ();
   roadmap_sprite_load       ();
   roadmap_layer_load        ();

   roadmap_screen_set_initial_position ();

   roadmap_history_load ();

   roadmap_track_activate ();
#ifdef ROADMAP_USES_EXPAT
   roadmap_track_autoload ();
   roadmap_landmark_load ();
   roadmap_features_load ();
#endif

   roadmap_spawn_initialize (argv[0]);

   roadmap_start_initialize_plugins();
   
   roadmap_driver_activate ();
   roadmap_gps_open ();

   roadmap_help_initialize ();

   roadmap_screen_obj_initialize ();

//#ifndef HAVE_TRIP_PLUGIN
   roadmap_trip_restore_focus ();
//#endif

#ifndef HAVE_TRIP_PLUGIN
# ifdef ROADMAP_USES_EXPAT
   if ( ! roadmap_trip_load (1, 0)) {
      roadmap_trip_new ();
   }
# else
   roadmap_trip_new ();
# endif
#else
# ifdef ROADMAP_USES_EXPAT
   if ( ! trip_load (1, 0)) {
      trip_new ();
   }
# else
   trip_new ();
# endif
#endif

   roadmap_locator_declare_downloader (&roadmap_start_no_download);

   period = atoi(roadmap_config_get(&RoadMapConfigDisplayRefresh));
   if (period <= 10) {
      period = ROADMAP_DEFAULT_REFRESH_INTERVAL;
   }
   roadmap_main_set_periodic (period, roadmap_start_periodic);

   roadmap_start_request_repaint_map (REPAINT_NOW);
}

/**
 * @brief terminate RoadMap
 */
void roadmap_start_exit (void)
{
    roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);
    roadmap_driver_shutdown ();
    roadmap_history_save();
#ifdef ROADMAP_USES_EXPAT
    roadmap_track_autowrite ();
    roadmap_landmark_save ();
#ifndef HAVE_TRIP_PLUGIN
    roadmap_trip_save ();
#endif
#endif
#ifndef HAVE_TRIP_PLUGIN
    roadmap_trip_preserve_focus();
#endif
    roadmap_config_save (0);
    roadmap_gps_shutdown ();
    roadmap_log (ROADMAP_WARNING, "RoadMap exiting, time %s", roadmap_start_now());
}

const RoadMapAction *roadmap_start_find_action (const char *name) {

   return roadmap_factory_find_action_or_menu (RoadMapStartActions, name);

}

