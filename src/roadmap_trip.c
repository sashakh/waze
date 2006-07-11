/* roadmap_trip.c - Manage a trip: destination & waypoints.
 *
 * LICENSE:
 *
 *   Copyright 2002,2005 Pascal F. Martin, Paul G. Fox
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
 *   See roadmap_trip.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
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
#include "roadmap_message.h"
#include "roadmap_messagebox.h"
#include "roadmap_preferences.h"
#include "roadmap_display.h"
#include "roadmap_adjust.h"
#include "roadmap_sunrise.h"
#include "roadmap_gpx.h"
#include "roadmap_track.h"
#include "roadmap_landmark.h"

#include "roadmap_trip.h"


static RoadMapConfigDescriptor RoadMapConfigTripName =
                        ROADMAP_CONFIG_ITEM("Trip", "Name");

static RoadMapConfigDescriptor RoadMapConfigTripRotate =
                        ROADMAP_CONFIG_ITEM("Display", "Rotate");

static RoadMapConfigDescriptor RoadMapConfigFocusName =
                        ROADMAP_CONFIG_ITEM("Focus", "Name");

static RoadMapConfigDescriptor RoadMapConfigFocusRotate =
                        ROADMAP_CONFIG_ITEM("Focus", "Rotate");

static RoadMapConfigDescriptor RoadMapConfigWaypointSize =
                        ROADMAP_CONFIG_ITEM("Trip", "Waypoint Radius");


/* Default location is: 1 Market St, San Francisco, California,
 *  but every effort is made to choose a more "local" initial
 *  position if one is available.  (i.e., this will probably only be
 *  used when the program is first run.) */
#define ROADMAP_INITIAL_LONGITUDE -122394181
#define ROADMAP_INITIAL_LATITUDE    37794928

extern route_head *RoadMapTrack;

route_head *RoadMapCurrentRoute = NULL;
static int RoadMapRouteInProgress = 0;
static int RoadMapRouteIsReversed = 0;
static int RoadMapTripRotate = 1;
static int RoadMapTripModified = 0;     /* Trip needs to be saved? */
static int RoadMapTripRefresh = 1;      /* Screen needs to be refreshed? */
static int RoadMapTripFocusChanged = 1;
static int RoadMapTripFocusMoved = 1;

typedef struct roadmap_trip_focal {

    char *id;
    char *sprite;

    char mobile;
    char save;
    char in_trip;
    char has_value;

    RoadMapPosition map;
    RoadMapGpsPosition gps;

    RoadMapConfigDescriptor config_position;
    RoadMapConfigDescriptor config_direction;


} RoadMapTripFocal;


#define ROADMAP_TRIP_ITEM(id, sprite, mobile, save, in_trip) \
    {id, sprite, mobile, save, in_trip, 0, \
     {0, 0}, \
     ROADMAP_GPS_NULL_POSITION, \
     ROADMAP_CONFIG_ITEM(id,"Position"), \
     ROADMAP_CONFIG_ITEM(id,"Direction") \
    }

/* The entries in the FocalPoints table are primarily used for
 * providing map focus state, but are also involved in other
 * trip bookkeeping roles -- where we are, where we're going,
 * where we've been.
 */
RoadMapTripFocal RoadMapTripFocalPoints[] = {
    ROADMAP_TRIP_ITEM ("GPS", "GPS", 1, 1, 0),
    ROADMAP_TRIP_ITEM ("Destination", NULL, 0, 0, 1),
    ROADMAP_TRIP_ITEM ("Start", NULL, 0, 0, 1),
    ROADMAP_TRIP_ITEM ("WayPoint", NULL, 0, 1, 0),
    ROADMAP_TRIP_ITEM ("Address", NULL, 0, 1, 0),
    ROADMAP_TRIP_ITEM ("Selection", NULL, 0, 1, 0),
    ROADMAP_TRIP_ITEM ("Departure", "Departure", 0, 0, 1),
    ROADMAP_TRIP_ITEM ("Hold", NULL, 1, 0, 0),
    ROADMAP_TRIP_ITEM (NULL, NULL, 0, 0, 0)
};

/* WARNING:  These are pointers into the above "predefined" table --
 *    order is important.  */
#define RoadMapTripGps           (&RoadMapTripFocalPoints[0])
#define RoadMapTripDestination   (&RoadMapTripFocalPoints[1])
#define RoadMapTripBeginning     (&RoadMapTripFocalPoints[2])
#define RoadMapTripWaypointFocus (&RoadMapTripFocalPoints[3])
#define RoadMapTripAddress       (&RoadMapTripFocalPoints[4])
#define RoadMapTripSelection     (&RoadMapTripFocalPoints[5])
#define RoadMapTripDeparture     (&RoadMapTripFocalPoints[6])
#define RoadMapTripHold          (&RoadMapTripFocalPoints[7])

/* These will point at one of the above. */
static RoadMapTripFocal *RoadMapTripFocus = NULL;
static RoadMapTripFocal *RoadMapTripLastSetPoint = NULL;

/* These point at waypoints in the current route. */
static waypoint *RoadMapTripDest = NULL;
static waypoint *RoadMapTripStart = NULL;
static waypoint *RoadMapTripNext = NULL;

/* This is the name of the waypoint used to set the WaypointFocus
 * focal point.
 */
static char *RoadMapTripWaypointFocusName = NULL;

static time_t RoadMapTripGPSTime = 0;

static queue RoadMapTripWaypointHead;
static queue RoadMapTripRouteHead;
static queue RoadMapTripTrackHead;

static route_head RoadMapTripQuickRoute = {
        {NULL, NULL},
        {NULL, NULL},
        "Quick Route",
        "Quick Route",
        0, 0, 0, NULL
};


static RoadMapPosition RoadMapTripDefaultPosition =
     {ROADMAP_INITIAL_LONGITUDE, ROADMAP_INITIAL_LATITUDE};

