/* roadmap_canvas.c - manage the canvas that is used to draw the map.
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
 *   See roadmap_canvas.h.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"

#include "roadmap_canvas.h"
#include "roadmap_gtkcanvas.h"


#define ROADMAP_CURSOR_SIZE           10
#define ROADMAP_CANVAS_POINT_BLOCK  1024


struct roadmap_canvas_pen {

   struct roadmap_canvas_pen *next;

   char  *name;
   GdkGC *gc;
};


static struct roadmap_canvas_pen *RoadMapPenList = NULL;

static GtkWidget  *RoadMapDrawingArea;
static GdkPixmap  *RoadMapDrawingBuffer;
static GdkGC      *RoadMapGc;


/* The canvas callbacks: all callbacks are initialized to do-nothing
 * functions, so that we don't care checking if one has been setup.
 */
static void roadmap_canvas_ignore_mouse (int button, RoadMapGuiPoint *point) {}

static RoadMapCanvasMouseHandler
         RoadMapCanvasMouseButtonPressed = roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler
         RoadMapCanvasMouseButtonReleased = roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler
         RoadMapCanvasMouseMoved = roadmap_canvas_ignore_mouse;


static void roadmap_canvas_ignore_configure (void) {}

static RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
                                     roadmap_canvas_ignore_configure;


void roadmap_canvas_get_text_extents 
        (const char *text, int size, int *width,
            int *ascent, int *descent, int *can_tilt) {

    static int RoadMapCanvasAscent = 0;
    static int RoadMapCanvasDescent = 0;

    int unused;

    gdk_text_extents (RoadMapDrawingArea->style->font,
                      text,
                      strlen(text),
                      &unused,
                      &unused,
                      width,
                      &unused,
                      &unused);

    if (RoadMapCanvasDescent == 0) {
        
        char *pattern = "([,%!@#$]){}|;/?Wjlq";
        
        gdk_text_extents (RoadMapDrawingArea->style->font,
                          pattern,
                          strlen(pattern),
                          &unused,
                          &unused,
                          &unused,
                          &RoadMapCanvasAscent,
                          &RoadMapCanvasDescent);
    }
    
    *ascent  = RoadMapCanvasAscent;
    *descent = RoadMapCanvasDescent;
    if (can_tilt) *can_tilt = 0;
}


RoadMapPen roadmap_canvas_select_pen (RoadMapPen pen) {

   static RoadMapPen CurrentPen;

   RoadMapPen old_pen = CurrentPen;
   CurrentPen = pen;
   RoadMapGc = pen->gc;
   return old_pen;
}


RoadMapPen roadmap_canvas_create_pen (const char *name) {

   struct roadmap_canvas_pen *pen;

   for (pen = RoadMapPenList; pen != NULL; pen = pen->next) {
      if (strcmp(pen->name, name) == 0) break;
   }

   if (pen == NULL) {

      /* This is a new pen: create it. */

      GdkGC *gc;

      gc = gdk_gc_new (RoadMapDrawingBuffer);

      gdk_gc_set_fill (gc, GDK_SOLID);

      pen = malloc (sizeof(*pen));
      roadmap_check_allocated(pen);

      pen->name = strdup (name);
      roadmap_check_allocated(pen->name);

      pen->gc   = gc;
      pen->next = RoadMapPenList;

      RoadMapPenList = pen;
   }

   roadmap_canvas_select_pen (pen);

   return pen;
}


void roadmap_canvas_set_foreground (const char *color) {

   GdkColor *native_color;


   native_color = (GdkColor *) g_malloc (sizeof(GdkColor));

   gdk_color_parse (color, native_color);
   gdk_color_alloc (gdk_colormap_get_system(), native_color);

   gdk_gc_set_foreground (RoadMapGc, native_color);
}


void roadmap_canvas_set_thickness  (int thickness) {

   gdk_gc_set_line_attributes
      (RoadMapGc, thickness, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
}


void roadmap_canvas_erase (void) {

   gdk_draw_rectangle (RoadMapDrawingBuffer,
                       RoadMapGc,
                       TRUE,
                       0, 0,
                       RoadMapDrawingArea->allocation.width,
                       RoadMapDrawingArea->allocation.height);
}


void roadmap_canvas_draw_string (RoadMapGuiPoint *position,
                                 int corner,
                                 const char *text) {

   int x;
   int y;
   int text_width;
   int text_ascent;
   int text_descent;

   roadmap_canvas_get_text_extents 
        (text, -1, &text_width, &text_ascent, &text_descent, NULL);

   switch (corner) {

   case ROADMAP_CANVAS_TOPLEFT:
      y = position->y + text_ascent;
      x = position->x;
      break;

   case ROADMAP_CANVAS_TOPRIGHT:
      y = position->y + text_ascent;
      x = position->x - text_width;
      break;

   case ROADMAP_CANVAS_BOTTOMRIGHT:
      y = position->y - text_descent;
      x = position->x - text_width;
      break;

   case ROADMAP_CANVAS_BOTTOMLEFT:
      y = position->y - text_descent;
      x = position->x;
      break;

   case ROADMAP_CANVAS_CENTER:
      y = position->y + (text_ascent / 2);
      x = position->x - (text_width / 2);
      break;

   default:
      return;
   }

   gdk_draw_string  (RoadMapDrawingBuffer,
                     RoadMapDrawingArea->style->font, RoadMapGc, x, y, text);
}

void roadmap_canvas_draw_string_angle (RoadMapGuiPoint *position,
                                       RoadMapGuiPoint *center,
                                       int angle, const char *text)
{
    /* no angle possible, currently.  at least try and center the text */
    roadmap_canvas_draw_string (center, ROADMAP_CANVAS_CENTER, text);
}


void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points) {

   gdk_draw_points (RoadMapDrawingBuffer,
                    RoadMapGc, (GdkPoint *)points, count);
}


