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
#include "roadmap_message.h"
#include "roadmap_line.h"
#include "roadmap_square.h"
#include "roadmap_street.h"
#include "roadmap_trip.h"
#include "roadmap_layer.h"
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
    int street;
    int detected;

    int distance_from;
    int distance_to;

    RoadMapPosition from;
    RoadMapPosition to;

} RoadMapTracking;

#define ROADMAP_TRACKING_NULL  {-1, 0, -1, 0, 0, 0, {0, 0}, {0, 0}};

static RoadMapTracking RoadMapConfirmedStreet = ROADMAP_TRACKING_NULL;
static RoadMapTracking RoadMapCandidateStreet = ROADMAP_TRACKING_NULL;
static RoadMapTracking RoadMapCrossingStreet = ROADMAP_TRACKING_NULL;


struct roadmap_navigate_rectangle {
    
    int west;
    int east;
    int north;
    int south;
};


static void roadmap_navigate_trace (const char *format, int line) {
    
#ifdef DEBUG
    char text[1024];
    RoadMapStreetProperties properties;
    
    roadmap_street_get_properties (line, &properties);
    
    roadmap_message_set
        ('#', roadmap_street_get_street_address (&properties));
    roadmap_message_set
        ('N', roadmap_street_get_street_name (&properties));
    roadmap_message_set
        ('C', roadmap_street_get_city_name (&properties));

    text[0] = 0;
    if (roadmap_message_format (text, sizeof(text), format)) {
        printf (text);
    }
    printf ("(line %d, street %d)\n", line, properties.street);
#endif
}


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


static const RoadMapPosition *roadmap_navigate_next_crossing
                                    (const RoadMapPosition *position) {

    const RoadMapPosition *crossing = NULL;

    int delta;
    int minimum = RoadMapAccuracyStreet / 3;
    int distance_from =
            roadmap_math_distance (position, &RoadMapConfirmedStreet.from);
    int distance_to =
            roadmap_math_distance (position, &RoadMapConfirmedStreet.to);


    if (distance_from <
            RoadMapConfirmedStreet.distance_from - minimum) {
        if (distance_to > RoadMapConfirmedStreet.distance_to) {
            crossing = &RoadMapConfirmedStreet.from;
        }
    } else if (distance_to <
            RoadMapConfirmedStreet.distance_to - minimum) {
        if (distance_from > RoadMapConfirmedStreet.distance_from) {
            crossing = &RoadMapConfirmedStreet.to;
        }
    }
    
    delta = distance_from - RoadMapConfirmedStreet.distance_from;
    if (delta > minimum || delta < 0 - minimum) {
        RoadMapConfirmedStreet.distance_from = distance_from;
    }
    
    delta = distance_to - RoadMapConfirmedStreet.distance_to;
    if (delta > minimum || delta < 0 - minimum) {
        RoadMapConfirmedStreet.distance_to = distance_to;
    }
    
    return crossing;
}


static int roadmap_navigate_next_intersection
                (const RoadMapPosition *crossing, int cfcc) {

    int i;
    int line;
    int square;
    int first_line;
    int last_line;

    int line_found[8];
    int line_count = 0;

    struct {
        int street;
        int line;
    } found[8];
    int count = 0;

    RoadMapPosition  line_end;

    RoadMapStreetProperties properties;


    square = roadmap_square_search (crossing);

    if (roadmap_line_in_square (square, cfcc, &first_line, &last_line) > 0) {

        for (line = first_line; line <= last_line; ++line) {

            if (line == RoadMapConfirmedStreet.line) continue;

            roadmap_line_from (line, &line_end);
            if ((line_end.latitude == crossing->latitude) &&
                (line_end.longitude == crossing->longitude)) {
                line_found[line_count++] = line;
                continue;
            }

            roadmap_line_to (line, &line_end);
            if ((line_end.latitude == crossing->latitude) &&
                (line_end.longitude == crossing->longitude)) {
                line_found[line_count++] = line;
                continue;
            }
        }
    }

    if (roadmap_line_in_square2 (square, cfcc, &first_line, &last_line) > 0) {

        int xline;

        for (xline = first_line; xline <= last_line; ++xline) {

            line = roadmap_line_get_from_index2 (xline);
            if (line == RoadMapConfirmedStreet.line) continue;

            roadmap_line_from (line, &line_end);
            if ((line_end.latitude == crossing->latitude) &&
                (line_end.longitude == crossing->longitude)) {
                line_found[line_count++] = line;
                continue;
            }

            roadmap_line_to (line, &line_end);
            if ((line_end.latitude == crossing->latitude) &&
                (line_end.longitude == crossing->longitude)) {
                line_found[line_count++] = line;
                continue;
            }
        }
    }

    if (line_count <= 0) return 0;

    while (line_count > 0) {
        
        line = line_found[--line_count];
        
        roadmap_street_get_properties (line, &properties);
        
        if (properties.street <= 0) continue;
        if (properties.street == RoadMapConfirmedStreet.street) continue;
            
        for (i = 0; i < count; ++i) {
            if (found[i].street == properties.street) break;
        }
        if (i >= count) {
            found[count].street = properties.street;
            found[count].line = line;
            count += 1;
        }
    }

    if (count <= 0) return 0;
    

    if (found[0].street != RoadMapCrossingStreet.street) {

        roadmap_navigate_trace
            ("found crossing %N, %C|found street %N", found[0].line);

        RoadMapCrossingStreet.street = found[0].street;
        RoadMapCrossingStreet.detected = 1;
    }
    
    if (RoadMapCrossingStreet.detected == 1) {
        roadmap_navigate_trace
            ("announce crossing %N, %C|announce crossing %N", found[0].line);
        roadmap_display_activate ("Approach", found[0].line, crossing);
        
        RoadMapCrossingStreet.detected += 1;
    }

    return 1;
}