static void roadmap_trip_set_modified(int modified) {
    RoadMapTripModified = modified;
}

static waypoint * roadmap_trip_beginning (void) {

    if (RoadMapRouteIsReversed)
        return (waypoint *) QUEUE_LAST (&RoadMapCurrentRoute->waypoint_list);
    else
        return (waypoint *) QUEUE_FIRST (&RoadMapCurrentRoute->waypoint_list);
}

static waypoint * roadmap_trip_ending (void) {

    if (RoadMapRouteIsReversed)
        return (waypoint *) QUEUE_FIRST (&RoadMapCurrentRoute->waypoint_list);
    else
        return (waypoint *) QUEUE_LAST (&RoadMapCurrentRoute->waypoint_list);
}

static waypoint * roadmap_trip_next (const waypoint *waypointp) {

    if (RoadMapRouteIsReversed)
        return (waypoint *)QUEUE_LAST(&waypointp->Q);
    else
        return (waypoint *)QUEUE_NEXT(&waypointp->Q);
}

static waypoint * roadmap_trip_prev (const waypoint *waypointp) {

    if (RoadMapRouteIsReversed)
        return (waypoint *)QUEUE_NEXT(&waypointp->Q);
    else
        return (waypoint *)QUEUE_LAST(&waypointp->Q);
}

static void roadmap_trip_set_route_focii (void) {

    /* Set the waypoints. */
    RoadMapTripDest = roadmap_trip_ending ();
    RoadMapTripStart = roadmap_trip_beginning ();

    /* Set the focal points. */
    RoadMapTripDestination->map = RoadMapTripDest->pos;
    RoadMapTripDestination->has_value = 1;
    RoadMapTripBeginning->map = RoadMapTripStart->pos;
    RoadMapTripBeginning->has_value = 1;

}

