/* roadmap_track.c - Keep track of where we've been.
 *
 * LICENSE:
 *
 *   Copyright 2005  Paul G. Fox
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

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_config.h"
#include "roadmap_gps.h"
#include "roadmap_math.h"
#include "roadmap_screen.h"
#include "roadmap_sprite.h"
#include "roadmap_path.h"
#include "roadmap_gpx.h"
#include "roadmap_track.h"

route_head *RoadMapTrack = NULL;
static int RoadMapTrackModified;
static int RoadMapTrackDisplay;
static int RoadMapTrackRefresh;
RoadMapGpsPosition RoadMapTrackGpsPosition;

typedef enum {
    RoadMapTrackPolicyOff,
    RoadMapTrackPolicyTime,
    RoadMapTrackPolicyDistance,
    RoadMapTrackPolicyDeviation
} RoadMapTrackPolicy;


static RoadMapConfigDescriptor RoadMapConfigPolicyString =
                        ROADMAP_CONFIG_ITEM("Track", "Policy");
static RoadMapConfigDescriptor RoadMapConfigTimeInterval =
                        ROADMAP_CONFIG_ITEM("Track", "Time Delta");
static RoadMapConfigDescriptor RoadMapConfigDistanceInterval =
                        ROADMAP_CONFIG_ITEM("Track", "Distance Delta");
static RoadMapConfigDescriptor RoadMapConfigDisplayString =
                        ROADMAP_CONFIG_ITEM("Track", "Initial Display");
static RoadMapConfigDescriptor RoadMapConfigLength =
                        ROADMAP_CONFIG_ITEM("Track", "Length");
static RoadMapConfigDescriptor RoadMapConfigTrackName =
                        ROADMAP_CONFIG_ITEM ("Track", "Name");




static void roadmap_track_add_trackpoint ( int gps_time,
                    const RoadMapGpsPosition *gps_position) {

    int maxtrack;
    waypoint *w;
    w = waypt_new ();

    w->pos.latitude = gps_position->latitude;
    w->pos.longitude = gps_position->longitude;

    /* Change whatever we have to meters. */
    w->altitude = roadmap_math_to_cm (gps_position->altitude) / 100.0;

    /* We have knots, but want meters per second. */
    w->speed = gps_position->speed / 1.94384449;
    w->course = gps_position->steering;
    w->creation_time = gps_time;

    route_add_wpt_tail (RoadMapTrack, w);

    RoadMapTrackGpsPosition = *gps_position;

    maxtrack = roadmap_config_get_integer (&RoadMapConfigLength);
    while (RoadMapTrack->rte_waypt_ct > maxtrack) {
        w = (waypoint *) ROADMAP_LIST_FIRST (&RoadMapTrack->waypoint_list);
        route_del_wpt ( RoadMapTrack, w);
    }

    RoadMapTrackModified = 1;
}

static RoadMapTrackPolicy roadmap_track_policy(void)
{
    const char *policy = roadmap_config_get (&RoadMapConfigPolicyString);
    switch (policy[0]) {
        case 'o':
        case 'O': 
            return RoadMapTrackPolicyOff;

        case 'T':
        case 't': 
            return RoadMapTrackPolicyTime;

        case 'D':
        case 'd': 
            switch(policy[1]) {
                case 'I':
                case 'i': 
                    return RoadMapTrackPolicyDistance;
                case 'E':
                case 'e': 
                    return RoadMapTrackPolicyDeviation;
            }
    }

    roadmap_log (ROADMAP_FATAL, "Invalid track policy %s", policy);

    return RoadMapTrackPolicyOff;

}


static void roadmap_track_gps_update (int gps_time,
                   const RoadMapGpsPrecision *dilution,
                   const RoadMapGpsPosition *gps_position) {

    int need_point = 0;
    int ab_dist, ac_dist, bc_dist;
    RoadMapTrackPolicy policy;
    RoadMapPosition pos[3];
    waypoint *w;

    policy = roadmap_track_policy();

    if (policy == RoadMapTrackPolicyOff) return;
   
    if ( ROADMAP_LIST_EMPTY(&RoadMapTrack->waypoint_list) ) {
        roadmap_track_add_trackpoint(gps_time, gps_position);
        return;
    }

    if (gps_position->speed == 0) return;
   
    pos[0].latitude = gps_position->latitude;
    pos[0].longitude = gps_position->longitude;
    w = (waypoint *)ROADMAP_LIST_LAST (&RoadMapTrack->waypoint_list);
    pos[1].latitude = w->pos.latitude;
    pos[1].longitude = w->pos.longitude;

    if ((pos[0].latitude == pos[1].latitude) &&
        (pos[0].longitude == pos[1].longitude)) {
         return;
    }


    ab_dist = roadmap_math_distance ( &pos[0], &pos[1]);

    switch(policy) {

    case RoadMapTrackPolicyOff:
        return;

    case RoadMapTrackPolicyTime:
        need_point = ((ab_dist > 10) &&
                (roadmap_config_get_integer (&RoadMapConfigTimeInterval) <
                        (gps_time - w->creation_time)));
        break;

    case RoadMapTrackPolicyDistance:
        need_point = ( ab_dist > 
            roadmap_config_get_integer (&RoadMapConfigDistanceInterval) );
   
        break;

    case RoadMapTrackPolicyDeviation:
        if (RoadMapTrack->rte_waypt_ct > 2) {
            w = (waypoint *)ROADMAP_LIST_PREV(&w->Q);
            pos[2].latitude = w->pos.latitude;
            pos[2].longitude = w->pos.longitude;

            /* The current most recent three points are A, B, and C.
             * Drop point B if dist(A,B)+dist(B,C) is approximately
             * equal to dist (A,C).
             */
            bc_dist = roadmap_math_distance (&pos[1], &pos[2]);
            ac_dist = roadmap_math_distance (&pos[0], &pos[2]);

            if (abs (ab_dist + bc_dist - ac_dist) < 10) {
                w = (waypoint *) ROADMAP_LIST_LAST (&RoadMapTrack->waypoint_list);
                route_del_wpt ( RoadMapTrack, w);
            }
        }

        need_point = 1;
        break;

    }

    if (need_point) {
        roadmap_track_add_trackpoint(gps_time, gps_position);
        RoadMapTrackRefresh = 1;

    }

}