void roadmap_canvas_draw_multiple_lines 
         (int count, int *lines, RoadMapGuiPoint *points, int fast_draw) {

   int i;

   for (i = 0; i < count; ++i) {

      int count_of_points = *lines;

      gdk_draw_lines (RoadMapDrawingBuffer,
                      RoadMapGc, (GdkPoint *)points, count_of_points);

      lines += 1;
      points += count_of_points;
   }
}


void roadmap_canvas_draw_multiple_polygons
         (int count, int *polygons, RoadMapGuiPoint *points, int filled,
            int fast_draw) {

   int i;

   for (i = 0; i < count; ++i) {

      int count_of_points = *polygons;

      gdk_draw_polygon (RoadMapDrawingBuffer,
                        RoadMapGc, filled, (GdkPoint *)points, count_of_points);

      polygons += 1;
      points += count_of_points;
   }
}


void roadmap_canvas_draw_multiple_circles
        (int count, RoadMapGuiPoint *centers, int *radius, int filled,
            int fast_draw) {

   int i;

   for (i = 0; i < count; ++i) {

      int r = radius[i];

      gdk_draw_arc (RoadMapDrawingBuffer,
                    RoadMapGc,
                    filled,
                    centers[i].x - r,
                    centers[i].y - r,
                    2 * r,
                    2 * r,
                    0,
                    (360 * 64));
   }
}


static gint roadmap_canvas_configure
               (GtkWidget *widget, GdkEventConfigure *event) {

   if (RoadMapDrawingBuffer != NULL) {
      gdk_pixmap_unref (RoadMapDrawingBuffer);
   }

   RoadMapDrawingBuffer =
      gdk_pixmap_new (widget->window,
                      widget->allocation.width,
                      widget->allocation.height,
                      -1);

   (*RoadMapCanvasConfigure) ();

   return TRUE;
}


void roadmap_canvas_register_configure_handler
                    (RoadMapCanvasConfigureHandler handler) {

   RoadMapCanvasConfigure = handler;
}


static gint roadmap_canvas_expose (GtkWidget *widget, GdkEventExpose *event) {

   gdk_draw_pixmap (widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                    RoadMapDrawingBuffer,
                    event->area.x, event->area.y,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);

   return FALSE;
}


static gint roadmap_canvas_mouse_event
               (GtkWidget *w, GdkEventButton *event, gpointer data) {

   RoadMapGuiPoint point;

   point.x = event->x;
   point.y = event->y;

   switch ((int) data) {
      case 1:
         (*RoadMapCanvasMouseButtonPressed) (event->button, &point);
         break;
      case 2:
         (*RoadMapCanvasMouseButtonReleased) (event->button, &point);
         break;
      case 3:
         (*RoadMapCanvasMouseMoved) (0, &point);
         break;
   }

   return FALSE;
}


void roadmap_canvas_register_button_pressed_handler
                    (RoadMapCanvasMouseHandler handler) {

   RoadMapCanvasMouseButtonPressed = handler;
}


void roadmap_canvas_register_button_released_handler
                    (RoadMapCanvasMouseHandler handler) {
       
   RoadMapCanvasMouseButtonReleased = handler;
}


void roadmap_canvas_register_mouse_move_handler
                    (RoadMapCanvasMouseHandler handler) {

   RoadMapCanvasMouseMoved = handler;
}


void roadmap_canvas_register_mouse_scroll_handler
                    (RoadMapCanvasMouseHandler handler) {

   // TBD.
}


int roadmap_canvas_width (void) {

   if (RoadMapDrawingArea == NULL) {
      return 0;
   }
   return RoadMapDrawingArea->allocation.width;
}

int roadmap_canvas_height (void) {

   if (RoadMapDrawingArea == NULL) {
      return 0;
   }
   return RoadMapDrawingArea->allocation.height;
}


void roadmap_canvas_refresh (void) {

   GdkRectangle update;

   update.x = 0;
   update.y = 0;
   update.width  = RoadMapDrawingArea->allocation.width;
   update.height = RoadMapDrawingArea->allocation.height;

   gtk_widget_draw (RoadMapDrawingArea, &update);
}


GtkWidget *roadmap_canvas_new (void) {

   RoadMapDrawingArea = gtk_drawing_area_new ();

   gtk_widget_set_events
      (RoadMapDrawingArea,
       GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_POINTER_MOTION_MASK);

   gtk_signal_connect (GTK_OBJECT(RoadMapDrawingArea),
                       "expose_event",
                       (GtkSignalFunc) roadmap_canvas_expose,
                       NULL);

   gtk_signal_connect (GTK_OBJECT(RoadMapDrawingArea),
                       "configure_event",
                       (GtkSignalFunc) roadmap_canvas_configure,
                       NULL);

   gtk_signal_connect (GTK_OBJECT(RoadMapDrawingArea),
                       "button_press_event",
                       (GtkSignalFunc) roadmap_canvas_mouse_event,
                       (gpointer)1);

   gtk_signal_connect (GTK_OBJECT(RoadMapDrawingArea),
                       "button_release_event",
                       (GtkSignalFunc) roadmap_canvas_mouse_event,
                       (gpointer)2);

   gtk_signal_connect (GTK_OBJECT(RoadMapDrawingArea),
                       "motion_notify_event",
                       (GtkSignalFunc) roadmap_canvas_mouse_event,
                       (gpointer)3);

   RoadMapGc = RoadMapDrawingArea->style->black_gc;

   return RoadMapDrawingArea;
}


void roadmap_canvas_save_screenshot (const char* filename) {
   // NOT IMPLEMENTED.
}

