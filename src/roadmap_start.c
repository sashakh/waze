/* roadmap_start.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   (c) Copyright 2002, 2003 Pascal F. Martin
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
 *   void roadmap_start (int argc, char **argv);
 */

#include <stdlib.h>
#include <string.h>

#ifdef ROADMAP_DEBUG_HEAP
#include <mcheck.h>
#endif

#include "roadmap.h"
#include "roadmap_copyright.h"
#include "roadmap_dbread.h"
#include "roadmap_math.h"
#include "roadmap_spawn.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_path.h"
#include "roadmap_gps.h"
#include "roadmap_voice.h"

#include "roadmap_preferences.h"
#include "roadmap_address.h"
#include "roadmap_coord.h"
#include "roadmap_crossing.h"
#include "roadmap_sprite.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include "roadmap_fuzzy.h"
#include "roadmap_navigate.h"
#include "roadmap_display.h"
#include "roadmap_locator.h"
#include "roadmap_copy.h"
#include "roadmap_httpcopy.h"
#include "roadmap_download.h"
#include "roadmap_factory.h"
#include "roadmap_main.h"
#include "roadmap_messagebox.h"


static const char *RoadMapMainTitle = "RoadMap";

static int RoadMapStartFrozen = 0;

/* Last trip orientation mode (1: follow GPS direction, 0: north up). */
static int RoadMapStartTripOrientation = 1;

static RoadMapConfigDescriptor RoadMapConfigGeneralUnit =
                        ROADMAP_CONFIG_ITEM("General", "Unit");

static RoadMapConfigDescriptor RoadMapConfigGeneralToolbar =
                        ROADMAP_CONFIG_ITEM("General", "Toolbar");

static RoadMapConfigDescriptor RoadMapConfigGeneralKeyboard =
                        ROADMAP_CONFIG_ITEM("General", "Keyboard");

static RoadMapConfigDescriptor RoadMapConfigGeometryMain =
                        ROADMAP_CONFIG_ITEM("Geometry", "Main");

static RoadMapConfigDescriptor RoadMapConfigMapPath =
                        ROADMAP_CONFIG_ITEM("Map", "Path");


/* The menu and toolbar callbacks: --------------------------------------- */

static void roadmap_start_periodic (void);

static void roadmap_start_console (void) {
   roadmap_spawn ("roadgps", "");
}

static void roadmap_start_purge (void) {
   roadmap_history_purge (10);
}

static void roadmap_start_show_destination (void) {
    roadmap_trip_set_focus ("Destination", 0);
    roadmap_screen_refresh ();
}

static void roadmap_start_show_location (void) {
    roadmap_trip_set_focus ("Address", 0);
    roadmap_screen_refresh ();
}

static void roadmap_start_show_gps (void) {
    roadmap_trip_set_focus ("GPS", 1);
    roadmap_screen_refresh ();
}

static void roadmap_start_show_gps_north_up (void) {
    roadmap_trip_set_focus ("GPS", 0);
    roadmap_screen_refresh ();
}

static void roadmap_start_hold_map (void) {
   roadmap_start_periodic (); /* To make sure the map is current. */
   roadmap_trip_copy_focus ("Hold");
   if (roadmap_trip_get_orientation () != 0) {
      roadmap_trip_set_focus ("Hold", 1);
   } else {
      roadmap_trip_set_focus ("Hold", 0);
   }
}

static void roadmap_start_rotate (void) {
    roadmap_screen_rotate (10);
}

static void roadmap_start_counter_rotate (void) {
    roadmap_screen_rotate (-10);
}

static void roadmap_start_about (void) {

   roadmap_messagebox ("About",
                       "RoadMap " ROADMAP_VERSION "\n"
                       "(c) " ROADMAP_YEAR " Pascal Martin\n"
                       "<pascal.martin@iname.com>\n"
                       "A Street navigation system\n"
                       "for Linux & UNIX");
}

