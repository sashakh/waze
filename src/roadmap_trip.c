/*
 * LICENSE:
 *
 *   Copyright 2002,2005 Pascal F. Martin, Paul G. Fox
 *   Copyright (c) 2008, 2009 Danny Backx
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
 * @brief Manage a trip: destination & waypoints.
 *
 * This file is somewhat overloaded with functionality : it appears to keep track
 * of quite a number of things (trip, waypoints, starting point/destination/..,
 * focal points, ..), it draws a visual indication (green dots and slim red lines
 * between them) of the current route, .. .
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_types.h"
#include "roadmap_time.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_dialog.h"
#include "roadmap_fileselection.h"
#include "roadmap_canvas.h"
#include "roadmap_sprite.h"
#include "roadmap_screen.h"
#include "roadmap_state.h"
#include "roadmap_pointer.h"
#include "roadmap_message.h"
#include "roadmap_messagebox.h"
#include "roadmap_preferences.h"
#include "roadmap_display.h"
#include "roadmap_adjust.h"
#include "roadmap_gpx.h"
#include "roadmap_track.h"
#include "roadmap_landmark.h"
#include "roadmap_street.h"
#include "roadmap_layer.h"
#include "roadmap_point.h"
#include "roadmap_skin.h"

#include "roadmap_trip.h"
#include "roadmap_tripdb.h"
#include "roadmap_plugin.h"

static RoadMapConfigDescriptor RoadMapConfigTripName =
                        ROADMAP_CONFIG_ITEM("Trip", "Name");

static RoadMapConfigDescriptor RoadMapConfigTripShowInactiveRoutes =
                        ROADMAP_CONFIG_ITEM("Trip", "Show Inactive Routes");

static RoadMapConfigDescriptor RoadMapConfigTripShowInactiveTracks =
                        ROADMAP_CONFIG_ITEM("Trip", "Show Inactive Tracks");

static RoadMapConfigDescriptor RoadMapConfigTripShowRouteLines =
                        ROADMAP_CONFIG_ITEM("Trip", "Connect Route Points");

static RoadMapConfigDescriptor RoadMapConfigTripRouteLineColor =
                        ROADMAP_CONFIG_ITEM("Trip", "Route Connect Color");

static RoadMapConfigDescriptor RoadMapConfigFocusName =
                        ROADMAP_CONFIG_ITEM("Focus", "Name");

static RoadMapConfigDescriptor RoadMapConfigWaypointSize =
                        ROADMAP_CONFIG_ITEM("Trip", "Waypoint Radius");

static RoadMapConfigDescriptor RoadMapConfigBackupFiles =
                        ROADMAP_CONFIG_ITEM("Files", "Make Backups");

static RoadMapConfigDescriptor RoadMapConfigPathTrips =
                        ROADMAP_CONFIG_ITEM("General", "TripsPath");

static RoadMapConfigDescriptor RoadMapConfigTripGPSFocusReleaseDelay =
                        ROADMAP_CONFIG_ITEM("Trip", "GPS Focus Release Delay");

/*
 * try and put the trip name in the window title
 */
#define TRIP_TITLE_FMT " - %s"

/* Default location is: 1 Market St, San Francisco, California,
 *  but every effort is made to choose a more "local" initial
 *  position if one is available.  (i.e., this will probably only be
 *  used when the program is first run.) */
#define ROADMAP_INITIAL_LONGITUDE -122394181
#define ROADMAP_INITIAL_LATITUDE    37794928

extern route_head *RoadMapTrack;

/* route following flags */
// static route_head *RoadMapCurrentRoute = NULL;
static int RoadMapRouteInProgress = 0;
static int RoadMapRouteIsReversed = 0;

/* display flags */
static int RoadMapTripRefresh = 1;      /**< Screen needs to be refreshed? */
static int RoadMapTripFocusChanged = 1;
static int RoadMapTripFocusMoved = 1;

/* trip file related flags */
static int RoadMapTripUntitled = 1;
static int RoadMapTripModified = 0;     /**< Trip needs to be saved? */

/**
 * @brief support for on-screen waypoint selection
 */
typedef struct {
   RoadMapListItem link;
   waypoint *wpt;
   const char *list;
   void *type;
} roadmap_place_pointer;

static RoadMapList RoadMapTripAreaPlaces;  /**< places near the mouse-click */
static roadmap_place_pointer *RoadMapTripSelectedPlace;

static const char *RoadMapAreaCurListName;
static void *RoadMapAreaCurListType;

static int RoadMapTripPlaceMoving;

/**
 * @brief We use common code to manage the various waypoint lists, even though
 * they have differing semantics.  These flags help keep them apart.
 * We use the address of the flag as the discriminator (since it gets passed
 * around as a "void *", this is convenient), and the flag itself indicates
 * whether the list in the dialog box is up-to-date with the actual data
 * structure.
 */
int RoadMapTripWaypointSelectionsNeedRefresh[4];
#define PERSONAL_WAYPOINTS (&RoadMapTripWaypointSelectionsNeedRefresh[0])
#define TRIP_WAYPOINTS     (&RoadMapTripWaypointSelectionsNeedRefresh[1])
#define ROUTE_WAYPOINTS    (&RoadMapTripWaypointSelectionsNeedRefresh[2])
#define LOST_WAYPOINTS     (&RoadMapTripWaypointSelectionsNeedRefresh[3])
#define WAYPOINTS_MODIFIED 1

static void roadmap_trip_set_selected_place(void *which, waypoint *w);
static void roadmap_trip_clear_selection_list(void);
static int roadmap_trip_move_last_cancel (RoadMapGuiPoint *point);

/* route display altering flags */
static RoadMapPen RoadMapTripRouteLinesPen = NULL;
static int RoadMapTripDrawingActiveRoute;
static int RoadMapTripShowInactiveRoutes;
static int RoadMapTripShowInactiveTracks;

typedef struct roadmap_trip_focal {
    char *id;			/**< the name of this item */
    char *sprite;		/**< */

    char mobile;		/**< */
    char save;			/**< do we save this value in configuration files */
    char in_trip;		/**< */
    char has_value;		/**< does this focal point have a value */
    char preserve;		/**< */

    RoadMapPosition map;	/**< */
    RoadMapGpsPosition gps;	/**< */

    RoadMapConfigDescriptor config_position;		/**< */
    RoadMapConfigDescriptor config_direction;		/**< */


} RoadMapTripFocal;


#define ROADMAP_TRIP_ITEM(id, sprite, mobile, save, in_trip, preserve) \
    {id, sprite, mobile, save, in_trip, 0, preserve, \
     {0, 0}, \
     ROADMAP_GPS_NULL_POSITION, \
     ROADMAP_CONFIG_ITEM(id,"Position"), \
     ROADMAP_CONFIG_ITEM(id,"Direction") \
    }

/**
 * @brief The entries in the FocalPoints table are primarily used for providing
 * map focus state, but are also involved in other trip bookkeeping roles --
 * where we are, where we're going, where we've been.
 */
RoadMapTripFocal RoadMapTripFocalPoints[] = {
    ROADMAP_TRIP_ITEM ("GPS", "GPS",                1, 1, 0, 1),
    ROADMAP_TRIP_ITEM ("Destination", NULL,         0, 1, 1, 0), /**< changed : gets saved */
    ROADMAP_TRIP_ITEM ("Start", NULL,               0, 0, 1, 0),
    ROADMAP_TRIP_ITEM ("WayPoint", NULL,            0, 1, 0, 0),
    ROADMAP_TRIP_ITEM ("Address", NULL,             0, 1, 0, 0),
    ROADMAP_TRIP_ITEM ("Selection", "Selection",    0, 0, 0, 0),
    ROADMAP_TRIP_ITEM ("Departure", "Departure",    0, 1, 1, 0),
    ROADMAP_TRIP_ITEM ("Hold", NULL,                0, 1, 0, 0),
    ROADMAP_TRIP_ITEM (NULL, NULL,                  0, 0, 0, 0)
};

/* WARNING:  These are pointers into the above "predefined" table -- order is important.  */
static RoadMapTripFocal
	*RoadMapTripGps = &RoadMapTripFocalPoints[0],
	*RoadMapTripDestination = &RoadMapTripFocalPoints[1],
	*RoadMapTripBeginning = &RoadMapTripFocalPoints[2],
	*RoadMapTripWaypointFocus = &RoadMapTripFocalPoints[3],
	*RoadMapTripAddress = &RoadMapTripFocalPoints[4],
	*RoadMapTripSelection = &RoadMapTripFocalPoints[5],
	*RoadMapTripDeparture = &RoadMapTripFocalPoints[6];

// 	*RoadMapTripHold = &RoadMapTripFocalPoints[7];

/* These will point at one of the above. */
static RoadMapTripFocal *RoadMapTripFocus = NULL;
static RoadMapTripFocal *RoadMapTripLastSetPoint = NULL;

/* These point at waypoints in the current route. */
// static waypoint *RoadMapTripDest = NULL;
static waypoint *RoadMapTripStart = NULL;
static waypoint *RoadMapTripNext = NULL;

/* This is the name of the waypoint used to set the WaypointFocus
 * focal point.
 */
static char *RoadMapTripWaypointFocusName = NULL;

static time_t RoadMapTripGPSTime = 0;

// static RoadMapList RoadMapTripWaypointHead;
// static RoadMapList RoadMapTripRouteHead;
// static RoadMapList RoadMapTripTrackHead;

static RoadMapList RoadMapTripLostRoutesHead;
static RoadMapList RoadMapTripLostPlacesHead;

static route_head RoadMapTripQuickRoute = {
        {NULL, NULL},
        {NULL, NULL},
        "Quick Route",
        "Quick Route",
        0, 0, 0, NULL
};

#define	ROUTE_PEN_WIDTH	4

int RoadMapTripEnabled = 0;

static RoadMapPosition RoadMapTripDefaultPosition =
     {ROADMAP_INITIAL_LONGITUDE, ROADMAP_INITIAL_LATITUDE};

/**
 * @brief
 * @param modified
 */
void roadmap_trip_set_modified(int modified)
{
	RoadMapTripModified = modified;
}

/**
 * @brief
 * @return
 */
static waypoint * roadmap_trip_beginning (void)
{
    if (RoadMapRouteIsReversed)
        return (waypoint *) ROADMAP_LIST_LAST (&RoadMapCurrentRoute->waypoint_list);
    else
        return (waypoint *) ROADMAP_LIST_FIRST (&RoadMapCurrentRoute->waypoint_list);
}

/**
 * @brief
 * @return
 */
static waypoint * roadmap_trip_ending (void) {

    if (RoadMapRouteIsReversed)
        return (waypoint *) ROADMAP_LIST_FIRST (&RoadMapCurrentRoute->waypoint_list);
    else
        return (waypoint *) ROADMAP_LIST_LAST (&RoadMapCurrentRoute->waypoint_list);
}

/**
 * @brief
 * @return
 */
static waypoint * roadmap_trip_next (const waypoint *waypointp) {

    if (RoadMapRouteIsReversed)
        return (waypoint *)ROADMAP_LIST_PREV(&waypointp->Q);
    else
        return (waypoint *)ROADMAP_LIST_NEXT(&waypointp->Q);
}

/**
 * @brief
 * @return
 */
static waypoint * roadmap_trip_prev (const waypoint *waypointp) {

    if (RoadMapRouteIsReversed)
        return (waypoint *)ROADMAP_LIST_NEXT(&waypointp->Q);
    else
        return (waypoint *)ROADMAP_LIST_PREV(&waypointp->Q);
}

static void roadmap_trip_set_route_focii (void) {

    /* Set the waypoints. */
    RoadMapTripDest = roadmap_trip_ending ();
    RoadMapTripStart = roadmap_trip_beginning ();

    /* Set the focal points. */
    if (RoadMapTripDest) {
	    RoadMapTripDestination->map = RoadMapTripDest->pos;
	    RoadMapTripDestination->has_value = 1;
    }
    if (RoadMapTripStart) {
	    RoadMapTripBeginning->map = RoadMapTripStart->pos;
	    RoadMapTripBeginning->has_value = 1;
    }
}

/**
 * @brief
 */
void roadmap_trip_unset_route_focii (void)
{
    /* waypoints */
    RoadMapTripDest = NULL;
    RoadMapTripStart = NULL;

    /* focal points */
    RoadMapTripDestination->has_value = 0;
    RoadMapTripBeginning->has_value = 0;
    RoadMapTripDeparture->has_value = 0;

    RoadMapRouteInProgress = 0;
    RoadMapRouteIsReversed = 0;
}

/**
 * @brief
 * @param id
 * @return
 */
static RoadMapTripFocal * roadmap_trip_find_focalpoint (const char *id)
{
    RoadMapTripFocal *focal;

    for (focal = RoadMapTripFocalPoints; focal->id != NULL; ++focal) {
        if (strcmp (id, focal->id) == 0) {
            return focal;
        }
    }
    return NULL;
}


/**
 * @brief translate from position to GUI point
 * @param position
 * @param guipoint
 */
static void roadmap_trip_coordinate (const RoadMapPosition *position, RoadMapGuiPoint *guipoint)
{
	if (roadmap_math_point_is_visible (position)) {
		roadmap_math_coordinate (position, guipoint);
	} else {
		guipoint->x = 32767;
		guipoint->y = 32767;
    }
}

static void roadmap_trip_dialog_cancel (const char *name, void *data) {

    roadmap_dialog_hide (name);
}

static void roadmap_trip_dialog_route_edit_okay (const char *name, void *data) {

    char *newname = (char *) roadmap_dialog_get_data ("Text", "Name:");
    char *newdesc = (char *) roadmap_dialog_get_data ("Text", "Comment:");
    route_head *route = data;

    if (route->rte_name) free(route->rte_name);
    route->rte_name = newname[0] ? strdup(newname) : NULL;

    if (route->rte_desc) free(route->rte_desc);
    route->rte_desc = newdesc[0] ? strdup(newdesc) : NULL;

    roadmap_trip_set_modified(1);

    roadmap_dialog_hide (name);
}

static void roadmap_trip_dialog_route_edit
        (route_head *route, int use_keyboard) {

    if (roadmap_dialog_activate ("Route Edit", route)) {

        roadmap_dialog_new_entry ("Text", "Name:");
        roadmap_dialog_new_entry ("Text", "Comment:");

        roadmap_dialog_add_button ("Cancel",
                roadmap_trip_dialog_cancel);
        roadmap_dialog_add_button ("Okay",
                roadmap_trip_dialog_route_edit_okay);

        roadmap_dialog_complete (use_keyboard);
    }

    roadmap_dialog_set_data
        ("Text", "Name:", route->rte_name ? route->rte_name : "");
    roadmap_dialog_set_data
        ("Text", "Comment:", route->rte_desc ? route->rte_desc : "");

}

