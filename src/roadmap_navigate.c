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
#include "roadmap_line_route.h"
#include "roadmap_square.h"
#include "roadmap_street.h"
#include "roadmap_layer.h"
#include "roadmap_display.h"
#include "roadmap_locator.h"
#include "roadmap_fuzzy.h"
#include "roadmap_adjust.h"

#include "roadmap_navigate.h"

static RoadMapConfigDescriptor RoadMapNavigateFlag =
                        ROADMAP_CONFIG_ITEM("Navigation", "Enable");

typedef struct {
   RoadMapNavigateRouteCB callbacks;
   PluginLine current_line;
   PluginLine next_line;
   int enabled;
   
} RoadMapNavigateRoute;

#define ROADMAP_NAVIGATE_ROUTE_NULL \
{ {0}, PLUGIN_LINE_NULL, PLUGIN_LINE_NULL, 0 }

static RoadMapNavigateRoute RoadMapRouteInfo;

/* The navigation flag will be set according to the session's content,
 * so that the user selected state is kept from one session to
 * the next.
 */
static int RoadMapNavigateEnabled = 0;

/* Avoid doing navigation work when the position has not changed. */
static RoadMapGpsPosition RoadMapLatestGpsPosition;
static RoadMapPosition    RoadMapLatestPosition;

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
        printf ("%s (plugin_id %d, line_id %d, street_id %d)\n",
      text,
      roadmap_plugin_get_id (line),
      roadmap_plugin_get_line_id (line),
      roadmap_plugin_get_street_id (&street));
    }
}