static void roadmap_start_create_trip (void) {
    
    roadmap_trip_new ();
}

static void roadmap_start_open_trip (void) {
    
    roadmap_trip_load (NULL);
}

static void roadmap_start_save_trip (void) {
    
    roadmap_trip_save (roadmap_trip_current());
}

static void roadmap_start_save_trip_as (void) {
    
    roadmap_trip_save (NULL);
}

static void roadmap_start_trip (void) {
    
    roadmap_trip_start (1);
}

static void roadmap_start_trip_resume (void) {
    
    RoadMapStartTripOrientation = 1;
    roadmap_trip_resume (1);
}

static void roadmap_start_trip_resume_north_up (void) {
    
    RoadMapStartTripOrientation = 0;
    roadmap_trip_resume (0);
}

static void roadmap_start_trip_reverse (void) {
    
    roadmap_trip_reverse (RoadMapStartTripOrientation);
}

static void roadmap_start_set_destination (void) {

    roadmap_trip_set_selection_as ("Destination");
    roadmap_screen_refresh();
}

static void roadmap_start_set_waypoint (void) {

    const char *id = roadmap_display_get_id ("Selected Street");

    if (id != NULL) {
       roadmap_trip_set_selection_as (id);
       roadmap_screen_refresh();
    }
}

static void roadmap_start_delete_waypoint (void) {
    
    roadmap_trip_remove_point (NULL);
}

static void roadmap_start_toggle_download_mode (void) {

   if (roadmap_download_enabled()) {

      roadmap_download_subscribe_when_done (NULL);
      roadmap_locator_declare (NULL);

   } else {

      static int ProtocolInitialized = 0;

      if (! ProtocolInitialized) {

         // roadmap_plugin_load_all
         //      ("download", roadmap_download_subscribe_protocol);

         roadmap_copy_init (roadmap_download_subscribe_protocol);
         roadmap_httpcopy_init (roadmap_download_subscribe_protocol);

         ProtocolInitialized = 1;
      }

      roadmap_download_subscribe_when_done (roadmap_screen_redraw);
      roadmap_locator_declare (roadmap_download_get_county);
   }

   roadmap_screen_redraw ();
}


/* The RoadMap menu and toolbar items: ----------------------------------- */