static int roadmap_navigate_update
                (RoadMapTracking *tracked, const RoadMapPosition *position) {

    int distance;


    if (tracked->line < 0) {
        return 0;
    }
    if (roadmap_locator_activate (tracked->fips) != ROADMAP_US_OK) {
        goto invalidate;
    }
    
    /* Confirm the current street if we are still at a "short"
     * distance from it. This is to avoid switching streets
     * randomly at the intersections.
     */
    distance =
        roadmap_math_get_distance_from_segment
                    (position, &tracked->from, &tracked->to);
            
    if (distance > RoadMapAccuracyStreet) {
        goto invalidate; /* We went far from this line. */
    }

    /* This line has been confirmed again. */
    
    return 1;
    
invalidate:
    
    tracked->line = -1;
    tracked->detected = 0;
    
    return 0;
}

void roadmap_navigate_locate (const RoadMapPosition *position) {
    
    int fips;
    int distance = RoadMapAccuracyStreet + 1;


    RoadMapAccuracyStreet =
        roadmap_config_get_integer (&RoadMapConfigAccuracyStreet);
    RoadMapAccuracyMouse =
        roadmap_config_get_integer (&RoadMapConfigAccuracyMouse);


    roadmap_navigate_update (&RoadMapCandidateStreet, position);
    
    if (roadmap_navigate_update (&RoadMapConfirmedStreet, position)) {

        int cfcc;
        const RoadMapPosition *crossing;

        crossing = roadmap_navigate_next_crossing (position);

        if (crossing == NULL) return;

        for (cfcc = ROADMAP_ROAD_FIRST; cfcc <= ROADMAP_ROAD_LAST; ++cfcc) {
            
            if (roadmap_navigate_next_intersection (crossing, cfcc)) {
                break;
            }
        }
        
        return; /* No need to look for a new line. */
    }

    if (RoadMapCandidateStreet.line < 0) {

        /* The previous lines, if any, are not valid suitors anymore.
         * Look around for another one. The closest line will be
         * selected only if it is close enough.
         */
        int line = roadmap_navigate_retrieve_line
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

            roadmap_line_from (RoadMapCandidateStreet.line,
                               &RoadMapCandidateStreet.from);
            roadmap_line_to   (RoadMapCandidateStreet.line,
                               &RoadMapCandidateStreet.to);
    
            RoadMapCandidateStreet.detected = 1;
        }
    }


    /* We have found a suitable candidate line. Wait for a confirmation
     * before we commit to an announcement.
     */
    if (RoadMapCandidateStreet.detected > 1) {

        int confirm =
                roadmap_config_get_integer (&RoadMapConfigAccuracyConfirm);
            
        if (RoadMapCandidateStreet.detected >= confirm) {

            RoadMapConfirmedStreet = RoadMapCandidateStreet;
            RoadMapConfirmedStreet.distance_to = 0;
            RoadMapConfirmedStreet.distance_from = 0;

            RoadMapConfirmedStreet.street =
                roadmap_display_activate
                    ("Current Street", RoadMapConfirmedStreet.line, NULL);

            roadmap_navigate_trace
                ("confirmed street %N, %C|confirmed street %N",
                 RoadMapConfirmedStreet.line);

            /* Crossing street will be different, now. */

            roadmap_navigate_next_crossing (position);
            
            roadmap_display_hide ("Approach");

            RoadMapCrossingStreet.line = -1;
            RoadMapCrossingStreet.street = -1;
            RoadMapCrossingStreet.detected = 0;

            /* Not a candidate anymore. */

            RoadMapCandidateStreet.line = -1;
            RoadMapCandidateStreet.detected = 0;
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
