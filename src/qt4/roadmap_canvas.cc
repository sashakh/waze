/* roadmap_canvas.cc - A C to C++ wrapper for the QT RoadMap canvas
 *
 * LICENSE:
 *
 *   (c) Copyright 2003 Latchesar Ionkov
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
 *   See roadmap_canvas.h
 */
extern "C" {
#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"

#include "roadmap_canvas.h"
}

#include "qimage.h"
#include "qt_canvas.h"



void roadmap_canvas_register_button_pressed_handler(RoadMapCanvasMouseHandler handler) {
   if (roadMapCanvas) {
      roadMapCanvas->registerButtonPressedHandler(handler);
   } else {
      phandler = handler;
   }
}

void roadmap_canvas_register_button_released_handler(RoadMapCanvasMouseHandler handler) {
   if (roadMapCanvas) {
      roadMapCanvas->registerButtonReleasedHandler(handler);
   } else {
      rhandler = handler;
   }
}

void roadmap_canvas_register_mouse_move_handler(RoadMapCanvasMouseHandler handler) {
   if (roadMapCanvas) {
      roadMapCanvas->registerMouseMoveHandler(handler);
   } else {
      mhandler = handler;
   }
}

void roadmap_canvas_register_mouse_scroll_handler
                    (RoadMapCanvasMouseHandler handler) {

   if (roadMapCanvas) {
      roadMapCanvas->registerMouseWheelHandler(handler);
   } else {
      whandler = handler;
   }
}


void roadmap_canvas_register_configure_handler(
   RoadMapCanvasConfigureHandler handler) {

   if (roadMapCanvas) {
      roadMapCanvas->registerConfigureHandler(handler);
   } else {
      chandler = handler;
   }
}

void roadmap_canvas_get_text_extents(const char *text, int size,
   int *width, int *ascent, int *descent, int *can_tilt) {

   roadMapCanvas->getTextExtents(text, width, ascent, descent, can_tilt);
}

RoadMapPen roadmap_canvas_create_pen (const char *name) {
   return roadMapCanvas->createPen(name);
}

RoadMapPen roadmap_canvas_select_pen (RoadMapPen pen) {

   static RoadMapPen CurrentPen;

   RoadMapPen old_pen = CurrentPen;
   CurrentPen = pen;
   roadMapCanvas->selectPen(pen);
   return old_pen;
}

void roadmap_canvas_set_foreground (const char *color) {
   roadMapCanvas->setPenColor(color);
}

void roadmap_canvas_set_linestyle (const char *style) {
   roadMapCanvas->setPenLineStyle(style);
}

void roadmap_canvas_set_thickness  (int thickness) {
   roadMapCanvas->setPenThickness(thickness);
}

void roadmap_canvas_erase (void) {
   roadMapCanvas->erase();
}

void roadmap_canvas_draw_string(RoadMapGuiPoint *position, int corner,
   int size,
   const char *text) {

   roadMapCanvas->drawString(position, corner, text);
}

void roadmap_canvas_draw_string_angle (RoadMapGuiPoint *position,
                                       int size,
                                       int angle, const char *text)
{
   roadMapCanvas->drawStringAngle(position, 0, text, angle);
}

void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points) {
   roadMapCanvas->drawMultiplePoints(count, points);
}

void roadmap_canvas_draw_multiple_lines(int count, int *lines, 
   RoadMapGuiPoint *points, int fast_draw) {

   roadMapCanvas->drawMultipleLines(count, lines, points);
}

void roadmap_canvas_draw_multiple_polygons(int count, int *polygons, 
   RoadMapGuiPoint *points, int filled, int fast_draw) {

   roadMapCanvas->drawMultiplePolygons(count, polygons, points, filled);
}

void roadmap_canvas_draw_multiple_circles(int count, RoadMapGuiPoint *centers, 
   int *radius, int filled, int fast_draw) {

   roadMapCanvas->drawMultipleCircles(count, centers, radius, filled);
}

int roadmap_canvas_width (void) {
   return roadMapCanvas->getWidth();
}

int roadmap_canvas_height (void) {
   return roadMapCanvas->getHeight();
}


void roadmap_canvas_refresh (void) {
   roadMapCanvas->refresh();
}


void roadmap_canvas_save_screenshot (const char* filename) {

   QPixmap pixmap;
   QString name (filename);

   pixmap = QPixmap::grabWidget (roadMapCanvas);
   pixmap.save (name, "PNG");
}