static RoadMapFactory RoadMapStartMenu[] = {

   {"File", NULL, NULL, NULL},

   {"Preferences", NULL,
       "Open the preferences editor", roadmap_preferences_edit},
   {"GPS Console", NULL,
       "Start the GPS console application", roadmap_start_console},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Mute Voice", NULL,
       "Mute all voice annoucements", roadmap_voice_mute},
   {"Enable Voice", NULL,
       "Enable all voice annoucements", roadmap_voice_enable},
   {"Disable Navigation", NULL,
      "Disable all GPS-based navigation functions", roadmap_navigate_disable},
   {"Enable Navigation", NULL,
      "Enable all GPS-based navigation functions", roadmap_navigate_enable},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Enable Log to File", NULL,
       "Save future log messages to the postmortem log file",
       roadmap_log_save_all},
   {"Purge Log File", NULL,
       "Delete the current postmortem log file", roadmap_log_purge},
   {"Purge History", NULL,
       "Remove all but the 10 most recent addresses", roadmap_start_purge},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Quit", NULL,
       "Quit RoadMap", roadmap_main_exit},


   {"View", NULL, NULL, NULL},

   {"Zoom In", NULL,
       "Enlarge the central part of the map", roadmap_screen_zoom_in},
   {"Zoom Out", NULL,
       "Show a larger area", roadmap_screen_zoom_out},
   {"Normal Size", NULL,
       "Set the map back to the default zoom level", roadmap_screen_zoom_reset},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Up", NULL,
       "Move the map view upward", roadmap_screen_move_up},
   {"Left", NULL,
       "Move the map view to the left", roadmap_screen_move_left},
   {"Right", NULL,
       "Move the map view to the right", roadmap_screen_move_right},
   {"Down", NULL,
       "Move the map view downward", roadmap_screen_move_down},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Rotate Clockwise", NULL,
       "Rotate the map view clockwise", roadmap_start_rotate},
   {"Rotate Counter-Clockwise", NULL,
       "Rotate the map view counter-clockwise", roadmap_start_counter_rotate},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Hold Map", NULL,
       "Hold the map view in its current position", roadmap_start_hold_map},


   {"Find", NULL, NULL, NULL},

   {"Address...", NULL,
       "Show a specified address", roadmap_address_location_by_city},
   {"Intersection...", NULL,
       "Show a specified street intersection", roadmap_crossing_dialog},
   {"Position...", NULL,
       "Show a position at the specified coordinates", roadmap_coord_dialog},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Destination", NULL,
       "Show the destination point", roadmap_start_show_destination},
   {"GPS Position", NULL,
       "Show the GPS position", roadmap_start_show_gps},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Map Download", NULL,
       "Enable/Disable the map download mode", roadmap_start_toggle_download_mode},
   {"Map Disk Space", NULL,
       "Show the amount of disk space occupied by the maps", roadmap_download_show_space},
   {"Delete Maps...", NULL,
       "Delete maps that are currently visible", roadmap_download_delete},


   {"Trip", NULL, NULL, NULL},

   {"New Trip", NULL,
       "Create a new trip", roadmap_start_create_trip},
   {"Open Trip...", NULL,
       "Open an existing trip", roadmap_start_open_trip},
   {"Save Trip", NULL,
       "Save the current trip", roadmap_start_save_trip},
   {"Save Trip As...", NULL,
       "Save the current trip under a different name", roadmap_start_save_trip_as},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Start Trip", NULL,
       "Start tracking the current trip", roadmap_start_trip},
   {"Stop Trip", NULL,
       "Stop tracking the current trip", roadmap_trip_stop},
   {"Resume Trip", NULL,
       "Resume the trip (keep the existing departure point)",
       roadmap_start_trip_resume},
   {"Resume Trip (North Up)", NULL,
       "Resume the trip (keep the existing departure point)",
       roadmap_start_trip_resume_north_up},
   {"Return Trip", NULL,
       "Start the trip back to the departure point",
       roadmap_start_trip_reverse},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"Set as Destination", NULL,
       "Set the selected street block as the trip's destination",
       roadmap_start_set_destination
   },
   {"Add as Waypoint",  NULL,
       "Set the selected street block as waypoint", roadmap_start_set_waypoint},
   {"Delete Waypoints...",  NULL,
       "Delete selected waypoints", roadmap_start_delete_waypoint},


   {"Help", NULL, NULL, NULL},

   {"About", NULL,
       "Show information about RoadMap", roadmap_start_about},

   {NULL, NULL, NULL, NULL}
};

static RoadMapFactory RoadMapStartToolbar[] = {

   {"D", NULL,
      "Center the map on the destination", roadmap_start_show_destination},
   {"L", NULL,
      "Center the map on the current location", roadmap_start_show_location},
   {"G", NULL,
      "Center the map on the GPS position", roadmap_start_show_gps},
   {"g", NULL,
      "Center the map on the GPS position (north up)",
       roadmap_start_show_gps_north_up},

   {"H", NULL,
       "Hold the map view in its current position", roadmap_start_hold_map},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"F", NULL,
       "Toggle the window full screen (if the window manager agrees)",
        roadmap_main_toggle_full_screen},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"R-", NULL,
       "Rotate the map view counter-clockwise", roadmap_start_counter_rotate},
   {"R+", NULL,
       "Rotate the map view clockwise", roadmap_start_rotate},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"+", NULL, "Zoom into the map", roadmap_screen_zoom_in},
   {"-", NULL, "Zoom out from the map", roadmap_screen_zoom_out},
   {"R", NULL,
       "Reset the map back to the default zoom level",
        roadmap_screen_zoom_reset},

   {RoadMapFactorySeparator, NULL, NULL, NULL},

   {"N", NULL, "Move the map up", roadmap_screen_move_up},
   {"W", NULL, "Move the map to the left", roadmap_screen_move_left},
   {"E", NULL, "Move the map to the right", roadmap_screen_move_right},
   {"S", NULL, "Move the map down", roadmap_screen_move_down},

   {NULL, NULL, NULL, NULL}
};


