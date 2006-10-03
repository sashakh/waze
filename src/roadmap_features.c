/* roadmap_features.c - GPX-based points of interest
 *
 * LICENSE:
 *
 *   Copyright 2006  Paul G. Fox
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
#include "roadmap_features.h"
#include "roadmap_label.h"
#include "roadmap_landmark.h"

#define MAX_FEATUREFILES 10  /* max number of feature/point-of-interest files */

static struct {
    RoadMapList head;
    RoadMapListItem *southern;
    RoadMapPen pen;
    char * sprite;
    int declutter;
    RoadMapArea area;
} RoadMapFeatureLists[MAX_FEATUREFILES], *RoadMapFeatureList;

static RoadMapConfigDescriptor RoadMapConfigFeatureFiles =
                        ROADMAP_CONFIG_ITEM ("FeatureFiles", "Path");


static void roadmap_features_draw(const waypoint *w) {

    if (roadmap_math_declutter(RoadMapFeatureList->declutter)) {
        roadmap_landmark_draw_waypoint
            (w, RoadMapFeatureList->sprite, 1, RoadMapFeatureList->pen);
    } else {
        /* above a certain zoom level, turn off labels, and draw only
         * the sprite.  if there was normally no sprite, provide one.
         */
        roadmap_landmark_draw_waypoint
            (w, RoadMapFeatureList->sprite ?
                RoadMapFeatureList->sprite  : "PointOfInterest", 1, NULL);
    }
}

static void roadmap_features_find_bounds(const waypoint *w) {

    if (w->pos.longitude > RoadMapFeatureList->area.east)
        RoadMapFeatureList->area.east = w->pos.longitude;
    if (w->pos.longitude < RoadMapFeatureList->area.west)
        RoadMapFeatureList->area.west = w->pos.longitude;

    if (w->pos.latitude > RoadMapFeatureList->area.north)
        RoadMapFeatureList->area.north = w->pos.latitude;
    if (w->pos.latitude < RoadMapFeatureList->area.south)
        RoadMapFeatureList->area.south = w->pos.latitude;
}

void roadmap_features_display (void) {

    RoadMapListItem *southern, *w;
    RoadMapArea screen;

    roadmap_math_screen_edges (&screen);

    for (RoadMapFeatureList = RoadMapFeatureLists;
          ROADMAP_LIST_FIRST(&RoadMapFeatureList->head) != NULL;
          RoadMapFeatureList++) {

        if (!roadmap_math_is_visible (&RoadMapFeatureList->area))
            continue;

        southern = RoadMapFeatureList->southern;

        if (!southern) {
            southern = ROADMAP_LIST_FIRST(&RoadMapFeatureList->head);
        }

        /* Assumes there are out-of-range "flag" waypoints at the 
         * start and end of the list */
        while (((waypoint *)southern)->pos.latitude >=
                            screen.south)
            southern = ROADMAP_LIST_PREV(southern);
        while (((waypoint *)southern)->pos.latitude <
                            screen.south)
            southern = ROADMAP_LIST_NEXT(southern);

        RoadMapFeatureList->southern = southern;
        w = southern;
        while (((waypoint *)w)->pos.latitude < screen.north) {
            roadmap_features_draw((waypoint *)w);
            w = ROADMAP_LIST_NEXT(w);
        }
    }

}

static int latitude_waypoint_cmp( RoadMapListItem *a, RoadMapListItem *b) {
    return ((waypoint *)a)->pos.latitude - ((waypoint *)b)->pos.latitude;
}