static void roadmap_track_waypoint_draw (const waypoint *waypointp)
{
    RoadMapGuiPoint guipoint;

    /* Don't draw the trackpoint if it coincides with our most recent
     * GPS position.  Otherwise, at least in "deviation" mode, we get
     * a continuously moving trackpoint, right under the tip of the
     * GPS arrow.
     */
    if (roadmap_math_point_is_visible (&waypointp->pos) &&
        (RoadMapTrackGpsPosition.latitude != waypointp->pos.latitude ||
         RoadMapTrackGpsPosition.longitude != waypointp->pos.longitude) ) {

        roadmap_math_coordinate (&waypointp->pos, &guipoint);
        roadmap_math_rotate_coordinates (1, &guipoint);
        roadmap_sprite_draw ("BreadCrumb", &guipoint, 0);
    }

}


void roadmap_track_display (void) {

    if (RoadMapTrackDisplay) {
        route_waypt_iterator (RoadMapTrack, roadmap_track_waypoint_draw);
    }

}

void roadmap_track_toggle_display(void) {

    RoadMapTrackDisplay = ! RoadMapTrackDisplay;
    RoadMapTrackRefresh = 1;
    roadmap_screen_refresh();

}

int roadmap_track_is_refresh_needed (void) {

    if (RoadMapTrackRefresh) {
        RoadMapTrackRefresh = 0;
        return 1;
    }
    return 0;
}

void roadmap_track_clear (void) {

    route_free(RoadMapTrack);
    RoadMapTrack = route_head_alloc();
    RoadMapTrackRefresh = 1;
    RoadMapTrackModified = 1;
    roadmap_screen_refresh();
}

void roadmap_track_save(void) {
    char name[40];
    time_t now;

    time(&now);
    strftime(name, sizeof(name), "Track-%Y-%m-%d-%H-%M-%S.gpx",
                localtime(&now));

    roadmap_gpx_write_track(roadmap_path_trips(), name, RoadMapTrack);
    RoadMapTrackModified = 0;

}

static const char *roadmap_track_filename(int *defaulted) {
    const char *name;
    name = roadmap_config_get (&RoadMapConfigTrackName);
    if (!name[0]) {
        name = "currenttrack.gpx";
        *defaulted = 1;
    } else {
        if (strcasecmp(name, "NONE") == 0) {
            return NULL;
        }
        *defaulted = 0;
    }
    return name;
}

void roadmap_track_autosave(void) {

    const char *name;
    int defaulted, ret;

    if (!RoadMapTrackModified) return;

    name = roadmap_track_filename (&defaulted);
    if (name == NULL) return;

    ret = roadmap_gpx_write_track(roadmap_path_user(), name, RoadMapTrack);

    if (ret == 0) return;

    if (defaulted) {
        roadmap_config_set (&RoadMapConfigTrackName, name);
    }

    RoadMapTrackModified = 0;

}

void roadmap_track_autoload(void) {

    const char *name;
    const char *path = roadmap_path_user();
    int defaulted, ret;

    name = roadmap_track_filename (&defaulted);
    if (name == NULL) return;

    if (! roadmap_file_exists(path, name)) return; /* not an error. */

    ret = roadmap_gpx_read_one_track(path, name, &RoadMapTrack);

    if (ret == 0) return;

    if (defaulted) {
        roadmap_config_set (&RoadMapConfigTrackName, name);
    }


    RoadMapTrackModified = 0;

}

void
roadmap_track_initialize(void) {

    /* We have to declare both the time and distance intervals,
     * though only one is used at a time.
     */
    roadmap_config_declare
        ("preferences", &RoadMapConfigTimeInterval, "15");

    roadmap_config_declare_distance
        ("preferences", &RoadMapConfigDistanceInterval, "100 m");

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigPolicyString,
            "off", "Deviation", "Distance", "Time", NULL);

    roadmap_config_declare
        ("preferences", &RoadMapConfigLength, "1000");

    roadmap_config_declare
        ("preferences", &RoadMapConfigTrackName, "");

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigDisplayString,
            "on", "off", NULL);


    RoadMapTrack = route_head_alloc();
    roadmap_gps_register_listener(roadmap_track_gps_update);

    RoadMapTrackDisplay = roadmap_config_match
                                (&RoadMapConfigDisplayString, "on");
}