int
safe_strcmp(char *s1, char *s2)
{
    if (!s1) s1 = "";
    if (!s2) s2 = "";
    return strcmp(s1, s2);
}

static void roadmap_trip_dialog_waypoint_edit_okay (const char *name, void *data) {

    char *newname = (char *) roadmap_dialog_get_data ("Data", "Name:");
    char *newdesc = (char *) roadmap_dialog_get_data ("Data", "Comment:");
    char *newlon = (char *) roadmap_dialog_get_data ("Data", "Longitude:");
    char *newlat = (char *) roadmap_dialog_get_data ("Data", "Latitude:");
    // can't edit timestamp yet
    // char *timestr = (char *) roadmap_dialog_get_data ("Data", "Timestamp:");
    void *which = roadmap_dialog_get_data ("Data", ".which");
    waypoint *wpt = data;
    int tmp;

    if (safe_strcmp(wpt->shortname, newname)) {
        if (wpt->shortname) free(wpt->shortname);
        wpt->shortname = newname[0] ? strdup(newname) : NULL;
        RoadMapTripRefresh = 1;
    }

    if (safe_strcmp(wpt->description, newdesc)) {
        if (wpt->description) free(wpt->description);
        wpt->description = newdesc[0] ? strdup(newdesc) : NULL;
        RoadMapTripRefresh = 1;
    }

    tmp = roadmap_math_from_floatstring(newlon, MILLIONTHS);
    if (tmp != 0 && abs(tmp) <= 180 * 1000000) {
        if (wpt->pos.longitude != tmp) {
            wpt->pos.longitude = tmp;
            RoadMapTripRefresh = 1;
        }
    }
    tmp = roadmap_math_from_floatstring(newlat, MILLIONTHS);
    if (tmp != 0 && abs(tmp) <= 90 * 1000000) {
        if (wpt->pos.latitude != tmp) {
            wpt->pos.latitude = tmp;
            RoadMapTripRefresh = 1;
        }
    }

    if (which == PERSONAL_WAYPOINTS) {
        roadmap_landmark_set_modified();
    } else {
        roadmap_trip_set_modified(1);
    }

    roadmap_dialog_hide (name);

    if (RoadMapTripRefresh)
        roadmap_screen_refresh ();
}

static void roadmap_trip_dialog_waypoint_edit
        (waypoint *wpt, void *which, int use_keyboard) {

    static char lon[20], lat[20], timestr[40];

    if (roadmap_dialog_activate ("Waypoint Edit", wpt)) {

        roadmap_dialog_new_entry ("Data", "Name:");
        roadmap_dialog_new_entry ("Data", "Comment:");
        roadmap_dialog_new_entry ("Data", "Longitude:");
        roadmap_dialog_new_entry ("Data", "Latitude:");
        roadmap_dialog_new_entry ("Data", "Timestamp:");

        roadmap_dialog_add_button ("Cancel",
                roadmap_trip_dialog_cancel);
        roadmap_dialog_add_button ("Okay",
                roadmap_trip_dialog_waypoint_edit_okay);

        roadmap_dialog_new_hidden ("Data", ".which");

        roadmap_dialog_complete (use_keyboard);
    }

    roadmap_dialog_set_data
        ("Data", "Name:", wpt->shortname ? wpt->shortname : "");
    roadmap_dialog_set_data
        ("Data", "Comment:", wpt->description ? wpt->description : "");

    if (wpt->creation_time) {
        strftime(timestr, sizeof(timestr), "%F %T", gmtime(&wpt->creation_time));
    } else {
        strcpy(timestr, "N/A");
    }
    roadmap_dialog_set_data
        ("Data", "Timestamp:", timestr);

    roadmap_dialog_set_data("Data", ".which", which);

    roadmap_math_to_floatstring(lon, wpt->pos.longitude, MILLIONTHS);
    roadmap_math_to_floatstring(lat, wpt->pos.latitude, MILLIONTHS);

    roadmap_dialog_set_data ("Data", "Longitude:", lon);
    roadmap_dialog_set_data ("Data", "Latitude:", lat);

}


/* Focus management */

/**
 * @brief set the focus to this point, this means everything points to this now
 * @param focal the point
 */
static void roadmap_trip_set_point_focus (RoadMapTripFocal * focal)
{
    if (focal == NULL || !focal->has_value) {
        return;
    }

    RoadMapTripRefresh = 1;
    RoadMapTripFocusChanged = 1;

    if (RoadMapTripFocus != focal) {
        roadmap_config_set (&RoadMapConfigFocusName, focal->id);
        RoadMapTripFocus = focal;
        roadmap_display_page (focal->id);
    }

    RoadMapTripLastSetPoint = focal;
}

/**
 * @brief
 * @param id
 */
void roadmap_trip_set_focus (const char *id)
{
	/* Callers from outside refer to focus points by name. */
	RoadMapTripFocal *focal = roadmap_trip_find_focalpoint (id);

	if (focal == NULL)
		return;

	roadmap_trip_set_point_focus (focal);
}

/**
 * @brief
 * @param waypointp
 */
static void roadmap_trip_set_focus_waypoint (waypoint * waypointp)
{
    if (waypointp == NULL) {
        RoadMapTripWaypointFocus->map = RoadMapTripDefaultPosition;
        free(RoadMapTripWaypointFocusName);
        RoadMapTripWaypointFocusName = NULL;
    } else {
        RoadMapTripWaypointFocus->map = waypointp->pos;
        free(RoadMapTripWaypointFocusName);
        RoadMapTripWaypointFocusName = strdup(waypointp->shortname);
    }
    RoadMapTripWaypointFocus->has_value = 1;

    roadmap_trip_set_point_focus (RoadMapTripWaypointFocus);

    roadmap_config_set_position (
            &RoadMapTripWaypointFocus->config_position,
            &RoadMapTripWaypointFocus->map );
}

int roadmap_trip_is_focus_changed (void) {

    if (RoadMapTripFocusChanged) {

        RoadMapTripFocusChanged = 0;
        return 1;
    }
    return 0;
}

int roadmap_trip_is_focus_moved (void) {

    if (RoadMapTripFocusMoved) {

        RoadMapTripFocusMoved = 0;
        return 1;
    }
    return 0;
}

void roadmap_trip_preserve_focus (void) {

    /* For most focii, we don't want to save the focus, but
     * rather the most recent screen position.  Current exception
     * is GPS position.
     */
    if (!RoadMapTripFocus->preserve)
        roadmap_screen_hold();
}

void roadmap_trip_restore_focus (void) {

    RoadMapTripFocal *focal, *fallback = NULL;

    for (focal = RoadMapTripFocalPoints; focal->id != NULL; ++focal) {

        if ( ! focal->save) continue;

        roadmap_config_get_position
            (&focal->config_position, &focal->map);

        if (focal->map.latitude != 0 ||
            focal->map.longitude != 0) {

            focal->has_value = 1;
            if (fallback == NULL)
                fallback = focal;

        }

        if (focal->mobile) {

            /* This is a mobile point: what was recorded was the GPS
             * position, not the adjusted map position.
             */
            focal->gps.longitude = focal->map.longitude;

            focal->gps.latitude = focal->map.latitude;

            focal->gps.steering =
                roadmap_config_get_integer (&focal->config_direction);

            roadmap_adjust_position (&focal->gps, &focal->map);
        }
    }

    focal = roadmap_trip_find_focalpoint
            (roadmap_config_get (&RoadMapConfigFocusName));

    if (focal == NULL || !focal->has_value) {
        focal = RoadMapTripGps;
    }

    if (focal == NULL || !focal->has_value) {
        focal = fallback;
    }

    if (focal == NULL || !focal->has_value) {
        roadmap_trip_set_focus_waypoint (NULL);
    } else {
        roadmap_trip_set_point_focus (focal);
    }

    RoadMapTripFocusChanged = 0;
}

/* Add Waypoint dialog */

#define PLACE_PERSONAL_MARK         0
#define PLACE_TRIP_MARK             1
#define PLACE_NEW_ROUTE             2
#define PLACE_ROUTE_MARK_DEST       3
#define PLACE_ROUTE_MARK_INSERT     4
#define PLACE_ROUTE_MARK_START      5

#define NUM_PLACEMENTS           3
#define NUM_ROUTE_PLACEMENTS     6

/* This is ordered so that the latter half can be left out if
 * there's no current route.
 */
static char *Placement_Choices [] =
{
    "Add to personal landmarks",
    "Add to trip's landmarks",
    "Start a new route",
    "Place at route end",
    "Insert in closest route segment",
    "Place before route start",
};

static void *Placement_Vals [] = {
    (void *)PLACE_PERSONAL_MARK,
    (void *)PLACE_TRIP_MARK,
    (void *)PLACE_NEW_ROUTE,
    (void *)PLACE_ROUTE_MARK_DEST,
    (void *)PLACE_ROUTE_MARK_INSERT,
    (void *)PLACE_ROUTE_MARK_START
};

static char **Placements = Placement_Choices;

/**
 * @brief
 * @param name
 * @param data
 */
static void roadmap_trip_add_waypoint_dialog_okay (const char *name, void *data)
{
    RoadMapPosition *pos = (RoadMapPosition *)data;
    char *point_name;
    int where;

    point_name = (char *) roadmap_dialog_get_data ("Name", "Name:");

    /* (long) cast to suppress warning on 64-bit platforms */
    where = (long) roadmap_dialog_get_data ("Name", ".placements");

    if (point_name && point_name[0] != 0) {
        roadmap_trip_add_waypoint (point_name, pos, where);
        roadmap_dialog_hide (name);
    }
}

/**
 * @brief
 * @param name
 * @param position
 */
static void roadmap_trip_add_waypoint_dialog (const char *name, RoadMapPosition * position)
{
    static RoadMapPosition point_position;
    int route_options = (RoadMapCurrentRoute &&
                RoadMapCurrentRoute != &RoadMapTripQuickRoute);

    if (name == NULL) name = "Waypoint";

    point_position = *position;

    /* If there's no current route, we don't want to offer the "add
     * to route" choices.  So we change the name to get two dialogs.
     */
    if (roadmap_dialog_activate
        (route_options ? "Add New Place" : "Add new place", &point_position)) {

        roadmap_dialog_new_entry ("Name", "Name:");
        roadmap_dialog_new_choice
            ("Name", ".placements",
                route_options ? NUM_ROUTE_PLACEMENTS : NUM_PLACEMENTS, 0,
                Placements, Placement_Vals, NULL);

        roadmap_dialog_add_button
                ("Cancel", roadmap_trip_dialog_cancel);
        roadmap_dialog_add_button
                ("Okay", roadmap_trip_add_waypoint_dialog_okay);

        roadmap_dialog_complete (roadmap_preferences_use_keyboard ());
    }
    roadmap_dialog_set_data ("Name", "Name:", name);

}

/* Manage waypoints dialog */

#define WAYPOINT_ACTION_DELETE 0
#define WAYPOINT_ACTION_MOVE_BACK 1
#define WAYPOINT_ACTION_MOVE_AHEAD 2
#define WAYPOINT_ACTION_EDIT 3

static int roadmap_trip_waypoint_manage_dialog_populate (void *which);



/* Invoked both from the waypoint_manage dialog buttons, and from
 * the waypoint managing menu items.
 */
static void roadmap_trip_waypoint_manage_action
        (waypoint *waypointp, void *which, int action) {

    waypoint *neighbor;
    
    switch(action) {
    case WAYPOINT_ACTION_MOVE_BACK:
        if (which != ROUTE_WAYPOINTS)
            return;
        if (waypointp == RoadMapTripStart)
            return;

        *(int *)which = WAYPOINTS_MODIFIED;
        neighbor = (waypoint *)ROADMAP_LIST_PREV(&waypointp->Q);
        waypt_del (waypointp);
        roadmap_list_put_before(&neighbor->Q, &waypointp->Q);
        break;

    case WAYPOINT_ACTION_MOVE_AHEAD:
        if (which != ROUTE_WAYPOINTS)
            return;
        if (waypointp == RoadMapTripDest)
            return;

        *(int *)which = WAYPOINTS_MODIFIED;
        neighbor = (waypoint *)ROADMAP_LIST_NEXT(&waypointp->Q);
        waypt_del (waypointp);
        roadmap_list_put_after(&neighbor->Q, &waypointp->Q);
        break;

    case WAYPOINT_ACTION_DELETE:

        if (which == LOST_WAYPOINTS)
            return;

        *(int *)which = WAYPOINTS_MODIFIED;

        roadmap_trip_clear_selection_list();

        if (which == PERSONAL_WAYPOINTS) {
            roadmap_list_insert
                (&RoadMapTripLostPlacesHead,
                    (RoadMapListItem *)roadmap_landmark_remove(waypointp));
            RoadMapTripRefresh = 1;
            roadmap_screen_refresh ();
            return;
        } else if (which == TRIP_WAYPOINTS) {
            roadmap_list_insert
                (&RoadMapTripLostPlacesHead,
                    roadmap_list_remove(&waypointp->Q));
        } else {
            /* NOTE!!!!  this throws off the count in rte_waypt_ct
             * in whatever route this waypoint belongs to.  but we
             * don't use that count, ever, so that's okay.
             */
            roadmap_list_insert
                (&RoadMapTripLostPlacesHead,
                    roadmap_list_remove(&waypointp->Q));
            /* last waypoint */
            if ( ROADMAP_LIST_EMPTY(&RoadMapCurrentRoute->waypoint_list)) {
                route_del (RoadMapCurrentRoute);
                RoadMapCurrentRoute = NULL;
                roadmap_trip_unset_route_focii ();
                roadmap_trip_set_modified(1);
                RoadMapTripRefresh = 1;
                roadmap_screen_refresh ();
                return;
            }

        }
        break;

    default:
        return;
    }

    if (which == ROUTE_WAYPOINTS) {
        roadmap_trip_set_route_focii ();
    }

    if (RoadMapRouteInProgress && (RoadMapTripNext == waypointp)) {
        roadmap_trip_route_resume ();
    }

    RoadMapTripRefresh = 1;

    roadmap_trip_set_modified(1);

    roadmap_screen_refresh ();

}

