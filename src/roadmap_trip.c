/* roadmap_trip.c - Manage a trip: destination & waypoints.
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
 *   See roadmap_trip.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_file.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_trip.h"


/* Default location is: 1 market St, san Francisco, California. */
#define ROADMAP_INITIAL_LONGITUDE -122394181
#define ROADMAP_INITIAL_LATITUDE    37794928
#define ROADMAP_INITIAL_POSITION  "-122394181,37794928"


static int RoadMapTripRotate   = 0;
static int RoadMapTripModified = 0; /* List needs to be saved ? */
static int RoadMapTripRefresh  = 0; /* Screen needs to be refreshed ? */


typedef struct roadmap_trip_point {

    RoadMapListItem link;
    
    char *id;
    const char *sprite;
    
    char predefined;
    char mobile;
    char in_trip;

    RoadMapPosition position;

    int speed;
    int direction;

} RoadMapTripPoint;


RoadMapTripPoint RoadMapTripPredefined[] = {
    {{NULL, NULL}, "Destination", "Destination", 1, 0, 1, {ROADMAP_INITIAL_LONGITUDE, ROADMAP_INITIAL_LATITUDE}},
    {{NULL, NULL}, "Location",    "Position",    1, 0, 0, {ROADMAP_INITIAL_LONGITUDE, ROADMAP_INITIAL_LATITUDE}},
    {{NULL, NULL}, "GPS",         "GPS",         1, 1, 0, {ROADMAP_INITIAL_LONGITUDE, ROADMAP_INITIAL_LATITUDE}},
    {{NULL, NULL}, NULL, NULL}
};


static RoadMapTripPoint *RoadMapTripFocus = NULL;

static RoadMapList RoadMapTripWaypoints = ROADMAP_LIST_EMPTY;



static RoadMapTripPoint *roadmap_trip_search (const char *name) {
    
    RoadMapTripPoint *item;
    
    for (item = (RoadMapTripPoint *)RoadMapTripWaypoints.first;
         item != NULL;
         item = (RoadMapTripPoint *)item->link.next) {

        if (strcmp (item->id, name) == 0) {
            return item;
        }
    }
    return NULL;
}


static void roadmap_trip_coordinate (const RoadMapPosition *position, RoadMapGuiPoint *point) {
    
    if (roadmap_math_point_is_visible (position)) {
        roadmap_math_coordinate (position, point);
    } else {
        point->x = 32767;
        point->y = 32767;
    }
}


static RoadMapTripPoint *roadmap_trip_update (const char *name, const RoadMapPosition *position, const char *sprite) {

    RoadMapTripPoint *result = roadmap_trip_search (name);
    
    
    if (result == NULL) {
        
        /* A new point: refresh is needed only if this point
         * is visible.
         */
        result = malloc (sizeof(RoadMapTripPoint));
        if (result == NULL) {
            roadmap_log (ROADMAP_FATAL, "no more memory");
        }
        result->id = strdup(name);
        if (result->id == NULL) {
            roadmap_log (ROADMAP_FATAL, "no more memory");
        }
        result->predefined = 0;
        result->in_trip = 1;
        result->sprite = sprite;
        result->mobile = 0;
        result->speed  = 0;
        result->direction = 0;
        
        if (roadmap_math_point_is_visible (position)) {
            RoadMapTripRefresh = 1;
        }
        
        roadmap_list_append (&RoadMapTripWaypoints, &result->link);
        RoadMapTripModified = 1;
        
    } else {
        
        /* An existing point: refresh is needed only if the point
         * moved in a visible fashion.
         */
        RoadMapGuiPoint point1;
        RoadMapGuiPoint point2;
        
        if (result->position.latitude != position->latitude ||
            result->position.longitude != position->longitude) {
            roadmap_trip_coordinate (position, &point1);
            roadmap_trip_coordinate (&result->position, &point2);
            if (point1.x != point2.x || point1.y != point2.y) {
                RoadMapTripRefresh = 1;
            }
            RoadMapTripModified = 1;
        }
    }
    
    result->position = *position;
    
    return result;
}


static FILE *roadmap_trip_open (const char *name, const char *mode) {

    char *full_name;
    
    if (name == NULL) {
        name = roadmap_config_get ("Locations", "Trip");
    } else {
        roadmap_config_set ("Locations", "Trip", name);
    }
    
    full_name = roadmap_file_join (roadmap_file_trips(), name);
    return fopen (full_name, mode);
}


void roadmap_trip_set_point (const char *name, RoadMapPosition *position) {

    roadmap_trip_update (name, position, "Waypoint");
}


void roadmap_trip_set_mobile (const char *name,
                              RoadMapPosition *position,
                              int speed,
                              int direction) {

    RoadMapTripPoint *result = roadmap_trip_update (name, position, "Mobile");
    
    result->mobile = 1;
    result->speed  = speed;
    result->direction = direction;
}


void roadmap_trip_remove (char *name) {
    
    RoadMapTripPoint *result = roadmap_trip_search (name);
    
    if (result == NULL) {
        roadmap_log (ROADMAP_ERROR, "cannot delete: point %s not found", name);
        return;
    }
    if (result->predefined) {
        roadmap_log (ROADMAP_ERROR, "cannot delete: point %s is predefined", name);
        return;
    }

    if (RoadMapTripFocus == result) {
        RoadMapTripFocus = NULL;
    }
    
    if (roadmap_math_point_is_visible (&result->position)) {
        RoadMapTripRefresh = 1;
    }
    
    roadmap_list_remove (&RoadMapTripWaypoints, &result->link);
    free (result->id);
    free(result);
    
    RoadMapTripModified = 1;
}


