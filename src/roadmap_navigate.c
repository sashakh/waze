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

#include <string.h>

#include "roadmap.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_message.h"
#include "roadmap_line.h"
#include "roadmap_square.h"
#include "roadmap_street.h"
#include "roadmap_layer.h"
#include "roadmap_display.h"
#include "roadmap_locator.h"
#include "roadmap_fuzzy.h"
#include "roadmap_adjust.h"

#include "roadmap_navigate.h"



typedef struct {
    RoadMapFuzzy direction;
    RoadMapFuzzy distance;
    RoadMapFuzzy connected;
} RoadMapDebug;

static RoadMapConfigDescriptor RoadMapNavigateFlag =
                        ROADMAP_CONFIG_ITEM("Navigation", "Enable");

/* The navigation flag will be set according to the session's content,
 * so that the user selected state is kept from one session to
 * the next.
 */
static int RoadMapNavigateEnabled = 0;

static int RoadMapCarMode;


/* Avoid doing navigation work when the position has not changed. */
static RoadMapGpsPosition RoadMapLatestGpsPosition;
static RoadMapPosition    RoadMapLatestPosition;

typedef struct {

    int valid;
    PluginStreet street;

    int direction;

    RoadMapFuzzy fuzzyfied;

    PluginLine intersection;

    RoadMapPosition entry;

    RoadMapDebug debug;

} RoadMapTracking;

#define ROADMAP_TRACKING_NULL  {0, PLUGIN_STREET_NULL, 0, 0, PLUGIN_LINE_NULL, {0, 0}, {0, 0, 0}};

#define ROADMAP_NEIGHBOURHOUD  16

static RoadMapTracking  RoadMapConfirmedStreet = ROADMAP_TRACKING_NULL;

static RoadMapNeighbour RoadMapConfirmedLine = ROADMAP_NEIGHBOUR_NULL;
static RoadMapNeighbour RoadMapNeighbourhood[ROADMAP_NEIGHBOURHOUD];


static void roadmap_navigate_trace (const char *format, PluginLine *line) {
    
    char text[1024];
    PluginStreet street;
    PluginStreetProperties properties;
    
    if (! roadmap_log_enabled (ROADMAP_DEBUG)) return;

    roadmap_plugin_get_street (line, &street);

    roadmap_plugin_get_street_properties (line, &properties);
    
    roadmap_message_set ('#', properties.address);
    roadmap_message_set ('N', properties.street);
    roadmap_message_set ('C', properties.city);

    text[0] = 0;
    if (roadmap_message_format (text, sizeof(text), format)) {
        printf ("%s (plugin %d, line %d, street %d)\n",
                text,
                roadmap_plugin_get_id(line),
                roadmap_plugin_get_line_id (line),
                roadmap_plugin_get_street_id (&street));
    }
}