static void roadmap_trip_waypoint_manage_dialog_action
        (const char *name, void *which, int action) {

    waypoint *w = (waypoint *)roadmap_dialog_get_data ("Names", ".Waypoints");
    if (w == NULL) {
        return;
    }


    if (*(int *)which == WAYPOINTS_MODIFIED) {
        
        roadmap_messagebox ("Note", "Trip modified -- repopulating list");
        roadmap_trip_waypoint_manage_dialog_populate (which);
        return;
    }


    if (action == WAYPOINT_ACTION_EDIT) {

        /* It might be nicer to repopulate after the rename, but I'm
         * not sure how to do that.  This works.
         */
        roadmap_dialog_hide (name);

        /* Edits are different, in that they invoke a new dialog
         * to do their work.
         */
        roadmap_trip_dialog_waypoint_edit
            ( w, which, roadmap_preferences_use_keyboard ());

    } else {

        roadmap_trip_waypoint_manage_action (w, which, action);

        if (roadmap_trip_waypoint_manage_dialog_populate (which) == 0) {
            roadmap_dialog_hide (name);
        }

    }


}

static void roadmap_trip_waypoint_manage_dialog_delete
        (const char *name, void *which) {
    
    roadmap_trip_waypoint_manage_dialog_action
        (name, which, WAYPOINT_ACTION_DELETE);
        
}

static void roadmap_trip_waypoint_manage_dialog_up
        (const char *name, void *which) {

    roadmap_trip_waypoint_manage_dialog_action
        (name, which, WAYPOINT_ACTION_MOVE_BACK);
        
}

static void roadmap_trip_waypoint_manage_dialog_down
        (const char *name, void *which) {

    roadmap_trip_waypoint_manage_dialog_action
        (name, which, WAYPOINT_ACTION_MOVE_AHEAD);
        
}

static void roadmap_trip_waypoint_manage_dialog_edit
        (const char *name, void *which) {

    roadmap_trip_waypoint_manage_dialog_action
        (name, which, WAYPOINT_ACTION_EDIT);

}

static void roadmap_trip_waypoint_manage_dialog_selected (
        const char *name, void *data) {

    waypoint *waypointp;
    void *which;

    which = roadmap_dialog_get_data ("Names", ".which");


    if (*(int *)which == WAYPOINTS_MODIFIED) {

        roadmap_messagebox ("Note", "Trip modified -- repopulating list");

        if (roadmap_trip_waypoint_manage_dialog_populate (which) == 0) {
            roadmap_dialog_hide(name);
            return;
        }

    } else {
    
        waypointp = (waypoint *) roadmap_dialog_get_data (
                "Names", ".Waypoints");

        if (waypointp != NULL) {
            roadmap_trip_set_selected_place(which, waypointp);
            roadmap_trip_set_focus_waypoint (waypointp);
            roadmap_screen_refresh ();
        }
    }
}

/**
 * @brief populate a list (created before) with items
 * @param which indicates which list (of waypoints) to use to populate
 * @return success indicator (1 if ok, 0 if not)
 */
static int roadmap_trip_waypoint_manage_dialog_populate (void *which) {

    char **names = NULL;
    waypoint **waypoints;
    RoadMapList *list = NULL;  /* warning suppression */
    int count;
    int i;
    RoadMapListItem *elem, *tmp;
    waypoint *waypointp;

    if (which == PERSONAL_WAYPOINTS) {
        list = roadmap_landmark_list();
    } else if (which == TRIP_WAYPOINTS) {
        list = &RoadMapTripWaypointHead;
    } else if (which == ROUTE_WAYPOINTS) {
        if (RoadMapCurrentRoute == NULL) {
            return 0;
        }
        list = &RoadMapCurrentRoute->waypoint_list;
    } else if (which == LOST_WAYPOINTS) {
        list = &RoadMapTripLostPlacesHead;
    }

    count = roadmap_list_count(list);
    if (count <= 0)
        return 0;

    names = calloc (count, sizeof (*names));
    roadmap_check_allocated (names);
    waypoints = calloc (count, sizeof (*waypoints));
    roadmap_check_allocated (waypoints);


    i = 0;
    ROADMAP_LIST_FOR_EACH (list, elem, tmp) {
        waypointp = (waypoint *) elem;
        names[i] = waypointp->shortname;
        waypoints[i++] = waypointp;
    }


    *(int *)which = ! WAYPOINTS_MODIFIED;

    roadmap_dialog_show_list
        ("Names", ".Waypoints", count, names, (void **) waypoints,
            roadmap_trip_waypoint_manage_dialog_selected);
    free (names);
    free (waypoints);

    roadmap_dialog_set_data("Names", ".which", which);

    return 1;
}

/**
 * @brief function called from roadmap_trip_waypoint_select_navigation_waypoint
 *   and roadmap_trip_waypoint_manage_dialog_worker
 *   to store the Departure waypoint
 * @param name
 * @param data
 */
static void roadmap_trip_set_nav_departure (const char *name, void *data)
{
    roadmap_log (ROADMAP_WARNING, "roadmap_trip_departure_waypoint");
    roadmap_trip_set_point ("Departure",
	    &RoadMapTripSelectedPlace->wpt->pos);
    roadmap_dialog_hide (name);
    roadmap_screen_refresh ();
}

/**
 * @brief function called from roadmap_trip_waypoint_select_navigation_waypoint
 *   and roadmap_trip_waypoint_manage_dialog_worker
 *   to store the Destination waypoint
 * @param name
 * @param data
 */
static void roadmap_trip_set_nav_destination (const char *name, void *data)
{
    roadmap_log (ROADMAP_DEBUG, "roadmap_trip_destination_waypoint");
    roadmap_trip_set_point ("Destination",
	    &RoadMapTripSelectedPlace->wpt->pos);
    roadmap_dialog_hide (name);
    roadmap_screen_refresh ();
}

/**
 * @brief function to manage the Trip waypoint dialog
 * @param which indicates which list of waypoints to display
 */
static void roadmap_trip_waypoint_manage_dialog_worker (void *which) {

    int empty = 1;      /* warning suppression */
    const char *name = NULL; /* ditto */

    if (which == TRIP_WAYPOINTS) {
        empty = ROADMAP_LIST_EMPTY(&RoadMapTripWaypointHead);
        name = "Trip Landmarks";
    } else if (which == PERSONAL_WAYPOINTS) {
        empty = ROADMAP_LIST_EMPTY(roadmap_landmark_list());
        name = "Personal Landmarks";
    } else if (which == ROUTE_WAYPOINTS) {
        if (RoadMapCurrentRoute == NULL) {
            return;     /* Nothing to edit. */
        }
        empty = ROADMAP_LIST_EMPTY(&RoadMapCurrentRoute->waypoint_list);
        name = "Route Points";
    } else if (which == LOST_WAYPOINTS) {
        empty = ROADMAP_LIST_EMPTY(roadmap_landmark_list());
        name = "Deleted Places";
    }

    if (empty) {
        return;         /* Nothing to edit. */
    }

    if (roadmap_dialog_activate ( name, which)) {

        roadmap_dialog_new_list ("Names", ".Waypoints");
        if (which != LOST_WAYPOINTS) {
            roadmap_dialog_add_button
                ("Delete", roadmap_trip_waypoint_manage_dialog_delete);
        }
        if (which == ROUTE_WAYPOINTS) {
            roadmap_dialog_add_button
                ("Back", roadmap_trip_waypoint_manage_dialog_up);
            roadmap_dialog_add_button
                ("Ahead", roadmap_trip_waypoint_manage_dialog_down);
        }
        roadmap_dialog_add_button
            ("Edit", roadmap_trip_waypoint_manage_dialog_edit);
        roadmap_dialog_add_button
            ("Okay", roadmap_trip_dialog_cancel);
	roadmap_dialog_add_button
	    ("Destination", roadmap_trip_set_nav_destination);
	roadmap_dialog_add_button
	    ("Departure", roadmap_trip_set_nav_departure);

        roadmap_dialog_new_hidden ("Names", ".which");

        roadmap_dialog_complete (0);    /* No need for a keyboard. */
    }

    roadmap_trip_waypoint_manage_dialog_populate (which);
}

void roadmap_trip_personal_waypoint_manage_dialog (void) {
    roadmap_trip_waypoint_manage_dialog_worker (PERSONAL_WAYPOINTS);
}

void roadmap_trip_trip_waypoint_manage_dialog (void) {
    roadmap_trip_waypoint_manage_dialog_worker (TRIP_WAYPOINTS);
}

void roadmap_trip_route_waypoint_manage_dialog (void) {
    roadmap_trip_waypoint_manage_dialog_worker (ROUTE_WAYPOINTS);
}

void roadmap_trip_lost_waypoint_manage_dialog (void) {
    roadmap_trip_waypoint_manage_dialog_worker (LOST_WAYPOINTS);
}

/* Route Manage dialog */

static void roadmap_trip_route_manage_dialog_populate (int count);
static void roadmap_trip_lost_route_manage_dialog_populate (int count);

/**
 * @brief
 * @param name
 * @param data
 */
static void roadmap_trip_route_manage_dialog_none (const char *name, void *data)
{
    RoadMapRouteInProgress = 0;
    RoadMapCurrentRoute = NULL;
    roadmap_trip_unset_route_focii ();
    RoadMapTripRefresh = 1;
    roadmap_dialog_hide (name);
    roadmap_trip_refresh ();
}

/**
 * @brief
 * @param name
 * @param data
 */
static void roadmap_trip_route_manage_dialog_delete (const char *name, void *data)
{
    int count = 0;
    route_head *route = (route_head *) roadmap_dialog_get_data ("Names", ".Routes");

    if (route != NULL) { 
        roadmap_list_remove( &route->Q );
        roadmap_list_insert (&RoadMapTripLostRoutesHead, &route->Q);

        roadmap_trip_set_modified(1);

        if (route == RoadMapCurrentRoute) { 
            RoadMapRouteInProgress = 0;
            RoadMapCurrentRoute = NULL;
            roadmap_trip_unset_route_focii ();
            RoadMapTripRefresh = 1;
        }

        count = roadmap_list_count (&RoadMapTripRouteHead) + 
                roadmap_list_count (&RoadMapTripTrackHead);
        if (count > 0) {
            roadmap_trip_route_manage_dialog_populate (count);
        } else {
            roadmap_dialog_hide (name);
        }
        roadmap_screen_refresh ();
    }
}

static void roadmap_trip_route_manage_dialog_restore
        (const char *name, void *data) {

    int count = 0;
    route_head *route =
        (route_head *) roadmap_dialog_get_data ("Names", ".Routes");

    if (route != NULL) {

        roadmap_list_remove( &route->Q );
        if (route->rte_is_track) {
            roadmap_list_insert (&RoadMapTripTrackHead, &route->Q);
        } else {
            roadmap_list_insert (&RoadMapTripRouteHead, &route->Q);
        }

        roadmap_trip_set_modified(1);

        count = roadmap_list_count (&RoadMapTripLostRoutesHead);
        if (count > 0) {
            roadmap_trip_lost_route_manage_dialog_populate (count);
        } else {
            roadmap_dialog_hide (name);
        }
        roadmap_screen_refresh ();
    }
}

static void roadmap_trip_route_manage_dialog_edit
        ( const char *name, void *data) {

    route_head *route = (route_head *) roadmap_dialog_get_data
        ( "Names", ".Routes");

    if (route == NULL) {
        return;
    }

    roadmap_trip_dialog_route_edit
        ( route, roadmap_preferences_use_keyboard ());

    /* It might be nicer to repopulate after the rename, but I'm
     * not sure how to do that.  This works. */
    roadmap_dialog_hide (name);
}

static void roadmap_trip_route_manage_dialog_selected
        (const char *name, void *data) {

    route_head *route =
        (route_head *) roadmap_dialog_get_data ("Names", ".Routes");

    if (route != RoadMapCurrentRoute) {
        RoadMapCurrentRoute = route;
        RoadMapRouteIsReversed = 0;
        RoadMapRouteInProgress = 0;
        roadmap_trip_set_route_focii ();
        roadmap_trip_set_point_focus (RoadMapTripBeginning);
    } else if (route == NULL) {
        roadmap_trip_unset_route_focii ();
    }
    roadmap_screen_refresh ();
}


static void roadmap_trip_route_manage_dialog_populate (int count) {

    char **names = NULL;
    route_head **routes = NULL;
    RoadMapListItem *elem, *tmp;
    int i;

    names = calloc (count, sizeof (*names));
    roadmap_check_allocated (names);
    routes = calloc (count, sizeof (*routes));
    roadmap_check_allocated (routes);

    i = 0;
    ROADMAP_LIST_FOR_EACH (&RoadMapTripRouteHead, elem, tmp) {
        route_head *rh = (route_head *) elem;
        names[i] = (rh->rte_name && rh->rte_name[0]) ?
                rh->rte_name : "Unnamed Route";
        routes[i++] = rh;
    }

    ROADMAP_LIST_FOR_EACH (&RoadMapTripTrackHead, elem, tmp) {
        route_head *th = (route_head *) elem;
        names[i] = (th->rte_name && th->rte_name[0]) ?
                th->rte_name : "Unnamed Track";
        routes[i++] = th;
    }

    roadmap_dialog_show_list
        ("Names", ".Routes", count, names, (void **) routes,
         roadmap_trip_route_manage_dialog_selected);
    free (names);
    free (routes);
}

static void roadmap_trip_lost_route_manage_dialog_populate (int count) {

    char **names = NULL;
    route_head **routes = NULL;
    RoadMapListItem *elem, *tmp;
    int i;

    names = calloc (count, sizeof (*names));
    roadmap_check_allocated (names);
    routes = calloc (count, sizeof (*routes));
    roadmap_check_allocated (routes);

    i = 0;
    ROADMAP_LIST_FOR_EACH (&RoadMapTripLostRoutesHead, elem, tmp) {
        route_head *rh = (route_head *) elem;
        names[i] = (rh->rte_name && rh->rte_name[0]) ?
                rh->rte_name : "Unnamed Route";
        routes[i++] = rh;
    }

    roadmap_dialog_show_list
        ("Names", ".Routes", count, names, (void **) routes,
         roadmap_trip_route_manage_dialog_selected);
    free (names);
    free (routes);
}

/**
 * @brief
 */
void roadmap_trip_route_manage_dialog (void)
{ 
    int count;

    if (ROADMAP_LIST_EMPTY(&RoadMapTripRouteHead) && ROADMAP_LIST_EMPTY(&RoadMapTripTrackHead)) {
        return;                 /* Nothing to manage. */
    }

    if (roadmap_dialog_activate ("Select Routes", NULL)) {
        roadmap_dialog_new_list ("Names", ".Routes");
        roadmap_dialog_add_button ("Delete", roadmap_trip_route_manage_dialog_delete);
        roadmap_dialog_add_button ("Edit", roadmap_trip_route_manage_dialog_edit);
        roadmap_dialog_add_button ("None", roadmap_trip_route_manage_dialog_none);
        roadmap_dialog_add_button ("Okay", roadmap_trip_dialog_cancel);

        roadmap_dialog_complete (0);    /* No need for a keyboard. */
    }

    count = roadmap_list_count (&RoadMapTripRouteHead) + 
            roadmap_list_count (&RoadMapTripTrackHead);

    roadmap_trip_route_manage_dialog_populate (count);
}