void roadmap_trip_set_focus (const char *name, int rotate) {
    
    RoadMapTripPoint *point = roadmap_trip_search (name);
    
    if (point == NULL) {
        roadmap_log (ROADMAP_ERROR, "cannot activate: point %s not found", name);
        return;
    }
    if (! point->mobile) {
        rotate = 0; /* Fixed point, no rotation. */
    }
    
    if (RoadMapTripRotate != rotate) {
        RoadMapTripRotate = rotate;
        RoadMapTripRefresh = 1;
    }
    if (RoadMapTripFocus != point) {
        RoadMapTripFocus = point;
        RoadMapTripRefresh = 1;
    }
}

RoadMapListItem *roadmap_trip_get_focus (void) {
    
    return &RoadMapTripFocus->link;
}


RoadMapListItem *roadmap_trip_retrieve (char *name) {
    
    return &(roadmap_trip_search(name)->link);
}


int roadmap_trip_refresh_needed (void) {
    
    int result = RoadMapTripRefresh;
    
    RoadMapTripRefresh = 0;
    
    return result;
}


int roadmap_trip_get_orientation (void) {
    
    if ((RoadMapTripRotate) && (RoadMapTripFocus != NULL)) {
        return RoadMapTripFocus->direction;
    }
    
    return 0;
}


RoadMapListItem *roadmap_trip_get_first (void) {
    
    return RoadMapTripWaypoints.first;
}


const char *roadmap_trip_get_sprite (const RoadMapListItem *item) {
    
    return ((RoadMapTripPoint *)item)->sprite;
}

const RoadMapPosition *roadmap_trip_get_position (const RoadMapListItem *item) {
    
    return &((RoadMapTripPoint *)item)->position;
}

int roadmap_trip_is_mobile (const RoadMapListItem *item) {
    
    return ((RoadMapTripPoint *)item)->mobile;
}

int roadmap_trip_get_speed (const RoadMapListItem *item) {
    
    return ((RoadMapTripPoint *)item)->speed;
}

int roadmap_trip_get_direction (const RoadMapListItem *item) {
    
    return ((RoadMapTripPoint *)item)->direction;
}


void roadmap_trip_clear (void) {
    
    RoadMapTripPoint *point;
    RoadMapTripPoint *next;
    
    for (point = (RoadMapTripPoint *)RoadMapTripWaypoints.first; point != NULL; point = next) {
        
        next = (RoadMapTripPoint *)point->link.next;
        
        if (! point->predefined) {
        
            if (RoadMapTripFocus == point) {
                RoadMapTripFocus = NULL;
            }
            roadmap_list_remove (&RoadMapTripWaypoints, &point->link);
            free (point->id);
            free(point);
            RoadMapTripModified = 1;
        }
    }
}


void roadmap_trip_initialize (void) {
    
    int i;
    
    for (i = 0; RoadMapTripPredefined[i].id != NULL; ++i) {
        if (! RoadMapTripPredefined[i].in_trip) {
            roadmap_config_declare
                ("session", "Locations", RoadMapTripPredefined[i].id, ROADMAP_INITIAL_POSITION);
        }
    }
    roadmap_config_declare ("session", "Locations", "Trip", "default");
}


void roadmap_trip_load (const char *name) {
    
    FILE *file;
    int   i;
    char *p;
    char *argv[8];
    char  line[1024];
    
    RoadMapPosition position;
    
        
    if (ROADMAP_LIST_IS_EMPTY (&RoadMapTripWaypoints)) {
        
        int i;
        
        for (i = 0; RoadMapTripPredefined[i].id != NULL; ++i) {
            if (! RoadMapTripPredefined[i].in_trip) {
                roadmap_config_get_position
                    (RoadMapTripPredefined[i].id, &RoadMapTripPredefined[i].position);
            }
            roadmap_list_append (&RoadMapTripWaypoints, &RoadMapTripPredefined[i].link);
        }
        
    } else {
        
        roadmap_trip_clear ();
    }
    
    /* Load waypoints from the user environment. */
    
    file = roadmap_trip_open (name, "r");
    
    if (file != NULL) {
        
      while (! feof(file)) {

         if (fgets (line, sizeof(line), file) == NULL) break;

         p = roadmap_config_extract_data (line, sizeof(line));

         if (p == NULL) continue;
         
         argv[0] = p; /* Not used at that time. */
             
         for (p = strchr (p, ','), i = 0; p != NULL && i < 8; p = strchr (p, ',')) {
             *p = 0;
             argv[++i] = ++p;
         }
         
         if (i != 3) {
             roadmap_log (ROADMAP_ERROR, "erroneous trip line (%d fields)", i);
             continue;
         }
         position.longitude = atoi(argv[2]);
         position.latitude  = atoi(argv[3]);
         roadmap_trip_update (argv[1], &position, "Waypoint");
      }
    
        fclose (file);
    }
    RoadMapTripModified = 0;
    
    if (RoadMapTripFocus == NULL) {

        roadmap_trip_set_focus ("GPS", 1); /* Default focus is GPS. */
    }
}

void roadmap_trip_save (const char *name) {
    
    RoadMapTripPoint *point;

    /* Save all existing points, if the list was ever modified. */
    if (RoadMapTripModified) {
        
        FILE *file;
        
        file = roadmap_trip_open (name, "w");

        if (file == NULL) {
            roadmap_log (ROADMAP_ERROR, "cannot save trip to %s", name);
            return;
        }
        
        for (point = (RoadMapTripPoint *)RoadMapTripWaypoints.first;
             point != NULL;
             point = (RoadMapTripPoint *)point->link.next) {
                  
            if (point->in_trip) {
                fprintf (file, "%s,%s,%d,%d\n",
                     point->sprite,
                     point->id,
                     point->position.longitude,
                     point->position.latitude);
            }
        }
        
        fclose (file);
        RoadMapTripModified = 0;
    }
}
