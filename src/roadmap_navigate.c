/* roadmap_navigate.c - Basic navigation engine for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2003 Pascal F. Martin
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
 *   See roadmap_navigate.h.
 */

#include "roadmap.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_line.h"
#include "roadmap_layer.h"
#include "roadmap_street.h"
#include "roadmap_display.h"
#include "roadmap_locator.h"

#include "roadmap_navigate.h"


static RoadMapConfigDescriptor RoadMapConfigAccuracyStreet =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Street");

static RoadMapConfigDescriptor RoadMapConfigAccuracyConfirm =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Confirm");

static RoadMapConfigDescriptor RoadMapConfigAccuracyMouse =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Mouse");

/* Avoid asking these too often. */
static int RoadMapAccuracyMouse;
static int RoadMapAccuracyStreet;

typedef struct {

    int line;
    int fips;
    int detected;

} RoadMapTracking;

static RoadMapTracking RoadMapConfirmedStreet = {-1, 0, 0};
static RoadMapTracking RoadMapCandidateStreet = {-1, 0, 0};


struct roadmap_navigate_rectangle {
    
    int west;
    int east;
    int north;
    int south;
};


static void roadmap_navigate_adjust_focus
                (struct roadmap_navigate_rectangle *focus,
                 const RoadMapGuiPoint *focused_point) {

    RoadMapPosition focus_position;

    roadmap_math_to_position (focused_point, &focus_position);

    if (focus_position.longitude < focus->west) {
        focus->west = focus_position.longitude;
    }
    if (focus_position.longitude > focus->east) {
        focus->east = focus_position.longitude;
    }
    if (focus_position.latitude < focus->south) {
        focus->south = focus_position.latitude;
    }
    if (focus_position.latitude > focus->north) {
        focus->north = focus_position.latitude;
    }
}


int roadmap_navigate_retrieve_line
        (const RoadMapPosition *position, int accuracy, int *distance) {

    int line;
    int count = 0;
    int layers[128];

    struct roadmap_navigate_rectangle focus;

    RoadMapGuiPoint focus_point;
    RoadMapPosition focus_position;


    roadmap_math_coordinate (position, &focus_point);
    roadmap_math_rotate_coordinates (1, &focus_point);

    focus_point.x += accuracy;
    focus_point.y += accuracy;
    roadmap_math_to_position (&focus_point, &focus_position);

    focus.west = focus_position.longitude;
    focus.east = focus_position.longitude;
    focus.north = focus_position.latitude;
    focus.south = focus_position.latitude;

    accuracy *= 2;
    
    focus_point.x -= accuracy;
    roadmap_navigate_adjust_focus (&focus, &focus_point);

    focus_point.y -= accuracy;
    roadmap_navigate_adjust_focus (&focus, &focus_point);

    focus_point.x += accuracy;
    roadmap_navigate_adjust_focus (&focus, &focus_point);

#ifdef DEBUG
printf ("Position: %d longitude, %d latitude\n",
        position.longitude,
        position.latitude);
fflush(stdout);
#endif

    count = roadmap_layer_visible_roads (layers, 128);
    
    if (count > 0) {

        roadmap_math_set_focus
            (focus.west, focus.east, focus.north, focus.south);

        line = roadmap_street_get_closest
                    (position, count, layers, distance);

        roadmap_math_release_focus ();

        return line;
    }
    
    return -1;
}


static int roadmap_navigate_update (const RoadMapPosition *position) {

    int distance;
    RoadMapPosition position1, position2;


    RoadMapAccuracyStreet =
        roadmap_config_get_integer (&RoadMapConfigAccuracyStreet);
    RoadMapAccuracyMouse =
        roadmap_config_get_integer (&RoadMapConfigAccuracyMouse);


    if (RoadMapConfirmedStreet.line < 0) {
        return -1;
    }
    if (roadmap_locator_activate
            (RoadMapConfirmedStreet.fips) != ROADMAP_US_OK) {
        RoadMapConfirmedStreet.line = -1;
        return -1;
    }
    
    /* Confirm the current street if we are still at a "short"
     * distance from it. This is to avoid switching streets
     * randomly at the intersections.
     */
    roadmap_line_from (RoadMapConfirmedStreet.line, &position1);
    roadmap_line_to   (RoadMapConfirmedStreet.line, &position2);
                
    distance =
        roadmap_math_get_distance_from_segment
                    (position, &position1, &position2);
            
    if (distance > RoadMapAccuracyStreet) {
        RoadMapConfirmedStreet.line = -1; /* We left this line. */
        return -1;
    }

    /* This line has been confirmed again. */
    
    RoadMapCandidateStreet.detected = 0; /* Candidate lost, whoever. */

    return RoadMapConfirmedStreet.line;
}

void roadmap_navigate_locate (const RoadMapPosition *position) {
    
    int line;
    int fips;
    int distance;


    if (roadmap_navigate_update (position) >= 0) {
        return; /* No need to look for a new line. */
    }
            
    /* The previous line, if any, is not a valid suitor anymore.
     * Look around for another one. The closest line will be
     * selected only if it is close enough.
     */
    line = roadmap_navigate_retrieve_line
                (position, RoadMapAccuracyMouse, &distance);

    if ((line < 0) || (distance > RoadMapAccuracyStreet)){

        /* There is no candidate line here. */
        RoadMapCandidateStreet.line = -1;
        RoadMapCandidateStreet.detected = 0;
        return;
    }
    
    fips = roadmap_locator_active ();
    
    if (line != RoadMapCandidateStreet.line ||
        fips != RoadMapCandidateStreet.fips) {

        /* This is a new candidate line: restart from scratch. */
        RoadMapCandidateStreet.line = line;
        RoadMapCandidateStreet.fips = fips;
        RoadMapCandidateStreet.detected = 1;
    }
        
            
    /* We have found a suitable line. Wait for a confirmation
     * before we commit to an announcement.
     */
    if (RoadMapCandidateStreet.detected > 1) {
                
        int confirm =
                roadmap_config_get_integer (&RoadMapConfigAccuracyConfirm);
            
        if (RoadMapCandidateStreet.detected == confirm) {
                
            RoadMapConfirmedStreet = RoadMapCandidateStreet;
                
            roadmap_display_activate
                ("Current Street", line, distance, NULL);
            
            RoadMapCandidateStreet.detected = 0; /* Not candidate anymore. */
            return;
        }
    }
    RoadMapCandidateStreet.detected += 1;
}


void roadmap_navigate_initialize (void) {
    
    roadmap_config_declare
        ("preferences", &RoadMapConfigAccuracyStreet, "200");
    roadmap_config_declare
        ("preferences", &RoadMapConfigAccuracyConfirm, "3");
}