void roadmap_trip_lost_route_manage_dialog (void) {

    int count;

    if (ROADMAP_LIST_EMPTY(&RoadMapTripLostRoutesHead)) {
        return;                 /* Nothing to manage. */
    }

    if (roadmap_dialog_activate ("Deleted Routes", NULL)) {

        roadmap_dialog_new_list ("Names", ".Routes");

        roadmap_dialog_add_button ("Restore",
                                   roadmap_trip_route_manage_dialog_restore);

        roadmap_dialog_add_button ("Okay", roadmap_trip_dialog_cancel);

        roadmap_dialog_complete (0);    /* No need for a keyboard. */
    }

    count = roadmap_list_count (&RoadMapTripRouteHead) + 
            roadmap_list_count (&RoadMapTripTrackHead);

    roadmap_trip_lost_route_manage_dialog_populate (count);
}


/**
 * @brief activate trip functionality now
 */
static void roadmap_trip_activate (void)
{
	if (RoadMapTripGps->has_value) {
		roadmap_trip_set_point_focus (RoadMapTripGps);
	} else {
		roadmap_trip_set_focus_waypoint (RoadMapTripNext);
	}
	RoadMapRouteInProgress = 1;
	roadmap_trip_refresh ();
}

/**
 * @brief
 * @param name
 * @param position
 * @param sprite
 * @return
 */
#if USE_ICON_NAME
waypoint * roadmap_trip_new_waypoint
	(const char *name, RoadMapPosition * position, const char *sprite)
#else
waypoint * roadmap_trip_new_waypoint (const char *name, RoadMapPosition * position)
#endif
{
    waypoint *w;
    w = waypt_new ();
    if (name) w->shortname = xstrdup (name);
#if USE_ICON_NAME
    if (sprite) w->icon_descr = xstrdup(sprite);
#endif
    w->pos = *position;
    w->creation_time = time(NULL);
    return w;
}

/**
 * @brief set the named focalpoint to the given position
 * @param id the name of the focalpoint to set
 * @param position the position
 */
void roadmap_trip_set_point (const char *id, RoadMapPosition * position)
{
    RoadMapTripFocal *focal;

    focal = roadmap_trip_find_focalpoint (id);
    if (focal == NULL)
	    return;

    focal->map = *position;

    if (focal->save) {
        roadmap_config_set_position (&focal->config_position, position);
    }

    focal->has_value = 1;
    RoadMapTripLastSetPoint = focal;

    if (focal->sprite != NULL)
	    RoadMapTripRefresh = 1;
}

/**
 * @brief
 * @param waypointp
 */
void roadmap_trip_new_route_waypoint(waypoint *waypointp)
{ 
    route_head *new_route;

    new_route = route_head_alloc();
    route_add(&RoadMapTripRouteHead, new_route);

    route_add_wpt_tail (new_route, waypointp);
    RoadMapTripRefresh = 1;
    roadmap_trip_set_modified(1);

    RoadMapCurrentRoute = new_route;
    RoadMapRouteIsReversed = 0;

    roadmap_trip_set_route_focii ();

    if (RoadMapRouteInProgress)
	    roadmap_trip_route_resume ();

    roadmap_trip_set_point_focus (RoadMapTripBeginning);
    roadmap_trip_refresh ();
}

/**
 * @brief Create a unique name for the last "GPS" waypoint
 * @return a string with a unique name
 */
char * roadmap_trip_last_gps_waypoint_name(void)
{
        static char gpsname[40];

        if (RoadMapTripGPSTime != 0) {
            strftime(gpsname, sizeof(gpsname),
                "GPS-%Y/%m/%d-%H:%M:%S", localtime(&RoadMapTripGPSTime));
        } else {
            sprintf(gpsname, "GPS-Waypoint");
        }
        return gpsname;
}

/**
 * @brief RoadMapTripLastSetPoint keeps track of the last focus or selection made by the user,
 * for use in starting routes or adding waypoints.
 * Here we try to associate it with the name it originated with.
 *
 * @return
 */
static const char *roadmap_trip_last_setpoint_name(void)
{
    if (RoadMapTripLastSetPoint == NULL || !RoadMapTripLastSetPoint->has_value) {
        return NULL;
    }

    if (RoadMapTripLastSetPoint == RoadMapTripSelection ||
        RoadMapTripLastSetPoint == RoadMapTripAddress) {
        return roadmap_display_get_id ("Selected Street");

    } else if (RoadMapTripLastSetPoint == RoadMapTripDestination) {
        return RoadMapTripDest->shortname;

    } else if (RoadMapTripLastSetPoint == RoadMapTripBeginning) {
        return RoadMapTripStart->shortname;

    } else if (RoadMapTripLastSetPoint == RoadMapTripWaypointFocus) {
        return RoadMapTripWaypointFocusName;

    } else if (RoadMapTripLastSetPoint == RoadMapTripGps) {
        return roadmap_trip_last_gps_waypoint_name();

    }

    return NULL;
}

static waypoint *roadmap_trip_last_setpoint_waypoint(void)
{
    const char *name;

    if (RoadMapTripLastSetPoint == NULL ||
            !RoadMapTripLastSetPoint->has_value) {
        return NULL;
    }

    name = roadmap_trip_last_setpoint_name();

    return roadmap_trip_new_waypoint
        (name, &RoadMapTripLastSetPoint->map);

}


void roadmap_trip_new_route (void) {

    waypoint *dest_wpt;

    dest_wpt = roadmap_trip_last_setpoint_waypoint();
    if (dest_wpt == NULL) return;

    roadmap_trip_new_route_waypoint(dest_wpt);
    roadmap_screen_refresh();

}

void roadmap_trip_set_as_destination(void)
{
    waypoint *dest_wpt;

    dest_wpt = roadmap_trip_last_setpoint_waypoint();
    if (dest_wpt == NULL) return;

    waypt_flush_queue (&RoadMapTripQuickRoute.waypoint_list);
    route_add_wpt_tail (&RoadMapTripQuickRoute, dest_wpt);

    RoadMapCurrentRoute = &RoadMapTripQuickRoute;
    RoadMapRouteIsReversed = 0;
    RoadMapRouteInProgress = 1;

    roadmap_trip_set_route_focii ();

    /* start it right away, regardless */
    roadmap_trip_route_resume();

    roadmap_screen_refresh ();
}

static void roadmap_trip_insert_routepoint_worker(int where)
{
    const char *name;

    if (RoadMapTripLastSetPoint == NULL ||
            !RoadMapTripLastSetPoint->has_value) {
        return;
    }

    name = roadmap_trip_last_setpoint_name();

    roadmap_trip_add_waypoint (name, &RoadMapTripLastSetPoint->map, where);
}

void roadmap_trip_insert_routepoint_best(void) {

    roadmap_trip_insert_routepoint_worker(PLACE_ROUTE_MARK_INSERT);
}

void roadmap_trip_insert_routepoint_dest(void) {

    roadmap_trip_insert_routepoint_worker(PLACE_ROUTE_MARK_DEST);
}

void roadmap_trip_insert_routepoint_start(void) {

    roadmap_trip_insert_routepoint_worker(PLACE_ROUTE_MARK_START);
}

void roadmap_trip_insert_trip_point(void) {

    roadmap_trip_insert_routepoint_worker(PLACE_TRIP_MARK);
}

void roadmap_trip_insert_personal_point(void) {

    roadmap_trip_insert_routepoint_worker(PLACE_PERSONAL_MARK);
}


static waypoint * roadmap_trip_choose_best_next (const RoadMapPosition *pos);

/**
 * @brief
 * @param name
 * @param position
 * @param where
 */
void roadmap_trip_add_waypoint ( const char *name, RoadMapPosition * position, int where)
{
    waypoint *waypointp, *next = NULL;
    int putafter;

    if (where >= NUM_PLACEMENTS && RoadMapCurrentRoute == NULL) {
        return;
    }

    waypointp = roadmap_trip_new_waypoint (name, position);
    switch (where) {
    case PLACE_ROUTE_MARK_START:
        /* becomes the new starting point. */
        route_add_wpt
            (RoadMapCurrentRoute,
             RoadMapTripStart,
            waypointp, RoadMapRouteIsReversed);

        break;

    case PLACE_ROUTE_MARK_INSERT:
        /* Insert into the middle of the route. */

        next = roadmap_trip_choose_best_next (&waypointp->pos);

        if ( RoadMapRouteIsReversed ) {
            putafter = 1;
            if (next == RoadMapTripStart) {
                putafter = 0;
            }
        } else {
            putafter = 0;
            if (next == RoadMapTripStart) {
                putafter = 1;
            }
        }

        /* add before the best next if we're not reversed. */
        route_add_wpt
            (RoadMapCurrentRoute, next, waypointp, putafter);

        break;

    default:
    case PLACE_ROUTE_MARK_DEST:
        /* becomes the new destination. */
        route_add_wpt
            (RoadMapCurrentRoute,
            RoadMapTripDest,
            waypointp, ! RoadMapRouteIsReversed);

        break;

    case PLACE_TRIP_MARK:
        waypt_add(&RoadMapTripWaypointHead, waypointp);
        break;

    case PLACE_PERSONAL_MARK:
        roadmap_landmark_add(waypointp);
        break;

    case PLACE_NEW_ROUTE:
        roadmap_trip_new_route_waypoint(waypointp);
        return;  /* note -- return, not break */
    }

    if (where >= NUM_PLACEMENTS) { 
        roadmap_trip_set_route_focii ();
    }

    if (!roadmap_math_point_is_visible (&waypointp->pos)) {
        /* Only set focus if the point we just added is off-screen.  This
         * will take us there.
         */
        roadmap_trip_set_focus_waypoint (waypointp);
    }

    RoadMapTripRefresh = 1;

    if (where != PLACE_PERSONAL_MARK) {
        roadmap_trip_set_modified(1);
    }

    roadmap_screen_refresh ();
}

void roadmap_trip_create_selection_waypoint (void) {

    const char *name;

    if (RoadMapTripLastSetPoint == NULL ||
            !RoadMapTripLastSetPoint->has_value) {
        return;
    }

    name = roadmap_trip_last_setpoint_name();

    roadmap_trip_add_waypoint_dialog (name, &RoadMapTripLastSetPoint->map);
}

void roadmap_trip_create_gps_waypoint(void) {

    if (!RoadMapTripGps->has_value) {
        return;
    }

    roadmap_trip_add_waypoint_dialog 
        (roadmap_trip_last_gps_waypoint_name(), &RoadMapTripGps->map);

}

/**
 * @brief
 * @param gps_time
 * @param gps_position
 */
void roadmap_trip_set_gps (int gps_time, const RoadMapGpsPosition *gps_position)
{
    RoadMapPosition position;
    RoadMapGuiPoint guipoint1;
    RoadMapGuiPoint guipoint2;
    static int lastSteering = 9999;

    roadmap_adjust_position (gps_position, &position);

    /* An existing point: refresh is needed only if the point
     * moved in a visible fashion.
     */


    if (RoadMapTripGps->map.latitude != position.latitude ||
        RoadMapTripGps->map.longitude != position.longitude) {

        roadmap_trip_coordinate (&position, &guipoint1);
        roadmap_trip_coordinate (&RoadMapTripGps->map, &guipoint2);

        if (guipoint1.x != guipoint2.x ||
            guipoint1.y != guipoint2.y ||
            gps_position->steering != lastSteering) {
            /* we only consider steering when we've moved enough
             * to matter.  this isn't quite accurate, but
             * steering changes cause far too many refreshes
             * otherwise.  if we're focused on the GPS, we track
             * closely -- this is when we're _not_ focused on the
             * GPS.
             */
            RoadMapTripRefresh = 1;
        }

        if (RoadMapTripGps == RoadMapTripFocus) {
            RoadMapTripFocusMoved = 1;
        }

    }

    /* reset the last visible time based on our previous
     * position, not the new.  this prevents us releasing because
     * we lost GPS contact for a while -- in that case we would
     * suddenly see a new invisible position with a long time
     * delay, when normally we would have been tracking it all along.
     */
    if (RoadMapTripGps == RoadMapTripFocus) {

        static time_t RoadMapTripGPSLastVisible;

        if (roadmap_math_point_is_visible (&RoadMapTripGps->map)) {
            RoadMapTripGPSLastVisible = time(NULL);
        } else {
            int release_delay;

            release_delay = roadmap_config_get_integer
                        (&RoadMapConfigTripGPSFocusReleaseDelay);

            if (release_delay != 0 &&
                    time(NULL) - RoadMapTripGPSLastVisible > release_delay) {
                roadmap_screen_hold();
            }
        }
    }

    RoadMapTripGps->gps = *gps_position;
    RoadMapTripGps->map = position;
    lastSteering = gps_position->steering;

    roadmap_config_set_position (&RoadMapTripGps->config_position, &position);
    if (RoadMapRouteInProgress && !RoadMapTripGps->has_value) {
        RoadMapTripGps->has_value = 1;
        roadmap_trip_route_resume();
    }
    RoadMapTripGPSTime = gps_time;

    RoadMapTripGps->has_value = 1;	/* ?? */
}


void roadmap_trip_copy_focus (const char *id) {

    RoadMapTripFocal *to = roadmap_trip_find_focalpoint (id);

    if (to != NULL && RoadMapTripFocus != NULL && to != RoadMapTripFocus) {

        to->has_value = RoadMapTripFocus->has_value;

        to->map = RoadMapTripFocus->map;
        to->gps = RoadMapTripFocus->gps;

        roadmap_config_set_position (&to->config_position, &to->map);
        roadmap_config_set_integer (&to->config_direction, to->gps.steering);
    }
}



int roadmap_trip_is_refresh_needed (void) {

    if (RoadMapTripRefresh || RoadMapTripFocusChanged ||
                RoadMapTripFocusMoved) {

        RoadMapTripFocusChanged = 0;
        RoadMapTripFocusMoved = 0;
        RoadMapTripRefresh = 0;

        return 1;
    }
    return 0;
}


int roadmap_trip_get_orientation (void) {

    if (RoadMapTripFocus != NULL) {
        return 360 - RoadMapTripFocus->gps.steering;
    }

    return 0;
}

/**
 * @brief
 * @return
 */
const char * roadmap_trip_get_focus_name (void)
{
	if (RoadMapTripFocus != NULL) {
		return RoadMapTripFocus->id;
	}

	return NULL;
}