static void roadmap_trip_unset_route_focii (void) {

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


static RoadMapTripFocal * roadmap_trip_find_focalpoint (const char *id) {

    RoadMapTripFocal *focal;

    for (focal = RoadMapTripFocalPoints; focal->id != NULL; ++focal) {
        if (strcmp (id, focal->id) == 0) {
            return focal;
        }
    }
    return NULL;
}


static void roadmap_trip_coordinate 
        (const RoadMapPosition * position, RoadMapGuiPoint * guipoint) {
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

/* Edit dialog.
 * Lets user change name and description for waypoints or routes.
 */
struct namepointers {
    char **nameptr1;
    char **nameptr2;
};

static void roadmap_trip_dialog_edit_okay (const char *name, void *data) {

    char *newname = (char *) roadmap_dialog_get_data ("Name", "Name:");
    char *newdesc = (char *) roadmap_dialog_get_data ("Comment", "Comment:");
    char **origname = ((struct namepointers *)data)->nameptr1;
    char **origdesc = ((struct namepointers *)data)->nameptr2;

    if (*origname) free(*origname);
    *origname = newname[0] ? strdup(newname) : NULL;

    if (*origdesc) free(*origdesc);
    *origdesc = newdesc[0] ? strdup(newdesc) : NULL;

    roadmap_dialog_hide (name);
}

void roadmap_trip_dialog_edit(char **namep, char **commentp, int use_keyboard) {

    static struct namepointers NamePointers;

    NamePointers.nameptr1 = namep;
    NamePointers.nameptr2 = commentp;

    if (roadmap_dialog_activate ("Rename", &NamePointers)) {

        roadmap_dialog_new_entry ("Comment", "Comment:");
        roadmap_dialog_new_entry ("Name", "Name:");

        roadmap_dialog_add_button ("Cancel",
                roadmap_trip_dialog_cancel);
        roadmap_dialog_add_button ("Okay",
                roadmap_trip_dialog_edit_okay);

        roadmap_dialog_complete (use_keyboard);
    }

    roadmap_dialog_set_data ("Comment", "Comment:", *commentp ? *commentp : "");
    roadmap_dialog_set_data ("Name", "Name:", *namep ? *namep : "");

}


/* Focus management */

static void roadmap_trip_set_point_focus (RoadMapTripFocal * focal) {

    int rotate;

    if (focal == NULL || !focal->has_value) {
        return;
    }

    if (!focal->mobile) {
        rotate = 0;             /* Fixed point, no rotation no matter what. */
    } else {
        rotate = roadmap_config_match (&RoadMapConfigTripRotate, "yes");
    }

    RoadMapTripRefresh = 1;
    RoadMapTripFocusChanged = 1;


    if (RoadMapTripRotate != rotate) {
        roadmap_config_set_integer (&RoadMapConfigFocusRotate, rotate);
        RoadMapTripRotate = rotate;
    }
    if (RoadMapTripFocus != focal) {
        roadmap_config_set (&RoadMapConfigFocusName, focal->id);
        RoadMapTripFocus = focal;
        roadmap_display_page (focal->id);
    }

    RoadMapTripLastSetPoint = focal;
}

void roadmap_trip_set_focus (const char *id) {

    /* Callers from outside refer to focus points by name. */
    RoadMapTripFocal *focal = roadmap_trip_find_focalpoint (id);

    if (focal == NULL) return;

    roadmap_trip_set_point_focus (focal);
}


static void roadmap_trip_set_focus_waypoint (waypoint * waypointp) {

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

// FIXME -- Pascal, I'm not confident that i'm using the dialog
//  routines correctly here.  Feel free to educate me.  :-)

static void roadmap_trip_add_waypoint_dialog_okay
        (const char *name, void *data) {

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

static void roadmap_trip_add_waypoint_dialog 
        (const char *name, RoadMapPosition * position) {

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

        roadmap_dialog_complete (0);
    }
    roadmap_dialog_set_data ("Name", "Name:", name);

}

/* Edit Waypoint dialog */

#define PERSONAL_WAYPOINTS (void *)0
#define TRIP_WAYPOINTS (void *)1
#define ROUTE_WAYPOINTS (void *)2

#define WAYPOINT_ACTION_DELETE 0
#define WAYPOINT_ACTION_MOVE_UP 1
#define WAYPOINT_ACTION_MOVE_DOWN 2

static void roadmap_trip_waypoint_manage_dialog_populate
        (int count, void *which);

static void roadmap_trip_waypoint_manage_dialog_action
        (const char *name, void *data, int action) {

    int count;
    waypoint *waypointp, *neighbor;
    
    waypointp = (waypoint *) roadmap_dialog_get_data ("Names", ".Waypoints");

    if (waypointp == NULL) {
        return;
    }


    switch(action) {
    case WAYPOINT_ACTION_MOVE_UP:
        if (waypointp == RoadMapTripStart)
            return;
        neighbor = (waypoint *)QUEUE_LAST(&waypointp->Q);
        waypt_del (waypointp);
        ENQUEUE_BEFORE(&neighbor->Q, &waypointp->Q);
        break;

    case WAYPOINT_ACTION_MOVE_DOWN:
        if (waypointp == RoadMapTripDest)
            return;
        neighbor = (waypoint *)QUEUE_NEXT(&waypointp->Q);
        waypt_del (waypointp);
        ENQUEUE_AFTER(&neighbor->Q, &waypointp->Q);
        break;

    case WAYPOINT_ACTION_DELETE:
        if (data == PERSONAL_WAYPOINTS) {
            roadmap_landmark_remove(waypointp);
        } else if (data == TRIP_WAYPOINTS) {
            waypt_del (waypointp);
            waypt_free (waypointp);
        } else {
            route_del_wpt( RoadMapCurrentRoute, waypointp);
            /* last waypoint */
            if ( RoadMapCurrentRoute->rte_waypt_ct == 0 ) {
                route_del (RoadMapCurrentRoute);
                RoadMapCurrentRoute = NULL;
                roadmap_trip_unset_route_focii ();
                roadmap_trip_set_modified(1);
                RoadMapTripRefresh = 1;
                roadmap_screen_refresh ();
                roadmap_dialog_hide (name);
                return;
            }

        }
        break;

    default:
        return;
    }

    if (data == ROUTE_WAYPOINTS) {
        roadmap_trip_set_route_focii ();
    }

    if (RoadMapRouteInProgress && (RoadMapTripNext == waypointp)) {
        roadmap_trip_route_resume ();
    }

    RoadMapTripRefresh = 1;

    if (data != PERSONAL_WAYPOINTS) {
        roadmap_trip_set_modified(1);
    }

    roadmap_screen_refresh ();

    if (data == ROUTE_WAYPOINTS) {
        count = RoadMapCurrentRoute->rte_waypt_ct;
    } else if (data == TRIP_WAYPOINTS) {
        count = queue_count(&RoadMapTripWaypointHead);
    } else {
        count = roadmap_landmark_count();
    }
    if (count > 0) {
        roadmap_trip_waypoint_manage_dialog_populate (count, data);
    } else {
        roadmap_dialog_hide (name);
    }
}

static void roadmap_trip_waypoint_manage_dialog_delete
        (const char *name, void *data) {
    roadmap_trip_waypoint_manage_dialog_action
        (name, data, WAYPOINT_ACTION_DELETE);
}

static void roadmap_trip_waypoint_manage_dialog_up
        (const char *name, void *data) {
    roadmap_trip_waypoint_manage_dialog_action
        (name, data, WAYPOINT_ACTION_MOVE_UP);
}

static void roadmap_trip_waypoint_manage_dialog_down
        (const char *name, void *data) {
    roadmap_trip_waypoint_manage_dialog_action
        (name, data, WAYPOINT_ACTION_MOVE_DOWN);
}

static void roadmap_trip_waypoint_manage_dialog_edit
        (const char *name, void *data) {
    waypoint *waypointp = (waypoint *) roadmap_dialog_get_data (
            "Names", ".Waypoints");

    if (waypointp == NULL) {
        return;
    }

    roadmap_trip_dialog_edit
        ( &waypointp->shortname, &waypointp->description,
            roadmap_preferences_use_keyboard ());

    roadmap_trip_set_modified(1);

    /* It might be nicer to repopulate after the rename, but I'm
     * not sure how to do that.  This works.
     */
    roadmap_dialog_hide (name);
}

static void roadmap_trip_waypoint_manage_dialog_selected (
        const char *name, void *data) {

    waypoint *waypointp = (waypoint *) roadmap_dialog_get_data (
            "Names", ".Waypoints");

    if (waypointp != NULL) {
        roadmap_trip_set_focus_waypoint (waypointp);
        roadmap_screen_refresh ();
    }
}


static void roadmap_trip_waypoint_manage_dialog_populate
        (int count, void *which) {

    static char **Names = NULL;
    static waypoint **Waypoints;
    queue *list = NULL;  /* warning suppression */

    int i;
    queue *elem, *tmp;
    waypoint *waypointp;


    if (Names != NULL) {
        free (Names);
    }
    if (Waypoints != NULL) {
        free (Waypoints);
    }
    Names = calloc (count, sizeof (*Names));
    roadmap_check_allocated (Names);
    Waypoints = calloc (count, sizeof (*Waypoints));
    roadmap_check_allocated (Waypoints);

    if (which == PERSONAL_WAYPOINTS) {
        list = roadmap_landmark_list();
    } else if (which == TRIP_WAYPOINTS) {
        list = &RoadMapTripWaypointHead;
    } else if (which == ROUTE_WAYPOINTS) {
        list = &RoadMapCurrentRoute->waypoint_list;
    }

    i = 0;
    QUEUE_FOR_EACH (list, elem, tmp) {
        waypointp = (waypoint *) elem;
        Names[i] = waypointp->shortname;
        Waypoints[i++] = waypointp;
    }

    roadmap_dialog_show_list
        ("Names", ".Waypoints", count, Names, (void **) Waypoints,
            roadmap_trip_waypoint_manage_dialog_selected);
}


static void roadmap_trip_waypoint_manage_dialog_worker (void *which) {

    int count = 0;      /* warning suppression */
    const char *name = NULL; /* ditto */

    if (which == TRIP_WAYPOINTS) {
        count = queue_count(&RoadMapTripWaypointHead);
        name = "Trip Landmarks";
    } else if (which == PERSONAL_WAYPOINTS) {
        count = roadmap_landmark_count();
        name = "Personal Landmarks";
    } else if (which == ROUTE_WAYPOINTS) {
        if (RoadMapCurrentRoute == NULL) {
            return;     /* Nothing to edit. */
        }
        count = RoadMapCurrentRoute->rte_waypt_ct;
        name = "Route Points";
    }
    if (count <= 0) {
        return;         /* Nothing to edit. */
    }

    if (roadmap_dialog_activate ( name, which)) {

        roadmap_dialog_new_list ("Names", ".Waypoints");
        roadmap_dialog_add_button
            ("Delete", roadmap_trip_waypoint_manage_dialog_delete);
        if (which == ROUTE_WAYPOINTS) {
            roadmap_dialog_add_button
                ("Up", roadmap_trip_waypoint_manage_dialog_up);
            roadmap_dialog_add_button
                ("Down", roadmap_trip_waypoint_manage_dialog_down);
        }
        roadmap_dialog_add_button
            ("Edit", roadmap_trip_waypoint_manage_dialog_edit);
        roadmap_dialog_add_button
            ("Okay", roadmap_trip_dialog_cancel);

        roadmap_dialog_complete (0);    /* No need for a keyboard. */
    }

    roadmap_trip_waypoint_manage_dialog_populate (count, which);
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

/* Route Manage dialog */

static void roadmap_trip_route_manage_dialog_populate (int count);

static void roadmap_trip_route_manage_dialog_none 
        ( const char *name, void *data) {
    RoadMapRouteInProgress = 0;
    RoadMapCurrentRoute = NULL;
    roadmap_trip_unset_route_focii ();
    RoadMapTripRefresh = 1;
    roadmap_dialog_hide (name);
    roadmap_screen_refresh ();
}

static void roadmap_trip_route_manage_dialog_delete
        (const char *name, void *data) {

    int count;
    route_head *route =
        (route_head *) roadmap_dialog_get_data ("Names", ".Routes");

    if (route != NULL) {
        route_del (route);
        roadmap_trip_set_modified(1);
        if (route == RoadMapCurrentRoute) {

            RoadMapRouteInProgress = 0;
            RoadMapCurrentRoute = NULL;
            roadmap_trip_unset_route_focii ();
            RoadMapTripRefresh = 1;
        }

        count = queue_count (&RoadMapTripRouteHead) + 
                queue_count (&RoadMapTripTrackHead);
        if (count > 0) {
            roadmap_trip_route_manage_dialog_populate (count);
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

    roadmap_trip_dialog_edit
        ( &route->rte_name, &route->rte_desc,
            roadmap_preferences_use_keyboard ());

    roadmap_trip_set_modified(1);

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

    static char **Names = NULL;
    static route_head **Routes = NULL;
    queue *elem, *tmp;
    int i;

    if (Names != NULL) {
        free (Names);
    }
    if (Routes != NULL) {
        free (Routes);
    }

    Names = calloc (count, sizeof (*Names));
    roadmap_check_allocated (Names);
    Routes = calloc (count, sizeof (*Routes));
    roadmap_check_allocated (Routes);

    i = 0;
    QUEUE_FOR_EACH (&RoadMapTripRouteHead, elem, tmp) {
        route_head *rh = (route_head *) elem;
        Names[i] = (rh->rte_name && rh->rte_name[0]) ?
                rh->rte_name : "Unnamed Route";
        Routes[i++] = rh;
    }

    QUEUE_FOR_EACH (&RoadMapTripTrackHead, elem, tmp) {
        route_head *th = (route_head *) elem;
        Names[i] = (th->rte_name && th->rte_name[0]) ?
                th->rte_name : "Unnamed Track";
        Routes[i++] = th;
    }

    roadmap_dialog_show_list
        ("Names", ".Routes", count, Names, (void **) Routes,
         roadmap_trip_route_manage_dialog_selected);
}

void roadmap_trip_route_manage_dialog (void) {

    int count;

    if (QUEUE_EMPTY(&RoadMapTripRouteHead) &&
        QUEUE_EMPTY(&RoadMapTripTrackHead)) {
        return;                 /* Nothing to manage. */
    }

    if (roadmap_dialog_activate ("Manage Routes", NULL)) {

        roadmap_dialog_new_list ("Names", ".Routes");

        roadmap_dialog_add_button ("Delete",
                                   roadmap_trip_route_manage_dialog_delete);
        roadmap_dialog_add_button ("Edit",
                                   roadmap_trip_route_manage_dialog_edit);
        roadmap_dialog_add_button ("None",
                                   roadmap_trip_route_manage_dialog_none);

        roadmap_dialog_add_button ("Okay", roadmap_trip_dialog_cancel);

        roadmap_dialog_complete (0);    /* No need for a keyboard. */
    }

    count = queue_count (&RoadMapTripRouteHead) + 
            queue_count (&RoadMapTripTrackHead);
    roadmap_trip_route_manage_dialog_populate (count);
}




static void roadmap_trip_activate (void) {

    if (RoadMapTripGps->has_value) {
        roadmap_trip_set_point_focus (RoadMapTripGps);
    } else {
        roadmap_trip_set_focus_waypoint (RoadMapTripNext);
    }
    RoadMapRouteInProgress = 1;
    roadmap_screen_redraw ();
}


static void roadmap_trip_clear (void) {


    waypt_flush_queue (&RoadMapTripWaypointHead);
    route_flush_queue (&RoadMapTripRouteHead);
    route_flush_queue (&RoadMapTripTrackHead);

    roadmap_trip_set_modified(1);

    RoadMapCurrentRoute = NULL;
    roadmap_trip_unset_route_focii ();

    RoadMapTripRefresh = 1;
}

waypoint * roadmap_trip_new_waypoint
        (const char *name, RoadMapPosition * position, const char *sprite) {
    waypoint *w;
    w = waypt_new ();
    if (name) w->shortname = xstrdup (name);
    if (sprite) w->icon_descr = xstrdup(sprite);
    w->pos = *position;
    w->creation_time = time(NULL);
    return w;
}

void roadmap_trip_set_point (const char *id, RoadMapPosition * position) {

    RoadMapTripFocal *focal;

    focal = roadmap_trip_find_focalpoint (id);

    if (focal == NULL) return;

    focal->map = *position;

    if (focal->save) {
        roadmap_config_set_position (&focal->config_position, position);
    }

    focal->has_value = 1;
    RoadMapTripLastSetPoint = focal;
}


void roadmap_trip_new_route_waypoint(waypoint *waypointp) {

    route_head *new_route;

    new_route = route_head_alloc();
    route_add(&RoadMapTripRouteHead, new_route);

    route_add_wpt_tail (new_route, waypointp);
    RoadMapTripRefresh = 1;
    roadmap_trip_set_modified(1);

    RoadMapCurrentRoute = new_route;
    RoadMapRouteIsReversed = 0;

    roadmap_trip_set_route_focii ();

    if (RoadMapRouteInProgress) roadmap_trip_route_resume ();

    roadmap_trip_set_point_focus (RoadMapTripBeginning);
    roadmap_screen_refresh ();
}

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

static const char *roadmap_trip_last_setpoint_name(void)
{
    /* RoadMapTripLastSetPoint keeps track of the last focus or selection
     * made by the user, for use in starting routes or adding waypoints.
     * here we try to associate it with the name it originated with.
     */

    if (RoadMapTripLastSetPoint == NULL ||
            !RoadMapTripLastSetPoint->has_value) {
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
        (name, &RoadMapTripLastSetPoint->map, NULL);

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

static waypoint * roadmap_trip_choose_best_next (const RoadMapPosition *pos);

void roadmap_trip_add_waypoint
        ( const char *name, RoadMapPosition * position, int where) {

    waypoint *waypointp, *next = NULL;
    int putafter;

    if (where >= NUM_PLACEMENTS && RoadMapCurrentRoute == NULL) {
        return;  /* shouldn't happen */
    }

    waypointp = roadmap_trip_new_waypoint (name, position, NULL);
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
        roadmap_trip_set_focus_waypoint (waypointp);
        break;

    case PLACE_PERSONAL_MARK:
        roadmap_landmark_add(waypointp);
        roadmap_trip_set_focus_waypoint (waypointp);
        break;

    case PLACE_NEW_ROUTE:
        roadmap_trip_new_route_waypoint(waypointp);
        return;  /* note -- return, not break */
    }

    if (where >= NUM_PLACEMENTS) { 
        roadmap_trip_set_route_focii ();
        roadmap_trip_set_focus_waypoint (waypointp);
    }

    RoadMapTripRefresh = 1;
    roadmap_trip_set_modified(1);

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

void roadmap_trip_set_gps
    (int gps_time, const RoadMapGpsPosition *gps_position) {

    RoadMapPosition position;
    RoadMapGuiPoint guipoint1;
    RoadMapGuiPoint guipoint2;
    roadmap_adjust_position (gps_position, &position);

    /* An existing point: refresh is needed only if the point
     * moved in a visible fashion.
     */


    if (RoadMapTripGps->map.latitude != position.latitude ||
        RoadMapTripGps->map.longitude != position.longitude) {

        roadmap_trip_coordinate (&position, &guipoint1);
        roadmap_trip_coordinate (&RoadMapTripGps->map, &guipoint2);

        if (guipoint1.x != guipoint2.x || guipoint1.y != guipoint2.y) {
            RoadMapTripRefresh = 1;
        }

        if (RoadMapTripGps == RoadMapTripFocus) {
            RoadMapTripFocusMoved = 1;
        }
    }
    RoadMapTripGps->gps = *gps_position;
    RoadMapTripGps->map = position;
    roadmap_config_set_position (&RoadMapTripGps->config_position, &position);
    if (RoadMapRouteInProgress && !RoadMapTripGps->has_value) {
        RoadMapTripGps->has_value = 1;
        roadmap_trip_route_resume();
    }
    RoadMapTripGPSTime = gps_time;

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

    if (RoadMapTripRotate && (RoadMapTripFocus != NULL)) {
        return 360 - RoadMapTripFocus->gps.steering;
    }

    return 0;
}


const char * roadmap_trip_get_focus_name (void) {

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

static waypoint * roadmap_trip_choose_best_next (const RoadMapPosition *pos) {

    queue *elem, *tmp;
    waypoint *nextpoint;
    int dist, distmin;
    int which;

    if (RoadMapCurrentRoute == NULL) {
        return NULL;
    }

    distmin = 500000000;
    nextpoint = (waypoint *)QUEUE_FIRST(&RoadMapCurrentRoute->waypoint_list);

    /* Find the closest segment to our current position. */
    QUEUE_FOR_EACH (&RoadMapCurrentRoute->waypoint_list, elem, tmp) {

        waypoint *waypointp = (waypoint *) elem;
        if (waypointp ==
            (waypoint *)QUEUE_LAST( &RoadMapCurrentRoute->waypoint_list))
            break;

        dist = roadmap_math_get_distance_from_segment
            (pos, &waypointp->pos,
            &((waypoint *)QUEUE_NEXT(&waypointp->Q))->pos,
            NULL, &which);

        if (dist < distmin) {
            distmin = dist;
            if (which == 1) {
                /* waypointp is closer */
                nextpoint = waypointp;

            } else if (which == 2) {
                /* waypointp->next is closer */
                nextpoint = (waypoint *)QUEUE_NEXT(&waypointp->Q);

            } else {
                /* segment itself is closest */
                if (RoadMapRouteIsReversed) {
                    nextpoint = waypointp;
                } else {
                    nextpoint = (waypoint *)QUEUE_NEXT(&waypointp->Q);
                }
            }
        }

    }

    return nextpoint;
}

static void roadmap_trip_set_distance(char which, int distance)
{
    int distance_far;

    distance_far = roadmap_math_to_trip_distance_tenths (distance);

    if (distance_far > 0) {
        roadmap_message_set (which, "%d.%d %s",
                             distance_far/10,
                             distance_far%10,
                             roadmap_math_trip_unit ());
    } else {
        roadmap_message_set (which, "%d %s",
                             distance,
                             roadmap_math_distance_unit ());
    }
}

static void roadmap_trip_set_directions
        (int dist_to_next, int suppress_dist, waypoint *next) {

    static waypoint *last_next;
    static char *desc;
    static int rest_of_distance;

    if (next == NULL) {
        roadmap_message_unset ('Y');
        roadmap_message_unset ('X');
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
            desc = "Arrive at destination";
        }

        roadmap_message_set ('X', "%s", desc);

    }


    if (suppress_dist) {
        roadmap_message_unset ('Y');
    } else {
        roadmap_trip_set_distance('Y', dist_to_next + rest_of_distance);
    }

}

static void roadmap_trip_set_departure (int force) {

    if (force || !RoadMapTripDeparture->has_value) {
        RoadMapTripDeparture->map = RoadMapTripGps->map;
        RoadMapTripDeparture->has_value = 1;
    }
}

static void roadmap_trip_unset_departure (void) {

    RoadMapTripDeparture->has_value = 0;

}

void roadmap_trip_route_start (void) {

    if (RoadMapCurrentRoute == NULL) {
        return;
    }

    RoadMapTripNext = RoadMapTripStart;
    roadmap_trip_set_departure (1);
    roadmap_trip_activate ();
    roadmap_trip_set_directions(0, 0, NULL);
}


void roadmap_trip_route_resume (void) {

    if (RoadMapCurrentRoute == NULL) {
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

    roadmap_screen_refresh ();
}

void roadmap_trip_route_return (void) {
    // FIXME -- "return" trips and "reverse" trips are really
    // different.  a true "return" trip would be re-navigated, or
    // maybe would follow the user's track, backwards.

    roadmap_trip_route_reverse ();
}


void roadmap_trip_route_stop (void) {

    if (RoadMapCurrentRoute == NULL) {
        return;
    }

    RoadMapRouteInProgress = 0;
    roadmap_trip_unset_departure ();

    RoadMapTripRefresh = 1;
    roadmap_screen_redraw ();
}


void roadmap_trip_format_messages (void) {

    RoadMapTripFocal *gps = RoadMapTripGps;
    int distance_to_destination;
    int distance_to_next;
    int distance_to_directions;
    int waypoint_size;
    time_t now = time (NULL);
    time_t sun;
    static RoadMapPosition lastgpsmap = {-1, -1};
    static waypoint *within_waypoint = NULL;

    if (! RoadMapRouteInProgress || RoadMapCurrentRoute == NULL) {

        RoadMapTripNext = NULL;

        roadmap_message_unset ('A');
        roadmap_message_unset ('D');
        roadmap_message_unset ('S');
        roadmap_message_unset ('T');
        roadmap_message_unset ('W');

        roadmap_message_unset ('M');
        roadmap_message_unset ('E');
        roadmap_message_unset ('X');
        roadmap_message_unset ('Y');
        lastgpsmap.latitude = -1;
        return;
    }

    roadmap_message_set ('T', roadmap_time_get_hours_minutes (now));

    if (!gps->has_value) {

        roadmap_message_unset ('A');
        roadmap_message_set ('D', "?? %s", roadmap_math_trip_unit());
        roadmap_message_set ('S', "?? %s", roadmap_math_speed_unit ());
        roadmap_message_set ('W', "?? %s", roadmap_math_distance_unit ());

        roadmap_message_set ('M', "??:??");
        roadmap_message_set ('E', "??:??");
        roadmap_message_set ('X', "??");
        roadmap_message_set ('Y', "?? %s", roadmap_math_trip_unit());
        lastgpsmap.latitude = -1;
        return;
    }

    if (lastgpsmap.latitude == -1) lastgpsmap = gps->map;


    distance_to_destination =
        roadmap_math_distance (&gps->map, &RoadMapTripDest->pos);
    roadmap_trip_set_distance('D', distance_to_destination);

    roadmap_log (ROADMAP_DEBUG,
                 "GPS: distance to destination = %d %s",
                 distance_to_destination, roadmap_math_distance_unit ());


    distance_to_next =
        distance_to_directions =
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

        roadmap_trip_set_distance ('W', distance_to_next);

    }

    roadmap_message_set ('S', "%3d %s",
                         roadmap_math_to_speed_unit (gps->gps.speed),
                         roadmap_math_speed_unit ());

    roadmap_message_set ('H', "%d %s",
                         gps->gps.altitude, roadmap_math_distance_unit ());

    sun = roadmap_sunset (&gps->gps);
    if (sun > now) {

        roadmap_message_unset ('M');

        roadmap_message_set ('E', roadmap_time_get_hours_minutes (sun));

    } else {

        roadmap_message_unset ('E');

        sun = roadmap_sunrise (&gps->gps);
        roadmap_message_set ('M', roadmap_time_get_hours_minutes (sun));
    }

    lastgpsmap = gps->map;

}

static void roadmap_trip_standalone_waypoint_draw(const waypoint *waypointp)
{
    RoadMapGuiPoint guipoint;
    const char *sprite;
    if (waypointp->icon_descr == NULL) {
        sprite = "TripLandmark";
    } else {
        sprite = waypointp->icon_descr;
    }

    if (roadmap_math_point_is_visible (&waypointp->pos)) {

        roadmap_math_coordinate (&waypointp->pos, &guipoint);
        roadmap_math_rotate_coordinates (1, &guipoint);
        roadmap_sprite_draw (sprite, &guipoint, 0);
    }
}

static void roadmap_trip_route_waypoint_draw (const waypoint *waypointp)
{


    const char *sprite;
    RoadMapGuiPoint guipoint;

    int rotate = 1;
    int azymuth = 0;

    if (RoadMapCurrentRoute->rte_is_track) {
        sprite = "Track";
        rotate = 0;
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

    if (roadmap_math_point_is_visible (&waypointp->pos)) {

        roadmap_math_coordinate (&waypointp->pos, &guipoint);
        roadmap_math_rotate_coordinates (1, &guipoint);
        if (rotate) {
            azymuth = roadmap_math_azymuth
                ( &waypointp->pos, &(roadmap_trip_next(waypointp))->pos);
        }
        roadmap_sprite_draw (sprite, &guipoint, azymuth);
    }
}

void roadmap_trip_display (void) {

    int azymuth;
    RoadMapGuiPoint guipoint;
    RoadMapTripFocal *focal;
    RoadMapTripFocal *gps = RoadMapTripGps;

    /* Show the standalone trip waypoints. */
    waypt_iterator (&RoadMapTripWaypointHead,
        roadmap_trip_standalone_waypoint_draw);

    /* Show all the on-route waypoints. */
    if (RoadMapCurrentRoute != NULL) {
        route_waypt_iterator
            (RoadMapCurrentRoute, roadmap_trip_route_waypoint_draw);
    }

    /* Show the focal points: e.g., GPS, departure, start, destination */
    for (focal = RoadMapTripFocalPoints; focal->id != NULL; focal++) {

        /* Skip those that aren't displayable, aren't initialized,
         * or that should only appear when there's an active route.
         */
        if (focal->sprite == NULL ||
            !focal->has_value ||
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


static const char *roadmap_trip_current() {
    return roadmap_config_get (&RoadMapConfigTripName);
}


void roadmap_trip_new (void) {

    const char *path = roadmap_path_trips();
    char name[50];
    int i;
    
    strcpy (name, "NewTrip");

    i = 1;
    while (roadmap_file_exists(path, name) && i < 1000) {
	sprintf(name, "NewTrip-%d", i++);
    }
    if (i == 1000) {
	roadmap_log (ROADMAP_WARNING, "over 1000 new trips!");
	strcpy (name, "NewTrip");
    }

    if (RoadMapTripModified) {
	roadmap_trip_save (0);
    }

    roadmap_trip_clear ();

    roadmap_config_set (&RoadMapConfigTripName, name);

    roadmap_trip_set_modified(1);

    roadmap_screen_refresh ();
}


void roadmap_trip_initialize (void) {

    RoadMapTripFocal *focal;

    QUEUE_INIT(&RoadMapTripWaypointHead);
    QUEUE_INIT(&RoadMapTripRouteHead);
    QUEUE_INIT(&RoadMapTripTrackHead);

    QUEUE_INIT(&RoadMapTripQuickRoute.waypoint_list);

    for (focal = RoadMapTripFocalPoints;
            focal->id != NULL; focal++) {

        roadmap_config_declare ("session",
            &focal->config_position, "0,0");

        if (focal->mobile) {
            roadmap_config_declare ("session", &focal->config_direction, "0");
        }

    }
    roadmap_config_declare
        ("session", &RoadMapConfigTripName, "default.gpx");
    roadmap_config_declare
        ("session", &RoadMapConfigFocusName, "GPS");
    roadmap_config_declare
        ("session", &RoadMapConfigFocusRotate, "1");

    roadmap_config_declare_distance
        ("preferences", &RoadMapConfigWaypointSize, "125 ft");

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigTripRotate, "yes", "no", NULL);

}

/* File dialog support */

static int  roadmap_trip_load_file (const char *name, int silent, int merge);
static int  roadmap_trip_save_file (const char *name, int force);

static void roadmap_trip_file_dialog_ok
        (const char *filename, const char *mode) {

    if (mode[0] == 'w') {
        roadmap_trip_save_file (filename, 1);
    } else {
        roadmap_trip_load_file (filename, 0, 0);
    }
}

static void roadmap_trip_file_merge_dialog_ok
        (const char *filename, const char *mode) {

    roadmap_trip_load_file (filename, 0, 1);
}


static void roadmap_trip_file_dialog (const char *mode) {

    roadmap_fileselection_new ("RoadMap Trip", NULL,    /* no filter. */
                               roadmap_path_trips (),
                               mode, roadmap_trip_file_dialog_ok);
}

static void roadmap_trip_file_merge_dialog (const char *mode) {

    roadmap_fileselection_new ("RoadMap Trip Merge", NULL,    /* no filter. */
                               roadmap_path_trips (),
                               mode, roadmap_trip_file_merge_dialog_ok);
}



static int roadmap_trip_load_file (const char *name, int silent, int merge) {

    int ret;
    const char *path = NULL;
    queue tmp_waypoint_list, tmp_route_list, tmp_track_list;

    if (! roadmap_path_is_full_path (name))
        path = roadmap_path_trips ();

    roadmap_log (ROADMAP_DEBUG, "roadmap_trip_load_file '%s'", name);

    QUEUE_INIT(&tmp_waypoint_list);
    QUEUE_INIT(&tmp_route_list);
    QUEUE_INIT(&tmp_track_list);

    ret = roadmap_gpx_read_file (path, name, &tmp_waypoint_list,
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

    if (merge) {

        QUEUE_SPLICE(&RoadMapTripWaypointHead, &tmp_waypoint_list);
        QUEUE_SPLICE(&RoadMapTripRouteHead, &tmp_route_list);
        QUEUE_SPLICE(&RoadMapTripTrackHead, &tmp_track_list);
        roadmap_trip_set_modified(1);

    } else {

	// FIXME if we happen to be loading the file we're about
	// to save to, then we'll either a) clobber what we want
	// to load, or b) potentially lose data we wanted to save.
	// we default to saving, but this means if you load a trip,
	// delete some stuff, and reload to get it back, that won't
	// work.
	if (RoadMapTripModified) {
	    roadmap_trip_save (0);
	}
        roadmap_trip_clear();

        QUEUE_MOVE(&RoadMapTripWaypointHead, &tmp_waypoint_list);
        QUEUE_MOVE(&RoadMapTripRouteHead, &tmp_route_list);
        QUEUE_MOVE(&RoadMapTripTrackHead, &tmp_track_list);

        roadmap_config_set (&RoadMapConfigTripName,
            roadmap_path_skip_directories(name));
        roadmap_trip_set_modified(0);
    }


    RoadMapCurrentRoute = NULL;

    if (queue_count (&RoadMapTripRouteHead) +
            queue_count (&RoadMapTripTrackHead) > 1) {
        /* Multiple choices?  Present a dialog. */
        roadmap_trip_route_manage_dialog ();
    } else { /* If there's only one route (or track), use it. */
        if (!QUEUE_EMPTY(&RoadMapTripRouteHead)) {
            RoadMapCurrentRoute = 
                    (route_head *)QUEUE_FIRST(&RoadMapTripRouteHead);
        } else if (!QUEUE_EMPTY(&RoadMapTripTrackHead)) {
            RoadMapCurrentRoute = 
                    (route_head *)QUEUE_FIRST(&RoadMapTripTrackHead);
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
    roadmap_screen_refresh ();

    return ret;
}

int roadmap_trip_load (int silent, int merge) {
    int ret;
    const char *name = roadmap_trip_current();

    ret = roadmap_trip_load_file ( name, silent, merge);

    if (ret == 0) return 0;

    return ret;
}

int roadmap_trip_load_ask (int merge) {

    if (merge) {
        roadmap_trip_file_merge_dialog ("r");
    } else {
        roadmap_trip_file_dialog ("r");
    }
    return 0;

}

static int roadmap_trip_save_file (const char *name, int force) {

    const char *path = NULL;

    if (! roadmap_path_is_full_path (name)) {
        path = roadmap_path_trips ();
    }

    if (!force && !RoadMapTripModified) return 1;

    /* Always save if user-initiated. */
    roadmap_log (ROADMAP_DEBUG, "trip save_forced, or modified '%s'", name);

    return roadmap_gpx_write_file (path, name, &RoadMapTripWaypointHead,
            &RoadMapTripRouteHead, &RoadMapTripTrackHead);

}

int roadmap_trip_save (int force) {

    int ret;
    const char *name = roadmap_trip_current();

    ret = roadmap_trip_save_file ( name, force);

    return ret;
}

void roadmap_trip_save_as(int force) {
    roadmap_trip_file_dialog ("w");
}



void roadmap_trip_save_screenshot (void) {

    const char *extension = ".png";
    const char *path = roadmap_path_trips ();
    const char *name = roadmap_trip_current ();

    unsigned int total_length;
    unsigned int name_length;
    char *dot;
    char *picture_name;

    name_length = strlen (name);   /* Shorten it later. */

    dot = strrchr (name, '.');
    if (dot != NULL) {
        name_length -= strlen (dot);
    }

    total_length = name_length + strlen (path) + strlen (extension) + 12;
    picture_name = malloc (total_length);
    roadmap_check_allocated (picture_name);

    memcpy (picture_name, name, name_length);
    sprintf (picture_name, "%s/%*.*s-%010d%s",
             path,
             name_length, name_length, name, (int) time (NULL), extension);

    roadmap_canvas_save_screenshot (picture_name);
    free (picture_name);
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
    queue *elem, *tmp;
    int dropped = 0;
    int ac_dist = -1, ab_dist = -1, bc_dist = -1;

    QUEUE_FOR_EACH (&orig_route->waypoint_list, elem, tmp) {

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
    queue *elem, *tmp;

    QUEUE_FOR_EACH (&orig_route->waypoint_list, elem, tmp) {
	waypt_add(&new_route->waypoint_list, waypt_dupe( (waypoint *)elem ));
    }

}

static void roadmap_trip_route_convert_worker
        (route_head *orig_route, char *new_name, int simplify, int wanttrack) {

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

    roadmap_trip_route_convert_worker (RoadMapCurrentRoute, namep, 0, 0);

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

    roadmap_trip_route_convert_worker (RoadMapCurrentRoute, namep, 1, 0);

}

void roadmap_trip_currenttrack_to_route (void) {

    char name[40];
    time_t now;


    time(&now);
    strftime(name, sizeof(name), "Backtrack-%Y-%m-%d-%H:%M:%S",
                localtime(&now));

    roadmap_trip_route_convert_worker (RoadMapTrack, name, 0, 0);

}

void roadmap_trip_currenttrack_to_track (void) {

    char name[40];
    time_t now;


    time(&now);
    strftime(name, sizeof(name), "Track-%Y-%m-%d-%H:%M:%S",
                localtime(&now));

    roadmap_trip_route_convert_worker (RoadMapTrack, name, 0, 1);

}

