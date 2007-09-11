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


/*
 * The trackpoints are stored in a list, like all other
 * waypoints.  We save the list to the "currenttrack" file on
 * exit, and read an initial list from it on startup.
 *
 * Periodically in between, we save the list entire to disk.  If
 * the disk file would become too big, we save an initial chunk
 * of the list to an archive file, and save the rest to the
 * currenttrack file.
 *
 * To protect against trackpoint loss from a crash that migh
 * occur in between the periodic saves, we write individual
 * trackpoints to a small CSV file.  On startup, if this file
 * exists, we appendit to what we got from teh currenttrack file.
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
#include "roadmap_messagebox.h"

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
                        ROADMAP_CONFIG_ITEM("Track", "Saved Track Points");
static RoadMapConfigDescriptor RoadMapConfigAutoSaveInterval =
                        ROADMAP_CONFIG_ITEM("Track", "Autosave Minutes");
static RoadMapConfigDescriptor RoadMapConfigTrackName =
                        ROADMAP_CONFIG_ITEM ("Track", "Name");


#define RECENT_CSV "recenttrack.csv"
#define ARCHIVED_GPX "savetrack" /* ".gpx" */

FILE *RoadMapTrackRecentCSV;

void roadmap_track_add_recent (waypoint *w) {

    if (!RoadMapTrackRecentCSV) {
        RoadMapTrackRecentCSV = roadmap_file_fopen
                (roadmap_path_user(), RECENT_CSV, "a");
    }
    fprintf(RoadMapTrackRecentCSV, "%ld, %d, %d, %d, %d, %d\n",
            w->creation_time,
            w->pos.longitude,
            w->pos.latitude,
            w->altitude,
            w->speed,
            w->course);

    fflush(RoadMapTrackRecentCSV);
}

void roadmap_track_fetch_recent(const char *path, char *name) {

    char trkpoint[128];
    time_t tim;
    int lon, lat, alt, speed, course;
    waypoint *w;

    /* read .csv file, add contents to RoadMapTrack */
    if ( roadmap_file_exists(path, name)) {

        RoadMapTrackRecentCSV = roadmap_file_fopen (path, name, "r");
        if (!RoadMapTrackRecentCSV) return;
       
        while (fgets(trkpoint, sizeof(trkpoint), RoadMapTrackRecentCSV)) {
            if (sscanf(trkpoint, "%ld, %d, %d, %d, %d, %d",
                    &tim, &lon, &lat, &alt, &speed, &course) != 6) {
                continue;
            }
            w = waypt_new();
            w->creation_time = tim;
            w->pos.latitude = lat;
            w->pos.longitude = lon;
            w->altitude = alt;
            w->speed = speed;
            w->course = course;
            route_add_wpt_tail (RoadMapTrack, w);
            RoadMapTrackModified = 1;
        }

        /* the csv file will be removed and closed at the end of
         * the autosave routine.
         */
    }
}


