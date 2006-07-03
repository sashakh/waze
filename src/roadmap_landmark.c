/* roadmap_landmark.c - Personal waypoints
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
#include "roadmap_fileselection.h"
#include "roadmap_gps.h"
#include "roadmap_math.h"
#include "roadmap_screen.h"
#include "roadmap_sprite.h"
#include "roadmap_path.h"
#include "roadmap_gpx.h"
#include "roadmap_landmark.h"

queue RoadMapLandmarkHead;
static int RoadMapLandmarkModified;
static int RoadMapLandmarkRefresh;

static RoadMapConfigDescriptor RoadMapConfigLandmarkName =
                        ROADMAP_CONFIG_ITEM ("Landmarks", "Name");



static void roadmap_landmark_draw(const waypoint *waypointp)
{
    RoadMapGuiPoint guipoint;
    const char *sprite;
    if (waypointp->icon_descr == NULL) {
        sprite = "PersonalLandmark";
    } else {
        sprite = waypointp->icon_descr;
    }

    if (roadmap_math_point_is_visible (&waypointp->pos)) {

        roadmap_math_coordinate (&waypointp->pos, &guipoint);
        roadmap_math_rotate_coordinates (1, &guipoint);
        roadmap_sprite_draw (sprite, &guipoint, 0);
    }
}

void roadmap_landmark_display (void) {

    waypt_iterator (&RoadMapLandmarkHead, roadmap_landmark_draw);

}

int roadmap_landmark_count() {

    return queue_count(&RoadMapLandmarkHead);
}

void roadmap_landmark_add(waypoint *waypointp) {

    waypt_add(&RoadMapLandmarkHead, waypointp);
    RoadMapLandmarkModified = 1;
    RoadMapLandmarkRefresh = 1;
    roadmap_screen_refresh ();

}

void roadmap_landmark_remove(waypoint *waypointp) {

    waypt_del (waypointp);
    waypt_free (waypointp);
    RoadMapLandmarkModified = 1;
    RoadMapLandmarkRefresh = 1;
    roadmap_screen_refresh ();

}


queue * roadmap_landmark_list(void) {
    // hack -- if roadmap_trip asked for the list, it's possible
    // someone is doing an edit.  we don't get informed of the actual
    // edit, which is done generically.  
    RoadMapLandmarkModified = 1;
    return &RoadMapLandmarkHead;
}

int roadmap_landmark_is_refresh_needed (void) {

    if (RoadMapLandmarkRefresh) {
        RoadMapLandmarkRefresh = 0;
        return 1;
    }
    return 0;
}

static const char *roadmap_landmark_filename(int *defaulted) {
    const char *name;
    name = roadmap_config_get (&RoadMapConfigLandmarkName);
    if (!name[0]) {
        name = "landmarks.gpx";
        *defaulted = 1;
    } else {
        *defaulted = 0;
    }
    return name;
}


void roadmap_landmark_save(void) {

    const char *name;
    int defaulted, ret;

    if (!RoadMapLandmarkModified) return;

    name = roadmap_landmark_filename (&defaulted);

    ret = roadmap_gpx_write_waypoints(roadmap_path_user(), name,
            &RoadMapLandmarkHead);

    if (ret == 0) return;

    if (defaulted) {
        roadmap_config_set (&RoadMapConfigLandmarkName, name);
    }

    RoadMapLandmarkModified = 0;

}

static void roadmap_landmark_merge_file(const char *name) {

    const char *trip_path = NULL;
    queue tmp_waypoint_list;
    int ret;

    if (!name || !name[0]) return;

    if (! roadmap_path_is_full_path (name))
        trip_path = roadmap_path_trips ();


    QUEUE_INIT(&tmp_waypoint_list);

    ret = roadmap_gpx_read_waypoints(trip_path, name, &tmp_waypoint_list);

    if (ret == 0) {
        waypt_flush_queue (&tmp_waypoint_list);
        return;
    }

    QUEUE_SPLICE(&RoadMapLandmarkHead, &tmp_waypoint_list);

    RoadMapLandmarkModified = 1;
    RoadMapLandmarkRefresh = 1;

}

static void roadmap_landmark_merge_dialog_ok
        (const char *filename, const char *mode) {

    roadmap_landmark_merge_file (filename);
}


static void roadmap_landmark_merge_dialog (const char *mode) {

    roadmap_fileselection_new ("RoadMap Merge Landmarks", NULL,
                               roadmap_path_trips (),
                               mode, roadmap_landmark_merge_dialog_ok);
}

void roadmap_landmark_merge(void) {

    roadmap_landmark_merge_dialog ("r");
}

void roadmap_landmark_load(void) {

    const char *name;
    const char *path = roadmap_path_user();
    queue tmp_waypoint_list;
    int defaulted, ret;

    name = roadmap_landmark_filename (&defaulted);

    if ( ! roadmap_file_exists (path, name) && defaulted) return;

    QUEUE_INIT(&tmp_waypoint_list);

    ret = roadmap_gpx_read_waypoints(path, name, &tmp_waypoint_list);

    if (ret == 0) {
        waypt_flush_queue (&tmp_waypoint_list);
        return;
    }

    if (defaulted) {
        roadmap_config_set (&RoadMapConfigLandmarkName, name);
    }

    waypt_flush_queue (&RoadMapLandmarkHead);

    QUEUE_MOVE(&RoadMapLandmarkHead, &tmp_waypoint_list);

    RoadMapLandmarkModified = 0;
    RoadMapLandmarkRefresh = 1;

}

void
roadmap_landmark_initialize(void) {

    roadmap_config_declare
        ("preferences", &RoadMapConfigLandmarkName, "");

    QUEUE_INIT(&RoadMapLandmarkHead);

}
