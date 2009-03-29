/* roadmap_screen.h - draw the map on the screen.
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
 */

#ifndef INCLUDE__ROADMAP_SCREEN__H
#define INCLUDE__ROADMAP_SCREEN__H

#include "roadmap_types.h"
#include "roadmap_canvas.h"

enum { ORIENTATION_DYNAMIC = 0,
       ORIENTATION_FIXED
};

enum { VIEW_MODE_2D = 0,
       VIEW_MODE_3D
};

void roadmap_screen_initialize (void);

void roadmap_screen_set_initial_position (void);

void roadmap_screen_zoom_in    (void);
void roadmap_screen_zoom_out   (void);
void roadmap_screen_zoom_reset (void);

void roadmap_screen_move_up    (void);
void roadmap_screen_move_down  (void);
void roadmap_screen_move_right (void);
void roadmap_screen_move_left  (void);

void roadmap_screen_toggle_view_mode (void);
void roadmap_screen_toggle_labels (void);
void roadmap_screen_toggle_orientation_mode (void);
void roadmap_screen_increase_horizon (void);
void roadmap_screen_decrease_horizon (void);

void roadmap_screen_rotate (int delta);

void roadmap_screen_refresh (void); /* checks if needed before repainting */

void roadmap_screen_repaint (void);

void roadmap_screen_hold     (void); /* Hold on at the current position. */
void roadmap_screen_freeze   (void); /* Forbid any screen refresh. */
void roadmap_screen_unfreeze (void); /* Enable screen refresh. */

void roadmap_screen_configure (void);

void roadmap_screen_set_cursor (RoadMapCursor newcursor);
int roadmap_screen_busy_check (int total, int completed);

typedef void (*RoadMapScreenSubscriber) (void);
typedef void (*RoadMapShapeIterator) (int shape, RoadMapPosition *position);

int roadmap_screen_is_dragging (void);

#define ROADMAP_TEXT_SIGNS  0x01
#define ROADMAP_TEXT_LABELS 0x02
void roadmap_screen_text
     (int id, RoadMapGuiPoint *center, int where, int size, const char *text);
void roadmap_screen_text_angle 
        (int id, RoadMapGuiPoint *center,
                int theta, int size, const char *text);
void roadmap_screen_text_extents 
        (int id, const char *text, int size,
         int *width, int *ascent, int *descent, int *can_tilt);

#ifdef ROADMAP_DBG_TIME

#define DBG_TIME_FULL 0
#define DBG_TIME_DRAW_SQUARE 1
#define DBG_TIME_DRAW_ONE_LINE 2
#define DBG_TIME_SELECT_PEN 3
#define DBG_TIME_DRAW_LINES 4
#define DBG_TIME_CREATE_PATH 5
#define DBG_TIME_ADD_PATH 6
#define DBG_TIME_FLIP 7
#define DBG_TIME_TEXT_FULL 8
#define DBG_TIME_TEXT_CNV 9
#define DBG_TIME_TEXT_LOAD 10
#define DBG_TIME_TEXT_ONE_LETTER 11
#define DBG_TIME_TEXT_GET_GLYPH 12
#define DBG_TIME_TEXT_ONE_RAS 13
#define DBG_TIME_DRAW_LONG_LINES 14
#define DBG_TIME_FIND_LONG_LINES 15

#define DBG_TIME_LAST_COUNTER 16

void dbg_time_start(int type);
void dbg_time_end(int type);

#else

#define dbg_time_start(x)
#define dbg_time_end(x)

#endif  // ROADMAP_DBG_TIME

#ifdef	HAVE_TRIP_PLUGIN
int roadmap_screen_height(void);
void roadmap_screen_move_center (int dy);
#endif

#endif // INCLUDE__ROADMAP_SCREEN__H
