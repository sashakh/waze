/* roadmap_display.c - Manage screen signs.
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
 *   See roadmap_display.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_line.h"
#include "roadmap_street.h"
#include "roadmap_config.h"
#include "roadmap_canvas.h"
#include "roadmap_message.h"
#include "roadmap_trip.h"
#include "roadmap_voice.h"

#include "roadmap_display.h"


static RoadMapConfigDescriptor RoadMapConfigDisplayDuration =
                        ROADMAP_CONFIG_ITEM("Display", "Duration");

static RoadMapConfigDescriptor RoadMapConfigDisplayBottomRight =
                        ROADMAP_CONFIG_ITEM("Display", "Bottom Right");

static RoadMapConfigDescriptor RoadMapConfigDisplayBottomLeft =
                        ROADMAP_CONFIG_ITEM("Display", "Bottom Left");

static RoadMapConfigDescriptor RoadMapConfigDisplayTopRight =
                        ROADMAP_CONFIG_ITEM("Display", "Top Right");

static RoadMapConfigDescriptor RoadMapConfigConsoleBackground =
                        ROADMAP_CONFIG_ITEM("Console", "Background");

static RoadMapConfigDescriptor RoadMapConfigConsoleForeground =
                        ROADMAP_CONFIG_ITEM("Console", "Foreground");


static RoadMapPen RoadMapMessageContour;
static RoadMapPen RoadMapConsoleBackground;
static RoadMapPen RoadMapConsoleForeground;


typedef struct {

    const char *title;
    
    char *content;
    char *id;
    
    int    has_position;
    time_t deadline;
    RoadMapPosition position;
    RoadMapPosition endpoint[2];
    
    RoadMapPen background;
    RoadMapPen foreground;
    
    int line;
    int distance;

    RoadMapConfigDescriptor format_descriptor;
    RoadMapConfigDescriptor background_descriptor;
    RoadMapConfigDescriptor foreground_descriptor;
    
    const char *default_format;
    const char *default_background;
    const char *default_foreground;
    
    RoadMapStreetProperties properties;

} RoadMapSign;


#define ROADMAP_SIGN(n,t,b,f) \
    {n, NULL, NULL, 0, 0, {0, 0},{{0,0}, {0,0}}, NULL, NULL, -1, 0, \
        {n, "Text", NULL}, \
        {n, "Background", NULL}, \
        {n, "Foreground", NULL}, \
     t, b, f}


RoadMapSign RoadMapStreetSign[] = {
    ROADMAP_SIGN("Current Street",  "%N, %C|%N", "DarkSeaGreen4", "white"),
    ROADMAP_SIGN("Approach",        "Approaching %N, %C|Approaching %N", "DarkSeaGreen4", "white"),
    ROADMAP_SIGN("Selected Street", "%F", "yellow", "black"),
    ROADMAP_SIGN(NULL, NULL, NULL, NULL)
};


static RoadMapPen roadmap_display_new_pen
                        (RoadMapConfigDescriptor * descriptor) {

    const char *color = roadmap_config_get (descriptor);
                            
    if (strcasecmp (color, "black") != 0) {
        
        RoadMapPen pen;
        char pen_name[256];
        
        strcpy (pen_name, descriptor->category);
        strcat (pen_name, ".");
        strcat (pen_name, descriptor->name);
        
        pen = roadmap_canvas_create_pen (pen_name);
        roadmap_canvas_set_foreground (color);
        
        return pen;
    }

    return RoadMapMessageContour;
}


static void roadmap_display_create_pens (void) {
    
    static int RoadMapDisplayPensCreated = 0;
    
    RoadMapSign *sign;
    
    
    if (RoadMapDisplayPensCreated) return;
        
    RoadMapDisplayPensCreated = 1;


    RoadMapMessageContour = roadmap_canvas_create_pen ("message.contour");
    roadmap_canvas_set_foreground ("black");
    
    RoadMapConsoleBackground =
        roadmap_display_new_pen (&RoadMapConfigConsoleBackground);
    
    RoadMapConsoleForeground =
        roadmap_display_new_pen (&RoadMapConfigConsoleForeground);
    
    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {
        
        sign->background =
            roadmap_display_new_pen (&sign->background_descriptor);

        sign->foreground =
            roadmap_display_new_pen (&sign->foreground_descriptor);
    }
}


static void roadmap_display_string
                (char *text, int lines, int height, RoadMapGuiPoint *position) {
    
    char *text_line = text;
    
    if (lines > 1) {
        
        char *text_end = text_line + strlen(text_line);
        char *p1 = text_line + (strlen(text_line) / 2);
        char *p2 = p1;
        
        while (p1 > text_line) {
            if (*p1 == ' ') {
                break;
            }
            p1 -= 1;
        }
        while (p2 < text_end) {
            if (*p2 == ' ') {
                break;
            }
            p2 += 1;
        }
        if (text_end - p1 > p2 - text_line) {
            p1 = p2;
        }
        if (p1 > text_line) {
            *p1 = 0;
            roadmap_canvas_draw_string
                (position, ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP, text_line);
            text_line = p1 + 1;
            position->y += height;
        }
            
    }

    roadmap_canvas_draw_string
        (position, ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP, text_line);
}


static RoadMapSign *roadmap_display_search_sign (const char *title) {
    
    RoadMapSign *sign;

    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {
        if (strcmp (sign->title, title) == 0) {
            return sign;
        }
    }
    roadmap_log (ROADMAP_ERROR, "unknown display sign '%s'", title);
    return NULL;
}


static void roadmap_display_sign (RoadMapSign *sign) {

    RoadMapGuiPoint points[7];
    RoadMapGuiPoint text_position;
    int width, height, ascent, descent;
    int screen_width;
    int sign_width, sign_height, text_height;
    int count;
    int lines;


    roadmap_canvas_get_text_extents
        (sign->content, &width, &ascent, &descent);

    width += 8; /* Keep some room around the text. */
    
    height = roadmap_canvas_height();
    screen_width = roadmap_canvas_width();
    
    if (width + 10 < screen_width) {
        sign_width = width;
        lines = 1;
    } else {
        sign_width = screen_width - 10;
        lines = 2;
    }
    
    text_height = ascent + descent + 3;
    sign_height = lines * text_height + 3;
    
    if (sign->has_position) {

        if (! roadmap_math_point_is_visible (&sign->position)) {
            sign->deadline = 0;
            return;
        }
        roadmap_math_coordinate (&sign->position, points);
        roadmap_math_rotate_coordinates (1, points);

        if (points[0].x < screen_width / 2) {

            points[1].x = 10;
            points[2].x = 5;
            points[4].x = sign_width + 5;
            points[6].x = 20;
        
            text_position.x = 9;

        } else {

            points[1].x = screen_width - 10;
            points[2].x = screen_width - 5;
            points[4].x = screen_width - sign_width - 5;
            points[6].x = screen_width - 20;
        
            text_position.x = points[4].x + 4;
        }
        points[3].x = points[2].x;
        points[5].x = points[4].x;
    

        if (points[0].y < height / 2) {
   
            points[1].y = height - sign_height - 5;
            points[3].y = height - 5;

            text_position.y = points[1].y + 3;

        } else {

            points[1].y = sign_height + 5;
            points[3].y = 5;

            text_position.y = 8;
        }
        points[2].y = points[1].y;
        points[4].y = points[3].y;
        points[5].y = points[1].y;
        points[6].y = points[1].y;

        count = 7;
        
    } else {
        
        points[0].x = (screen_width - sign_width) / 2;
        points[1].x = (screen_width + sign_width) / 2;
        points[2].x = points[1].x;
        points[3].x = points[0].x;
    
        points[0].y = height - sign_height - 5;
        points[1].y = points[0].y;
        points[2].y = height - 5;
        points[3].y = points[2].y;
    
    
        text_position.x = points[0].x + 4;
        text_position.y = points[0].y + 3;
        
        count = 4;
    }
    
    roadmap_canvas_select_pen (sign->background);
    roadmap_canvas_draw_multiple_polygons (1, &count, points, 1);

    roadmap_canvas_select_pen (RoadMapMessageContour);
    roadmap_canvas_draw_multiple_polygons (1, &count, points, 0);
    
    roadmap_canvas_select_pen (sign->foreground);
    roadmap_display_string
        (sign->content, lines, text_height, &text_position);
}


