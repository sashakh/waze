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

static PangoLayout  *RoadMapLayout = NULL;


/* The canvas callbacks: all callbacks are initialized to do-nothing
 * functions, so that we don't care checking if one has been setup.
 */
static void roadmap_canvas_ignore_button (RoadMapGuiPoint *point) {}

static RoadMapCanvasButtonHandler RoadMapCanvasMouseButton =
                                     roadmap_canvas_ignore_button;


static void roadmap_canvas_ignore_configure (void) {}

static RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
                                     roadmap_canvas_ignore_configure;

static void roadmap_canvas_convert_points
                (GdkPoint *gdkpoints, RoadMapGuiPoint *points, int count) {

    RoadMapGuiPoint *end = points + count;

    while (points < end) {
        gdkpoints->x = points->x;
        gdkpoints->y = points->y;
        gdkpoints += 1;
        points += 1;
    }
}


void roadmap_canvas_get_text_extents 
        (const char *text, int *width, int *ascent, int *descent) {

   PangoRectangle rectangle;

   if (RoadMapLayout == NULL) {
       RoadMapLayout = gtk_widget_create_pango_layout
                           (GTK_WIDGET(RoadMapDrawingArea), text);
       pango_layout_set_width (RoadMapLayout, -1);
   }
   pango_layout_set_text (RoadMapLayout, text, -1);

   pango_layout_get_extents (RoadMapLayout, NULL, &rectangle);

   *width   = rectangle.width / PANGO_SCALE;
   *ascent  = PANGO_ASCENT(rectangle) / PANGO_SCALE;
   *descent = PANGO_DESCENT(rectangle) / PANGO_SCALE;
}


void roadmap_canvas_select_pen (RoadMapPen pen) {

   RoadMapGc = pen->gc;
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
      if (pen == NULL) {
         roadmap_log (ROADMAP_FATAL, "no more memory");
      }

      pen->name = strdup (name);
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
   int text_height;

   roadmap_canvas_get_text_extents 
        (text, &text_width, &text_ascent, &text_descent);

   text_height = text_ascent + text_descent;

   switch (corner) {

   case ROADMAP_CANVAS_TOPLEFT:
      y = position->y;
      x = position->x;
      break;

   case ROADMAP_CANVAS_TOPRIGHT:
      y = position->y;
      x = position->x - text_width;
      break;

   case ROADMAP_CANVAS_BOTTOMRIGHT:
      y = position->y - text_height;
      x = position->x - text_width;
      break;

   case ROADMAP_CANVAS_BOTTOMLEFT:
      y = position->y - text_height;
      x = position->x;
      break;

   case ROADMAP_CANVAS_CENTER:
      y = position->y + (text_height / 2);
      x = position->x - (text_width / 2);
      break;

   default:
      return;
   }

   gdk_draw_layout  (RoadMapDrawingBuffer, RoadMapGc, x, y, RoadMapLayout);
}


void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points) {

   GdkPoint gdkpoints[1024];

   while (count > 1024) {
       roadmap_canvas_convert_points (gdkpoints, points, 1024);
       gdk_draw_points (RoadMapDrawingBuffer, RoadMapGc, gdkpoints, 1024);
       points += 1024;
       count -= 1024;
   }
   roadmap_canvas_convert_points (gdkpoints, points, count);
   gdk_draw_points (RoadMapDrawingBuffer, RoadMapGc, gdkpoints, count);
}


void roadmap_canvas_draw_multiple_lines 
         (int count, int *lines, RoadMapGuiPoint *points) {

   int i;
   int count_of_points;
   GdkPoint gdkpoints[1024];

   for (i = 0; i < count; ++i) {

      count_of_points = *lines;

      while (count_of_points > 1024) {
          roadmap_canvas_convert_points (gdkpoints, points, 1024);
          gdk_draw_lines (RoadMapDrawingBuffer, RoadMapGc, gdkpoints, 1024);

          /* We shift by 1023 only, because we must link the lines. */
          points += 1023;
          count_of_points -= 1023;
      }

      roadmap_canvas_convert_points (gdkpoints, points, count_of_points);
      gdk_draw_lines (RoadMapDrawingBuffer,
                      RoadMapGc, gdkpoints, count_of_points);

      points += count_of_points;
      lines += 1;
   }
}


void roadmap_canvas_draw_multiple_polygons
         (int count, int *polygons, RoadMapGuiPoint *points, int filled) {

   int i;
   int count_of_points;
   GdkPoint gdkpoints[1024];

   for (i = 0; i < count; ++i) {

      count_of_points = *polygons;

      while (count_of_points > 1024) {
          roadmap_canvas_convert_points (gdkpoints, points, 1024);
          gdk_draw_polygon (RoadMapDrawingBuffer,
                            RoadMapGc, filled, gdkpoints, 1024);

          /* We shift by 1023 only, because we must link the lines. */
          points += 1023;
          count_of_points -= 1023;
      }

      roadmap_canvas_convert_points (gdkpoints, points, count_of_points);
      gdk_draw_polygon (RoadMapDrawingBuffer,
                        RoadMapGc, filled, gdkpoints, count_of_points);

      polygons += 1;
      points += count_of_points;
   }
}


void roadmap_canvas_draw_multiple_circles
        (int count, RoadMapGuiPoint *centers, int *radius, int filled) {

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


static gint roadmap_canvas_button_pressed
               (GtkWidget *w, GdkEventButton *event, gpointer data) {

   RoadMapGuiPoint point;

   point.x = event->x;
   point.y = event->y;

   (*RoadMapCanvasMouseButton) (&point);

   return FALSE;
}


void roadmap_canvas_register_button_handler
                    (RoadMapCanvasButtonHandler handler) {

   RoadMapCanvasMouseButton = handler;
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

   gtk_widget_queue_draw_area
       (RoadMapDrawingArea, 0, 0,
        RoadMapDrawingArea->allocation.width,
        RoadMapDrawingArea->allocation.height);
}


GtkWidget *roadmap_canvas_new (void) {

   RoadMapDrawingArea = gtk_drawing_area_new ();

   gtk_widget_set_events (RoadMapDrawingArea, GDK_BUTTON_PRESS_MASK);

   g_signal_connect (RoadMapDrawingArea,
                     "expose_event",
                     (GCallback) roadmap_canvas_expose,
                     NULL);

   g_signal_connect (RoadMapDrawingArea,
                     "configure_event",
                     (GCallback) roadmap_canvas_configure,
                     NULL);

   g_signal_connect (RoadMapDrawingArea,
                     "button_press_event",
                     (GCallback) roadmap_canvas_button_pressed,
                     NULL);

   RoadMapGc = RoadMapDrawingArea->style->black_gc;

   return RoadMapDrawingArea;
}