const RoadMapPosition * roadmap_trip_get_focus_position (void) {

    if (RoadMapTripFocus != NULL) {
        return &RoadMapTripFocus->map;
    }

    return &RoadMapTripDefaultPosition;
}

void roadmap_trip_set_focus_position (RoadMapPosition *pos ) {

    if (RoadMapTripFocus != NULL) {
        RoadMapTripFocus->map = *pos;
        roadmap_config_set_position
            (&RoadMapTripFocus->config_position, pos);
        RoadMapTripFocusMoved = 1;
    }
}

/**
 * @brief choose the next waypoint to go to
 * @param pos the current position
 * @return the next waypoint
 */
static waypoint * roadmap_trip_choose_best_next (const RoadMapPosition *pos)
{
    RoadMapListItem *elem, *tmp;
    waypoint *nextpoint;
    int dist, distmin;
    int which;

    if (RoadMapCurrentRoute == NULL) {
        return NULL;
    }

    distmin = 500000000;
    nextpoint = (waypoint *)ROADMAP_LIST_FIRST(&RoadMapCurrentRoute->waypoint_list);

    /* Find the closest segment to our current position. */
    ROADMAP_LIST_FOR_EACH (&RoadMapCurrentRoute->waypoint_list, elem, tmp) {

        waypoint *waypointp = (waypoint *) elem;
        if (waypointp ==
            (waypoint *)ROADMAP_LIST_LAST( &RoadMapCurrentRoute->waypoint_list))
            break;

        dist = roadmap_math_get_distance_from_segment
            (pos, &waypointp->pos,
            &((waypoint *)ROADMAP_LIST_NEXT(&waypointp->Q))->pos,
            NULL, &which);

        if (dist < distmin) {
            distmin = dist;
            if (which == 1) {
                /* waypointp is closer */
                nextpoint = waypointp;

            } else if (which == 2) {
                /* waypointp->next is closer */
                nextpoint = (waypoint *)ROADMAP_LIST_NEXT(&waypointp->Q);

            } else {
                /* segment itself is closest */
                if (RoadMapRouteIsReversed) {
                    nextpoint = waypointp;
                } else {
                    nextpoint = (waypoint *)ROADMAP_LIST_NEXT(&waypointp->Q);
                }
            }
        }

    }

    return nextpoint;
}

/**
 * @brief create the directions
 * @param dist_to_next
 * @param suppress_dist
 * @param next
 */
static void roadmap_trip_set_directions (int dist_to_next, int suppress_dist, waypoint *next)
{
    static waypoint *last_next;
    static char *desc;
    static int rest_of_distance;

    /* Info from another call if !suppress_dist */
    if (suppress_dist)
	roadmap_log(ROADMAP_WARNING, "roadmap_trip_set_directions(%d, %d)",
			dist_to_next, suppress_dist);

    if (next == NULL) {
        roadmap_message_unset ('Y'); /* distance to the next waypoint which includes directions */
        roadmap_message_unset ('X'); /* directions at next waypoint */
        last_next = NULL;
    }

    /* Try to avoid some repetitive distance calculations. */
    if (last_next != next) {
        last_next = next;

        rest_of_distance = 0;
        while (next->description == NULL && next != RoadMapTripDest) {
            waypoint *tmp = roadmap_trip_next(next);
            rest_of_distance += roadmap_math_distance (&next->pos, &tmp->pos);
            next = tmp;
        }

        desc = next->description;
        if (desc == NULL) {
            // desc = "Arrive at destination";
            desc = "Arrive";
        }

        roadmap_message_set ('X', "%s", desc); /* directions at next waypoint */
    }

    if (suppress_dist) {
        roadmap_message_unset ('Y'); /* distance to the next waypoint which includes directions */
    } else {
	/* distance to the next waypoint which includes directions */
        roadmap_math_trip_set_distance('Y', dist_to_next + rest_of_distance);
    }

}

/**
 * @brief
 * @param force
 */
static void roadmap_trip_set_departure (int force)
{
	if (force || !RoadMapTripDeparture->has_value) {
		RoadMapTripDeparture->map = RoadMapTripGps->map;
		RoadMapTripDeparture->has_value = 1;
	}
}

/**
 * @brief
 */
static void roadmap_trip_unset_departure (void)
{
	RoadMapTripDeparture->has_value = 0;
}

/**
 * @brief
 */
void roadmap_trip_route_start (void)
{
    if (RoadMapCurrentRoute == NULL) {
        return;
    }

    RoadMapTripNext = RoadMapTripStart;
    roadmap_trip_set_departure (1);
    roadmap_trip_activate ();
    roadmap_trip_set_directions(0, 0, NULL);
}

/**
 * @brief resume a route
 */
void roadmap_trip_route_resume (void)
{
    if (RoadMapCurrentRoute == NULL) {
        /* convenient side effect.  if there _is_ a route, the code
         * below forces GPS to be centered.  so may as well do that
         * when there isn't a route too.  saves key bindings.
         */
        if (RoadMapTripGps->has_value) {
            roadmap_trip_set_point_focus (RoadMapTripGps);
            roadmap_screen_refresh ();
        }
        return;
    }

    if (RoadMapTripGps->has_value) {
        RoadMapTripNext = roadmap_trip_choose_best_next (&RoadMapTripGps->map);
        roadmap_trip_set_departure (0);
        roadmap_trip_activate ();
        roadmap_trip_set_directions(0, 0, NULL);
    }
}


void roadmap_trip_route_reverse (void) {

    if (RoadMapCurrentRoute == NULL) {
        return;
    }

    RoadMapRouteIsReversed = ! RoadMapRouteIsReversed;

    roadmap_trip_set_route_focii ();

    RoadMapTripRefresh = 1;

    if (RoadMapRouteInProgress) roadmap_trip_route_resume ();

    roadmap_trip_refresh ();
}

void roadmap_trip_route_return (void) {
    // FIXME -- "return" trips and "reverse" trips are really
    // different.  a true "return" trip would be re-navigated, or
    // maybe would follow the user's track, backwards.

    roadmap_trip_route_reverse ();
}

/**
 * @brief stop a route
 */
void roadmap_trip_route_stop (void)
{
    if (RoadMapCurrentRoute == NULL) {
        return;
    }

    RoadMapRouteInProgress = 0;
    roadmap_trip_unset_departure ();

    RoadMapTripRefresh = 1;
    roadmap_trip_refresh ();
}

/**
 * @brief this displays an arrow pointing to the next point in the trip
 * @return
 */
static int roadmap_trip_next_point_state(void)
{
    int angle;

    if (!RoadMapRouteInProgress || RoadMapCurrentRoute == NULL || !RoadMapTripGps->has_value) {
        return -1;
    }

    if (RoadMapTripNext == RoadMapTripDest) {
        return -1;
    }

    angle = roadmap_math_azymuth (&RoadMapTripGps->map, &RoadMapTripNext->pos);
    angle -= RoadMapTripGps->gps.steering + roadmap_math_get_orientation();

    return ROADMAP_STATE_ENCODE_STATE (angle, 0);
}

/**
 * @brief this displays an arrow pointing to the 2nd next point in the trip
 * @return
 */
static int roadmap_trip_2nd_point_state(void)
{
    waypoint *tmp;
    int angle;

    if (! RoadMapRouteInProgress || RoadMapCurrentRoute == NULL || !RoadMapTripGps->has_value) {
        return -1;
    }

    if (RoadMapTripNext == RoadMapTripDest) {
        return -1;
    }

    tmp = roadmap_trip_next(RoadMapTripNext);
    if (tmp == RoadMapTripDest) {
        return -1;
    }

    angle = roadmap_math_azymuth (&RoadMapTripGps->map, &tmp->pos);
    angle -= RoadMapTripGps->gps.steering + roadmap_math_get_orientation();

    return ROADMAP_STATE_ENCODE_STATE (angle, 0);
}

/**
 * @brief this displays an arrow pointing to the trip destination
 * @return
 */
static int roadmap_trip_dest_state(void)
{
    int angle;

    if (! RoadMapRouteInProgress || RoadMapCurrentRoute == NULL || !RoadMapTripGps->has_value) {
        return -1;
    }

    angle = roadmap_math_azymuth (&RoadMapTripGps->map, &RoadMapTripDest->pos);
    angle -= RoadMapTripGps->gps.steering + roadmap_math_get_orientation();

    return ROADMAP_STATE_ENCODE_STATE (angle, 0);
}

void roadmap_trip_show_nextpoint(void) {
    if (!RoadMapRouteInProgress ||
        RoadMapCurrentRoute == NULL ||
        RoadMapTripNext == NULL) {
        return;
    }

    roadmap_trip_set_focus_waypoint (RoadMapTripNext);
    roadmap_screen_refresh ();
}

void roadmap_trip_show_2ndnextpoint(void) {
    if (!RoadMapRouteInProgress ||
        RoadMapCurrentRoute == NULL ||
        RoadMapTripNext == NULL) {
        return;
    }

    roadmap_trip_set_focus_waypoint (roadmap_trip_next(RoadMapTripNext));
    roadmap_screen_refresh ();
}

/**
 * @brief send messages to the user indicating current state
 *
 * This table outlines which message means what
 *      A       estimated time of arrival (not yet implemented).
 *      B       Direction of travel (bearing).
 *      D       Distance to the destination (set only when a trip is active).
 *      S       Speed.
 *      W       Distance to the next waypoint (set only when a trip is active).
 *      X       Directions to be followed when the next waypoint (with directions) is reached.
 *              (set only when a trip is active).
 *      Y       Distance to the next waypoint which includes directions, unless the GPS is
 *              "at" that waypoint.  (set only when a trip is active).
 */
void roadmap_trip_format_messages (void)
{
    RoadMapTripFocal *gps = RoadMapTripGps;
    int distance_to_destination;
    int distance_to_next;
    int distance_to_directions;
    int waypoint_size;
    static RoadMapPosition lastgpsmap = {-1, -1};
    static waypoint *within_waypoint = NULL;

    if (! RoadMapRouteInProgress || RoadMapCurrentRoute == NULL) {

        RoadMapTripNext = NULL;

        roadmap_message_unset ('A');
        roadmap_message_unset ('D');
        roadmap_message_unset ('W');

        roadmap_message_unset ('X');
        roadmap_message_unset ('Y');
        lastgpsmap.latitude = -1;
        return;
    }

    if (!gps->has_value) {

        roadmap_message_unset ('A');
        roadmap_message_set ('D', "?? %s", roadmap_math_trip_unit());
        roadmap_message_set ('W', "?? %s", roadmap_math_distance_unit ());

        roadmap_message_set ('X', "??");
        roadmap_message_set ('Y', "?? %s", roadmap_math_trip_unit());
        lastgpsmap.latitude = -1;
        return;
    }

    if (lastgpsmap.latitude == -1)
	    lastgpsmap = gps->map;

    distance_to_destination = roadmap_math_distance (&gps->map, &RoadMapTripDest->pos);
    roadmap_math_trip_set_distance('D', distance_to_destination);

    roadmap_log (ROADMAP_DEBUG, "GPS: distance to destination = %d %s",
                 distance_to_destination, roadmap_math_distance_unit ());


    distance_to_next = distance_to_directions =
            roadmap_math_distance (&gps->map, &RoadMapTripNext->pos);

    waypoint_size = roadmap_config_get_integer (&RoadMapConfigWaypointSize);

    if (RoadMapTripNext == RoadMapTripDest) {
        roadmap_trip_set_directions
            (distance_to_next, (distance_to_next < waypoint_size),
                RoadMapTripNext);
        roadmap_message_unset ('W');

    } else {

        static int getting_close;
        int need_newgoal = 0;

        if (within_waypoint != NULL) {
            /* We really attained the waypoint, and may have now
             * left its minimal vicinity.  If so, update on-screen
             * directions.
             */
            int distance =
                roadmap_math_distance ( &gps->map, &within_waypoint->pos);
            if (distance > waypoint_size) {
                within_waypoint = NULL;
                getting_close = 0;
            } else {
                distance_to_directions = distance;
            }

        } else if (distance_to_next < waypoint_size) {

            roadmap_log (ROADMAP_DEBUG, "attained waypoint %s, distance %d",
                         RoadMapTripNext->shortname, distance_to_next);

            within_waypoint = RoadMapTripNext;
            need_newgoal = 1;
            getting_close = 0;

        } else if (distance_to_next < waypoint_size * 2) {

            /* We're within the 2x vicinity.  Do nothing yet. */
            getting_close = 1;

        } else if (getting_close) {

            /* If we got close (within 2x), but never quite there. */
            waypoint *goal = RoadMapTripNext;
            int distance = roadmap_math_distance (&gps->map, &goal->pos);

            if (distance > waypoint_size * 2) {
                /* Leaving 2x vicinity -- assume we're on our way to
                 * next, if we're closer to that path than the path
                 * we were just on.
                 */
                int dist1, dist2;
                if (goal == RoadMapTripStart) {
                        /* We've left the bigger circle, we're on
                         * our way. */
                        need_newgoal = 1;
                } else {
                    /* Distance to the previous segment. */
                    dist1 = roadmap_math_get_distance_from_segment
                        (&gps->map, &goal->pos,
                            &(roadmap_trip_prev(goal))->pos, NULL, NULL);
                    /* Distance to the next segment. */
                    dist2 = roadmap_math_get_distance_from_segment
                        (&gps->map, &goal->pos,
                            &(roadmap_trip_next(goal))->pos, NULL, NULL);
                    if (dist2 < dist1) {
                        /* We've left the bigger circle, and
                         * we're closer to the next segment than
                         * to the one we were on before, so we use it.  */
                        need_newgoal = 1;
                    }
                }
                getting_close = 0;
            }
        }

        /* We've decided we're ready for the next waypoint. */
        if (need_newgoal) {
            RoadMapTripNext = roadmap_trip_next(RoadMapTripNext);
        }

        if (within_waypoint != NULL) {
            roadmap_trip_set_directions (distance_to_directions,
                (within_waypoint->description != NULL), within_waypoint);
        } else {
            roadmap_trip_set_directions (distance_to_directions,
                0, RoadMapTripNext);
        }

        roadmap_math_trip_set_distance ('W', distance_to_next);

#if 1
	/* Hack */
	roadmap_message_set ('D', "?? %s", roadmap_math_trip_unit());
	roadmap_message_set ('X', "??");
	roadmap_message_set ('Y', "?? %s", roadmap_math_trip_unit());
#endif
    }

    lastgpsmap = gps->map;
}

/**
 * @brief
 * @param waypointp
 */
static void roadmap_trip_standalone_waypoint_draw(const waypoint *waypointp)
{
	roadmap_landmark_draw_waypoint (waypointp, "TripLandmark", 0, RoadMapLandmarksPen);
}

