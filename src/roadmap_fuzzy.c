/* roadmap_fuzzy.c - implement fuzzy operators for roadmap navigation.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_fuzzy.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_line.h"

#include "roadmap_fuzzy.h"


#define FUZZY_TRUTH_MAX 1024


static RoadMapConfigDescriptor RoadMapConfigAccuracyStreet =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Street");

static int RoadMapAccuracyStreet;
static int RoadMapError;


void roadmap_fuzzy_start_cycle (void) {

    RoadMapAccuracyStreet =
        roadmap_config_get_integer (&RoadMapConfigAccuracyStreet);
}


int roadmap_fuzzy_max_distance (void) {

    roadmap_fuzzy_start_cycle ();

    return RoadMapAccuracyStreet;
}


RoadMapFuzzy roadmap_fuzzy_direction (int direction, int reference) {

    int delta = (direction - reference) % 180;

    /* The membership function is symetrical around the zero point,
     * and each side is symetrical around the 90 point: use these
     * properties to fold the delta.
     */
    if (delta < 0) delta += 180;
    if (delta > 90) delta = 180 - delta;

    return (FUZZY_TRUTH_MAX * delta) / 90;
}


RoadMapFuzzy roadmap_fuzzy_distance  (int distance) {

    if (distance >= RoadMapAccuracyStreet) return 0;

    if (distance <= RoadMapError) return FUZZY_TRUTH_MAX;

    return (FUZZY_TRUTH_MAX * (distance - RoadMapError)) /
                                  RoadMapAccuracyStreet - RoadMapError;
}


RoadMapFuzzy roadmap_fuzzy_connected (int line, int reference) {

    /* The logic for the connection membership function is that
     * the line is the most connected to itself, is well connected
     * to lines to which it share a common point and is not well
     * connected to other lines.
     * The weight of each state is a matter fine tuning.
     */
    RoadMapPosition line_point[2];
    RoadMapPosition reference_point[2];

    int i, j;


    if (line == reference) return FUZZY_TRUTH_MAX;


    roadmap_line_from (line, &(line_point[0]));
    roadmap_line_to   (line, &(line_point[1]));

    roadmap_line_from (reference, &(reference_point[0]));
    roadmap_line_to   (reference, &(reference_point[1]));

    for (i = 0; i <= 1; ++i) {
        for (j = 0; j <= 1; ++j) {
            if ((line_point[i].latitude == line_point[j].latitude) &&
                (line_point[i].longitude == line_point[j].longitude)) {
                return (FUZZY_TRUTH_MAX * 2) / 3;
            }
        }
    }

    return FUZZY_TRUTH_MAX / 3;
}


RoadMapFuzzy roadmap_fuzzy_and (RoadMapFuzzy a, RoadMapFuzzy b) {

    if (a < b) return a;

    return b;
}


RoadMapFuzzy roadmap_fuzzy_or (RoadMapFuzzy a, RoadMapFuzzy b) {

    if (a > b) return a;

    return b;
}


RoadMapFuzzy roadmap_fuzzy_not (RoadMapFuzzy a) {

    return FUZZY_TRUTH_MAX - a;
}


void roadmap_fuzzy_initialize (void) {

    roadmap_config_declare
        ("preferences", &RoadMapConfigAccuracyStreet, "80");
}

