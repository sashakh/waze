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

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_canvas.h"

#include "roadmap_display.h"


static char *RoadMapDisplayParameters[128] = {NULL};

static RoadMapPen RoadMapDisplayForeground;
static RoadMapPen RoadMapDisplayBackground;


static int roadmap_display_format (char *text, int length, const char *format) {

    char *f;
    char *p = text+1;
    char *end = text + length - 1;
    
    *text = '('; /* A hack to force a constant ascent/descent. */
    
    while (*format) {
        
        if (*format == '%') {
            
            format += 1;
            if (*format <= 0) {
                break;
            }
            
            f = RoadMapDisplayParameters[(int)(*(format++))];
            if (f != NULL) {
                while (*f && p < end) {
                    *(p++) = *(f++);
                }
            } else {
                format = strchr (format, '|');
                
                if (format == NULL) {
                    return 0; /* Cannot build the string. */
                }
                format += 1;
                p = text + 1; /* Restart. */
            }

        } else if (*format == '|') {
            
            break; /* We completed this alternative successfully. */
            
        } else {

            *(p++) = *(format++);
        }
        
        if (p >= end) {
            break;
        }
    }

    *p = 0;

    return p > text+1;
}


void roadmap_display_set (char parameter, const char *format, ...) {
    
    va_list ap;
    char    value[256];
    
    if (parameter <= 0) {
        roadmap_log (ROADMAP_ERROR,
                     "invalid parameter code %d (value %s)",
                     parameter, value);
        return;
    }
    
    va_start(ap, format);
    vsnprintf(value, sizeof(value), format, ap);
    va_end(ap);
    
    if (RoadMapDisplayParameters[(int)parameter] != NULL) {
        free (RoadMapDisplayParameters[(int)parameter]);
    }
    RoadMapDisplayParameters[(int)parameter] = strdup (value);
}


void roadmap_display_unset (char parameter) {
    
    if (parameter <= 0) {
        roadmap_log (ROADMAP_ERROR,
                     "invalid parameter code %d",
                     parameter);
        return;
    }
    
    if (RoadMapDisplayParameters[(int)parameter] != NULL) {
        free (RoadMapDisplayParameters[(int)parameter]);
        RoadMapDisplayParameters[(int)parameter] = NULL;
    }
}


void roadmap_display_colors (RoadMapPen foreground, RoadMapPen background) {
    
    RoadMapDisplayForeground = foreground;
    RoadMapDisplayBackground = background;
}


void roadmap_display_details (RoadMapPosition *position, char *format) {

    int count;
    int corner;
    RoadMapGuiPoint points[7];
    RoadMapGuiPoint text_position;
    char text[256];
    int width, height, ascent, descent;


    if (! roadmap_display_format (text, sizeof(text), format)) {
        return;
    }

    roadmap_canvas_get_text_extents (text, &width, &ascent, &descent);

    roadmap_math_coordinate (position, points);
    roadmap_math_rotate_coordinates (1, points);

    width = roadmap_canvas_width();
    
    if (points[0].x < width / 2) {

        points[1].x = 10;
        points[2].x = 5;
        points[4].x = width - 5;
        points[6].x = 20;

    } else {

        points[1].x = width - 10;
        points[2].x = width - 5;
        points[4].x = 5;
        points[6].x = width - 20;
    }
    points[3].x = points[2].x;
    points[5].x = points[4].x;
    
    corner = ROADMAP_CANVAS_LEFT;
    text_position.x = 9;

    height = roadmap_canvas_height();

    if (points[0].y < height / 2) {
   
        points[1].y = height - ascent - descent - 11;
        points[3].y = height - 5;

        corner |= ROADMAP_CANVAS_BOTTOM;
        text_position.y = height - 8;

    } else {

        points[1].y = ascent + descent + 11;
        points[3].y = 5;

        corner |= ROADMAP_CANVAS_TOP;
        text_position.y = 8;
    }
    points[2].y = points[1].y;
    points[4].y = points[3].y;
    points[5].y = points[1].y;
    points[6].y = points[1].y;

    roadmap_canvas_select_pen (RoadMapDisplayBackground);

    count = 7;
    roadmap_canvas_draw_multiple_polygons (1, &count, points, 1);

    roadmap_canvas_select_pen (RoadMapDisplayForeground);

    roadmap_canvas_draw_multiple_polygons (1, &count, points, 0);

    roadmap_canvas_draw_string (&text_position, corner, text+1);
}


void roadmap_display_message (int corner, char *format) {
    
    char text[256];
    int count;
    int width, ascent, descent;
    
    RoadMapGuiPoint frame[4];


    if (! roadmap_display_format (text, sizeof(text), format)) {
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
    roadmap_canvas_select_pen (RoadMapDisplayBackground);
    roadmap_canvas_draw_multiple_polygons (1, &count, frame, 1);
    
    roadmap_canvas_select_pen (RoadMapDisplayForeground);
    roadmap_canvas_draw_multiple_polygons (1, &count, frame, 0);

    frame[0].x = frame[3].x - 3;
    frame[0].y = frame[3].y + 3;
    
    roadmap_canvas_draw_string (frame,
                                ROADMAP_CANVAS_RIGHT|ROADMAP_CANVAS_TOP,
                                text+1);
}


void roadmap_display_show (void) {
    
    roadmap_display_message
        (ROADMAP_CANVAS_BOTTOM|ROADMAP_CANVAS_RIGHT,
         roadmap_config_get ("Display", "Bottom Right"));
    
    roadmap_display_message
        (ROADMAP_CANVAS_BOTTOM|ROADMAP_CANVAS_LEFT,
         roadmap_config_get ("Display", "Bottom Left"));
    
    roadmap_display_message
        (ROADMAP_CANVAS_TOP|ROADMAP_CANVAS_RIGHT,
         roadmap_config_get ("Display", "Top Right"));
}


void roadmap_display_initialize (void) {
    
    roadmap_config_declare ("preferences", "Display", "Bottom Right", "%D (%W)|%D");
    roadmap_config_declare ("preferences", "Display", "Bottom Left", "%S");
    roadmap_config_declare ("preferences", "Display", "Top Right", "ETA: %A|%T");
}