/**
 * @brief
 * @param waypointp
 * @param sprite
 * @param rotate
 */
static void roadmap_trip_sprite_draw (const waypoint *waypointp, const char *sprite, int rotate)
{
    RoadMapGuiPoint guipoint;
    int azymuth = 0;

    if (roadmap_math_point_is_visible (&waypointp->pos)) {
        roadmap_math_coordinate (&waypointp->pos, &guipoint);
        roadmap_math_rotate_coordinates (1, &guipoint);
        if (rotate) {
            azymuth = roadmap_math_azymuth
                (&waypointp->pos, &(roadmap_trip_next(waypointp))->pos);
        }
        roadmap_sprite_draw (sprite, &guipoint, azymuth);
    }
}

/**
 * @brief
 * @param waypointp
 */
static void roadmap_trip_route_waypoint_draw (const waypoint *waypointp)
{


    const char *sprite;

    int rotate = 1;

    if ( !RoadMapTripDrawingActiveRoute) {
        sprite = "Waypoint";
    } else if (waypointp == RoadMapTripDest) {
        sprite = "Destination";
        rotate = 0;
    } else if (waypointp == RoadMapTripStart) {
        sprite = "Start";
    } else if (waypointp->description != NULL) {
        sprite = "RoutePoint";
    } else {
        sprite = "SecondaryRoutePoint";
    }

    roadmap_trip_sprite_draw (waypointp, sprite, rotate);

}

/**
 * @brief
 * @param waypointp
 */
static void roadmap_trip_track_waypoint_draw (const waypoint *waypointp)
{
    const char *sprite;

    if ( !RoadMapTripDrawingActiveRoute) {
        sprite = "InactiveTrack";
    } else {
        sprite = "Track";
    }

    roadmap_trip_sprite_draw (waypointp, sprite, 1);
}

static int RoadMapTripFirstRoutePoint;

static void roadmap_trip_route_routelines_draw (const waypoint *waypointp)
{
    static RoadMapPosition lastpos;

    if ( !RoadMapTripFirstRoutePoint ) {
       RoadMapGuiPoint points[2];
       int count = 2;
        if (roadmap_math_get_visible_coordinates
               (&lastpos, &waypointp->pos, &points[0], &points[1])) {
            roadmap_math_rotate_coordinates (2, points);
            roadmap_canvas_draw_multiple_lines(1, &count, points, 1);
       }
    }

    RoadMapTripFirstRoutePoint = 0;
    lastpos = waypointp->pos;
}

/**
 * @brief
 */
void roadmap_trip_display (void)
{
    int azymuth;
    RoadMapGuiPoint guipoint;
    RoadMapTripFocal *focal;
    RoadMapTripFocal *gps = RoadMapTripGps;
    RoadMapListItem *elem, *tmp;

    /* Show all the inactive route waypoints first */
    if (RoadMapTripShowInactiveRoutes) {
        RoadMapTripDrawingActiveRoute = 0;
        ROADMAP_LIST_FOR_EACH (&RoadMapTripRouteHead, elem, tmp) {
            route_head *rh = (route_head *) elem;
            if (rh == RoadMapCurrentRoute) continue;
            route_waypt_iterator
                (rh, roadmap_trip_route_waypoint_draw);
        }
    }

    if (RoadMapTripShowInactiveTracks) {
        RoadMapTripDrawingActiveRoute = 0;
        ROADMAP_LIST_FOR_EACH (&RoadMapTripTrackHead, elem, tmp) {
            route_head *rh = (route_head *) elem;
            if (rh == RoadMapCurrentRoute) continue;
            route_waypt_iterator
                (rh, roadmap_trip_track_waypoint_draw);
        }
    }

    /* Show the standalone trip waypoints. */
    waypt_iterator (&RoadMapTripWaypointHead, roadmap_trip_standalone_waypoint_draw);

    if (RoadMapCurrentRoute != NULL) {
        /* Show the route outline? */
        if (roadmap_config_match (&RoadMapConfigTripShowRouteLines, "yes")) {
            if (RoadMapTripRouteLinesPen == NULL) {
                RoadMapTripRouteLinesPen = roadmap_canvas_create_pen ("Map.RouteLines");
                roadmap_canvas_set_foreground
                    (roadmap_config_get (&RoadMapConfigTripRouteLineColor));
            } else {
                roadmap_canvas_select_pen (RoadMapTripRouteLinesPen);
            }
            RoadMapTripFirstRoutePoint = 1;
             route_waypt_iterator (RoadMapCurrentRoute, roadmap_trip_route_routelines_draw);
        }
 
        /* Show all the on-route waypoint sprites. */
        RoadMapTripDrawingActiveRoute = 1;
        if (RoadMapCurrentRoute->rte_is_track) {
            route_waypt_iterator (RoadMapCurrentRoute, roadmap_trip_track_waypoint_draw);
        } else {
            route_waypt_iterator (RoadMapCurrentRoute, roadmap_trip_route_waypoint_draw);
        }
    }

    /* Show the focal points: e.g., GPS, departure, start, destination */
    for (focal = RoadMapTripFocalPoints; focal->id != NULL; focal++) {
        /* Skip those that aren't displayable, aren't initialized,
         * or that should only appear when there's an active route.
         */
        if (focal->sprite == NULL || !focal->has_value ||
            (focal->in_trip && RoadMapCurrentRoute == NULL)) {
            continue;
        }

        if (roadmap_math_point_is_visible (&focal->map)) {

            roadmap_math_coordinate (&focal->map, &guipoint);
            roadmap_math_rotate_coordinates (1, &guipoint);
            roadmap_sprite_draw
                (focal->sprite, &guipoint, focal->gps.steering);
        }
    }

    /* Show the pointer from GPS to the next waypoint. */
    if (RoadMapRouteInProgress && gps->has_value) {
        azymuth = roadmap_math_azymuth (&gps->map, &RoadMapTripNext->pos);
        roadmap_math_coordinate (&gps->map, &guipoint);
        roadmap_math_rotate_coordinates (1, &guipoint);
        roadmap_sprite_draw ("Direction", &guipoint, azymuth);
    }
}

void roadmap_trip_toggle_show_inactive_routes(void) {

    RoadMapTripShowInactiveRoutes = ! RoadMapTripShowInactiveRoutes;
    RoadMapTripRefresh = 1;
    roadmap_screen_refresh();
}

void roadmap_trip_toggle_show_inactive_tracks(void) {

    RoadMapTripShowInactiveTracks = ! RoadMapTripShowInactiveTracks;
    RoadMapTripRefresh = 1;
    roadmap_screen_refresh();
}


static const char *roadmap_trip_current() {
    return roadmap_config_get (&RoadMapConfigTripName);
}

/**
 * @brief start a new trip
 */
void roadmap_trip_new (void)
{
	if (RoadMapTripModified) {
		if (!roadmap_trip_save ()) {
			return;
		}
	}

	roadmap_tripdb_clear ();

	roadmap_trip_set_modified(0);

	roadmap_config_set (&RoadMapConfigTripName, "");

	roadmap_main_title("");

	RoadMapTripUntitled = 1;

	roadmap_trip_refresh ();

	roadmap_tripdb_empty_list ();
	roadmap_trip_set_point_focus (RoadMapTripBeginning);
	// TripRouteInProgress = 1;	/* Caused a crash */
	roadmap_trip_refresh ();
}


const char *roadmap_trip_path_relative_to_trips(const char *filename) {
    const char *p;
    int pl;
    p = roadmap_path_trips();
    pl = strlen(p);
    if (strncmp(p, filename, pl) == 0) {
        filename = filename + pl;
        filename = roadmap_path_skip_separator (filename);
    }
    return filename;
}

/**
 * @brief refresh the visual after loading a new trip
 */
void roadmap_trip_refresh(void)
{
    RoadMapTripRefresh = 1;
    roadmap_screen_refresh ();
}

/* File dialog support */

static int  roadmap_trip_load_file (const char *name, int silent, int merge);
static int  roadmap_trip_save_file (const char *name);

/**
 * @brief the OK button has been pressed, load the file
 * @param filename
 * @param mode
 */
static void roadmap_trip_file_dialog_ok (const char *filename, const char *mode)
{
    if (mode[0] == 'w') {
        if (roadmap_trip_save_file (filename)) {
            if ( RoadMapTripUntitled ) {
                filename = roadmap_trip_path_relative_to_trips(filename);
                roadmap_config_set (&RoadMapConfigTripName, filename);
		roadmap_main_title(TRIP_TITLE_FMT, roadmap_path_skip_directories(filename));
                RoadMapTripUntitled = 0;
            }
        }
    } else {
        roadmap_trip_load_file (filename, 0, 0);
    }
}

/**
 * @brief
 * @param filename
 * @param mode
 */
static void roadmap_trip_file_merge_dialog_ok (const char *filename, const char *mode)
{
    roadmap_trip_load_file (filename, 0, 1);
}


/**
 * @brief create a file selection dialog
 * @param mode
 */
static void roadmap_trip_file_dialog (const char *mode)
{
	roadmap_fileselection_new ("RoadMap Trip",
#ifdef _WIN32
		"*.gpx",
#else
		NULL,    /* no filter. */
#endif
		roadmap_path_trips (),
		mode,
		roadmap_trip_file_dialog_ok);
}

static void roadmap_trip_file_merge_dialog (const char *mode) {

    roadmap_fileselection_new ("RoadMap Trip Merge", NULL,    /* no filter. */
                               roadmap_path_trips (),
                               mode, roadmap_trip_file_merge_dialog_ok);
}

/**
 * @brief
 * @param a
 * @param b
 * @return
 */
static int alpha_waypoint_cmp( RoadMapListItem *a, RoadMapListItem *b)
{
	return strcasecmp(((waypoint *)a)->shortname, ((waypoint *)b)->shortname);
}

/**
 * @brief refresh the visual after loading a new trip
 */
void trip_refresh(void)
{
	extern int RoadMapTripRefresh;

	RoadMapTripRefresh = 1;
	roadmap_screen_refresh ();
}

/**
 * @brief
 * @param name
 * @param silent
 * @param merge
 * @return
 */
static int roadmap_trip_load_file (const char *name, int silent, int merge) {

    int ret;
    const char *path = NULL;
    RoadMapList tmp_waypoint_list, tmp_route_list, tmp_track_list;

    if (! roadmap_path_is_full_path (name))
        path = roadmap_path_trips ();

    roadmap_log (ROADMAP_DEBUG, "roadmap_trip_load_file '%s'", name);

    ROADMAP_LIST_INIT(&tmp_waypoint_list);
    ROADMAP_LIST_INIT(&tmp_route_list);
    ROADMAP_LIST_INIT(&tmp_track_list);

    ret = roadmap_gpx_read_file (path, name, &tmp_waypoint_list, 0,
            &tmp_route_list, &tmp_track_list);

    if (ret == 0) {

        /* Clear the lists, in case we had a partial read, and
         * there's data here.
         */
        waypt_flush_queue (&tmp_waypoint_list);
        route_flush_queue (&tmp_route_list);
        route_flush_queue (&tmp_track_list);

        if (RoadMapCurrentRoute != NULL) {
            roadmap_trip_set_route_focii ();
            roadmap_trip_set_point_focus (RoadMapTripBeginning);
        }
        return 0;
    }

    *TRIP_WAYPOINTS = WAYPOINTS_MODIFIED;
    *ROUTE_WAYPOINTS = WAYPOINTS_MODIFIED;

    if (merge) {

        ROADMAP_LIST_SPLICE(&RoadMapTripWaypointHead, &tmp_waypoint_list);
        ROADMAP_LIST_SPLICE(&RoadMapTripRouteHead, &tmp_route_list);
        ROADMAP_LIST_SPLICE(&RoadMapTripTrackHead, &tmp_track_list);
        roadmap_trip_set_modified(1);

    } else {

        // FIXME if we happen to be loading the file we're about
        // to save to, then we'll either a) clobber what we want
        // to load, or b) potentially lose data we wanted to save.
        // we default to saving, but this means if you load a trip,
        // delete some stuff, and reload to get it back, that won't
        // work.
        if (RoadMapTripModified) {
            if (!roadmap_trip_save ()) {
                return 0;
            }
        }
        roadmap_tripdb_clear();

        ROADMAP_LIST_MOVE(&RoadMapTripWaypointHead, &tmp_waypoint_list);
        ROADMAP_LIST_MOVE(&RoadMapTripRouteHead, &tmp_route_list);
        ROADMAP_LIST_MOVE(&RoadMapTripTrackHead, &tmp_track_list);

        roadmap_config_set (&RoadMapConfigTripName,
            roadmap_trip_path_relative_to_trips(name));
        roadmap_trip_set_modified(0);
        roadmap_main_title(TRIP_TITLE_FMT, roadmap_path_skip_directories(name));
        RoadMapTripUntitled = 0;
    }

    roadmap_list_sort(&RoadMapTripWaypointHead, alpha_waypoint_cmp);

    RoadMapCurrentRoute = NULL;

    if (roadmap_list_count (&RoadMapTripRouteHead) +
            roadmap_list_count (&RoadMapTripTrackHead) > 1) {
        /* Multiple choices?  Present a dialog. */
        roadmap_trip_route_manage_dialog ();
    } else { /* If there's only one route (or track), use it. */
        if (!ROADMAP_LIST_EMPTY(&RoadMapTripRouteHead)) {
            RoadMapCurrentRoute = 
                    (route_head *)ROADMAP_LIST_FIRST(&RoadMapTripRouteHead);
        } else if (!ROADMAP_LIST_EMPTY(&RoadMapTripTrackHead)) {
            RoadMapCurrentRoute = 
                    (route_head *)ROADMAP_LIST_FIRST(&RoadMapTripTrackHead);
        } else {
            RoadMapCurrentRoute = NULL;
        }
    }

    if (RoadMapCurrentRoute != NULL) {
        RoadMapRouteIsReversed = 0;
        RoadMapRouteInProgress = 0;
        roadmap_trip_set_route_focii ();
        roadmap_trip_set_point_focus (RoadMapTripBeginning);
    } else {
        roadmap_trip_unset_route_focii ();
    }

    /* Fill up our own structure based on this list */
    roadmap_tripdb_empty_list();

    if (! RoadMapCurrentRoute) {
	    trip_refresh();
	    return 0;
    }

    route_waypt_iterator (RoadMapCurrentRoute, roadmap_tripdb_waypoint_iter);

    roadmap_main_title(TRIP_TITLE_FMT,
		    roadmap_path_skip_directories(roadmap_trip_current()));
    roadmap_trip_refresh ();

    return ret;
}

/**
 * @brief
 * @param silent
 * @param merge
 * @return
 */
