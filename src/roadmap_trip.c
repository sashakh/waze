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
#include "roadmap_dialog.h"
#include "roadmap_sprite.h"
#include "roadmap_screen.h"
#include "roadmap_canvas.h"


/* Default location is: 1 market St, san Francisco, California. */
#define ROADMAP_INITIAL_LONGITUDE -122394181
#define ROADMAP_INITIAL_LATITUDE    37794928
#define ROADMAP_INITIAL_POSITION  "-122394181,37794928"


static int RoadMapTripRotate   = 1;
static int RoadMapTripModified = 0; /* List needs to be saved ? */
static int RoadMapTripRefresh  = 1; /* Screen needs to be refreshed ? */
static int RoadMapTripOnTheRoad = 0;
static int RoadMapTripFocusChanged = 1;
static int RoadMapTripFocusMoved   = 1;


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
    int distance;   /* .. from the destination point. */

} RoadMapTripPoint;


RoadMapTripPoint RoadMapTripPredefined[] = {
    {{NULL, NULL}, "GPS",         "GPS",         1, 1, 0, {ROADMAP_INITIAL_LONGITUDE, ROADMAP_INITIAL_LATITUDE}},
    {{NULL, NULL}, "Destination", "Destination", 1, 0, 1, {ROADMAP_INITIAL_LONGITUDE, ROADMAP_INITIAL_LATITUDE}},
    {{NULL, NULL}, "Location",    "Position",    1, 0, 0, {ROADMAP_INITIAL_LONGITUDE, ROADMAP_INITIAL_LATITUDE}},
    {{NULL, NULL}, NULL, NULL}
};


static RoadMapTripPoint *RoadMapTripFocus = NULL;

static RoadMapList RoadMapTripWaypoints = ROADMAP_LIST_EMPTY;

static RoadMapPosition RoadMapTripLastPosition;



static void roadmap_trip_unfocus (void) {
    
    if (RoadMapTripFocus != NULL) {
        
        RoadMapTripLastPosition = RoadMapTripFocus->position;
        RoadMapTripFocus = NULL;
        RoadMapTripOnTheRoad = 0;
    }
}


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

    RoadMapTripPoint *result;
    
    
    if (strchr (name, ',') != NULL) {
        
        /* Because we use a 'simplified' CSV format, we cannot have
         * commas in the name.
         */
        char *to;
        char *from;
        char *expurged = strdup (name);
            
        for (from = expurged, to = expurged; *from != 0; ++from) {
            if (*from != ',') {
                if (to != from) {
                    *to = *from;
                }
                ++to;
            }
        }
        *to = 0;
            
        result = roadmap_trip_update (expurged, position, sprite);
        free (expurged);
        
        return result;
    }
        
    result = roadmap_trip_search (name);
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
        
            if (result == RoadMapTripFocus) {
                RoadMapTripFocusMoved = 1;
            }
            result->distance = 0;
        }
    }
    
    result->position = *position;
    
    return result;
}


static void roadmap_trip_dialog_cancel (const char *name, void *data) {
    
    roadmap_dialog_hide (name);
}


static void roadmap_trip_file_dialog_ok (const char *name, void *data) {
    
    char *file_name = (char *) roadmap_dialog_get_data ("Name", "Name:");
    
    if (file_name[0] != 0) {
        
        if (((char *)data)[0] == 'w') {
            roadmap_trip_save (file_name);
        } else {
            roadmap_trip_load (file_name);
        }
        roadmap_dialog_hide (name);
    }
}


static void roadmap_trip_file_dialog (const char *mode) {

   if (roadmap_dialog_activate ("RoadMap Trip", (void *)mode)) {

      int use_keyboard;

      use_keyboard = (strcasecmp (roadmap_config_get ("General", "Keyboard"),
                                  "yes") == 0);

      roadmap_dialog_new_entry  ("Name", "Name:");
      roadmap_dialog_add_button ("OK", roadmap_trip_file_dialog_ok);
      roadmap_dialog_add_button ("Cancel", roadmap_trip_dialog_cancel);

      roadmap_dialog_complete (use_keyboard);
   }
}


static void roadmap_trip_set_dialog_ok (const char *name, void *data) {
    
    char *point_name = (char *) roadmap_dialog_get_data ("Name", "Name:");
    
    if (point_name[0] != 0) {
        roadmap_trip_set_point (point_name, (RoadMapPosition *)data);
        roadmap_dialog_hide (name);
    }
}


