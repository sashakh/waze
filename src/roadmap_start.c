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
#include "roadmap_string.h"
#include "roadmap_config.h"
#include "roadmap_history.h"

#include "roadmap_spawn.h"
#include "roadmap_path.h"
#include "roadmap_io.h"

#include "roadmap_object.h"
#include "roadmap_voice.h"
#include "roadmap_gps.h"

#include "roadmap_preferences.h"
#include "roadmap_address.h"
#include "roadmap_coord.h"
#include "roadmap_crossing.h"
#include "roadmap_sprite.h"
#include "roadmap_screen_obj.h"
#include "roadmap_trip.h"
#include "roadmap_adjust.h"
#include "roadmap_screen.h"
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
#include "roadmap_messagebox.h"
#include "roadmap_help.h"
#include "roadmap_pointer.h"
#include "roadmap_sound.h"
#include "roadmap_lang.h"
#include "roadmap_start.h"

#ifdef SSD
#include "ssd/ssd_menu.h"
#endif

#include "navigate/navigate_main.h"
#include "editor/editor_main.h"
#include "editor/db/editor_db.h"
#include "editor/static/update_range.h"
#include "editor/static/edit_marker.h"
#include "editor/static/notes.h"
#include "editor/export/editor_export.h"
#include "editor/export/editor_upload.h"
#include "editor/export/editor_download.h"
#include "editor/export/editor_sync.h"

static const char *RoadMapMainTitle = "RoadMap";

static int RoadMapStartFrozen = 0;

static RoadMapDynamicString RoadMapStartGpsID;

static RoadMapConfigDescriptor RoadMapConfigGeneralUnit =
                        ROADMAP_CONFIG_ITEM("General", "Unit");

static RoadMapConfigDescriptor RoadMapConfigGeneralKeyboard =
                        ROADMAP_CONFIG_ITEM("General", "Keyboard");

static RoadMapConfigDescriptor RoadMapConfigGeometryMain =
                        ROADMAP_CONFIG_ITEM("Geometry", "Main");

static RoadMapConfigDescriptor RoadMapConfigMapPath =
                        ROADMAP_CONFIG_ITEM("Map", "Path");

static RoadMapMenu LongClickMenu;
#ifndef SSD
static RoadMapMenu QuickMenu;
#endif

static RoadMapStartSubscriber  RoadMapStartSubscribers = NULL;
static RoadMapScreenSubscriber roadmap_start_prev_after_refresh = NULL;

/* The menu and toolbar callbacks: --------------------------------------- */

static void roadmap_start_periodic (void);

static void roadmap_start_console (void) {

   const char *url = roadmap_gps_source();

   if (url == NULL) {
      roadmap_spawn ("roadgps", "");
   } else {
      char arguments[1024];
      snprintf (arguments, sizeof(arguments), "--gps=%s", url);
      roadmap_spawn ("roadgps", arguments);
   }
}

static void roadmap_start_purge (void) {
   roadmap_history_purge (10);
}

static void roadmap_start_show_destination (void) {
    roadmap_trip_set_focus ("Destination");
    roadmap_screen_refresh ();
}

static void roadmap_start_show_location (void) {
    roadmap_trip_set_focus ("Address");
    roadmap_screen_refresh ();
}

