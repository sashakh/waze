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
#include "roadmap_locator.h"

#include "roadmap_fuzzy.h"


#define FUZZY_TRUTH_MAX 1024


static RoadMapConfigDescriptor RoadMapConfigConfidence =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Confidence");

static RoadMapConfigDescriptor RoadMapConfigAccuracyStreet =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Street");

static int RoadMapAccuracyStreet;
static int RoadMapConfidence;
static int RoadMapError;


void roadmap_fuzzy_start_cycle (void) {

    RoadMapAccuracyStreet =
        roadmap_config_get_integer (&RoadMapConfigAccuracyStreet);
    RoadMapConfidence =
        roadmap_config_get_integer (&RoadMapConfigConfidence);
}


int roadmap_fuzzy_max_distance (void) {

    roadmap_fuzzy_start_cycle ();

    return RoadMapAccuracyStreet;
}


/* Angle fuzzyfication:
 * max fuzzy value if same angle as reference, 0 if 90 degree difference,
 * constant slope in between.
 */
RoadMapFuzzy roadmap_fuzzy_direction
                (int direction, int reference, int symetric) {

    int delta = (direction - reference);

    if (symetric) {
       
       delta = delta % 180;

       /* The membership function is symetrical around the zero point,
        * and each side is symetrical around the 90 point: use these
        * properties to fold the delta.
        */
       if (delta < 0) delta += 180;
       if (delta > 90) delta = 180 - delta;

    } else {

       delta = delta % 360;
       if (delta < 0) delta += 360;
       if (delta > 180) delta = 360 - delta;
    }

    if (delta >= 90) return 0;

    return (FUZZY_TRUTH_MAX * (90 - delta)) / 90;
}


/* Distance fuzzyfication:
 * max fuzzy value if distance is small, 0 if objects far away,
 * constant slope in between.
 */
RoadMapFuzzy roadmap_fuzzy_distance  (int distance) {

    if (distance >= RoadMapAccuracyStreet) return 0;

    if (distance <= RoadMapError) return FUZZY_TRUTH_MAX;

    return (FUZZY_TRUTH_MAX * (RoadMapAccuracyStreet - distance)) /
                                  RoadMapAccuracyStreet - RoadMapError;
}


RoadMapFuzzy roadmap_fuzzy_connected
                 (const RoadMapNeighbour *street,
                  const RoadMapNeighbour *reference,
                        RoadMapPosition  *connection) {

    /* The logic for the connection membership function is that
     * the line is the most connected to itself, is well connected
     * to lines to which it share a common point and is not well
     * connected to other lines.
     * The weight of each state is a matter fine tuning.
     */
    RoadMapPosition line_point[2];
    RoadMapPosition reference_point[2];

    int i, j;


    if ((street->fips == reference->fips) &&
        (street->line == reference->line)) return FUZZY_TRUTH_MAX;


    if (roadmap_locator_activate (street->fips) != ROADMAP_US_OK) {
       return 0;
    }
    roadmap_line_from (street->line, &(line_point[0]));
    roadmap_line_to   (street->line, &(line_point[1]));

    if (roadmap_locator_activate (reference->fips) != ROADMAP_US_OK) {
       return 0;
    }
    roadmap_line_from (reference->line, &(reference_point[0]));
    roadmap_line_to   (reference->line, &(reference_point[1]));

    for (i = 0; i <= 1; ++i) {
        for (j = 0; j <= 1; ++j) {
            if ((line_point[i].latitude == line_point[j].latitude) &&
                (line_point[i].longitude == line_point[j].longitude)) {
                *connection = line_point[i];
                return (FUZZY_TRUTH_MAX * 2) / 3;
            }
        }
    }

    connection->latitude  = 0;
    connection->longitude = 0;

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


RoadMapFuzzy roadmap_fuzzy_false (void) {
    return 0;
}


int roadmap_fuzzy_is_acceptable (RoadMapFuzzy a) {
    return (a >= RoadMapConfidence);
}

int roadmap_fuzzy_is_good (RoadMapFuzzy a) {
    return (a >= FUZZY_TRUTH_MAX / 2);
}

int roadmap_fuzzy_is_certain (RoadMapFuzzy a) {

    return (a >= (FUZZY_TRUTH_MAX * 2) / 3);
}


void roadmap_fuzzy_initialize (void) {

    roadmap_config_declare
        ("preferences", &RoadMapConfigAccuracyStreet, "150");

    roadmap_config_declare
        ("preferences", &RoadMapConfigConfidence, "25");
}