static void roadmap_navigate_adjust_focus
                (RoadMapArea *focus, const RoadMapGuiPoint *focused_point) {

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


static int roadmap_navigate_get_neighbours
              (const RoadMapPosition *position, int accuracy,
               RoadMapNeighbour *neighbours, int max) {

    int count;
    int layers[128];

    RoadMapArea focus;

    RoadMapGuiPoint focus_point;
    RoadMapPosition focus_position;


    roadmap_log_push ("roadmap_navigate_get_neighbours");

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

    count = roadmap_layer_navigable (RoadMapCarMode, layers, 128);
    
    if (count > 0) {

        roadmap_math_set_focus (&focus);

        count = roadmap_street_get_closest
                    (position, layers, count, neighbours, max);

        roadmap_math_release_focus ();
    }

    roadmap_log_pop ();

    return count;
}


void roadmap_navigate_disable (void) {
    
    RoadMapNavigateEnabled = 0;
    roadmap_display_hide ("Approach");
    roadmap_display_hide ("Current Street");

    roadmap_config_set (&RoadMapNavigateFlag, "no");
}


void roadmap_navigate_enable  (void) {
    
    RoadMapNavigateEnabled  = 1;

    roadmap_config_set (&RoadMapNavigateFlag, "yes");
}


int roadmap_navigate_retrieve_line (const RoadMapPosition *position,
                                    int accuracy,
                                    PluginLine *line,
                                    int *distance) {

    RoadMapNeighbour closest;

    if (roadmap_navigate_get_neighbours
           (position, accuracy, &closest, 1) <= 0) {

       return -1;
    }

    *distance = closest.distance;

    if (roadmap_plugin_activate_db (&closest.line) != ROADMAP_US_OK) {
       return -1;
    }
    *line = closest.line;
    return 0;
}



static int roadmap_navigate_fuzzify
                (RoadMapTracking *tracked,
                 RoadMapNeighbour *line, int direction) {

    RoadMapFuzzy fuzzyfied_distance;
    RoadMapFuzzy fuzzyfied_direction;
    RoadMapFuzzy connected;

    fuzzyfied_distance = roadmap_fuzzy_distance (line->distance);

    if (! roadmap_fuzzy_is_acceptable (fuzzyfied_distance)) {
       return roadmap_fuzzy_false ();
    }

    tracked->direction = roadmap_math_azymuth (&line->from, &line->to);

    fuzzyfied_direction =
        roadmap_fuzzy_direction (tracked->direction, direction, 1);

    if (! roadmap_fuzzy_is_acceptable (fuzzyfied_direction)) {
       return roadmap_fuzzy_false ();
    }

    if (RoadMapConfirmedStreet.valid) {
        connected =
            roadmap_fuzzy_connected
                (line, &RoadMapConfirmedLine, &tracked->entry);
    } else {
        connected = roadmap_fuzzy_not (0);
    }

    tracked->debug.direction = fuzzyfied_direction;
    tracked->debug.distance  = fuzzyfied_distance;
    tracked->debug.connected = connected;

    return roadmap_fuzzy_and
               (connected,
                roadmap_fuzzy_and (fuzzyfied_distance, fuzzyfied_direction));
}


static int roadmap_navigate_confirm_intersection
                             (const RoadMapGpsPosition *position) {

    int delta;

    if (!PLUGIN_VALID(RoadMapConfirmedStreet.intersection)) return 0;

    delta = roadmap_math_delta_direction (position->steering,
                                          RoadMapConfirmedStreet.direction);

    return (delta < 90 && delta > -90);
}


/* Check if the given street block is a possible intersection and
 * evaluate how good an intersection guess it would be.
 * The criteria for a good intersection is that it must be as far
 * as possible from matching our current direction.
 */
static RoadMapFuzzy roadmap_navigate_is_intersection
                        (PluginLine *line,
                         int direction,
                         const RoadMapPosition *crossing) {

    RoadMapPosition line_to;
    RoadMapPosition line_from;

    if (roadmap_plugin_same_line (line, &RoadMapConfirmedLine.line)) {
       return roadmap_fuzzy_false();
    }

    roadmap_plugin_line_to   (line, &line_to);
    roadmap_plugin_line_from (line, &line_from);

    if ((line_from.latitude != crossing->latitude) ||
        (line_from.longitude != crossing->longitude)) {

        if ((line_to.latitude != crossing->latitude) ||
            (line_to.longitude != crossing->longitude)) {

            return roadmap_fuzzy_false();
        }
    }

    return roadmap_fuzzy_not
               (roadmap_fuzzy_direction
                    (roadmap_math_azymuth (&line_from, &line_to),
                     direction,
                     1));
}


/* Find the next intersection point.
 * We compare the current direction with the azimuth to the two ends of
 * the closest segment of line. From there, we deduct which end of the line
 * we are going to. Note that we do not consider the azimuth toward the ends
 * of the line, as the line might be a curve, and the result might be quite
 * confusing (imagine a mountain road..).
 * Once we know where we are going, we select a street that intersects
 * this point and seems the best guess as an intersection.
 */
static int roadmap_navigate_find_intersection
                  (const RoadMapGpsPosition *position, PluginLine *found) {

    int layer;
    int square;
    int first_line;
    int last_line;

    int delta_to;
    int delta_from;
    RoadMapPosition crossing;

    RoadMapFuzzy match;
    RoadMapFuzzy best_match;

    RoadMapStreetProperties properties;

    int fips = roadmap_locator_active ();

    PluginLine plugin_lines[100];
    int count;
    int i;


    delta_from =
        roadmap_math_delta_direction
            (position->steering,
             roadmap_math_azymuth (&RoadMapLatestPosition,
                                   &RoadMapConfirmedLine.from));

    delta_to =
        roadmap_math_delta_direction
            (position->steering,
             roadmap_math_azymuth (&RoadMapLatestPosition,
                                   &RoadMapConfirmedLine.to));

    if (delta_to < delta_from - 30) {
       roadmap_plugin_line_to (&RoadMapConfirmedLine.line, &crossing);
    } else if (delta_from < delta_to - 30) {
       roadmap_plugin_line_from (&RoadMapConfirmedLine.line, &crossing);
    } else {
       return 0; /* Not sure enough. */
    }

    /* Should we compare to RoadMapConfirmedStreet.entry so
     * to detect U-turns ?
     */

    /* Now we know which endpoint we are headed toward.
     * Let find if there is any street block that represents a valid
     * intersection street. We select the one that matches the best.
     */
    best_match = roadmap_fuzzy_false();
    square = roadmap_square_search (&crossing);

    if (square != -1) {

       int i;
       int layers[128];
       int count = roadmap_layer_navigable (RoadMapCarMode, layers, 128);

       for (i = 0; i < count; ++i) {

          layer = layers[i];

          if (roadmap_line_in_square
                  (square, layer, &first_line, &last_line) > 0) {

             int line;

             for (line = first_line; line <= last_line; ++line) {

                PluginLine p_line =
                   PLUGIN_MAKE_LINE (ROADMAP_PLUGIN_ID, line, layer, fips);

                if (roadmap_plugin_override_line (line, layer, fips)) {
                   continue;
                }

                match = roadmap_navigate_is_intersection
                            (&p_line, position->steering, &crossing);

                if (best_match < match) {

                   PluginStreet street;

                   roadmap_street_get_properties (line, &properties);

                   roadmap_plugin_set_street
                      (&street, ROADMAP_PLUGIN_ID, properties.street);

                    if ((properties.street > 0) &&
                        (! roadmap_plugin_same_street
                               (&street, &RoadMapConfirmedStreet.street))) {

                        *found = p_line;
                        best_match = match;
                    }
                }
             }
          }

          if (roadmap_line_in_square2
                (square, layer, &first_line, &last_line) > 0) {

             int xline;

             for (xline = first_line; xline <= last_line; ++xline) {

                int line = roadmap_line_get_from_index2 (xline);
                PluginLine p_line =
                   PLUGIN_MAKE_LINE (ROADMAP_PLUGIN_ID, line, layer, fips);

                if (roadmap_plugin_override_line (line, layer, fips)) {
                   continue;
                }

                match = roadmap_navigate_is_intersection
                           (&p_line, position->steering, &crossing);

                if (best_match < match) {

                   PluginStreet street;

                   roadmap_street_get_properties (line, &properties);

                   roadmap_plugin_set_street
                      (&street, ROADMAP_PLUGIN_ID, properties.street);

                   if ((properties.street > 0) &&
                         (! roadmap_plugin_same_street
                          (&street, &RoadMapConfirmedStreet.street))) {

                      *found = p_line;
                      best_match = match;
                   }
                }
             }
          }
       }
    }

    /* Search among the plugin lines. */

    count = roadmap_plugin_find_connected_lines
                  (&crossing,
                   plugin_lines,
                   sizeof(plugin_lines)/sizeof(*plugin_lines));

    for (i=0; i<count; ++i) {

        PluginStreet street;

        match = roadmap_navigate_is_intersection
                    (&plugin_lines[i], position->steering, &crossing);

        if (best_match < match) {

            roadmap_plugin_get_street (&plugin_lines[i], &street);

            if (!roadmap_plugin_same_street
                  (&street,
                   &RoadMapConfirmedStreet.street)) {

                *found = plugin_lines[i];
                best_match = match;
            }
        }
    }


    if (! roadmap_fuzzy_is_acceptable (best_match)) {
        roadmap_display_hide ("Current Street");
        return 0;
    }

    roadmap_navigate_trace
        ("Announce crossing %N, %C|Announce crossing %N", found);

    if (! roadmap_plugin_same_line
             (found, &RoadMapConfirmedStreet.intersection)) {
       PluginStreet street;
       RoadMapConfirmedStreet.intersection = *found;
       roadmap_display_activate ("Approach", found, &crossing, &street);
    }

    return 1;
}


void roadmap_navigate_locate (const RoadMapGpsPosition *gps_position) {

    int i;
    int found;
    int count;

    RoadMapFuzzy best;
    RoadMapFuzzy result;

    static RoadMapTracking candidate;
    static RoadMapTracking nominated;


    if (! RoadMapNavigateEnabled) {
       
       if (strcasecmp
             (roadmap_config_get (&RoadMapNavigateFlag), "yes") == 0) {
          RoadMapNavigateEnabled = 1;
       } else {
          return;
       }
    }

    if ((RoadMapLatestGpsPosition.latitude == gps_position->latitude) &&
        (RoadMapLatestGpsPosition.longitude == gps_position->longitude)) return;

    RoadMapLatestGpsPosition = *gps_position;

    if (gps_position->speed < roadmap_gps_speed_accuracy()) return;


    roadmap_fuzzy_start_cycle ();

    roadmap_adjust_position (gps_position, &RoadMapLatestPosition);

    if (RoadMapConfirmedStreet.valid) {

        /* We have an existing street match: check it is still valid. */

        RoadMapFuzzy before = RoadMapConfirmedStreet.fuzzyfied;

        if (roadmap_plugin_activate_db
                (&RoadMapConfirmedLine.line) == -1) {
            return;
        }
        roadmap_plugin_get_distance (&RoadMapLatestPosition,
                                     &RoadMapConfirmedLine.line,
                                     &RoadMapConfirmedLine);

        if (roadmap_navigate_fuzzify
                (&RoadMapConfirmedStreet,
                 &RoadMapConfirmedLine, gps_position->steering) >= before) {

            if (! roadmap_navigate_confirm_intersection (gps_position)) {
                PluginLine p_line;
                roadmap_navigate_find_intersection (gps_position, &p_line);
            }

            return; /* We are on the same street. */
        }
    }

    /* We must search again for the best street match. */

    count = roadmap_navigate_get_neighbours
                (&RoadMapLatestPosition, roadmap_fuzzy_max_distance(),
                 RoadMapNeighbourhood, ROADMAP_NEIGHBOURHOUD);

    for (i = 0, best = roadmap_fuzzy_false(), found = 0; i < count; ++i) {

        result = roadmap_navigate_fuzzify
                     (&candidate, RoadMapNeighbourhood+i,
                      gps_position->steering);

        if (result > best) {
            found = i;
            best = result;
            nominated = candidate;
        }
    }

    if (roadmap_fuzzy_is_acceptable (best)) {

        if (roadmap_plugin_activate_db
               (&RoadMapNeighbourhood[found].line) == -1) {
            return;
        }

        if (! roadmap_plugin_same_line
                  (&RoadMapConfirmedLine.line,
                   &RoadMapNeighbourhood[found].line)) {

            if (PLUGIN_VALID(RoadMapConfirmedLine.line)) {
                roadmap_navigate_trace
                    ("Quit street %N", &RoadMapConfirmedLine.line);
            }
            roadmap_navigate_trace
                ("Enter street %N, %C|Enter street %N",
                 &RoadMapNeighbourhood[found].line);

            roadmap_display_hide ("Approach");
        }

        RoadMapConfirmedLine   = RoadMapNeighbourhood[found];
        RoadMapConfirmedStreet = nominated;

        RoadMapConfirmedStreet.valid = 1;
        RoadMapConfirmedStreet.fuzzyfied = best;
        INVALIDATE_PLUGIN(RoadMapConfirmedStreet.intersection);

        roadmap_display_activate
           ("Current Street",
            &RoadMapConfirmedLine.line,
            NULL,
            &RoadMapConfirmedStreet.street);

        if (gps_position->speed > roadmap_gps_speed_accuracy()) {
           PluginLine p_line;
           roadmap_navigate_find_intersection (gps_position, &p_line);
        }

    } else {

        if (PLUGIN_VALID(RoadMapConfirmedLine.line)) {

            if (roadmap_plugin_activate_db
                   (&RoadMapConfirmedLine.line) == -1) {
                return;
            }

            roadmap_navigate_trace ("Lost street %N",
                                    &RoadMapConfirmedLine.line);
            roadmap_display_hide ("Current Street");
            roadmap_display_hide ("Approach");
        }

        INVALIDATE_PLUGIN(RoadMapConfirmedLine.line);
        RoadMapConfirmedStreet.valid = 0;
    }
}


void roadmap_navigate_initialize (void) {

    RoadMapCarMode = roadmap_layer_declare_navigation_mode ("Car");

    roadmap_config_declare_enumeration
        ("session", &RoadMapNavigateFlag, "yes", "no", NULL);
}