int roadmap_trip_load (int silent, int merge)
{
	const char *name = roadmap_trip_current();

	if (!name || !name[0])
		return 0;

	return roadmap_trip_load_file (name, silent, merge);
}

/**
 * @brief
 */
void roadmap_trip_load_ask (void)
{
	roadmap_trip_file_dialog ("r");
}

void roadmap_trip_merge_ask (void)
{
	roadmap_trip_file_merge_dialog ("r");
}

static int roadmap_trip_save_file (const char *name)
{
    const char *path = NULL;

    if (! roadmap_path_is_full_path (name)) {
        path = roadmap_path_trips ();
    }

    if (roadmap_gpx_write_file
            (roadmap_config_match (&RoadMapConfigBackupFiles, "yes"),
             path, name,
             &RoadMapTripWaypointHead,
             &RoadMapTripRouteHead,
             &RoadMapTripTrackHead)) {
        roadmap_trip_set_modified(0);
        return 1;
    }
    return 0;

}

int roadmap_trip_save (void) {

    int ret = 1; /* success */

    if (RoadMapTripUntitled) {
        if (RoadMapTripModified) { /* need to choose a name */
            const char *path = roadmap_path_trips();
            char name[50];
            int i;
            
            strcpy (name, "SavedTrip.gpx");

            i = 1;
            while (roadmap_file_exists(path, name) && i < 1000) {
                sprintf(name, "SavedTrip-%d.gpx", i++);
            }
            if (i == 1000) {
                roadmap_log (ROADMAP_WARNING, "over 1000 SavedTrips!");
                strcpy (name, "SavedTrip.gpx");
            }

            if (roadmap_trip_save_file (name)) {
                roadmap_config_set (&RoadMapConfigTripName, name);
		roadmap_main_title(TRIP_TITLE_FMT, roadmap_path_skip_directories(name));
                RoadMapTripUntitled = 0;
            }
        }
    } else {
        if (RoadMapTripModified) {
            ret = roadmap_trip_save_file (roadmap_trip_current());
        }
    }

    return ret;
}

void roadmap_trip_save_manual (void) {

    if (RoadMapTripUntitled) {
        roadmap_trip_save_as();
    } else {
        roadmap_trip_save_file (roadmap_trip_current());
    }
}

void roadmap_trip_save_as() {
    roadmap_trip_file_dialog ("w");
}



void roadmap_trip_save_screenshot (void) {

    time_t now;
    const char *path;
    char picturename[128];
    char *fullname;

    path = roadmap_path_trips ();
    now = time(NULL);
    strftime(picturename, sizeof(picturename),
            "map-%Y-%m-%d-%H-%M-%S-UTC.png", gmtime(&now));

    fullname = roadmap_path_join(path, picturename);

    roadmap_canvas_save_screenshot (fullname);

    free(fullname);
}


/* This simplifies a route (usually a track, but perhaps an
 * over-specified route as generated by google, for example). 
 * For each triplet, it calculates the simple arc length
 * differences -- i.e., if the distance from A to B plus the
 * distance from B to C is very close to the distance from A to
 * C, then point B will be dropped.  Doing a cross-track-error
 * simplification would give better results, but that's
 * much more expensive.  (gpsbabel's smplroute.c contains code
 * for this, and there are implementationso fo Douglas-Peucker on
 * the web:  e.g. http://geometryalgorithms.com/Archive/algorithm_0205/)
 */
int roadmap_trip_simplify_route
        (const route_head *orig_route, route_head *new_route) {

    waypoint *wa = NULL, *wb = NULL, *wc = NULL;
    RoadMapListItem *elem, *tmp;
    int dropped = 0;
    int ac_dist = -1, ab_dist = -1, bc_dist = -1;

    ROADMAP_LIST_FOR_EACH (&orig_route->waypoint_list, elem, tmp) {

        wa = (waypoint *) elem;
        if (wc != NULL) {

            if (wb->description == NULL && wb->notes == NULL)  {


                ac_dist = roadmap_math_distance (&wa->pos, &wc->pos);
                ab_dist = roadmap_math_distance (&wa->pos, &wb->pos);
                if (bc_dist == -1) {
                    bc_dist = roadmap_math_distance (&wb->pos, &wc->pos);
                }

                if (abs (ab_dist + bc_dist - ac_dist) < 10) {
                    wb = wa;
                    bc_dist = ac_dist;
                    dropped++;
                    continue;
                }
            }

            waypt_add(&new_route->waypoint_list, waypt_dupe( wc ) );

        }
        wc = wb;
        wb = wa;
        bc_dist = ab_dist;
    }
    if (wc) waypt_add(&new_route->waypoint_list, waypt_dupe( wc ) );
    if (wb) waypt_add(&new_route->waypoint_list, waypt_dupe( wb ) );

    return dropped;
}

void roadmap_trip_copy_route
        (const route_head *orig_route, route_head *new_route) {
    RoadMapListItem *elem, *tmp;

    ROADMAP_LIST_FOR_EACH (&orig_route->waypoint_list, elem, tmp) {
        waypt_add(&new_route->waypoint_list, waypt_dupe( (waypoint *)elem ));
    }

}


static void roadmap_trip_route_convert_worker
        (route_head *orig_route, char *new_name, 
            int simplify, int wanttrack, int reverse) {

    int dropped = 0;
    route_head *new_route;

    new_route = route_head_alloc();
    
    if (simplify) {
        dropped = roadmap_trip_simplify_route (orig_route, new_route);
    } else {
        roadmap_trip_copy_route (orig_route, new_route);
    }

    if (!simplify || dropped > 0) {

        if (new_name) {
            new_route->rte_name = xstrdup(new_name);
        }

        if (reverse) {
            route_reverse(new_route);
        }

        if (wanttrack) {
            new_route->rte_is_track = 1;
            route_add(&RoadMapTripTrackHead, new_route);
        } else {
            route_add(&RoadMapTripRouteHead, new_route);
        }

        RoadMapCurrentRoute = new_route;
        RoadMapRouteIsReversed = 0;
        RoadMapRouteInProgress = 0;
        roadmap_trip_set_route_focii ();
        roadmap_trip_set_point_focus (RoadMapTripBeginning);
        roadmap_screen_refresh ();
        roadmap_trip_set_modified(1);

    } else {
        route_free(new_route);
    }

}


void roadmap_trip_track_to_route (void) {

    char name[50];
    char *namep = NULL;

    if (RoadMapCurrentRoute == NULL) {
        return;
    }

    if (! RoadMapCurrentRoute->rte_is_track) {
        return;
    }

    if (RoadMapCurrentRoute->rte_name) {
        snprintf(name, 50, "TrackRoute %s", RoadMapCurrentRoute->rte_name);
        namep = name;
    }

    roadmap_trip_route_convert_worker (RoadMapCurrentRoute, namep, 0, 0, 0);

}

void roadmap_trip_route_simplify (void) {

    char name[50];
    char *namep = NULL;

    if (RoadMapCurrentRoute == NULL) {
        return;
    }

    if (RoadMapCurrentRoute->rte_name) {
        snprintf(name, 50, "Simple %s", RoadMapCurrentRoute->rte_name);
        namep = name;
    }

    roadmap_trip_route_convert_worker (RoadMapCurrentRoute, namep, 1, 0, 0);

}

void roadmap_trip_currenttrack_to_route (void) {

    char name[40];
    time_t now;


    time(&now);
    strftime(name, sizeof(name), "Backtrack-%Y-%m-%d-%H:%M:%S",
                localtime(&now));

    roadmap_trip_route_convert_worker (RoadMapTrack, name, 0, 0, 1);

}

void roadmap_trip_currenttrack_to_track (void) {

    char name[40];
    time_t now;


    time(&now);
    strftime(name, sizeof(name), "Track-%Y-%m-%d-%H:%M:%S",
                localtime(&now));

    roadmap_trip_route_convert_worker (RoadMapTrack, name, 0, 1, 0);

}


/* ---- support for finding the clicked-on waypoint ---- */

void roadmap_landmark_iterate(waypt_cb cb) {
    waypt_iterator (roadmap_landmark_list(), cb);
}


void roadmap_trip_landmark_iterate (waypt_cb cb) {
    waypt_iterator (&RoadMapTripWaypointHead, cb);
}

void roadmap_trip_routepoint_iterate (waypt_cb cb) {

    RoadMapListItem *relem, *rtmp;
    RoadMapListItem *elem, *tmp;
    waypoint *w;

    ROADMAP_LIST_FOR_EACH (&RoadMapTripRouteHead, relem, rtmp) {
        route_head *rh = (route_head *) relem;
        RoadMapAreaCurListName = rh->rte_name;
        QUEUE_FOR_EACH(&rh->waypoint_list, elem, tmp) {
                w = (waypoint *) elem;
                (*cb)(w);
        }
    }
    ROADMAP_LIST_FOR_EACH (&RoadMapTripTrackHead, relem, rtmp) {
        route_head *rh = (route_head *) relem;
        RoadMapAreaCurListName = rh->rte_name;
        QUEUE_FOR_EACH(&rh->waypoint_list, elem, tmp) {
                w = (waypoint *) elem;
                (*cb)(w);
        }
    }
}

void
roadmap_trip_area_waypoint(const waypoint *waypointp) {

    roadmap_place_pointer *point_ptr;

    if (roadmap_math_point_is_visible (&waypointp->pos)) {
        point_ptr = malloc(sizeof(*point_ptr));
        roadmap_check_allocated (point_ptr);
        point_ptr->wpt = (waypoint *)waypointp;
        point_ptr->list = RoadMapAreaCurListName;
        point_ptr->type = RoadMapAreaCurListType;
        roadmap_list_insert(&RoadMapTripAreaPlaces, &point_ptr->link);
    }
}

void roadmap_trip_clear_selection_list(void) {

   RoadMapListItem *element, *tmp;

   roadmap_dialog_hide ("Multiple Places");

   ROADMAP_LIST_FOR_EACH(&RoadMapTripAreaPlaces, element, tmp) {
      free (roadmap_list_remove(element));
   }
   RoadMapTripSelectedPlace = NULL;
}

void roadmap_trip_waypoint_show_selected(void) {

      roadmap_place_pointer *pp;
      waypoint *wpt;
      pp = (roadmap_place_pointer *)
            ROADMAP_LIST_FIRST(&RoadMapTripAreaPlaces);

      wpt = pp->wpt;

      roadmap_message_set ('R', pp->list ? pp->list : "");
      roadmap_message_set ('P', wpt->shortname);

      roadmap_display_text("Place", "%s%s%s%s%s%s",
        pp->list ? pp->list : "",
        pp->list ? " / " : "",
        wpt->shortname,
        wpt->description ? " - " : "",
        wpt->description ? wpt->description  : "",
        roadmap_list_count(&RoadMapTripAreaPlaces) > 1 ? " (more)" : "");

      roadmap_trip_move_last_cancel (NULL);
      RoadMapTripSelectedPlace = pp;

      roadmap_screen_refresh ();
}

static void roadmap_trip_waypoint_selection_dialog_selected (
        const char *name, void *data) {

    roadmap_place_pointer *pp;

    pp = (roadmap_place_pointer *) roadmap_dialog_get_data ("Names", ".Places");

    if (pp != NULL) {
        /* force it to the start of the list */
        roadmap_list_insert
            ( &RoadMapTripAreaPlaces, roadmap_list_remove(&pp->link));
        roadmap_trip_waypoint_show_selected ();
        roadmap_trip_set_focus_waypoint (pp->wpt);
        roadmap_screen_refresh ();
    }
}

static void roadmap_trip_waypoint_selection_dialog (void) {

    char **names;
    roadmap_place_pointer **places;
    int count;
    int i;
    RoadMapListItem *elem, *tmp;
    waypoint *wpt;
    roadmap_place_pointer *pp;


    if (roadmap_dialog_activate ( "Multiple Places", NULL)) {

        roadmap_dialog_new_list ("Names", ".Places");
        roadmap_dialog_add_button
            ("Okay", roadmap_trip_dialog_cancel);

        roadmap_dialog_complete (0);    /* No need for a keyboard. */
    }

    count = roadmap_list_count(&RoadMapTripAreaPlaces);
    if (count <= 1) {
        return;
    }

    names = calloc (count, sizeof (*names));
    roadmap_check_allocated (names);
    places = calloc (count, sizeof (*places));
    roadmap_check_allocated (places);


    i = 0;
    ROADMAP_LIST_FOR_EACH (&RoadMapTripAreaPlaces, elem, tmp) {
        char buf[128];
        pp = (roadmap_place_pointer *)elem;
        wpt = pp->wpt;

        snprintf(buf, sizeof(buf), "%s%s%s%s%s", 
            pp->list ? pp->list : "",
            pp->list ? " / " : "",
            wpt->shortname,
            wpt->description ? " - " : "",
            wpt->description ? wpt->description  : "");
        names[i] = strdup(buf);
        places[i++] = pp;
    }
    roadmap_dialog_show_list
        ("Names", ".Places", count, names, (void **) places,
            roadmap_trip_waypoint_selection_dialog_selected);

    while (i > 0)
        free(names[--i]);
    free (names);
    free (places);

}

int roadmap_trip_retrieve_area_points
        (RoadMapArea *area, RoadMapPosition *position) {

   int count;

   /* clear old contents */
   roadmap_trip_clear_selection_list();

   roadmap_math_set_focus (area);

   RoadMapAreaCurListName = "Personal landmark";
   RoadMapAreaCurListType = PERSONAL_WAYPOINTS;
   roadmap_landmark_iterate (roadmap_trip_area_waypoint);

   RoadMapAreaCurListName = "Trip landmark";
   RoadMapAreaCurListType = TRIP_WAYPOINTS;
   roadmap_trip_landmark_iterate (roadmap_trip_area_waypoint);

   RoadMapAreaCurListType = ROUTE_WAYPOINTS;
   roadmap_trip_routepoint_iterate (roadmap_trip_area_waypoint);

   roadmap_math_release_focus ();

   count = roadmap_list_count(&RoadMapTripAreaPlaces);

   if (!count) {
      roadmap_display_hide("Place");
      return 0;
   }

   if (count) {

      roadmap_place_pointer *pp;
      waypoint *wpt;

      while (count > 1) { /* find the closest point(s) */
          int dist, mindist = 999999999;
          roadmap_place_pointer *minpp = NULL;
          int ocount = count;
          RoadMapListItem *element, *tmp;

          ROADMAP_LIST_FOR_EACH(&RoadMapTripAreaPlaces, element, tmp) {
             pp = (roadmap_place_pointer *)element;
             wpt = pp->wpt;
             dist = roadmap_math_distance (&wpt->pos, position);
             if (dist < mindist) {
                mindist = dist;
                if (minpp) {
                    free (roadmap_list_remove(&minpp->link));
                    count--;
                }
                minpp = pp;
             } else if (dist > mindist) {
                free (roadmap_list_remove(&pp->link));
                count--;
             } /* else they're equal, keep both */
          }
          /* the loop is in case we have two or more coincident points
           * that are farther than the one we're searching for.  we
           * can only remove one point each time through the loop, so
           * if they're both the same distance, we need to go through
           * twice.
           */
          if (ocount == count) break;
      }

      if (count > 1) {
          roadmap_trip_waypoint_selection_dialog ();
      } else {
          roadmap_trip_waypoint_show_selected();
      }

   }

   return 1;
}