static void roadmap_trip_set_dialog (RoadMapPosition *position) {

    static RoadMapPosition point_position;

    point_position = *position;
    
    if (roadmap_dialog_activate ("Add Waypoint", &point_position)) {

        int use_keyboard;

        use_keyboard = (strcasecmp (roadmap_config_get ("General", "Keyboard"),
                                    "yes") == 0);

        roadmap_dialog_new_entry  ("Name", "Name:");
        roadmap_dialog_add_button ("OK", roadmap_trip_set_dialog_ok);
        roadmap_dialog_add_button ("Cancel", roadmap_trip_dialog_cancel);

        roadmap_dialog_complete (use_keyboard);
    }
}


static void roadmap_trip_remove_dialog_populate (int count);

static void roadmap_trip_remove_dialog_delete (const char *name, void *data) {
    
    int count;
    char *point_name = (char *) roadmap_dialog_get_data ("Names", ".Waypoints");
    
    if (point_name[0] != 0) {
        
        roadmap_trip_remove_point (point_name);
        
        count = roadmap_list_count (&RoadMapTripWaypoints);
        if (count > 0) {
            roadmap_trip_remove_dialog_populate (count);
        } else {
            roadmap_dialog_hide (name);
        }
    }
}


static void roadmap_trip_remove_dialog_populate (int count) {
    
    int i;
    char **list;
    RoadMapListItem *item;
    RoadMapTripPoint *point;
    
    list = calloc (count, sizeof(*list));
    
    if (list == NULL) {
        roadmap_log (ROADMAP_FATAL, "No more memory");
    }
    
    for (i = 0, item = RoadMapTripWaypoints.first; item != NULL; item = item->next) {
        point = (RoadMapTripPoint *)item;
        if (! point->predefined) {
            list[i++] = point->id;
        }
    }
    
    roadmap_dialog_show_list ("Names", ".Waypoints", count, list, (void **)list, NULL);
}


static void roadmap_trip_remove_dialog (void) {
    
    int count;
    
    count = roadmap_list_count (&RoadMapTripWaypoints);
    if (count <= 0) {
        return; /* Nothing to delete. */
    }
    
    if (roadmap_dialog_activate ("Delete Waypoints", NULL)) {

        int use_keyboard;

        use_keyboard = (strcasecmp (roadmap_config_get ("General", "Keyboard"),
                                    "yes") == 0);

        roadmap_dialog_new_list   ("Names", ".Waypoints");
        roadmap_dialog_add_button ("Delete", roadmap_trip_remove_dialog_delete);
        roadmap_dialog_add_button ("Done", roadmap_trip_dialog_cancel);

        roadmap_dialog_complete (use_keyboard);
    }

    roadmap_trip_remove_dialog_populate (count);
}


static FILE *roadmap_trip_fopen (const char *name, const char *mode) {

    char *full_name;
    FILE *file;
    
    if (name == NULL) {
        
        roadmap_trip_file_dialog (mode);
        return NULL;
    }
        
    roadmap_config_set ("Locations", "Trip", name);
    
    full_name = roadmap_file_join (roadmap_file_trips(), name);
    file = fopen (full_name, mode);
    
    if (file == NULL) {
        roadmap_log (ROADMAP_ERROR, "cannot open file %s", full_name);
    }
    return file;
}


static void roadmap_trip_set_point_focus (RoadMapTripPoint *point, int rotate) {
    
    if (point == NULL) return;
        
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
        RoadMapTripFocusChanged = 1;
    }
    if (RoadMapTripOnTheRoad) {
        RoadMapTripRefresh = 1;
        RoadMapTripOnTheRoad = 0;
    }
}


void roadmap_trip_set_point (const char *name, RoadMapPosition *position) {

    if (name == NULL) {
        roadmap_trip_set_dialog (position);
    } else {
        roadmap_trip_set_point_focus
            (roadmap_trip_update (name, position, "Waypoint"), 0);
        roadmap_screen_refresh();
    }
}


void roadmap_trip_set_mobile (const char *name,
                              RoadMapPosition *position,
                              int speed,
                              int direction) {

    RoadMapTripPoint *result = roadmap_trip_update (name, position, "Mobile");
    
    result->mobile = 1;
    result->speed  = speed;
    result->direction = direction;

    roadmap_screen_refresh();
}


