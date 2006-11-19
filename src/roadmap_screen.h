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

void roadmap_screen_refresh (void); /* Conditional: only if needed. */
void roadmap_screen_redraw  (void); /* Force a screen redraw, no move. */

void roadmap_screen_hold     (void); /* Hold on at the current position. */
void roadmap_screen_freeze   (void); /* Forbid any screen refresh. */
void roadmap_screen_unfreeze (void); /* Enable screen refresh. */

void roadmap_screen_get_center (RoadMapPosition *center);

typedef void (*RoadMapScreenSubscriber) (void);
typedef void (*RoadMapShapeIterator) (int shape, RoadMapPosition *position);

void roadmap_screen_subscribe_after_refresh (RoadMapScreenSubscriber handler);

void roadmap_screen_draw_line (const RoadMapPosition *from,
                               const RoadMapPosition *to,
                               int fully_visible,
                               const RoadMapPosition *shape_start,
                               int first_shape,
                               int last_shape,
                               RoadMapShapeIterator shape_next,
                               RoadMapPen pen,
                               int *total_length,
                               RoadMapGuiPoint *middle,
                               int *angle);

int roadmap_screen_is_dragging (void);

#define ROADMAP_TEXT_SIGNS  0x01
#define ROADMAP_TEXT_LABELS 0x02
void roadmap_screen_text
     (int id, RoadMapGuiPoint *center, int where, int size, const char *text);
void roadmap_screen_text_angle 
        (int id, RoadMapGuiPoint *start, RoadMapGuiPoint *center,
                int theta, int size, const char *text);
void roadmap_screen_text_extents 
        (int id, const char *text, int size,
         int *width, int *ascent, int *descent, int *can_tilt);

#endif // INCLUDE__ROADMAP_SCREEN__H