/* The "tooltip" text specified here is only used for the --help option,
 * to document the currently supported keyboard shortcuts.
 */
static RoadMapFactory RoadMapStartKeyBinding[] = {

   {"Button-Left",  NULL,
      "Move the map view to the left", roadmap_screen_move_left},
   {"Button-Right", NULL,
      "Move the map view to the right", roadmap_screen_move_right},
   {"Button-Up",    NULL,
      "Move the map view up", roadmap_screen_move_up},
   {"Button-Down",  NULL,
      "Move the map view down", roadmap_screen_move_down},

   /* These binding are for the iPAQ buttons: */
   {"Button-Menu",     NULL,
      "Set the map back to the default zoom level", roadmap_screen_zoom_reset},
   {"Button-Contact",  NULL,
      "Zoom in: enlarge the central part of the map", roadmap_screen_zoom_in},
   {"Button-Calendar", NULL,
      "Zoom out: show a larger area", roadmap_screen_zoom_out},
   {"Button-Start",    NULL,
      "Quit RoadMap", roadmap_main_exit},

   /* These binding are for regular keyboards (case unsensitive !): */
   {"+", NULL,
      "Zoom in: enlarge the central part of the map", roadmap_screen_zoom_in},
   {"-", NULL,
      "Zoom out: show a larger area", roadmap_screen_zoom_out},
   {"A", NULL,
      "Show a specified address", roadmap_address_location_by_city},
   // B Unused.
   // C Unused.
   {"D", NULL,
      "Show the destination point", roadmap_start_show_destination},
   {"E", NULL,
      "Erase maps among those currently visible", roadmap_download_delete},
   {"F", NULL,
      "Toggle the full screen mode", roadmap_main_toggle_full_screen},
   {"G", NULL,
      "Show the GPS position", roadmap_start_show_gps},
   {"H", NULL,
      "Hold the map view in its current position", roadmap_start_hold_map},
   {"I", NULL,
      "Show a specified street intersection", roadmap_crossing_dialog},
   // J Unused.
   // K Unused.
   {"L", NULL,
      "Show the last selected location", roadmap_start_show_location},
   {"M", NULL,
      "Enable/Disable map download", roadmap_start_toggle_download_mode},
   {"N", NULL,
      "Create a new trip", roadmap_start_create_trip},
   {"O", NULL,
      "Open an existing trip", roadmap_start_open_trip},
   {"P", NULL,
      "Stop tracking the current trip", roadmap_trip_stop},
   {"Q", NULL,
      "Quit RoadMap", roadmap_main_exit},
   {"R", NULL,
      "Set the map back to the default zoom level", roadmap_screen_zoom_reset},
   {"S", NULL,
      "Start tracking the current trip", roadmap_start_trip},
   // T Unused.
   {"U", NULL,
      "Start the trip back to the departure point", roadmap_start_trip_reverse},
   // V Unused.
   {"W",  NULL,
      "Set the selected street block as waypoint", roadmap_start_set_waypoint},
   // X Unused.
   // Y Unused.
   // Z Unused.
   {NULL, NULL, NULL, NULL}
};


static void roadmap_start_usage (void) {

   RoadMapFactory *cursor;

   printf ("RoadMap button & key bindings:\n\n");
   for (cursor = RoadMapStartKeyBinding; cursor->name != NULL; ++cursor) {
      printf ("  %-20.20s\t%s.\n", cursor->name, cursor->tip);
   }
}


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