void roadmap_display_activate
        (const char *title,
         int line,
         int distance,
         const RoadMapPosition *position) {

    int   message_has_changed;
    int   street;
    const char *format;
    char  text[256];
    RoadMapSign *sign;


    sign = roadmap_display_search_sign (title);
    if (sign == NULL) return;

    street = sign->properties.street;
    
    if (sign->line != line) {
        
       roadmap_street_get_properties (line, &sign->properties);
        
        if (sign->id != NULL) {
            free (sign->id);
        }
        sign->id =
            strdup (roadmap_street_get_full_name (&sign->properties));
        
        sign->line = line;
    }


    roadmap_message_set ('F', sign->id);
    
    roadmap_message_set
        ('#', roadmap_street_get_street_address (&sign->properties));
    roadmap_message_set
        ('N', roadmap_street_get_street_name (&sign->properties));
    roadmap_message_set
        ('C', roadmap_street_get_city_name (&sign->properties));

    
    format = roadmap_config_get (&sign->format_descriptor);
    
    if (! roadmap_message_format (text, sizeof(text), format)) {
        return;
    }
    message_has_changed =
        (sign->content == NULL || strcmp (sign->content, text) != 0);


    if (street != sign->properties.street) {
        roadmap_voice_announce (sign->title);
    }
   
    if (message_has_changed || (position != NULL)) {
        sign->deadline =
            time(NULL)
                + roadmap_config_get_integer
                        (&RoadMapConfigDisplayDuration);
    }
    
    if (message_has_changed) {

        if (sign->content != NULL) {
            free (sign->content);
        }
        if (text[0] == 0) {
            sign->content = strdup("(this street has no name)");
        } else {
            sign->content = strdup (text);
        }
    }

    roadmap_line_from (line, &sign->endpoint[0]);
    roadmap_line_to   (line, &sign->endpoint[1]);
    
    if (position == NULL) {
        sign->has_position = 0;
    } else {
        sign->has_position = 1;
        sign->position = *position;
    }
}


