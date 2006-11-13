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
#include "roadmap_label.h"

RoadMapList RoadMapLandmarkHead;
static int RoadMapLandmarkModified;
static int RoadMapLandmarkRefresh;
RoadMapPen RoadMapLandmarksPen;

/* create backup files when writing? */
extern int RoadMapTripMakeBackups;

static RoadMapConfigDescriptor RoadMapConfigLandmarkName =
                        ROADMAP_CONFIG_ITEM ("Landmarks", "Name");

static RoadMapConfigDescriptor RoadMapConfigLandmarksColor =
                        ROADMAP_CONFIG_ITEM("Landmarks", "Color");

/* in roadmap_trip.c */
extern RoadMapConfigDescriptor RoadMapConfigBackupFiles;


void roadmap_landmark_set_modified(void) {
    RoadMapLandmarkModified = 1;
}


#define ROADMAP_LANDMARK_LABEL_SIZE 18

void roadmap_landmark_draw_waypoint
        (const waypoint *waypointp,
            const char *sprite, int override_sprite, RoadMapPen pen) {

    RoadMapGuiPoint guipoint;

    if (!roadmap_math_point_is_visible (&waypointp->pos))
        return;

#if USE_ICON_NAME
    if ( !override_sprite && waypointp->icon_descr) {
        sprite = waypointp->icon_descr;
    }
#endif

    if (!sprite && !pen)
        return;  /* unlikely */

    roadmap_math_coordinate (&waypointp->pos, &guipoint);
    roadmap_math_rotate_coordinates (1, &guipoint);

    if (sprite) {
        roadmap_sprite_draw (sprite, &guipoint, 0);
    }

    if (pen) {
        roadmap_canvas_select_pen (pen);
        if (sprite)
            guipoint.y += 15; /* space for sprite */

        /* FIXME -- We should do label collision detection, which
         * means joining in the fun in roadmap_label_add() and
         * roadmap_label_draw_cache().  Landmark labels should
         * probably take priority, and be positioned first.  They
         * should probably be drawn last, however, so that their
         * labels come out on "top" of other map features.
         */
        roadmap_label_draw_text(waypointp->shortname,
           &guipoint, &guipoint, 0, 0, ROADMAP_LANDMARK_LABEL_SIZE);
    }
}

void roadmap_landmark_draw_weepoint
        (const weepoint *weepointp, const char *sprite, RoadMapPen pen) {

    RoadMapGuiPoint guipoint;

    if (!roadmap_math_point_is_visible (&weepointp->pos))
        return;

    if (!sprite && !pen)
        return;  /* unlikely */

    roadmap_math_coordinate (&weepointp->pos, &guipoint);
    roadmap_math_rotate_coordinates (1, &guipoint);

    if (sprite) {
        roadmap_sprite_draw (sprite, &guipoint, 0);
    }

    if (pen) {
        roadmap_canvas_select_pen (pen);
        if (sprite)
            guipoint.y += 15; /* space for sprite */

        /* FIXME -- We should do label collision detection, which
         * means joining in the fun in roadmap_label_add() and
         * roadmap_label_draw_cache().  Landmark labels should
         * probably take priority, and be positioned first.  They
         * should probably be drawn last, however, so that their
         * labels come out on "top" of other map features.
         */
        roadmap_label_draw_text(weepointp->name,
           &guipoint, &guipoint, 0, 0, ROADMAP_LANDMARK_LABEL_SIZE);
    }
}

static void roadmap_landmark_draw(const waypoint *waypointp) {
    roadmap_landmark_draw_waypoint
        (waypointp, "PersonalLandmark", 0, RoadMapLandmarksPen);
}

void roadmap_landmark_display (void) {

    waypt_iterator (&RoadMapLandmarkHead, roadmap_landmark_draw);

}

int roadmap_landmark_count() {

    return roadmap_list_count(&RoadMapLandmarkHead);
}

void roadmap_landmark_add(waypoint *waypointp) {

    waypt_add(&RoadMapLandmarkHead, waypointp);
    roadmap_landmark_set_modified();
    RoadMapLandmarkRefresh = 1;
    roadmap_screen_refresh ();

}

waypoint *roadmap_landmark_remove(waypoint *waypointp) {

    waypt_del (waypointp);

    roadmap_landmark_set_modified();
    RoadMapLandmarkRefresh = 1;
    roadmap_screen_refresh ();

    return waypointp;

}


RoadMapList * roadmap_landmark_list(void) {

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

    ret = roadmap_gpx_write_waypoints
            (roadmap_config_match (&RoadMapConfigBackupFiles, "yes"),
             roadmap_path_user(), name,
             &RoadMapLandmarkHead);

    if (ret == 0) return;

    if (defaulted) {
        roadmap_config_set (&RoadMapConfigLandmarkName, name);
    }

    RoadMapLandmarkModified = 0;

}

static int alpha_waypoint_cmp( RoadMapListItem *a, RoadMapListItem *b) {
    return strcasecmp(((waypoint *)a)->shortname, ((waypoint *)b)->shortname);
}

static void roadmap_landmark_merge_file(const char *name) {

    const char *trip_path = NULL;
    RoadMapList tmp_waypoint_list;
    int ret;

    if (!name || !name[0]) return;

    if (! roadmap_path_is_full_path (name))
        trip_path = roadmap_path_trips ();


    ROADMAP_LIST_INIT(&tmp_waypoint_list);

    ret = roadmap_gpx_read_waypoints(trip_path, name, &tmp_waypoint_list, 0);

    if (ret == 0) {
        waypt_flush_queue (&tmp_waypoint_list);
        return;
    }

    ROADMAP_LIST_SPLICE(&RoadMapLandmarkHead, &tmp_waypoint_list);

    roadmap_list_sort(&RoadMapLandmarkHead, alpha_waypoint_cmp);

    roadmap_landmark_set_modified();
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
    RoadMapList tmp_waypoint_list;
    int defaulted, ret;

    RoadMapLandmarksPen = roadmap_canvas_create_pen ("landmarks.labels");
    roadmap_canvas_set_foreground
        (roadmap_config_get (&RoadMapConfigLandmarksColor));

    roadmap_canvas_set_thickness (2);

    name = roadmap_landmark_filename (&defaulted);

    if ( ! roadmap_file_exists (path, name) && defaulted) return;

    ROADMAP_LIST_INIT(&tmp_waypoint_list);

    ret = roadmap_gpx_read_waypoints(path, name, &tmp_waypoint_list, 0);

    if (ret == 0) {
        waypt_flush_queue (&tmp_waypoint_list);
        return;
    }

    if (defaulted) {
        roadmap_config_set (&RoadMapConfigLandmarkName, name);
    }

    waypt_flush_queue (&RoadMapLandmarkHead);

    ROADMAP_LIST_MOVE(&RoadMapLandmarkHead, &tmp_waypoint_list);

    roadmap_list_sort(&RoadMapLandmarkHead, alpha_waypoint_cmp);

    RoadMapLandmarkModified = 0;
    RoadMapLandmarkRefresh = 1;

}

void
roadmap_landmark_initialize(void) {

    roadmap_config_declare
        ("preferences", &RoadMapConfigLandmarkName, "");

    roadmap_config_declare
       ("preferences", &RoadMapConfigLandmarksColor,  "darkred");


    ROADMAP_LIST_INIT(&RoadMapLandmarkHead);

}