static void roadmap_navigate_adjust_focus
                (RoadMapArea *focus, const RoadMapGuiPoint *focused_point) {

    RoadMapPosition focus_position;

    roadmap_math_to_position (focused_point, &focus_position, 1);

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
               RoadMapNeighbour *neighbours, int max, int type) {

    int count = 0;
    int layers[128];
    int layer_count;

    RoadMapArea focus;

    RoadMapGuiPoint focus_point;
    RoadMapPosition focus_position;


    roadmap_log_push ("roadmap_navigate_retrieve_line");

    if (roadmap_math_point_is_visible (position)) {

       roadmap_math_coordinate (position, &focus_point);
       roadmap_math_rotate_coordinates (1, &focus_point);

       focus_point.x += accuracy;
       focus_point.y += accuracy;
       roadmap_math_to_position (&focus_point, &focus_position, 1);

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
       
    } else {

       accuracy *= 100;

       focus.west = position->longitude - accuracy;
       focus.east = position->longitude + accuracy;
       focus.north = position->latitude + accuracy;
       focus.south = position->latitude - accuracy;
    }

    if (type == LAYER_VISIBLE_ROADS) {
       layer_count = roadmap_layer_visible_roads (layers, 128);
    } else {
       layer_count = roadmap_layer_all_roads (layers, 128);
    }
    
    if (layer_count > 0) {

        roadmap_math_set_focus (&focus);

        count = roadmap_street_get_closest
                    (position, layers, layer_count, neighbours, max);

        count = roadmap_plugin_get_closest
                    (position, layers, layer_count, neighbours, count, max);

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


int roadmap_navigate_retrieve_line
        (const RoadMapPosition *position, int accuracy, PluginLine *line,
         int *distance, int type) {

    RoadMapNeighbour closest;

    if (roadmap_navigate_get_neighbours
           (position, accuracy, &closest, 1, type) <= 0) {

       return -1;
    }

    *distance = closest.distance;

    if (roadmap_plugin_activate_db (&closest.line) == -1) {
       return -1;
    }

    *line = closest.line;
    return 0;
}


int roadmap_navigate_fuzzify
                (RoadMapTracking *tracked,
                 RoadMapTracking *previous_street,
                 RoadMapNeighbour *previous_line,
                 RoadMapNeighbour *line,
                 int against_direction,
                 int direction) {

    RoadMapFuzzy fuzzyfied_distance;
    RoadMapFuzzy fuzzyfied_direction;
    RoadMapFuzzy fuzzyfied_direction_with_line = 0;
    RoadMapFuzzy fuzzyfied_direction_against_line = 0;
    RoadMapFuzzy connected;
    int line_direction = 0;
    int azymuth_with_line = 0;
    int azymuth_against_line = 0;
    int symetric = 0;

    if (!previous_street || !previous_street->valid) {
       previous_line = NULL;
    }

    fuzzyfied_distance = roadmap_fuzzy_distance (line->distance);

    if (! roadmap_fuzzy_is_acceptable (fuzzyfied_distance)) {
       return roadmap_fuzzy_false ();
    }

    line_direction =
       roadmap_plugin_get_direction (&line->line, ROUTE_CAR_ALLOWED);

    if ((line_direction == ROUTE_DIRECTION_NONE) ||
          (line_direction == ROUTE_DIRECTION_ANY)) {
       symetric = 1;
    } else if (against_direction) {
       if (line_direction == ROUTE_DIRECTION_WITH_LINE) {
          line_direction = ROUTE_DIRECTION_AGAINST_LINE;
       } else {
          line_direction = ROUTE_DIRECTION_WITH_LINE;
       }
    }

    if (symetric || (line_direction == ROUTE_DIRECTION_WITH_LINE)) {
       azymuth_with_line = roadmap_math_azymuth (&line->from, &line->to);
       fuzzyfied_direction_with_line =
          roadmap_fuzzy_direction (azymuth_with_line, direction, 0);

    }
    
    if (symetric || (line_direction == ROUTE_DIRECTION_AGAINST_LINE)) {
       azymuth_against_line = roadmap_math_azymuth (&line->to, &line->from);
       fuzzyfied_direction_against_line =
          roadmap_fuzzy_direction (azymuth_against_line, direction, 0);
    }
 
    if (fuzzyfied_direction_against_line >
          fuzzyfied_direction_with_line) {
       
       fuzzyfied_direction = fuzzyfied_direction_against_line;
       tracked->azymuth = azymuth_against_line;
       tracked->line_direction = ROUTE_DIRECTION_AGAINST_LINE;
    } else {

       fuzzyfied_direction = fuzzyfied_direction_with_line;
       tracked->azymuth = azymuth_with_line;
       tracked->line_direction = ROUTE_DIRECTION_WITH_LINE;
    }

    if (! roadmap_fuzzy_is_acceptable (fuzzyfied_direction)) {
       return roadmap_fuzzy_false ();
    }

    if (previous_line != NULL) {
        connected =
            roadmap_fuzzy_connected
              (line, previous_line, previous_street->line_direction,
               tracked->line_direction, &tracked->entry);
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
                                     RoadMapConfirmedStreet.azymuth);

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

    if (roadmap_plugin_same_line (line, &RoadMapConfirmedLine.line))
          return roadmap_fuzzy_false();

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
                    (roadmap_math_azymuth (&line_from, &line_to), direction, 1));
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

    int cfcc;
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
       
        for (cfcc = ROADMAP_ROAD_FIRST; cfcc <= ROADMAP_ROAD_LAST; ++cfcc) {

            if (roadmap_line_in_square
                (square, cfcc, &first_line, &last_line) > 0) {

                int line;
                for (line = first_line; line <= last_line; ++line) {

                    PluginLine p_line =
                        PLUGIN_MAKE_LINE (ROADMAP_PLUGIN_ID, line, cfcc, fips);

                    if (roadmap_plugin_override_line (line, cfcc, fips)) {
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
                           !roadmap_plugin_same_street
                              (&street,
                              &RoadMapConfirmedStreet.street)) {

                            *found = p_line;
                            best_match = match;
                        }
                    }
                }
            }

            if (roadmap_line_in_square2
                (square, cfcc, &first_line, &last_line) > 0) {

                int xline;

                for (xline = first_line; xline <= last_line; ++xline) {

                    int line = roadmap_line_get_from_index2 (xline);
                    PluginLine p_line =
                        PLUGIN_MAKE_LINE (ROADMAP_PLUGIN_ID, line, cfcc, fips);

                    if (roadmap_plugin_override_line (line, cfcc, fips)) {
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
                           !roadmap_plugin_same_street
                              (&street,
                               &RoadMapConfirmedStreet.street)) {

                            *found = p_line;
                            best_match = match;
                        }
                    }
                }
            }
        }
    }

    /* search plugin lines */
    count = roadmap_plugin_find_connected_lines
                  (&crossing,
                   plugin_lines,
                   sizeof(plugin_lines)/sizeof(*plugin_lines));

    for (i=0; i<count; i++) {
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

    if (!roadmap_plugin_same_line
          (found, &RoadMapConfirmedStreet.intersection)) {
       PluginStreet street;
       RoadMapConfirmedStreet.intersection = *found;

       if (!RoadMapRouteInfo.enabled) {
          roadmap_display_activate ("Approach", found, &crossing, &street);
       }
    }

    return 1;
}