static void roadmap_start_show_gps (void) {
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

static void roadmap_start_about (void) {

   char about[500];

   snprintf (about, sizeof(about),
                       "RoadMap " ROADMAP_VERSION "\n"
                       "(c) " ROADMAP_YEAR " Pascal Martin\n"
                       "<pascal.martin@iname.com>\n"
                       "(c) Ehud Shabtai\n"
                       "eshabtai@gmail.com\n"
                       "A Street navigation system\n"
                       "for Linux, UNIX & WinCE"
                       "\n\nEditor Plugin %s\n",
                       editor_main_get_version ());

   roadmap_messagebox ("About", about);
}

static void roadmap_start_export_data (void) {

   editor_export_gpx ();
}

static void roadmap_start_export_reset (void) {

   editor_export_reset_dirty ();
}

static void roadmap_start_download_map (void) {

   editor_download_update_map (NULL);
}

static void roadmap_start_upload_gpx (void) {
   editor_upload_select ();
}

static void roadmap_start_create_trip (void) {
    
    roadmap_trip_new ();
}

static void roadmap_start_open_trip (void) {
    
    roadmap_trip_load (NULL, 0);
}

static void roadmap_start_save_trip (void) {
    
    roadmap_trip_save (roadmap_trip_current());
}

static void roadmap_start_save_trip_as (void) {
    
    roadmap_trip_save (NULL);
}

static void roadmap_start_trip (void) {
    
    roadmap_trip_start ();
}

static void roadmap_start_trip_resume (void) {
    
    roadmap_trip_resume ();
}

static void roadmap_start_trip_reverse (void) {
    
    roadmap_trip_reverse ();
}

static void roadmap_start_navigate (void) {
    
    navigate_main_calc_route ();
}

static void roadmap_start_set_destination (void) {

    roadmap_trip_set_selection_as ("Destination");
    roadmap_screen_refresh();
}

static void roadmap_start_set_departure (void) {

    roadmap_trip_set_selection_as ("Departure");
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

static int roadmap_start_no_download (int fips) {

   if (! roadmap_download_blocked (fips)) {
      roadmap_log (ROADMAP_WARNING, "cannot open map database usc%05d", fips);
      roadmap_download_block (fips);
   }
   return 0;
}

static void roadmap_start_toggle_download (void) {

   if (roadmap_download_enabled()) {

      roadmap_download_subscribe_when_done (NULL);
      roadmap_locator_declare (&roadmap_start_no_download);

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

      roadmap_download_subscribe_when_done (roadmap_screen_redraw);
      //roadmap_locator_declare (roadmap_download_get_county);
      roadmap_download_unblock_all ();
   }

   roadmap_screen_redraw ();
}


static void roadmap_start_detect_receiver (void) {
    
    roadmap_gps_detect_receiver ();
}


static void roadmap_start_sync_data (void) {
    
    export_sync ();
}


static void roadmap_start_quick_menu (void);

/* The RoadMap menu and toolbar items: ----------------------------------- */

/* This table lists all the RoadMap actions that can be initiated
 * fom the user interface (a sort of symbol table).
 * Any other part of the user interface (menu, toolbar, etc..)
 * will reference an action.
 */
static RoadMapAction RoadMapStartActions[] = {

   {"preferences", "Preferences", "Preferences", "P",
      "Open the preferences editor", roadmap_preferences_edit},

   {"gpsconsole", "GPS Console", "Console", "C",
      "Start the GPS console application", roadmap_start_console},

   {"mutevoice", "Mute Voice", "Mute", NULL,
      "Mute all voice annoucements", roadmap_voice_mute},

   {"enablevoice", "Enable Voice", "Mute Off", NULL,
      "Enable all voice annoucements", roadmap_voice_enable},

   {"nonavigation", "Disable Navigation", "Nav Off", NULL,
      "Disable all navigation functions", roadmap_navigate_disable},

   {"navigation", "Enable Navigation", "Nav On", NULL,
      "Enable all GPS-based navigation functions", roadmap_navigate_enable},

   {"logtofile", "Log to File", "Log", NULL,
      "Save future log messages to the postmortem file",
      roadmap_log_save_all},

   {"nolog", "Disable Log", "No Log", NULL,
      "Do not save future log messages to the postmortem file",
      roadmap_log_save_all},

   {"purgelogfile", "Purge Log File", "Purge", NULL,
      "Delete the current postmortem log file", roadmap_log_purge},

   {"purgehistory", "Purge History", "Forget", NULL,
      "Remove all but the 10 most recent addresses", roadmap_start_purge},

   {"quit", "Quit", NULL, NULL,
      "Quit RoadMap", roadmap_main_exit},

   {"zoomin", "Zoom In", "+", NULL,
      "Enlarge the central part of the map", roadmap_screen_zoom_in},

   {"zoomout", "Zoom Out", "-", NULL,
      "Show a larger area", roadmap_screen_zoom_out},

   {"zoom1", "Normal Size", ":1", NULL,
      "Set the map back to the default zoom level", roadmap_screen_zoom_reset},

   {"up", "Up", "N", NULL,
      "Move the map view upward", roadmap_screen_move_up},

   {"left", "Left", "W", NULL,
      "Move the map view to the left", roadmap_screen_move_left},

   {"right", "Right", "E", NULL,
      "Move the map view to the right", roadmap_screen_move_right},

   {"down", "Down", "S", NULL,
      "Move the map view downward", roadmap_screen_move_down},

   {"toggleview", "Toggle view mode", "M", NULL,
      "Toggle view mode 2D / 3D", roadmap_screen_toggle_view_mode},

   {"toggleorientation", "Toggle orientation mode", "", NULL,
      "Toggle orientation mode dynamic / fixed",
      roadmap_screen_toggle_orientation_mode},

   {"IncHorizon", "Increase Horizon", "I", NULL,
      "Increase the 3D horizon", roadmap_screen_increase_horizon},

   {"DecHorizon", "Decrease Horizon", "DI", NULL,
      "Decrease the 3D horizon", roadmap_screen_decrease_horizon},

   {"clockwise", "Rotate Clockwise", "R+", NULL,
      "Rotate the map view clockwise", roadmap_start_rotate},

   {"counterclockwise", "Rotate Counter-Clockwise", "R-", NULL,
      "Rotate the map view counter-clockwise", roadmap_start_counter_rotate},

   {"hold", "Hold Map", "Hold", "H",
      "Hold the map view in its current position", roadmap_start_hold_map},

   {"address", "Address...", "Addr", "A",
      "Show a specified address", roadmap_address_location_by_city},

   {"intersection", "Intersection...", "X", NULL,
      "Show a specified street intersection", roadmap_crossing_dialog},

   {"position", "Position...", "P", NULL,
      "Show a position at the specified coordinates", roadmap_coord_dialog},

   {"destination", "Destination", "D", NULL,
      "Show the current destination point", roadmap_start_show_destination},

   {"gps", "GPS Position", "GPS", "G",
      "Center the map on the current GPS position", roadmap_start_show_gps},

   {"location", "Location", "L", NULL,
      "Center the map on the last selected location",
      roadmap_start_show_location},

   {"mapdownload", "Map Download", "Download", NULL,
      "Enable/Disable the map download mode", roadmap_start_toggle_download},

   {"mapdiskspace", "Map Disk Space", "Disk", NULL,
      "Show the amount of disk space occupied by the maps",
      roadmap_download_show_space},

   {"deletemaps", "Delete Maps...", "Delete", "Del",
      "Delete maps that are currently visible", roadmap_download_delete},

   {"newtrip", "New Trip", "New", NULL,
      "Create a new trip", roadmap_start_create_trip},

   {"opentrip", "Open Trip", "Open", "O",
      "Open an existing trip", roadmap_start_open_trip},

   {"savetrip", "Save Trip", "Save", "S",
      "Save the current trip", roadmap_start_save_trip},

   {"savescreenshot", "Make a screenshot of the map", "Screenshot", "Y",
      "Make a screenshot of the current map under the trip name",
      roadmap_trip_save_screenshot},

   {"savetripas", "Save Trip As...", "Save As", "As",
      "Save the current trip under a different name",
      roadmap_start_save_trip_as},

   {"starttrip", "Start Trip", "Start", NULL,
      "Start tracking the current trip", roadmap_start_trip},

   {"stoptrip", "Stop Trip", "Stop", NULL,
      "Stop tracking the current trip", roadmap_trip_stop},

   {"resumetrip", "Resume Trip", "Resume", NULL,
      "Resume the trip (keep the existing departure point)",
      roadmap_start_trip_resume},

   {"returntrip", "Return Trip", "Return", NULL,
      "Start the trip back to the departure point",
      roadmap_start_trip_reverse},

   {"setasdeparture", "Set as Departure", NULL, NULL,
      "Set the selected street block as the trip's departure",
      roadmap_start_set_departure},

   {"setasdestination", "Set as Destination", NULL, NULL,
      "Set the selected street block as the trip's destination",
      roadmap_start_set_destination},

   {"navigate", "Navigate", NULL, NULL,
      "Calculate route",
      roadmap_start_navigate},

   {"addaswaypoint", "Add as Waypoint", "Waypoint", "W",
      "Set the selected street block as waypoint", roadmap_start_set_waypoint},

   {"deletewaypoints", "Delete Waypoints...", "Delete...", NULL,
      "Delete selected waypoints", roadmap_start_delete_waypoint},

   {"full", "Full Screen", "Full", "F",
      "Toggle the window full screen mode (depends on the window manager)",
      roadmap_main_toggle_full_screen},

   {"about", "About", NULL, NULL,
      "Show information about RoadMap", roadmap_start_about},

   {"exportdata", "Export Data", NULL, NULL,
      "Export editor data", roadmap_start_export_data},

   {"resetexport", "Reset export data", NULL, NULL,
      "Reset export data", roadmap_start_export_reset},

   {"updatemap", "Update map", NULL, NULL,
      "Export editor data", roadmap_start_download_map},

   {"uploadgpx", "Upload GPX file", NULL, NULL,
      "Export editor data", roadmap_start_upload_gpx},

   {"detectreceiver", "Detect GPS receiver", NULL, NULL,
      "Auto-detect GPS receiver", roadmap_start_detect_receiver},

   {"sync", "Sync", NULL, NULL,
      "Sync map and data", roadmap_start_sync_data},

   {"quickmenu", "Open quick menu", NULL, NULL,
      "Open quick menu", roadmap_start_quick_menu},

   {"updaterange", "Update street range", NULL, NULL,
      "Update street range", update_range_dialog},

   {"viewmarkers", "View markers", NULL, NULL,
      "View / Edit markers", edit_markers_dialog},

   {"addquicknote", "Add a quick note", NULL, NULL,
      "Add a quick note", editor_notes_add_quick},

   {"addeditnote", "Add a note", NULL, NULL,
      "Add a note and open edit dialog", editor_notes_add_edit},

   {"addvoicenote", "Add a voice note", NULL, NULL,
      "Add a voice note", editor_notes_add_voice},

   {NULL, NULL, NULL, NULL, NULL, NULL}
};


static const char *RoadMapStartCfgActions[] = {

   "preferences",
   "mutevoice",
   "enablevoice",
   "quit",
   "zoomin",
   "zoomout",
   "zoom1",
   "up",
   "left",
   "right",
   "down",
   "toggleview",
   "toggleorientation",
   "IncHorizon",
   "DecHorizon",
   "clockwise",
   "counterclockwise",
   "hold",
   "address",
   "destination",
   "gps",
   "location",
   "full",
   "sync",
   "quickmenu",
   "updaterange",
   "viewmarkers",
   "addquicknote",
   "addeditnote",
   "addvoicenote",
   NULL
};


static const char *RoadMapStartMenu[] = {

   ROADMAP_MENU "File",

   "preferences",
   "gpsconsole",

   RoadMapFactorySeparator,

   "sync",
   "exportdata",
   "updatemap",
   "uploadgpx",
/*   "resetexport", */

   RoadMapFactorySeparator,
   "mutevoice",
   "enablevoice",
   "nonavigation",
   "navigation",

   RoadMapFactorySeparator,

   "logtofile",
   "nolog",
   "purgehistory",

   RoadMapFactorySeparator,

   "quit",


   ROADMAP_MENU "View",

   "zoomin",
   "zoomout",
   "zoom1",

   RoadMapFactorySeparator,

   "up",
   "left",
   "right",
   "down",
   "toggleorientation",
   "toggleview",
   "full",

   RoadMapFactorySeparator,

   "clockwise",
   "counterclockwise",

   RoadMapFactorySeparator,

   "hold",


   ROADMAP_MENU "Find",

   "address",
   "intersection",
   "position",

   RoadMapFactorySeparator,

   "destination",
   "gps",

   RoadMapFactorySeparator,

   "mapdownload",
   "mapdiskspace",
   "deletemaps",


   ROADMAP_MENU "Trip",

   "newtrip",
   "opentrip",
   "savetrip",
   "savetripas",
   "savescreenshot",

   RoadMapFactorySeparator,

   "starttrip",
   "stoptrip",
   "resumetrip",
   "resumetripnorthup",
   "returntrip",
   "navigate",

   RoadMapFactorySeparator,

   "setasdestination",
   "setasdeparture",
   "addaswaypoint",
   "deletewaypoints",

   ROADMAP_MENU "Tools",

   "detectreceiver",

   ROADMAP_MENU "Help",

   RoadMapFactoryHelpTopics,

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


static char const *RoadMapStartLongClickMenu[] = {

   "setasdeparture",
   "setasdestination",
   "navigate",

   NULL,
};


static char const *RoadMapStartQuickMenu[] = {

   "address",
   RoadMapFactorySeparator,
   "detectreceiver",
   "preferences",
   "about",
   "quit",

   NULL,
};

#ifndef UNDER_CE
static char const *RoadMapStartKeyBinding[] = {

   "Button-Left"     ROADMAP_MAPPED_TO "left",
   "Button-Right"    ROADMAP_MAPPED_TO "right",
   "Button-Up"       ROADMAP_MAPPED_TO "up",
   "Button-Down"     ROADMAP_MAPPED_TO "down",

   /* These binding are for the iPAQ buttons: */
   "Button-Menu"     ROADMAP_MAPPED_TO "zoom1",
   "Button-Contact"  ROADMAP_MAPPED_TO "zoomin",
   "Button-Calendar" ROADMAP_MAPPED_TO "zoomout",
   "Button-Start"    ROADMAP_MAPPED_TO "quit",

   /* These binding are for regular keyboards (case unsensitive !): */
   "+"               ROADMAP_MAPPED_TO "zoomin",
   "-"               ROADMAP_MAPPED_TO "zoomout",
   "A"               ROADMAP_MAPPED_TO "address",
   "B"               ROADMAP_MAPPED_TO "returntrip",
   /* C Unused. */
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
   /* T Unused. */
   "U"               ROADMAP_MAPPED_TO "gpsnorthup",
   /* V Unused. */
   "W"               ROADMAP_MAPPED_TO "addaswaypoint",
   "X"               ROADMAP_MAPPED_TO "intersection",
   "Y"               ROADMAP_MAPPED_TO "savesscreenshot",
   /* Z Unused. */
   NULL
};

#else
static char const *RoadMapStartKeyBinding[] = {

   "Button-Left"     ROADMAP_MAPPED_TO "counterclockwise",
   "Button-Right"    ROADMAP_MAPPED_TO "clockwise",
   "Button-Up"       ROADMAP_MAPPED_TO "zoomin",
   "Button-Down"     ROADMAP_MAPPED_TO "zoomout",

   "Button-App1"     ROADMAP_MAPPED_TO "",
   "Button-App2"     ROADMAP_MAPPED_TO "",
   "Button-App3"     ROADMAP_MAPPED_TO "",
   "Button-App4"     ROADMAP_MAPPED_TO "",

   NULL
};
#endif


static void roadmap_start_quick_menu (void) {

#ifdef SSD
   ssd_menu_activate ("quick", RoadMapStartQuickMenu, RoadMapStartActions);
   //ssd_keyboard_show ();
   //ssd_test_list ();
#else
   if (QuickMenu == NULL) {

       QuickMenu = roadmap_factory_menu ("quick",
                                     RoadMapStartQuickMenu,
                                     RoadMapStartActions);
   }

   if (QuickMenu != NULL) {
      const RoadMapGuiPoint *point = roadmap_pointer_position ();
      roadmap_main_popup_menu (QuickMenu, point->x, point->y);
   }
#endif   
}


#ifdef UNDER_CE
static void roadmap_start_init_key_cfg (void) {

   const char **keys = RoadMapStartKeyBinding;
   RoadMapConfigDescriptor config = ROADMAP_CONFIG_ITEM("KeyBinding", "");

   while (*keys) {
      char *text;
      char *separator;
      const RoadMapAction *this_action;
      const char **cfg_actions = RoadMapStartCfgActions;
      RoadMapConfigItem *item;

      text = strdup (*keys);
      roadmap_check_allocated(text);

      separator = strstr (text, ROADMAP_MAPPED_TO);
      if (separator != NULL) {

         const char *new_config = NULL;
         char *p;
         for (p = separator; *p <= ' '; --p) *p = 0;

         p = separator + strlen(ROADMAP_MAPPED_TO);
         while (*p && (*p <= ' ')) ++p;

         this_action = roadmap_start_find_action (p);

         config.name = text;
         config.reference = NULL;

         if (this_action != NULL) {

            item = roadmap_config_declare_enumeration
                   ("preferences", &config,
                    roadmap_lang_get (this_action->label_long), NULL);

            if (strcmp(roadmap_lang_get (this_action->label_long),
                     roadmap_config_get (&config))) {

               new_config = roadmap_config_get (&config);
            }

            roadmap_config_add_enumeration_value (item, "");
         } else {

            item = roadmap_config_declare_enumeration
                   ("preferences", &config, "", NULL);

            if (strlen(roadmap_config_get (&config))) {
               new_config = roadmap_config_get (&config);
            }
         }

         while (*cfg_actions) {
            const RoadMapAction *cfg_action =
                        roadmap_start_find_action (*cfg_actions);
            if (new_config &&
                  !strcmp(new_config,
                          roadmap_lang_get (cfg_action->label_long))) {
               new_config = cfg_action->name;
            }

            roadmap_config_add_enumeration_value
                     (item, roadmap_lang_get (cfg_action->label_long));
            cfg_actions++;
         }

         if (new_config != NULL) {
            char str[100];
            snprintf(str, sizeof(str), "%s %s %s", text, ROADMAP_MAPPED_TO,
                                                   new_config);
            *keys = strdup(str);
         }
      }

      keys++;
   }
}
#endif


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
               (time_t gps_time,
                const RoadMapGpsPrecision *dilution,
                const RoadMapGpsPosition *gps_position) {

   static int RoadMapSynchronous = -1;

   if (RoadMapStartFrozen) {

      RoadMapStartGpsRefresh = 0;

   } else {

      roadmap_object_move (RoadMapStartGpsID, gps_position);

      roadmap_trip_set_mobile ("GPS", gps_position);
      roadmap_log_reset_stack ();

      roadmap_navigate_locate (gps_position);
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

   roadmap_main_set_periodic (3000, callback);
}


static void roadmap_start_long_click (RoadMapGuiPoint *point) {
   
   RoadMapPosition position;

   roadmap_math_to_position (point, &position, 1);
   roadmap_trip_set_point ("Selection", &position);
   
   if (LongClickMenu != NULL) {
      roadmap_main_popup_menu (LongClickMenu, point->x, point->y);
   }
}
 

static void roadmap_start_window (void) {

   roadmap_main_new (RoadMapMainTitle,
                     roadmap_option_width("Main"),
                     roadmap_option_height("Main"));

   roadmap_factory ("roadmap",
                    RoadMapStartActions,
                    RoadMapStartMenu,
                    RoadMapStartToolbar);

   LongClickMenu = roadmap_factory_menu ("long_click",
                                         RoadMapStartLongClickMenu,
                                         RoadMapStartActions);

   roadmap_pointer_register_long_click (roadmap_start_long_click);

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

   if (roadmap_start_prev_after_refresh) {
      (*roadmap_start_prev_after_refresh) ();
   }
}


static void roadmap_start_usage (const char *section) {

   roadmap_factory_keymap (RoadMapStartActions, RoadMapStartKeyBinding);
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

int roadmap_start_is_frozen (void) {

   return RoadMapStartFrozen;
}


void roadmap_start (int argc, char **argv) {

#ifdef ROADMAP_DEBUG_HEAP
   /* Do not forget to set the trace file using the env. variable MALLOC_TRACE,
    * then use the mtrace tool to analyze the output.
    */
   mtrace();
#endif

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralUnit, "imperial", "metric", NULL);
   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralKeyboard, "yes", "no", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigGeometryMain, "800x600");

   roadmap_option_initialize   ();
   roadmap_math_initialize     ();
   roadmap_trip_initialize     ();
   roadmap_pointer_initialize  ();
   roadmap_screen_initialize   ();
   roadmap_fuzzy_initialize    ();
   roadmap_navigate_initialize ();
   roadmap_label_initialize    ();
   roadmap_display_initialize  ();
   roadmap_voice_initialize    ();
   roadmap_gps_initialize      ();
   roadmap_history_initialize  ();
   roadmap_download_initialize ();
   roadmap_adjust_initialize   ();
   roadmap_driver_initialize   ();
   roadmap_config_initialize   ();
   roadmap_lang_initialize     ();
   roadmap_sound_initialize    ();

   roadmap_start_set_title (roadmap_lang_get ("RoadMap"));
   roadmap_gps_register_listener (&roadmap_gps_update);

   RoadMapStartGpsID = roadmap_string_new("GPS");

   roadmap_object_add (roadmap_string_new("RoadMap"),
                       RoadMapStartGpsID,
                       NULL,
                       NULL);

   roadmap_path_set("maps", roadmap_config_get(&RoadMapConfigMapPath));

#if UNDER_CE
   roadmap_start_init_key_cfg ();
#endif   

   roadmap_option (argc, argv, roadmap_start_usage);

   roadmap_start_set_unit ();
   
   roadmap_math_restore_zoom ();
   roadmap_start_window      ();
   roadmap_factory_keymap (RoadMapStartActions, RoadMapStartKeyBinding);
   roadmap_label_activate    ();
   roadmap_sprite_initialize ();
   roadmap_screen_obj_initialize ();

   roadmap_screen_set_initial_position ();

   roadmap_history_load ();
   
   roadmap_spawn_initialize (argv[0]);

   roadmap_driver_activate ();

   roadmap_help_initialize ();

   roadmap_start_prev_after_refresh =
      roadmap_screen_subscribe_after_refresh (roadmap_start_after_refresh);

   if (RoadMapStartSubscribers) RoadMapStartSubscribers (ROADMAP_START_INIT);

   editor_main_initialize ();
   navigate_main_initialize ();

   roadmap_trip_restore_focus ();

   if (! roadmap_trip_load (roadmap_trip_current(), 1)) {
      roadmap_start_create_trip ();
   }

   roadmap_gps_open ();

   roadmap_locator_declare (&roadmap_start_no_download);
   roadmap_main_set_periodic (200, roadmap_start_periodic);
}


void roadmap_start_exit (void) {
    
    roadmap_plugin_shutdown ();
    roadmap_driver_shutdown ();
    roadmap_sound_shutdown ();
    roadmap_history_save ();
    roadmap_screen_shutdown ();
    roadmap_start_save_trip ();
    roadmap_config_save (0);
    roadmap_db_end ();
    roadmap_gps_shutdown ();
}


const RoadMapAction *roadmap_start_find_action (const char *name) {

   const RoadMapAction *actions = RoadMapStartActions;

   while (actions->name != NULL) {
      if (strcmp (actions->name, name) == 0) return actions;
      ++actions;
   }

   return NULL;
}


void roadmap_start_set_title (const char *title) {
   RoadMapMainTitle = title;
}


RoadMapStartSubscriber roadmap_start_subscribe
                                 (RoadMapStartSubscriber handler) {

   RoadMapStartSubscriber previous = RoadMapStartSubscribers;

   RoadMapStartSubscribers = handler;

   return previous;
}