void roadmap_trip_remove_point (char *name) {
    
    RoadMapTripPoint *result;

    if (name == NULL) {
        roadmap_trip_remove_dialog ();
        return;
    }
    
    result = roadmap_trip_search (name);
    
    if (result == NULL) {
        roadmap_log (ROADMAP_ERROR, "cannot delete: point %s not found", name);
        return;
    }
    if (result->predefined) {
        roadmap_log (ROADMAP_ERROR, "cannot delete: point %s is predefined", name);
        return;
    }

    if (RoadMapTripFocus == result) {
        roadmap_trip_unfocus ();
    }
    
    if (roadmap_math_point_is_visible (&result->position)) {
        RoadMapTripRefresh = 1;
    }
    
    roadmap_list_remove (&RoadMapTripWaypoints, &result->link);
    free (result->id);
    free(result);
    
    RoadMapTripModified = 1;

    roadmap_screen_refresh();
}


void roadmap_trip_set_focus (const char *name, int rotate) {
    
    RoadMapTripPoint *point = roadmap_trip_search (name);
    
    if (point == NULL) {
        roadmap_log (ROADMAP_ERROR, "cannot activate: point %s not found", name);
        return;
    }
    
    roadmap_trip_set_point_focus (point, rotate);
}

int roadmap_trip_is_focus_changed (void) {
    
    if (RoadMapTripFocusChanged) {
        
        RoadMapTripFocusChanged = 0;
        return 1;
    }
    return 0;
}

int roadmap_trip_is_focus_moved (void) {
    
    if (RoadMapTripFocusMoved) {
        
        RoadMapTripFocusMoved = 0;
        return 1;
    }
    return 0;
}

int roadmap_trip_is_refresh_needed (void) {
    
    if (RoadMapTripRefresh) {
    
        RoadMapTripRefresh = 0;
        return 1;
    }
    return 0;
}


int roadmap_trip_get_orientation (void) {
    
    if (RoadMapTripRotate && (RoadMapTripFocus != NULL)) {
        return 360 - RoadMapTripFocus->direction;
    }
    
    return 0;
}


const RoadMapPosition *roadmap_trip_get_focus_position (void) {
    
    if (RoadMapTripFocus != NULL) {
        return &RoadMapTripFocus->position;
    }
    
    return &RoadMapTripLastPosition;
}


void  roadmap_trip_start (int rotate) {

    RoadMapTripPoint *waypoint;
    RoadMapTripPoint *destination = roadmap_trip_search ("Destination");

    
    /* Compute the distances to the destination. */
    
    for (waypoint = (RoadMapTripPoint *)RoadMapTripWaypoints.first;
         waypoint != NULL;
         waypoint = (RoadMapTripPoint *)waypoint->link.next) {

        if (! waypoint->predefined) {
            
            waypoint->distance =
                roadmap_math_distance
                    (&destination->position, &waypoint->position);
            
            roadmap_log (ROADMAP_DEBUG,
                            "Waypoint %s: distance to destination = %d %s",
                            waypoint->id,
                            waypoint->distance,
                            roadmap_math_distance_unit());
        }
    }
    destination->distance = 0;
    
    roadmap_trip_set_focus ("GPS", rotate);
    RoadMapTripOnTheRoad = 1;
    roadmap_screen_refresh ();
}


void roadmap_trip_display_points (void) {
    
    RoadMapGuiPoint   point;
    RoadMapTripPoint *waypoint;

    
    for (waypoint = (RoadMapTripPoint *)RoadMapTripWaypoints.first;
         waypoint != NULL;
         waypoint = (RoadMapTripPoint *)waypoint->link.next) {
        
        if (roadmap_math_point_is_visible (&waypoint->position)) {

            roadmap_math_coordinate (&waypoint->position, &point);
            roadmap_math_rotate_coordinates (1, &point);
            roadmap_sprite_draw (waypoint->sprite, &point, waypoint->direction);
        }
    }
}