static void roadmap_display_console_box
                (int corner, RoadMapConfigDescriptor *item) {
    
    const char *format;
    char text[256];
    int count;
    int width, ascent, descent;
    
    RoadMapGuiPoint frame[4];


    format = roadmap_config_get (item);
    
    if (! roadmap_message_format (text, sizeof(text), format)) {
        return;
    }
    
    roadmap_canvas_get_text_extents (text, &width, &ascent, &descent);

    if (corner & ROADMAP_CANVAS_RIGHT) {
        frame[2].x = roadmap_canvas_width() - 5;
        frame[0].x = frame[2].x - width - 6;
    } else {
        frame[0].x = 5;
        frame[2].x = frame[0].x + width + 6;
    }
    frame[1].x = frame[0].x;
    frame[3].x = frame[2].x;

    if (corner & ROADMAP_CANVAS_BOTTOM) {
        frame[0].y = roadmap_canvas_height () - ascent - descent - 11;
        frame[1].y = roadmap_canvas_height () - 6;
    } else {
        frame[0].y = 6;
        frame[1].y = ascent + descent + 11;
    }
    frame[2].y = frame[1].y;
    frame[3].y = frame[0].y;
    
    count = 4;
    roadmap_canvas_select_pen (RoadMapConsoleBackground);
    roadmap_canvas_draw_multiple_polygons (1, &count, frame, 1);
    
    roadmap_canvas_select_pen (RoadMapConsoleForeground);
    roadmap_canvas_draw_multiple_polygons (1, &count, frame, 0);

    frame[0].x = frame[3].x - 3;
    frame[0].y = frame[3].y + 3;
    
    roadmap_canvas_draw_string (frame,
                                ROADMAP_CANVAS_RIGHT|ROADMAP_CANVAS_TOP,
                                text);
}


void roadmap_display_console (void) {
    
    roadmap_display_create_pens ();
    
    roadmap_display_console_box
        (ROADMAP_CANVAS_BOTTOM|ROADMAP_CANVAS_RIGHT,
         &RoadMapConfigDisplayBottomRight);
    
    roadmap_display_console_box
        (ROADMAP_CANVAS_BOTTOM|ROADMAP_CANVAS_LEFT,
         &RoadMapConfigDisplayBottomLeft);
    
    roadmap_display_console_box
        (ROADMAP_CANVAS_TOP|ROADMAP_CANVAS_RIGHT,
         &RoadMapConfigDisplayTopRight);
}


void roadmap_display_signs (void) {

    time_t now = time(NULL);
    RoadMapSign *sign;


    roadmap_display_create_pens ();
    
    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {
        if (sign->deadline > now && sign->content != NULL) {
            roadmap_display_sign (sign);
        }
    }
}


void roadmap_display_set_waypoint (const char *title) {

    RoadMapSign *sign = roadmap_display_search_sign (title);

    if (sign == NULL || (! sign->has_position)) return;
        
    roadmap_trip_set_point (sign->id, &sign->position);
}


void roadmap_display_initialize (void) {

    RoadMapSign *sign;
    
    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayDuration, "10");
    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayBottomRight, "%D (%W)|%D");
    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayBottomLeft, "%S");
    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayTopRight, "ETA: %A|%T");

    roadmap_config_declare
        ("preferences", &RoadMapConfigConsoleBackground, "yellow");
    roadmap_config_declare
        ("preferences", &RoadMapConfigConsoleForeground, "black");
    
    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {
        
        roadmap_config_declare
            ("preferences",
             &sign->format_descriptor, sign->default_format);

        roadmap_config_declare
            ("preferences",
             &sign->background_descriptor, sign->default_background);

        roadmap_config_declare
            ("preferences",
             &sign->foreground_descriptor, sign->default_foreground);
    }
}