void roadmap_features_load(void) {

    char *name, *semi;
    RoadMapList tmp_waypoint_list;
    int ret;
    char pen_name[256];
    char *color, *sprite, *declutter;
    const char *p;


    /* the configuration for "features" files is a comma-separated path.
     * each comma-separated path element is itself a semi-colon separated
     * sequence of filename, spritename, label color, and zoom level
     * at which label should no longer be printed.  here's an example --
     * the line is broken for clarity.
     *
     * FeatureFiles.Path: &/vt_places.gpx;PointOfInterest;darkblue;550,\
     *  &/ca.place.gpx;PointOfInterest;blue;1300
     */

    p = roadmap_config_get(&RoadMapConfigFeatureFiles);
    if (!*p) return;

    roadmap_path_set("features", p);

    ROADMAP_LIST_INIT(&tmp_waypoint_list);

    RoadMapFeatureList = RoadMapFeatureLists;

    /* override const -- we know it's okay to tamper, since we
     * know the data is malloced
     */
    for (name = (char *)roadmap_path_first("features");
        name != NULL;
        name = (char *)roadmap_path_next("features", name)) {

        semi = strchr(name, ';');
        if (semi == name) continue;  /* null filename */
        if (semi) *semi = '\0';

        if ( ! roadmap_file_exists (NULL, name) ) {
            roadmap_log
                (ROADMAP_WARNING, "Features file %s missing", name);
            if (semi) *semi++ = ';';
            continue;
        }

        ret = roadmap_gpx_read_waypoints(NULL, name, &tmp_waypoint_list);

        if (semi) *semi++ = ';';

        if (ret == 0) {
            waypt_flush_queue (&tmp_waypoint_list);
            continue;
        }

        if (ROADMAP_LIST_EMPTY(&tmp_waypoint_list)) {
            continue;
        }

        ROADMAP_LIST_INIT(&RoadMapFeatureList->head);

        ROADMAP_LIST_MOVE
            (&RoadMapFeatureList->head, &tmp_waypoint_list);

        roadmap_list_sort(&RoadMapFeatureList->head, latitude_waypoint_cmp);

        /* default sprite and label pen */
        RoadMapFeatureList->sprite = "PointOfInterest";
        RoadMapFeatureList->pen = RoadMapLandmarksPen;
        RoadMapFeatureList->declutter = 550;

        if (semi && *semi) {  /* sprite info */

            sprite = semi;
            semi = strchr(sprite, ';');
            if (semi) *semi = '\0';

            if (*sprite) {
                if (strcmp(sprite,"NONE") != 0)
                    RoadMapFeatureList->sprite = strdup(sprite);
                else
                    RoadMapFeatureList->sprite = NULL;
            }

            if (semi) *semi++ = ';';

            if (semi && *semi) {  /* color info */

                color = semi;
                semi = strchr(color, ';');
                if (semi) *semi = '\0';

                if (*color) {
                    if (strcmp(color, "NONE") != 0) {
                        sprintf(pen_name, "features.%d",
                                RoadMapFeatureList - RoadMapFeatureLists );
                        RoadMapFeatureList->pen = roadmap_canvas_create_pen (pen_name);
                        roadmap_canvas_set_foreground(color);
                        roadmap_canvas_set_thickness (2);
                    } else {
                        RoadMapFeatureList->pen = NULL;
                    }
                }
                if (semi) *semi++ = ';';

                if (semi && *semi) {  /* declutter info */

                    declutter = semi;
                    semi = strchr(declutter, ';');
                    if (semi) *semi = '\0';

                    if (*declutter) {
                        int de;
                        de = atoi(declutter);
                        if (de != 0) {
                            RoadMapFeatureList->declutter = de;
                        }
                    }
                    if (semi) *semi++ = ';';
                }
            }
        }

        RoadMapFeatureList++;

        if (RoadMapFeatureList - RoadMapFeatureLists > MAX_FEATUREFILES - 1)
            break;

    }

    for (RoadMapFeatureList = RoadMapFeatureLists;
          ROADMAP_LIST_FIRST(&RoadMapFeatureList->head) != NULL;
          RoadMapFeatureList++) {

        waypoint *wpt;

        /* Calculate the list's bounding box
         * (since the list is sorted and we check latitude that way,
         * we don't really need the whole bounding box, but it's
         * simpler just to do all four sides the same.)
         */

        RoadMapFeatureList->area.west =  181 * 1000000;
        RoadMapFeatureList->area.east = -181 * 1000000;
        RoadMapFeatureList->area.north = -91 * 1000000;
        RoadMapFeatureList->area.south =  91 * 1000000;
        waypt_iterator
            (&RoadMapFeatureList->head, roadmap_features_find_bounds);

        /* Insert out-of-range waypoints at front and rear, to make
         * search algorithm simpler */
        wpt = waypt_new();
        wpt->pos.latitude =  -91 * 1000000;
        roadmap_list_insert(&RoadMapFeatureList->head, &(wpt->Q));

        wpt = waypt_new();
        wpt->pos.latitude =  91 * 1000000;
        roadmap_list_append(&RoadMapFeatureList->head, &(wpt->Q));

    }

}

void
roadmap_features_initialize(void) {

    roadmap_config_declare
        ("preferences", &RoadMapConfigFeatureFiles, "");

}
