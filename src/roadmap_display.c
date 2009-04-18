/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2009, Danny Backx
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
 */

/**
 * @file
 * @brief Manage screen signs.
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
#include "roadmap_screen.h"
#include "roadmap_canvas.h"
#include "roadmap_message.h"
#include "roadmap_sprite.h"
#include "roadmap_voice.h"
#include "roadmap_plugin.h"
#include "roadmap_time.h"
#include "roadmap_main.h"

#define __ROADMAP_DISPLAY_NO_LANG
#include "roadmap_display.h"


static char *RoadMapDisplayPage = NULL;

static RoadMapConfigDescriptor RoadMapConfigDisplayDuration =
                        ROADMAP_CONFIG_ITEM("Display", "Duration");

static RoadMapConfigDescriptor RoadMapConfigDisplayBottomRight =
                        ROADMAP_CONFIG_ITEM("Display", "Bottom Right");

static RoadMapConfigDescriptor RoadMapConfigDisplayBottomLeft =
                        ROADMAP_CONFIG_ITEM("Display", "Bottom Left");

static RoadMapConfigDescriptor RoadMapConfigDisplayTopRight =
                        ROADMAP_CONFIG_ITEM("Display", "Top Right");

RoadMapConfigDescriptor RoadMapConfigDisplayTopLeft =
                        ROADMAP_CONFIG_ITEM("Display", "Top Left");

static RoadMapConfigDescriptor RoadMapConfigConsoleBackground =
                        ROADMAP_CONFIG_ITEM("Console", "Background");

static RoadMapConfigDescriptor RoadMapConfigConsoleForeground =
                        ROADMAP_CONFIG_ITEM("Console", "Foreground");

static RoadMapConfigDescriptor RoadMapConfigDisplayFontSize =
                        ROADMAP_CONFIG_ITEM("Display", "Font Size");


static RoadMapPen RoadMapMessageContour;
static RoadMapPen RoadMapConsoleBackground;
static RoadMapPen RoadMapConsoleForeground;

static int RoadMapDisplayFontSize;

static int RoadMapDisplayRefreshNeeded;

static int RoadMapDisplayDeadline;

#define SIGN_BOTTOM   0
#define SIGN_TOP      1
#define SIGN_CENTER   2

/**
 * @brief
 */
typedef struct {

    const char *page;
    const char *title;

    char *content;
    char *id;

    int    on_current_page;
    int    has_position;
    int    was_visible;
    time_t deadline;
    RoadMapPosition position;
    RoadMapPosition endpoint[2];

    RoadMapPen background;
    RoadMapPen foreground;

    PluginLine line;

    RoadMapConfigDescriptor format_descriptor;
    RoadMapConfigDescriptor background_descriptor;
    RoadMapConfigDescriptor foreground_descriptor;

    int         where;
    const char *default_format;
    const char *default_background;
    const char *default_foreground;

    PluginStreet street;	/**< remember current street, this is used in
				  roadmap_display_activate() to figure out
				  whether we're still in the same street. */

} RoadMapSign;


#define ROADMAP_SIGN(p,n,w,t,b,f) \
    {p, n, NULL, NULL, 0, 0, 0, 0, {0, 0},{{0,0}, {0,0}}, NULL, NULL, \
     PLUGIN_LINE_NULL, \
        {n, "Text", NULL}, \
        {n, "Background", NULL}, \
        {n, "Foreground", NULL}, \
     w, t, b, f, PLUGIN_STREET_NULL}

/**
 * @brief RoadMapSign table, indicates what to show under predefined conditions.
 */