static void roadmap_track_add_trackpoint ( int gps_time,
                    const RoadMapGpsPosition *gps_position) {

    static time_t lastsave = -1;
    time_t now;
    int autosaveminutes;
    waypoint *w;
    
    w = waypt_new ();
    w->pos.latitude = gps_position->latitude;
    w->pos.longitude = gps_position->longitude;

    /* Change whatever we have to millimeters. */
    w->altitude = 10 * roadmap_math_to_cm (gps_position->altitude);

    /* We have knots, but want centimeters per second. */
    w->speed = gps_position->speed * 51.4444444;

    /* Want hundredths of degrees */
    w->course = gps_position->steering * 100.0;

    w->creation_time = gps_time;

    /* append to the list */
    route_add_wpt_tail (RoadMapTrack, w);

    /* and write a new line to the interim .csv file */
    roadmap_track_add_recent(w);

    RoadMapTrackGpsPosition = *gps_position;

    RoadMapTrackModified = 1;

    autosaveminutes = roadmap_config_get_integer
                                (&RoadMapConfigAutoSaveInterval);
    time(&now);
    if (lastsave < 0) lastsave = now;
    if (now - autosaveminutes * 60 > lastsave) {
        roadmap_track_autowrite();
        lastsave = now;
    }


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


static void roadmap_track_gps_update (int reception, int gps_time,
                   const RoadMapGpsPrecision *dilution,
                   const RoadMapGpsPosition *gps_position) {

    int need_point = 0;
    int ab_dist, ac_dist, bc_dist;
    RoadMapTrackPolicy policy;
    RoadMapPosition pos[3];
    waypoint *w;

    if (reception <= GPS_RECEPTION_NONE) return;

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

static int roadmap_track_save_worker
            (const char *path, const char *prefix, route_head *track) {
    char fullname[80];
    char basename[50];
    int prefixlen;
    waypoint *w;
    int ret;

    prefixlen = sprintf(basename, "%s", prefix);

    w = (waypoint *) ROADMAP_LIST_LAST (&track->waypoint_list);
    
    strftime(&basename[prefixlen], sizeof(basename) - prefixlen,
                "-%Y-%m-%d-%H-%M-%S-UTC", gmtime(&w->creation_time));

    track->rte_name = basename;

    sprintf(fullname, "%s%s", basename, ".gpx");

    ret = roadmap_gpx_write_track(0, path, fullname, track);

    track->rte_name = NULL;

    return ret;
}

void roadmap_track_save(void) {

    if (RoadMapTrack == NULL ||
        ROADMAP_LIST_EMPTY(&RoadMapTrack->waypoint_list)) {
        roadmap_messagebox ("Note", "No current track data -- nothing saved.");
        return;
    }
    roadmap_track_save_worker(roadmap_path_trips(), ARCHIVED_GPX, RoadMapTrack);
    RoadMapTrackModified = 0;
}

static void roadmap_track_autosave(int hiwater, int lowater) {

    const char *name;
    const char *path = roadmap_path_user();
    const char *trippath = roadmap_path_trips();
    int defaulted, ret;

    if (!RoadMapTrackModified) return;

    name = roadmap_track_filename (&defaulted);
    if (name == NULL) return;

    if (RoadMapTrack->rte_waypt_ct > hiwater) {

        route_head *archive = route_head_alloc();

        while (RoadMapTrack->rte_waypt_ct > lowater) {
                waypoint *w;
                w = (waypoint *)roadmap_list_remove
                        (ROADMAP_LIST_FIRST (&RoadMapTrack->waypoint_list));
                RoadMapTrack->rte_waypt_ct--;

                roadmap_list_append ( &archive->waypoint_list, &w->Q);
                archive->rte_waypt_ct++;
        }

        ret = roadmap_track_save_worker(trippath, ARCHIVED_GPX, archive);
        if (ret == 0) {  /* write failed -- restore points */
            /* this looks backwards, due to SPLICE's append behavior */
            ROADMAP_LIST_SPLICE
                (&archive->waypoint_list, &RoadMapTrack->waypoint_list);
            archive->rte_waypt_ct += RoadMapTrack->rte_waypt_ct;

            ROADMAP_LIST_MOVE(&RoadMapTrack->waypoint_list, &archive->waypoint_list);
            RoadMapTrack->rte_waypt_ct = archive->rte_waypt_ct;
            archive->rte_waypt_ct = 0;
        } else {
            route_free(archive);
        }
    }

    ret = roadmap_gpx_write_track(0, path, name, RoadMapTrack);
    if (ret == 0) return;

    /* we don't need the "recent points" cache anymore */
    if (RoadMapTrackRecentCSV) {
        fclose(RoadMapTrackRecentCSV);
        RoadMapTrackRecentCSV = NULL;
    }
    roadmap_file_remove (path, RECENT_CSV);

    if (defaulted) {
        roadmap_config_set (&RoadMapConfigTrackName, name);
    }

    RoadMapTrackModified = 0;

}

void roadmap_track_reset (void) {

    RoadMapTrackModified = 1; /* otherwise autosave does nothing */
    roadmap_track_autosave(0, 0);
    RoadMapTrackRefresh = 1;
    roadmap_screen_refresh();
}



void roadmap_track_autowrite(void) {

    int nominal;
    
    /* essentially, if we aquire 25% more points than we've been
     * asked to save, we tell the autowriter to archive all but
     * that 75% of what we've been asked for, and continue from
     * there.
     */
    nominal = roadmap_config_get_integer (&RoadMapConfigLength);
    roadmap_track_autosave (5 * nominal / 4, 3 * nominal / 4);

}

void roadmap_track_autoload(void) {

    const char *name;
    const char *path = roadmap_path_user();
    int defaulted, ret;

    name = roadmap_track_filename (&defaulted);
    if (name == NULL) return;

    if ( roadmap_file_exists(path, name)) {

        ret = roadmap_gpx_read_one_track(path, name, &RoadMapTrack);
        if (ret == 0) return;

        if (defaulted) {
            roadmap_config_set (&RoadMapConfigTrackName, name);
        }
    }

    /* there may be "recent" trackpoints even if no currenttrack file */
    if ( roadmap_file_exists(path, RECENT_CSV) ) {
        roadmap_track_fetch_recent(path, RECENT_CSV);
        roadmap_track_autowrite();
    }

    RoadMapTrackModified = 0;

}

void
roadmap_track_initialize(void) {

    roadmap_config_declare
        ("preferences", &RoadMapConfigAutoSaveInterval, "10"); /* minutes */

    roadmap_config_declare
        ("preferences", &RoadMapConfigLength, "1000");

    roadmap_config_declare
        ("preferences", &RoadMapConfigTrackName, "");

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

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigDisplayString,
            "on", "off", NULL);

    RoadMapTrack = route_head_alloc();
    roadmap_gps_register_listener(roadmap_track_gps_update);
}

void
roadmap_track_activate(void) {

    RoadMapTrackDisplay = roadmap_config_match
                                (&RoadMapConfigDisplayString, "on");
}