/* normally the SelectedPlace is set via mouse click, but we want
 * the selection dialog to be able to set it as well.
 */
static void roadmap_trip_set_selected_place(void *which, waypoint *w) {

   roadmap_place_pointer *pp;

   /* clear old contents */
   roadmap_trip_clear_selection_list();

   pp = calloc (1, sizeof (*pp));
   roadmap_check_allocated (pp);

   pp->wpt = w;
   pp->type = which;

   if (which == PERSONAL_WAYPOINTS) {
      pp->list = "Personal landmark";
   } else if (which == TRIP_WAYPOINTS) {
      pp->list = "Trip landmark";
   } else {
      if ( RoadMapCurrentRoute != NULL)
         pp->list = RoadMapCurrentRoute->rte_name;
   }

   roadmap_list_insert(&RoadMapTripAreaPlaces, &pp->link);

   roadmap_trip_move_last_cancel (NULL);
   RoadMapTripSelectedPlace = pp;

}



void roadmap_trip_delete_last_place(void)
{
    if (RoadMapTripSelectedPlace == NULL)
        return;

    roadmap_trip_waypoint_manage_action
        (RoadMapTripSelectedPlace->wpt,
                RoadMapTripSelectedPlace->type, WAYPOINT_ACTION_DELETE);

}

void roadmap_trip_move_routepoint_ahead (void) {

    if (RoadMapTripSelectedPlace == NULL)
        return;

    roadmap_trip_waypoint_manage_action
        (RoadMapTripSelectedPlace->wpt,
                RoadMapTripSelectedPlace->type,  WAYPOINT_ACTION_MOVE_AHEAD);

}

void roadmap_trip_move_routepoint_back (void) {

    if (RoadMapTripSelectedPlace == NULL)
        return;

    roadmap_trip_waypoint_manage_action
        (RoadMapTripSelectedPlace->wpt,
                RoadMapTripSelectedPlace->type,  WAYPOINT_ACTION_MOVE_BACK);

}

void roadmap_trip_edit_last_place(void)
{
    if (RoadMapTripSelectedPlace == NULL)
        return;

    roadmap_trip_dialog_waypoint_edit
        ( RoadMapTripSelectedPlace->wpt,
          RoadMapTripSelectedPlace->type,
          roadmap_preferences_use_keyboard ());

}

static int roadmap_trip_move_last_handler (RoadMapGuiPoint *point) {

    waypoint *w;
    roadmap_place_pointer *pp;

    if (RoadMapTripSelectedPlace != NULL) {

        pp = RoadMapTripSelectedPlace;
        w = pp->wpt;
        roadmap_math_to_position (point, &w->pos, 1);

        if (pp->type == PERSONAL_WAYPOINTS) {
            roadmap_landmark_set_modified();
        } else {
            roadmap_trip_set_modified(1);
            RoadMapTripRefresh = 1;
        }
    }

    roadmap_trip_move_last_cancel (point);
    return 1;
}


/* When moving a place, we replace the mouse click handlers with
 * our own.  A short click executes the move, the other two clicks
 * simply cancel it.
 */

static int roadmap_trip_move_last_cancel (RoadMapGuiPoint *point) {

    roadmap_pointer_unregister_short_click (&roadmap_trip_move_last_handler);
    roadmap_pointer_unregister_long_click (&roadmap_trip_move_last_cancel);
    roadmap_pointer_unregister_right_click (&roadmap_trip_move_last_cancel);

    RoadMapTripPlaceMoving = 0;

    roadmap_display_hide("Moving");
    roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);
    roadmap_screen_refresh ();
    return 1;
}

void roadmap_trip_move_last_place(void)
{
    waypoint *w;
    roadmap_place_pointer *pp;

    if (RoadMapTripPlaceMoving) {
        /* Possible if "Move" selected from a button or keyboard. */
        roadmap_trip_move_last_cancel (NULL);
        return;
    }

    if (RoadMapTripSelectedPlace == NULL)
        return;

    pp = RoadMapTripSelectedPlace;
    w = pp->wpt;

    roadmap_display_hide("Place");
    roadmap_display_text
        ("Moving", "Moving %s%s%s%s%s (Long or Right Click to Cancel)",
       pp->list ? pp->list : "",
       pp->list ? " / " : "",
       w->shortname,
       w->description ? " - " : "",
       w->description ? w->description  : "");

    RoadMapTripRefresh = 1;

    roadmap_screen_refresh ();
    roadmap_main_set_cursor (ROADMAP_CURSOR_CROSS);

    RoadMapTripPlaceMoving = 1;

    roadmap_pointer_register_short_click (&roadmap_trip_move_last_handler, POINTER_HIGHEST);
    roadmap_pointer_register_long_click (&roadmap_trip_move_last_cancel, POINTER_HIGHEST);
    roadmap_pointer_register_right_click (&roadmap_trip_move_last_cancel, POINTER_HIGHEST);
}

#if WGET_GOOGLE_ROUTE
/* for fun -- should be cleaned up.  given a route with just
 * start and a dest, query google for the driving directions
 * between those endpoints, and populate a new route with the
 * result.  */
void roadmap_trip_replace_with_google_route(void) {


    char slat[32], slon[32], dlat[32], dlon[32];
    char cmdbuf[256];
    int ret;

    if ( RoadMapCurrentRoute == NULL ||
        roadmap_list_count(&RoadMapCurrentRoute->waypoint_list) != 2) {
        return;
    }


    snprintf(cmdbuf, sizeof(cmdbuf), 
        "wget -O - 'http://maps.google.com/maps?q=%s,%s"
           " to %s,%s&output=js' 2>/dev/null  |"
            "gpsbabel -r -i google -f - "
            "-x simplify,error=0.05 -o gpx "
            "-F %s/getroute.gpx",
            roadmap_math_to_floatstring(slat,
                RoadMapTripStart->pos.latitude, MILLIONTHS),
            roadmap_math_to_floatstring(slon,
                RoadMapTripStart->pos.longitude, MILLIONTHS),
            roadmap_math_to_floatstring(dlat,
                RoadMapTripDest->pos.latitude, MILLIONTHS),
            roadmap_math_to_floatstring(dlon,
                RoadMapTripDest->pos.longitude, MILLIONTHS),
            roadmap_path_trips ());

    ret = system(cmdbuf);

    if (ret < 0)
        return;

    roadmap_trip_load_file ("getroute.gpx", 0, 1);

}
#endif

/**
 * @brief
 */
void roadmap_trip_initialize (void)
{
    RoadMapTripFocal *focal;

    ROADMAP_LIST_INIT(&RoadMapTripTrackHead);

    ROADMAP_LIST_INIT(&RoadMapTripLostRoutesHead);
    ROADMAP_LIST_INIT(&RoadMapTripLostPlacesHead);

    ROADMAP_LIST_INIT(&RoadMapTripAreaPlaces);

    ROADMAP_LIST_INIT(&RoadMapTripQuickRoute.waypoint_list);

    for (focal = RoadMapTripFocalPoints;
            focal->id != NULL; focal++) {

        roadmap_config_declare ("session", &focal->config_position, "0,0");

        if (focal->mobile) {
            roadmap_config_declare ("session", &focal->config_direction, "0");
        }

    }
    roadmap_config_declare ("session", &RoadMapConfigTripName, "");
    roadmap_config_declare ("session", &RoadMapConfigFocusName, "GPS");

    roadmap_config_declare_distance
        ("preferences", &RoadMapConfigWaypointSize, "125 ft");

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigTripShowInactiveRoutes, "yes", "no", NULL);

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigTripShowInactiveTracks, "yes", "no", NULL);

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigTripShowRouteLines, "yes", "no", NULL);
    roadmap_config_declare
        ("preferences", &RoadMapConfigTripRouteLineColor,  "red");

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigBackupFiles , "yes", "no", NULL);

    roadmap_config_declare
        ("preferences", &RoadMapConfigPathTrips, "&/trips");

    roadmap_config_declare
        ("preferences", &RoadMapConfigTripGPSFocusReleaseDelay, "5");


    RoadMapTripShowInactiveRoutes = roadmap_config_match
                                (&RoadMapConfigTripShowInactiveRoutes, "yes");

    RoadMapTripShowInactiveTracks = roadmap_config_match
                                (&RoadMapConfigTripShowInactiveTracks, "no");

    
    roadmap_state_add ("get_direction_next", &roadmap_trip_next_point_state);
    roadmap_state_add ("get_direction_2nd", &roadmap_trip_2nd_point_state);
    roadmap_state_add ("get_direction_dest", &roadmap_trip_dest_state);

    /* Some stuff inherited from temporary trip plugin */
    roadmap_trip_enable(1);

    RoadMapPosition pos;
    roadmap_trip_set_focus("GPS");
    pos = RoadMapTripGps->map;
    roadmap_trip_set_point ("Destination", &pos);
}

/**
 * @brief enable or disable navigation
 * @param status whether to set TripEnabled
 */
void roadmap_trip_enable (int status)
{
	if (status && RoadMapTripEnabled) {
		return;
	} else if (!status && !RoadMapTripEnabled) {
		return;
	}

	RoadMapTripEnabled = status;
}

#ifdef HAVE_NAVIGATE_PLUGIN
/**
 * @brief Copy the position of the selection into the internal variable named in the parameter
 * @param name indicate which variable to change
 */
void roadmap_trip_set_selection_as (const char *name)
{
	RoadMapPosition *sp = &RoadMapTripSelection->map;
	roadmap_trip_set_point (name, sp);
}

/**
 * @brief Set the internal variables of the selection to the coordinates specified
 * @param lon longitude
 * @param lat latitude
 */
void roadmap_trip_set_selection (const int lon, const int lat)
{
	roadmap_log (ROADMAP_DEBUG, "roadmap_trip_set_selection(%d,%d)", lon, lat);
	RoadMapTripSelection->map.longitude = lon;
	RoadMapTripSelection->map.latitude = lat;
}

/**
 * @brief look up the position of the named item in CONFIG (!)
 * @param name name of a position (Departure, GPS, Destination, ..)
 * @return the position requested
 */
const RoadMapPosition *roadmap_trip_get_position (const char *name)
{
	int i;
	RoadMapConfigDescriptor     *fp;
	static RoadMapPosition       pos;

	for (i=0; RoadMapTripFocalPoints[i].id; i++)
		if (strcmp(RoadMapTripFocalPoints[i].id, name) == 0) {
			fp = &RoadMapTripFocalPoints[i].config_position;
			roadmap_config_get_position (fp, &pos);
			roadmap_log (ROADMAP_DEBUG, "roadmap_trip_get_position(%s) -> %d,%d",
				name, pos.longitude, pos.latitude);
			return &pos;
		}
	return NULL;
}
#endif

/**
 * @brief indicate that refresh is needed
 */
void roadmap_trip_refresh_needed(void)
{
	RoadMapTripRefresh = 1;
}

#if 1
/*
 * Stuff to add menu items in the navigate menu
 * These could be integrated with the existing Trip UI.
 * (Done so experimentally already)
 */
/**
 * @brief Pop up a dialog to select a departure or destination from a list of waypoints
 * @param which selects the list of waypoints to use for display
 * @param destination do we select a destination or a departure
 */
void roadmap_trip_waypoint_select_navigation_waypoint (void *which, int destination) {

    int empty = 1;      /* warning suppression */
    const char *name = NULL; /* ditto */

    if (destination)
	    name = "Select destination";
    else
	    name = "Select departure";

    if (which == TRIP_WAYPOINTS) {
	    empty = ROADMAP_LIST_EMPTY(&RoadMapTripWaypointHead);
    } else if (which == PERSONAL_WAYPOINTS) {
	    empty = ROADMAP_LIST_EMPTY(roadmap_landmark_list());
    } else if (which == ROUTE_WAYPOINTS) {
	    if (RoadMapCurrentRoute == NULL) {
		    return;     /* Nothing to edit. */
	    }
	    empty = ROADMAP_LIST_EMPTY(&RoadMapCurrentRoute->waypoint_list);
    } else if (which == LOST_WAYPOINTS) {
	    empty = ROADMAP_LIST_EMPTY(roadmap_landmark_list());
    }

    if (empty) {
        return;         /* Nothing to edit. */
    }

    if (roadmap_dialog_activate ( name, which)) {
        roadmap_dialog_new_list ("Names", ".Waypoints");
        if (which == ROUTE_WAYPOINTS) {
            roadmap_dialog_add_button
                ("Back", roadmap_trip_waypoint_manage_dialog_up);
            roadmap_dialog_add_button
                ("Ahead", roadmap_trip_waypoint_manage_dialog_down);
        }
        roadmap_dialog_add_button
            ("Show", roadmap_trip_dialog_cancel);

	if (destination)
		roadmap_dialog_add_button
			("Destination", roadmap_trip_set_nav_destination);
	else
		roadmap_dialog_add_button
			("Departure", roadmap_trip_set_nav_departure);

        roadmap_dialog_new_hidden ("Names", ".which");

        roadmap_dialog_complete (0);    /* No need for a keyboard. */
    }

    roadmap_trip_waypoint_manage_dialog_populate (which);
}

/**
 * @brief Pop up a dialog to select a departure from the list of Personal waypoints
 * Calls the (static) worker function roadmap_trip_waypoint_select_navigation_waypoint
 * which is a slightly modified version of roadmap_trip_waypoint_manage_dialog_worker .
 */
void roadmap_trip_departure_waypoint (void)
{
	roadmap_trip_waypoint_select_navigation_waypoint(PERSONAL_WAYPOINTS, 0);
}

/**
 * @brief Pop up a dialog to select a destination from the list of Personal waypoints
 * Calls the (static) worker function roadmap_trip_waypoint_select_navigation_waypoint
 * which is a slightly modified version of roadmap_trip_waypoint_manage_dialog_worker .
 */
void roadmap_trip_destination_waypoint (void)
{
	roadmap_trip_waypoint_select_navigation_waypoint(PERSONAL_WAYPOINTS, 1);
}
#endif
