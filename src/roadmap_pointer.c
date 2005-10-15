/* roadmap_pointer.c - Manage mouse/pointer events
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *   See roadmap_pointer.h.
 */

#include <time.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_pointer.h"
#include "roadmap_gui.h"
#include "roadmap_canvas.h"
#include "roadmap_main.h"

#define LONG_CLICK_TIMEOUT 1000
#define DRAG_FLOW_CONTROL_TIMEOUT 30

static int is_button_down = 0;
static int is_dragging = 0;
static int is_drag_flow_control_on = 0;

static RoadMapGuiPoint last_pointer_point;

/* The pointer events callbacks: all callbacks are initialized to do-nothing
 * functions, so that we don't care checking if one has been setup.
 */
static void roadmap_pointer_ignore_event (RoadMapGuiPoint *point) {}

static RoadMapPointerHandler RoadMapPointerShortClick =
                                     roadmap_pointer_ignore_event;

static RoadMapPointerHandler RoadMapPointerLongClick =
                                     roadmap_pointer_ignore_event;

static RoadMapPointerHandler RoadMapPointerDragStart =
                                     roadmap_pointer_ignore_event;

static RoadMapPointerHandler RoadMapPointerDragMotion =
                                     roadmap_pointer_ignore_event;

static RoadMapPointerHandler RoadMapPointerDragEnd =
                                     roadmap_pointer_ignore_event;


static void roadmap_pointer_button_timeout(void) {

   roadmap_main_remove_periodic(roadmap_pointer_button_timeout);
   RoadMapPointerLongClick(&last_pointer_point);
   is_button_down = 0;
}
 
/* Instead of calling the drag motion event with every mouse move,
 * we use this timer as a flow control. It may take time for the
 * application to finish the task of drawing the screen and we don't
 * want to lag.
 */
static void roadmap_pointer_drag_flow_control(void) {

   roadmap_main_remove_periodic(roadmap_pointer_drag_flow_control);
   RoadMapPointerDragMotion(&last_pointer_point);
   is_drag_flow_control_on = 0;
}
   

static void roadmap_pointer_button_pressed (RoadMapGuiPoint *point) {
   is_button_down = 1;    
   last_pointer_point = *point;
   roadmap_main_set_periodic
      (LONG_CLICK_TIMEOUT, roadmap_pointer_button_timeout);
}


static void roadmap_pointer_button_released (RoadMapGuiPoint *point) {
    
   if (is_dragging) {
      if (is_drag_flow_control_on) {
         roadmap_main_remove_periodic(roadmap_pointer_drag_flow_control);
         is_drag_flow_control_on = 0;
      }
      RoadMapPointerDragEnd(point);
      is_dragging = 0;
      is_button_down = 0;
   } else if (is_button_down) {
      roadmap_main_remove_periodic(roadmap_pointer_button_timeout);
      RoadMapPointerShortClick(point);
      is_button_down = 0;
   }
}


static void roadmap_pointer_moved (RoadMapGuiPoint *point) {
   if (!is_button_down && !is_dragging) return;

   if (!is_dragging) {

      /* Less sensitive, since a car is not a quiet environment... */
      if ((abs(point->x - last_pointer_point.x) <= 3) &&
          (abs(point->y - last_pointer_point.y) <= 3)) return;

      roadmap_main_remove_periodic(roadmap_pointer_button_timeout);
      RoadMapPointerDragStart(&last_pointer_point);
      last_pointer_point = *point;
      is_drag_flow_control_on = 1;
      roadmap_main_set_periodic
         (DRAG_FLOW_CONTROL_TIMEOUT, roadmap_pointer_drag_flow_control);
      is_dragging = 1;
   } else {
      /* the flow control timer will execute the handler */
      last_pointer_point = *point;
      if (!is_drag_flow_control_on) {
         is_drag_flow_control_on = 1;
         roadmap_main_set_periodic
            (DRAG_FLOW_CONTROL_TIMEOUT, roadmap_pointer_drag_flow_control);
      }
   }
}


void roadmap_pointer_initialize (void) {

   roadmap_canvas_register_button_pressed_handler
      (&roadmap_pointer_button_pressed);
   roadmap_canvas_register_button_released_handler
      (&roadmap_pointer_button_released);
   roadmap_canvas_register_mouse_move_handler
      (&roadmap_pointer_moved);
}


RoadMapPointerHandler roadmap_pointer_register_short_click
                    (RoadMapPointerHandler handler) {

   RoadMapPointerHandler old = RoadMapPointerShortClick;
   RoadMapPointerShortClick = handler;

   return old;
}


RoadMapPointerHandler roadmap_pointer_register_long_click
                    (RoadMapPointerHandler handler) {

   RoadMapPointerHandler old = RoadMapPointerLongClick;
   RoadMapPointerLongClick = handler;

   return old;
}


RoadMapPointerHandler roadmap_pointer_register_drag_start
                    (RoadMapPointerHandler handler) {

   RoadMapPointerHandler old = RoadMapPointerDragStart;
   RoadMapPointerDragStart = handler;

   return old;
}


RoadMapPointerHandler roadmap_pointer_register_drag_motion
                    (RoadMapPointerHandler handler) {

   RoadMapPointerHandler old = RoadMapPointerDragMotion;
   RoadMapPointerDragMotion = handler;

   return old;
}


RoadMapPointerHandler roadmap_pointer_register_drag_end
                    (RoadMapPointerHandler handler) {

   RoadMapPointerHandler old = RoadMapPointerDragEnd;
   RoadMapPointerDragEnd = handler;

   return old;
}