void roadmap_navigate_locate (const RoadMapGpsPosition *gps_position) {

    int i;
    int found;
    int count;
    int candidate_in_route = 0;
    int nominated_in_route = 0;

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

    if (gps_position->speed < roadmap_gps_speed_accuracy()) return;

    RoadMapLatestGpsPosition = *gps_position;

    roadmap_fuzzy_start_cycle ();

    roadmap_adjust_position (gps_position, &RoadMapLatestPosition);

    if (RoadMapConfirmedStreet.valid) {

        /* We have an existing street match: check it is still valid. */

        RoadMapFuzzy before = RoadMapConfirmedStreet.fuzzyfied;
        RoadMapFuzzy current_fuzzy;
        int previous_direction = RoadMapConfirmedStreet.line_direction;

        if (roadmap_plugin_activate_db
                (&RoadMapConfirmedLine.line) == -1) {
            return;
        }

        if (!roadmap_plugin_get_distance
               (&RoadMapLatestPosition,
                &RoadMapConfirmedLine.line,
                &RoadMapConfirmedLine)) {

            current_fuzzy = 0;
        } else {

            current_fuzzy = roadmap_navigate_fuzzify
                              (&RoadMapConfirmedStreet,
                               &RoadMapConfirmedStreet,
                               &RoadMapConfirmedLine,
                               &RoadMapConfirmedLine,
                               0,
                               gps_position->steering);
        }

        if ((previous_direction == RoadMapConfirmedStreet.line_direction) &&
            ((current_fuzzy >= before) ||
            roadmap_fuzzy_is_certain(current_fuzzy))) {

            RoadMapConfirmedStreet.fuzzyfied = current_fuzzy;
   
            if (! roadmap_navigate_confirm_intersection (gps_position)) {
                PluginLine p_line;
                roadmap_navigate_find_intersection (gps_position, &p_line);
            }

            if (RoadMapRouteInfo.enabled) {

               RoadMapRouteInfo.callbacks.update
                           (&RoadMapLatestPosition,
                            &RoadMapConfirmedLine.line);
            }
            return; /* We are on the same street. */
        }
    }

    /* We must search again for the best street match. */

    count = roadmap_navigate_get_neighbours
                (&RoadMapLatestPosition, roadmap_fuzzy_max_distance(),
                 RoadMapNeighbourhood, ROADMAP_NEIGHBOURHOUD, LAYER_ALL_ROADS);

    for (i = 0, best = roadmap_fuzzy_false(), found = 0; i < count; ++i) {

        result = roadmap_navigate_fuzzify
                     (&candidate,
                      &RoadMapConfirmedStreet,
                      &RoadMapConfirmedLine,
                      RoadMapNeighbourhood+i,
                      0,
                      gps_position->steering);


        if (RoadMapRouteInfo.enabled &&
            (roadmap_plugin_same_line
              (&RoadMapNeighbourhood[i].line,
               &RoadMapRouteInfo.current_line) ||
              roadmap_plugin_same_line
              (&RoadMapNeighbourhood[i].line,
               &RoadMapRouteInfo.next_line))) {

           candidate_in_route = 1;
        } else {

           candidate_in_route = 0;
        }
        
        if ((result > best) ||
              (!nominated_in_route &&
               candidate_in_route &&
               roadmap_fuzzy_is_acceptable (result))) {

            if (nominated_in_route && !candidate_in_route) {
                /* Prefer one of the routing segments */

                continue;
            }

            found = i;
            best = result;
            nominated = candidate;
            nominated_in_route = candidate_in_route;
        }
    }

    if (roadmap_fuzzy_is_acceptable (best)) {

        PluginLine old_line = RoadMapConfirmedLine.line;

        if (roadmap_plugin_activate_db
                (&RoadMapNeighbourhood[found].line) == -1) {
            return;
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

        if (!roadmap_plugin_same_line
              (&RoadMapConfirmedLine.line, &old_line)) {
            if (PLUGIN_VALID(old_line)) {
                roadmap_navigate_trace
                    ("Quit street %N", &old_line);
            }
            roadmap_navigate_trace
                ("Enter street %N, %C|Enter street %N",
                 &RoadMapConfirmedLine.line);

            roadmap_display_hide ("Approach");

            if (RoadMapRouteInfo.enabled) {

               RoadMapRouteInfo.current_line = RoadMapConfirmedLine.line;

               RoadMapRouteInfo.callbacks.get_next_line
                  (&RoadMapConfirmedLine.line,
                   RoadMapConfirmedStreet.line_direction,
                   &RoadMapRouteInfo.next_line);
            }
        }

        if (gps_position->speed > roadmap_gps_speed_accuracy()) {
           PluginLine p_line;
           roadmap_navigate_find_intersection (gps_position, &p_line);
        }

    } else {

        if (PLUGIN_VALID(RoadMapConfirmedLine.line)) {

            if (roadmap_plugin_activate_db
                   (&RoadMapConfirmedLine.line) == -1)
                return;

            roadmap_navigate_trace ("Lost street %N",
                                    &RoadMapConfirmedLine.line);
            roadmap_display_hide ("Current Street");
            roadmap_display_hide ("Approach");
        }

        INVALIDATE_PLUGIN(RoadMapConfirmedLine.line);
        RoadMapConfirmedStreet.valid = 0;
    }

    if (RoadMapRouteInfo.enabled) {

       RoadMapRouteInfo.callbacks.update
          (&RoadMapLatestPosition,
           &RoadMapConfirmedLine.line);
    }
}


void roadmap_navigate_initialize (void) {

    roadmap_config_declare_enumeration
        ("session", &RoadMapNavigateFlag, "yes", "no", NULL);
}


void roadmap_navigate_route (RoadMapNavigateRouteCB callbacks) {

   RoadMapRouteInfo.callbacks = callbacks;

   if (RoadMapConfirmedStreet.valid) {

      RoadMapRouteInfo.current_line = RoadMapConfirmedLine.line;

      callbacks.get_next_line (&RoadMapConfirmedLine.line,
                               RoadMapConfirmedStreet.line_direction,
                               &RoadMapRouteInfo.next_line);

      callbacks.update (&RoadMapLatestPosition, &RoadMapConfirmedLine.line);
   }

   RoadMapRouteInfo.enabled = 1;
}


void roadmap_navigate_end_route (RoadMapNavigateRouteCB callbacks) {

   RoadMapRouteInfo.enabled = 0;
}


int roadmap_navigate_get_current (RoadMapGpsPosition *position,
                                  PluginLine *line,
                                  int *direction) {

   *position = RoadMapLatestGpsPosition;
   
   if (RoadMapConfirmedStreet.valid) {
      
      *line = RoadMapConfirmedLine.line;
      *direction = RoadMapConfirmedStreet.line_direction;
      return 0;
   } else {

      line->plugin_id = -1;
      *direction = ROUTE_DIRECTION_NONE;
      return -1;
   }
}