static int RoadMapStartGpsRefresh = 0;

static void roadmap_gps_update
                (RoadMapPosition *position, int speed, int direction) {

   static int RoadMapSynchronous = -1;

   if (RoadMapStartFrozen) {

      RoadMapStartGpsRefresh = 0;

   } else {

      roadmap_trip_set_mobile ("GPS", position, speed, direction);
      roadmap_log_reset_stack ();

      roadmap_navigate_locate (position, speed, direction);
      roadmap_log_reset_stack ();

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


static void roadmap_start_periodic (void) {

   roadmap_spawn_check ();

   if (RoadMapStartGpsRefresh) {

      RoadMapStartGpsRefresh = 0;

      roadmap_screen_refresh();
      roadmap_log_reset_stack ();
   }
}


static void roadmap_start_add_gps (int fd) {

   roadmap_main_set_input (fd, roadmap_gps_input);
}


static void roadmap_start_set_timeout (RoadMapCallback callback) {

   roadmap_main_set_periodic (3000, callback);
}


static void roadmap_start_window (void) {

   roadmap_main_new (RoadMapMainTitle,
                     roadmap_option_width("Main"),
                     roadmap_option_height("Main"));

   roadmap_factory (RoadMapStartMenu,
                    RoadMapStartToolbar,
                    RoadMapStartKeyBinding);

   roadmap_main_add_canvas ();

   roadmap_main_show ();

   roadmap_gps_register_link_control
      (roadmap_start_add_gps, roadmap_main_remove_input);

   roadmap_gps_register_periodic_control
      (roadmap_start_set_timeout, roadmap_main_remove_periodic);
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


static void roadmap_start_after_refresh (void) {

   if (roadmap_download_enabled()) {

      RoadMapGuiPoint download_point = {0, 20};

      download_point.x = roadmap_canvas_width() - 20;
      if (download_point.x < 0) {
         download_point.x = 0;
      }
      roadmap_sprite_draw
         ("Download", &download_point, 0 - roadmap_math_get_orientation());
   }
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


void roadmap_start (int argc, char **argv) {

#ifdef ROADMAP_DEBUG_HEAP
   // Do not forget to set the trace file using the env. variable MALLOC_TRACE,
   // then use the mtrace tool to analyze the output.
   mtrace();
#endif

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralUnit, "imperial", "metric", NULL);
   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralToolbar, "yes", "no", NULL);
   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralKeyboard, "yes", "no", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigGeometryMain, "800x600");

   roadmap_option_initialize   ();
   roadmap_math_initialize     ();
   roadmap_trip_initialize     ();
   roadmap_screen_initialize   ();
   roadmap_fuzzy_initialize    ();
   roadmap_navigate_initialize ();
   roadmap_display_initialize  ();
   roadmap_voice_initialize    ();
   roadmap_gps_initialize      (&roadmap_gps_update);
   roadmap_history_initialize  ();
   roadmap_download_initialize ();
   roadmap_config_initialize   ();

   roadmap_path_set(roadmap_config_get(&RoadMapConfigMapPath));

   roadmap_option (argc, argv, roadmap_start_usage);

   roadmap_start_set_unit ();
   
   roadmap_math_restore_zoom ();
   roadmap_start_window      ();
   roadmap_sprite_initialize ();

   roadmap_screen_set_initial_position ();

   roadmap_history_load ();
   
   roadmap_gps_open ();

   roadmap_spawn_initialize (argv[0]);

   roadmap_screen_subscribe_after_refresh (roadmap_start_after_refresh);

   roadmap_trip_restore_focus ();

   if (! roadmap_trip_load (roadmap_trip_current())) {
      roadmap_start_create_trip ();
   }

   roadmap_main_set_periodic (1000, roadmap_start_periodic);
}


void roadmap_start_exit (void) {
    
    roadmap_history_save();
    roadmap_config_save (0);
    roadmap_start_save_trip ();
}