void roadmap_trip_display_console (RoadMapPen foreground, RoadMapPen background) {
    
    int count;
    int azymuth;
    int width, ascent, descent;
    int distance_to_destination;
    char *unit;
    char  text[128];
    RoadMapGuiPoint frame[4];
    RoadMapGuiPoint text_position;
    RoadMapTripPoint *gps = roadmap_trip_search ("GPS");
    RoadMapTripPoint *destination;
    RoadMapTripPoint *waypoint;
    RoadMapTripPoint *next_waypoint;

    
    if (RoadMapTripFocus == gps && RoadMapTripOnTheRoad) {
        
        destination = roadmap_trip_search ("Destination");
        
        distance_to_destination =
            roadmap_math_distance
                (&gps->position, &destination->position);
    
        roadmap_log (ROADMAP_DEBUG,
                        "GPS: distance to destination = %d %s",
                        distance_to_destination,
                        roadmap_math_distance_unit());
        next_waypoint = destination;
        
        for (waypoint = (RoadMapTripPoint *)RoadMapTripWaypoints.first;
             waypoint != NULL;
             waypoint = (RoadMapTripPoint *)waypoint->link.next) {

            if (! waypoint->predefined) {
                if ((waypoint->distance <= distance_to_destination) &&
                    (waypoint->distance > next_waypoint->distance)) {
                    next_waypoint = waypoint;
                }
            }
        }
        
        unit = roadmap_math_trip_unit();
        
        if (next_waypoint != destination) {
            
            int distance_to_waypoint =
                    roadmap_math_distance (&gps->position, &next_waypoint->position);
            
            roadmap_log (ROADMAP_DEBUG,
                            "GPS: distance to next waypoint %s = %d %s",
                            next_waypoint->id,
                            distance_to_waypoint,
                            roadmap_math_distance_unit());
            snprintf (text, sizeof(text), "%d %s (%d %s)",
                        roadmap_math_to_trip_distance(distance_to_destination),
                        unit,
                        roadmap_math_to_trip_distance(distance_to_waypoint),
                        unit);
            
        } else {
            snprintf (text, sizeof(text), "%d %s",
                        roadmap_math_to_trip_distance(distance_to_destination),
                        unit);
        }
        
        roadmap_canvas_get_text_extents (text, &width, &ascent, &descent);
        
        frame[0].x = roadmap_canvas_width() - 5;
        frame[0].y = 5;
        frame[1].x = frame[0].x;
        frame[1].y = frame[0].y + ascent + descent + 5;
        frame[2].x = frame[0].x - width - 8;
        frame[2].y = frame[1].y;
        frame[3].x = frame[2].x;
        frame[3].y = frame[0].y;
        
        count = 4;
        roadmap_canvas_select_pen (background);
        roadmap_canvas_draw_multiple_polygons (1, &count, frame, 1);
        roadmap_canvas_select_pen (foreground);
        // roadmap_canvas_draw_multiple_polygons (1, &count, frame, 0);
        
        text_position.x = roadmap_canvas_width() - 9;
        text_position.y = 8;
        roadmap_canvas_draw_string (&text_position,
                                    ROADMAP_CANVAS_RIGHT|ROADMAP_CANVAS_TOP,
                                    text);
        
        azymuth = roadmap_math_azymuth (&gps->position,
                                        &next_waypoint->position);
        roadmap_math_coordinate (&gps->position, frame);
        roadmap_sprite_draw ("Direction", frame, azymuth);
    }
}


void roadmap_trip_clear (void) {
    
    RoadMapTripPoint *point;
    RoadMapTripPoint *next;
    
    for (point = (RoadMapTripPoint *)RoadMapTripWaypoints.first; point != NULL; point = next) {
        
        next = (RoadMapTripPoint *)point->link.next;
        
        if (! point->predefined) {
        
            if (RoadMapTripFocus == point) {
                roadmap_trip_unfocus ();
            }
            if (roadmap_math_point_is_visible (&point->position)) {
                RoadMapTripRefresh = 1;
            }
            roadmap_list_remove (&RoadMapTripWaypoints, &point->link);
            free (point->id);
            free(point);
            RoadMapTripModified = 1;
        }
    }
    
    if (RoadMapTripModified) {
        roadmap_config_set ("Locations", "Trip", "default");
    }
}


char *roadmap_trip_current (void) {
    return roadmap_config_get ("Locations", "Trip");
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
    
    file = roadmap_trip_fopen (name, "r");
    
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
      RoadMapTripModified = 0;
      
      roadmap_screen_refresh();
    }
}

void roadmap_trip_save (const char *name) {
    
    RoadMapTripPoint *point;

    /* Save all existing points, if the list was ever modified. */
    if (RoadMapTripModified) {
        
        FILE *file;
        
        file = roadmap_trip_fopen (name, "w");

        if (file == NULL) {
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