RoadMapSign RoadMapStreetSign[] = {
    ROADMAP_SIGN("GPS", "Current Street",  SIGN_BOTTOM, "%N, %C|%N", "DarkSeaGreen4", "white"),
    ROADMAP_SIGN("GPS", "Approach",        SIGN_TOP, "Approaching %N, %C|Approaching %N", "DarkSeaGreen4", "white"),
    ROADMAP_SIGN(NULL, "Selected Street", SIGN_BOTTOM, "%F", "yellow", "black"),
    ROADMAP_SIGN(NULL, "Place", SIGN_BOTTOM, NULL, "yellow", "black"),
    ROADMAP_SIGN(NULL, "Moving", SIGN_BOTTOM, NULL, "black", "yellow"),
    ROADMAP_SIGN(NULL, "Info",  SIGN_CENTER, NULL, "yellow", "black"),
    ROADMAP_SIGN(NULL, "Error", SIGN_CENTER, NULL, "red", "white"),
    ROADMAP_SIGN(NULL, NULL, 0, NULL, NULL, NULL)
};


static RoadMapPen roadmap_display_new_pen
                        (RoadMapConfigDescriptor * descriptor) {

    const char *color = roadmap_config_get (descriptor);

    if (strcasecmp (color, "black") != 0) {

        RoadMapPen pen;
        char pen_name[256];

        if (sizeof(pen_name) <
              strlen(descriptor->category) + strlen(descriptor->name) + 2) {
           roadmap_log(ROADMAP_FATAL,
                       "not enough space for pen name %s.%s\n",
                       descriptor->category,
                       descriptor->name);
        }
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
    int eachline = strlen(text_line) / lines;
    char saved;

    while (lines-- > 1) {

        /* There is more than one line of text to display:
         * find where to cut the text. We choose to cut at
         * a space, before the string breakpoint.
         */
        char *text_end = text_line + strlen(text_line);
        char *p1 = text_line + eachline + 2; /* we left extra space */

	if (p1 > text_end) p1 = text_end;

        while (p1 > text_line) {
            if (*p1 == ' ') {
                break;
            }
            p1 -= 1;
        }
        if (p1 == text_line) {
	    p1 = text_line + eachline;
	}

	saved = *p1;
	*p1 = 0;

	roadmap_screen_text (ROADMAP_TEXT_SIGNS,  position, 
	    ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
	    RoadMapDisplayFontSize, text_line);

	*p1 = saved;
	text_line = p1;
	position->y += height;

    }

    roadmap_screen_text (ROADMAP_TEXT_SIGNS, position,
                ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
                RoadMapDisplayFontSize, text_line);
}

/**
 * @brief look up a named sign
 * @param title the name/title of this sign
 * @return pointer to the structure of this sign
 */
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


static void roadmap_display_highlight (const RoadMapPosition *position) {

    RoadMapGuiPoint point;

    roadmap_math_coordinate (position, &point);
    roadmap_math_rotate_coordinates (1, &point);
    roadmap_sprite_draw ("Highlight", &point, 0);
}


static void roadmap_display_sign (RoadMapSign *sign) {

    RoadMapGuiPoint points[7];
    RoadMapGuiPoint text_position;
    int width, height, ascent, descent;
    int screen_width;
    int sign_width, sign_height, text_height;
    int count;
    int lines;

    roadmap_log_push ("roadmap_display_sign");

    roadmap_screen_text_extents
        (ROADMAP_TEXT_SIGNS, sign->content, RoadMapDisplayFontSize,
            &width, &ascent, &descent, NULL);

    width += 8; /* Keep some room around the text. */

    height = roadmap_canvas_height();
    screen_width = roadmap_canvas_width();

    /* Check if the text fits into one line, or if we need to use
     * more than one.
     */
    if (width + 10 < screen_width) {
        sign_width = width;
        lines = 1;
    } else {
        /* give ourselves an extra room, because breaking up the
         * string is inexact -- proportional fonts don't match up
         * with strlen() lengths used when splitting text later on.
         */
        lines = 1 + ((width + (screen_width - 10) / 2) / (screen_width - 10));
        sign_width = screen_width - 10;
    }

    text_height = ascent + descent + 3;
    sign_height = lines * text_height + 3;

    if (sign->has_position) {

        int visible = roadmap_math_point_is_visible (&sign->position);

        if (sign->was_visible && (! visible)) {
            sign->deadline = 0;
            roadmap_log_pop ();
            return;
        }
        sign->was_visible = visible;

        roadmap_math_coordinate (&sign->position, points);
        roadmap_math_rotate_coordinates (1, points);

        if (sign->where == SIGN_TOP) {

            points[1].x = 5 + (screen_width - sign_width) / 2;
            points[2].x = points[1].x - 5;
            points[4].x = (screen_width + sign_width) / 2;
            points[6].x = points[1].x + 10;

            text_position.x = points[2].x + 4;

        } else if (points[0].x < screen_width / 2) {

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


        if (sign->where == SIGN_TOP || (points[0].y > height / 2)) {

            points[1].y = sign_height + 5;
            points[3].y = 5;

            text_position.y = 8;

        } else {

            points[1].y = height - sign_height - 5;
            points[3].y = height - 5;

            text_position.y = points[1].y + 3;
        }
        points[2].y = points[1].y;
        points[4].y = points[3].y;
        points[5].y = points[1].y;
        points[6].y = points[1].y;

        count = 7;

        roadmap_display_highlight (&sign->endpoint[0]);
        roadmap_display_highlight (&sign->endpoint[1]);

    } else {

        points[0].x = (screen_width - sign_width) / 2;
        points[1].x = (screen_width + sign_width) / 2;
        points[2].x = points[1].x;
        points[3].x = points[0].x;

        switch (sign->where)
        {
           case SIGN_BOTTOM:

              points[0].y = height - sign_height - 5;
              break;

           case SIGN_TOP:

              points[0].y = 5;
              break;

           case SIGN_CENTER:

              points[0].y = (height - sign_height) / 2;
              break;
        }
        points[1].y = points[0].y;
        points[2].y = points[0].y + sign_height;
        points[3].y = points[2].y;

        text_position.x = points[0].x + 4;
        text_position.y = points[0].y + 3;

        count = 4;
    }

    roadmap_canvas_select_pen (sign->background);
    roadmap_canvas_draw_multiple_polygons (1, &count, points, 1, 0);

    roadmap_canvas_select_pen (RoadMapMessageContour);
    roadmap_canvas_draw_multiple_polygons (1, &count, points, 0, 0);

    roadmap_canvas_select_pen (sign->foreground);
    roadmap_display_string
        (sign->content, lines, text_height, &text_position);

    roadmap_log_pop ();
}


void roadmap_display_page (const char *name) {

   RoadMapSign *sign;

   if (RoadMapDisplayPage != NULL) {
      free (RoadMapDisplayPage);
   }

   if (name == NULL) {

      RoadMapDisplayPage = NULL;

      for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {
         sign->on_current_page = 0;
      }

   } else {

      RoadMapDisplayPage = strdup(name);

      for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {

         if ((sign->page == NULL) ||
             (! strcmp (sign->page, RoadMapDisplayPage))) {
            sign->on_current_page = 1;
         } else {
            sign->on_current_page = 0;
         }
      }
   }
   RoadMapDisplayRefreshNeeded = 1;
}

/**
 * @brief Announce where we are. This function also figures out whether the street changed.
 *
 * This function is not only called when the GPS tells us about our position,
 * but also when the user highlights a position on the map.
 *
 * @param title name of the sign to use (failure if this is not predefined)
 * @param line
 * @param position
 * @param street
 * @return 0 on success
 */
int roadmap_display_activate (const char *title,
                              PluginLine *line,
                              const RoadMapPosition *position,
                              PluginStreet *street) {

    int   street_has_changed;
    int   message_has_changed;
    const char *format;
    char  text[256];
    RoadMapSign *sign;

    PluginStreetProperties properties;

    roadmap_log_push ("roadmap_display_activate");

    sign = roadmap_display_search_sign (title);
    if (sign == NULL) {
        roadmap_log_pop ();
        return -1;
    }

    if (sign->format_descriptor.category == NULL) {
       return -1; /* This is not a sign: this is a text. */
    }
    format = roadmap_config_get (&sign->format_descriptor);

    street_has_changed = 0;

    if (! roadmap_plugin_same_line (&sign->line, line)) {

       roadmap_plugin_get_street (line, street);

        if (sign->id != NULL) {
            free (sign->id);
        }
        sign->id = strdup (roadmap_plugin_street_full_name (line));

        sign->line = *line;
        sign->was_visible = 0;

        if (! roadmap_plugin_same_street (street, &sign->street)) {
           sign->street = *street;
           street_has_changed = 1;
        }
	roadmap_log (ROADMAP_WARNING, "roadmap_display_activate(%d, %s) %s",
			line ? line->line_id : 0,
			sign ? sign->id : "",
			street_has_changed ? " changed" : " unchanged");
    }


    roadmap_message_set ('F', sign->id);

    roadmap_plugin_get_street_properties (line, &properties);
    roadmap_message_set ('#', properties.address);
    roadmap_message_set ('N', properties.street);
    roadmap_message_set ('C', properties.city);


    if (! roadmap_message_format (text, sizeof(text), format)) {
        roadmap_log_pop ();
        *street = sign->street;
        return 0;
    }
    message_has_changed =
        (sign->content == NULL || strcmp (sign->content, text) != 0);

    sign->deadline =
        time(NULL)
            + roadmap_config_get_integer (&RoadMapConfigDisplayDuration);


    if (street_has_changed) {
        roadmap_voice_announce (sign->title);
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

    roadmap_plugin_line_from (line, &sign->endpoint[0]);
    roadmap_plugin_line_to   (line, &sign->endpoint[1]);

    if (position == NULL) {
        sign->has_position = 0;
    } else {
        sign->has_position = 1;
        sign->position = *position;
    }

    roadmap_plugin_update_position (position, line, street, street_has_changed);

    roadmap_log_pop ();
    *street = sign->street;
    RoadMapDisplayRefreshNeeded = 1;
    return 0;
}


void roadmap_display_hide (const char *title) {

    RoadMapSign *sign;

    sign = roadmap_display_search_sign (title);
    if (sign != NULL) {
        sign->deadline = 0;
        RoadMapDisplayRefreshNeeded = 1;
    }
}

static void roadmap_display_console_box
                (int corner, RoadMapConfigDescriptor *item) {

    const char *format;
    char text[256];
    int count;
    int width, ascent, descent;
    int canvas_width, canvas_height;

    RoadMapGuiPoint frame[4];


    format = roadmap_config_get (item);

    if (!format || !format[0])
        return;

    if (! roadmap_message_format (text, sizeof(text), format)) {
        return;
    }

    roadmap_screen_text_extents
        (ROADMAP_TEXT_SIGNS, text, RoadMapDisplayFontSize,
            &width, &ascent, &descent, NULL);

    canvas_width = roadmap_canvas_width();
    canvas_height = roadmap_canvas_height();

    if (corner & ROADMAP_CANVAS_RIGHT) {
        frame[2].x = canvas_width - 5;
        frame[0].x = frame[2].x - width - 6;
        if (frame[0].x < 5) {
            frame[0].x = 5;
        }
    } else {
        frame[0].x = 5;
        frame[2].x = frame[0].x + width + 6;
        if (frame[2].x > canvas_width) {
            frame[2].x = canvas_width - 5;
        }
    }
    frame[1].x = frame[0].x;
    frame[3].x = frame[2].x;

    if (corner & ROADMAP_CANVAS_BOTTOM) {
        frame[0].y = canvas_height - ascent - descent - 11;
        frame[1].y = canvas_height - 6;
    } else {
        frame[0].y = 6;
        frame[1].y = ascent + descent + 11;
    }
    frame[2].y = frame[1].y;
    frame[3].y = frame[0].y;


    count = 4;
    roadmap_canvas_select_pen (RoadMapConsoleBackground);
    roadmap_canvas_draw_multiple_polygons (1, &count, frame, 1, 0);

    roadmap_canvas_select_pen (RoadMapConsoleForeground);
    roadmap_canvas_draw_multiple_polygons (1, &count, frame, 0, 0);

    frame[0].x += 3;
    frame[0].y += 3;

    roadmap_screen_text(ROADMAP_TEXT_SIGNS, frame,
                ROADMAP_CANVAS_LEFT|ROADMAP_CANVAS_TOP,
                RoadMapDisplayFontSize, text);
}


void roadmap_display_text (const char *title, const char *format, ...) {

   RoadMapSign *sign = roadmap_display_search_sign (title);

   char text[1024];
   va_list parameters;

   va_start(parameters, format);
   vsnprintf (text, sizeof(text), format, parameters);
   va_end(parameters);

   if (sign->content != NULL) {
      free (sign->content);
   }
   sign->content = strdup(text);

   sign->deadline =
      time(NULL) + roadmap_config_get_integer (&RoadMapConfigDisplayDuration);

   RoadMapDisplayRefreshNeeded = 1;
}


void roadmap_display_signs (void) {

    time_t now = time(NULL);
    RoadMapSign *sign;

    RoadMapDisplayFontSize =
        roadmap_config_get_integer (&RoadMapConfigDisplayFontSize);

    roadmap_display_create_pens ();

    roadmap_display_console_box
        (ROADMAP_CANVAS_BOTTOM|ROADMAP_CANVAS_RIGHT,
         &RoadMapConfigDisplayBottomRight);

    roadmap_display_console_box
        (ROADMAP_CANVAS_BOTTOM|ROADMAP_CANVAS_LEFT,
         &RoadMapConfigDisplayBottomLeft);

    roadmap_display_console_box
        (ROADMAP_CANVAS_TOP|ROADMAP_CANVAS_LEFT,
         &RoadMapConfigDisplayTopLeft);

    roadmap_display_console_box
        (ROADMAP_CANVAS_TOP|ROADMAP_CANVAS_RIGHT,
         &RoadMapConfigDisplayTopRight);

    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {

        if ((sign->page == NULL) ||
            (RoadMapDisplayPage == NULL) ||
            (! strcmp (sign->page, RoadMapDisplayPage))) {

           if (sign->deadline > now && sign->content != NULL) {
               roadmap_display_sign (sign);
               if (sign->deadline > RoadMapDisplayDeadline) {
                  RoadMapDisplayDeadline = sign->deadline;
               }
           }
        }
    }
}


const char *roadmap_display_get_id (const char *title) {

    RoadMapSign *sign = roadmap_display_search_sign (title);

    if (sign == NULL || (! sign->has_position)) return NULL;

    return sign->id;
}

void roadmap_display_periodic(void) {

    static char thentime[16];
    int need_time_update = 0;
    time_t now = time(NULL);
    
    if (roadmap_message_time_in_use()) {
        char nowtime[32];
        strcpy(nowtime, roadmap_time_get_hours_minutes (now));

        if (strcmp(thentime, nowtime) != 0) {
            strcpy(thentime, nowtime);
            need_time_update = 1;
        }
    }

    if (need_time_update ||
            (RoadMapDisplayDeadline && now > RoadMapDisplayDeadline)) {
        roadmap_message_set ('T', thentime);
        RoadMapDisplayDeadline = 0;
        RoadMapDisplayRefreshNeeded = 1;
        roadmap_screen_refresh ();
    }
}


int roadmap_display_is_refresh_needed (void) {

    if (RoadMapDisplayRefreshNeeded) {
        RoadMapDisplayRefreshNeeded = 0;
        return 1;
    }
    return 0;
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
        ("preferences", &RoadMapConfigDisplayTopRight, "In %Y, %X|%X");
    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayTopLeft, "");

    roadmap_config_declare
        ("preferences", &RoadMapConfigConsoleBackground, "yellow");
    roadmap_config_declare
        ("preferences", &RoadMapConfigConsoleForeground, "black");

    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {

        if (sign->default_format != NULL) {
           roadmap_config_declare
              ("preferences",
               &sign->format_descriptor, sign->default_format);
        }

        roadmap_config_declare
            ("preferences",
             &sign->background_descriptor, sign->default_background);

        roadmap_config_declare
            ("preferences",
             &sign->foreground_descriptor, sign->default_foreground);
    }

    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayFontSize, "20");

    roadmap_main_set_periodic (3000, roadmap_display_periodic);
}
