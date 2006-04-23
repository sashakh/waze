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
#include <wchar.h>

#include <agg_rendering_buffer.h>
#include <agg_curves.h>
#include <agg_conv_stroke.h>
#include <agg_conv_contour.h>
#include <agg_conv_transform.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_rasterizer_outline_aa.h>
#include <agg_rasterizer_outline.h>
#include <agg_renderer_primitives.h>
#include <agg_ellipse.h>
#include <agg_renderer_scanline.h>
#include <agg_scanline_p.h>
#include <agg_renderer_outline_aa.h>
#include <agg_pixfmt_rgb_packed.h>
#include <agg_path_storage.h>

#include <agg_font_freetype.h>

#ifdef USE_FRIBIDI
#include <fribidi.h>
#define MAX_STR_LEN 65000
#endif

extern "C" {
#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_config.h"
#include "roadmap_messagebox.h"
#include "roadmap_screen.h"
#include "roadmap_path.h"

#include "roadmap_canvas.h"
#include "roadmap_gtkcanvas.h"
}

#define ROADMAP_CURSOR_SIZE           10
#define ROADMAP_CANVAS_POINT_BLOCK  1024

#define GetRValue(x) x.red
#define GetGValue(x) x.green
#define GetBValue(x) x.blue

typedef agg::pixfmt_rgb565 pixfmt;
typedef agg::renderer_base<pixfmt> renbase_type;
typedef agg::renderer_primitives<renbase_type> renderer_pr;
typedef agg::font_engine_freetype_int32 font_engine_type;
typedef agg::font_cache_manager<font_engine_type> font_manager_type;

static agg::rendering_buffer rbuf;

static pixfmt pixf(rbuf);
static agg::renderer_base<pixfmt> renb;

static agg::line_profile_aa profile(2, agg::gamma_none());

static agg::renderer_outline_aa<renbase_type> reno(renb, profile);
static agg::rasterizer_outline_aa< agg::renderer_outline_aa<renbase_type> >  raso(reno);

static agg::rasterizer_scanline_aa<> ras;
static agg::scanline_p8 sl;
static agg::renderer_scanline_aa_solid<agg::renderer_base<pixfmt> > ren_solid(renb);

static font_engine_type             m_feng;
static font_manager_type            m_fman(m_feng);

static RoadMapConfigDescriptor RoadMapConfigFont =
                        ROADMAP_CONFIG_ITEM("Labels", "FontName");


struct roadmap_canvas_pen {

   struct roadmap_canvas_pen *next;

   char  *name;
   GdkGC *gc;
   GdkColor color;
	int thickness;
};


static struct roadmap_canvas_pen *RoadMapPenList = NULL;

static GtkWidget  *RoadMapDrawingArea;
static GdkImage   *RoadMapDrawingBuffer;
static RoadMapPen	CurrentPen;

static int        RoadMapCanvasFontLoaded = 0;


/* The canvas callbacks: all callbacks are initialized to do-nothing
 * functions, so that we don't care checking if one has been setup.
 */
static void roadmap_canvas_ignore_mouse (RoadMapGuiPoint *point) {}

static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonPressed =
                                     roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonReleased =
                                     roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler RoadMapCanvasMouseMoved =
                                     roadmap_canvas_ignore_mouse;


static void roadmap_canvas_ignore_configure (void) {}

static RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
                                     roadmap_canvas_ignore_configure;


void roadmap_canvas_get_text_extents 
        (const char *text, int *width, int *ascent, int *descent) {

   *ascent = 0;
   *descent = 0;

   wchar_t wstr[255];
   int length = mbstowcs(wstr, text, 254);

   if (length <=0) {
      *width = 0;
      return;
   }

   wstr[length] = 0;

   double x  = 0;
   double y  = 0;
   const wchar_t* p = wstr;
   
   while(*p) {
      const agg::glyph_cache* glyph = m_fman.glyph(*p);

      if(glyph) {
         x += glyph->advance_x;
         y += glyph->advance_y;
         //if (glyph->bounds.y2 < *ascent) *ascent=glyph->bounds.y2;
         if (-glyph->bounds.y1 > *descent) *descent=-glyph->bounds.y1 - 1;
      }
      ++p;
   }

   *width = (int)x;
}


void roadmap_canvas_select_pen (RoadMapPen pen) {

   if (!CurrentPen || (pen->thickness != CurrentPen->thickness)) {
      profile.width(pen->thickness);
   }
   CurrentPen = pen;

   reno.color(agg::rgba8(GetRValue(pen->color), GetGValue(pen->color),
            GetBValue(pen->color)));

}


RoadMapPen roadmap_canvas_create_pen (const char *name) {

   struct roadmap_canvas_pen *pen;

   for (pen = RoadMapPenList; pen != NULL; pen = pen->next) {
      if (strcmp(pen->name, name) == 0) break;
   }

   if (pen == NULL) {

      /* This is a new pen: create it. */

      pen = (struct roadmap_canvas_pen *)
                malloc (sizeof(struct roadmap_canvas_pen));
      roadmap_check_allocated(pen);

      pen->name = strdup (name);
      pen->next = RoadMapPenList;
		pen->thickness = 1;

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

	CurrentPen->color = *native_color;
	roadmap_canvas_select_pen(CurrentPen);
}


void roadmap_canvas_set_thickness  (int thickness) {

   if (CurrentPen && (CurrentPen->thickness != thickness)) {
		CurrentPen->thickness = thickness;
      profile.width(thickness);
	}
}


void roadmap_canvas_erase (void) {

   renb.clear(agg::rgba8(GetRValue(CurrentPen->color), GetGValue(CurrentPen->color), GetBValue(CurrentPen->color)));
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
      y = position->y - (text_height / 2);
      x = position->x - (text_width / 2);
      break;

   default:
      return;
   }

   RoadMapGuiPoint start = {x, y+text_height};
   roadmap_canvas_draw_string_angle (&start, position, 0, text);
}


void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points) {

   int i;

   for (i=0; i<count; i++) {

      if (points[i].x < 0) continue;
      else if (points[i].x >= roadmap_canvas_width()) continue;
      else if (points[i].y < 0) continue;
      else if (points[i].y >= roadmap_canvas_height()) continue;

      pixf.copy_pixel(points[i].x, points[i].y,
                      agg::rgba8(GetRValue(CurrentPen->color),
                        GetGValue(CurrentPen->color),
                        GetBValue(CurrentPen->color)));
   }
}

void roadmap_canvas_draw_multiple_lines (int count, int *lines,
      RoadMapGuiPoint *points) {
   int i;
   int count_of_points;

   raso.round_cap(true);
   raso.accurate_join(true);

   agg::path_storage path;

   for (i = 0; i < count; ++i) {

      int first = 1;

      count_of_points = *lines;

      if (count_of_points < 2) continue;

      for (int j=0; j<count_of_points; j++) {

         if (first) {
            first = 0;
            path.move_to(points->x, points->y);            
         } else {
            path.line_to(points->x, points->y);
         }

         points++;
      }

      if (roadmap_screen_is_dragging()) {
         renderer_pr ren_pr(renb);
         agg::rasterizer_outline<renderer_pr> ras_line(ren_pr);
         ren_pr.line_color(agg::rgba8(GetRValue(CurrentPen->color), GetGValue(CurrentPen->color), GetBValue(CurrentPen->color)));
         ras_line.add_path(path);

      } else {

         raso.add_path(path);
      }

      path.remove_all ();

      lines += 1;
   }
}

void roadmap_canvas_draw_multiple_polygons
         (int count, int *polygons, RoadMapGuiPoint *points, int filled) {

   int i;
   int count_of_points;
   
   agg::path_storage path;
   
   for (i = 0; i < count; ++i) {
      
      count_of_points = *polygons;
      
      int first = 1;
      
      for (int j=0; j<count_of_points; j++) {
         
         if (first) {
            first = 0;
            path.move_to(points->x, points->y);            
         } else {
            path.line_to(points->x, points->y);
         }
         points++;
      }
      
      path.close_polygon();
      
      if (filled) {
         
         ras.reset();
         ras.add_path(path);
         ren_solid.color(agg::rgba8(GetRValue(CurrentPen->color), GetGValue(CurrentPen->color), GetBValue(CurrentPen->color)));
         agg::render_scanlines( ras, sl, ren_solid);
         
      } else if (roadmap_screen_is_dragging()) {
         renderer_pr ren_pr(renb);
         agg::rasterizer_outline<renderer_pr> ras_line(ren_pr);
         ren_pr.line_color(agg::rgba8(GetRValue(CurrentPen->color), GetGValue(CurrentPen->color), GetBValue(CurrentPen->color)));
         ras_line.add_path(path);
         
      } else {
         
         raso.add_path(path);
      }
      
      path.remove_all ();
      
      polygons += 1;
   }
            
}


void roadmap_canvas_draw_multiple_circles
        (int count, RoadMapGuiPoint *centers, int *radius, int filled) {

   int i;
   
   agg::path_storage path;
   
   for (i = 0; i < count; ++i) {
      
      int r = radius[i];
      
      int x = centers[i].x;
      int y = centers[i].y;
      
      ras.reset ();

      
      agg::ellipse e( x, y, r, r);
      path.add_path(e);
      
      if (filled) {
         
         ras.reset();
         ras.add_path(path);
         ren_solid.color(agg::rgba8(GetRValue(CurrentPen->color), GetGValue(CurrentPen->color), GetBValue(CurrentPen->color)));
         agg::render_scanlines( ras, sl, ren_solid);
         
      } else if (roadmap_screen_is_dragging()) {
         renderer_pr ren_pr(renb);
         agg::rasterizer_outline<renderer_pr> ras_line(ren_pr);
         ren_pr.line_color(agg::rgba8(GetRValue(CurrentPen->color), GetGValue(CurrentPen->color), GetBValue(CurrentPen->color)));
         ras_line.add_path(path);
         
      } else {
         
         raso.add_path(path);
      }
      
      path.remove_all ();
   }
}


static gint roadmap_canvas_configure
               (GtkWidget *widget, GdkEventConfigure *event) {

   static GdkPixmap *tmp_buf;

   if (tmp_buf != NULL) {
      gdk_pixmap_unref (tmp_buf);
   }

   tmp_buf =
      gdk_pixmap_new (widget->window,
            widget->allocation.width,
            widget->allocation.height,
            -1);

   if (RoadMapDrawingBuffer != NULL) {
      gdk_image_unref (RoadMapDrawingBuffer);
   }

   RoadMapDrawingBuffer = gdk_drawable_get_image (tmp_buf, 0, 0,
                           widget->allocation.width, widget->allocation.height);

   if (RoadMapDrawingBuffer->visual->type != GDK_VISUAL_TRUE_COLOR) {
      exit(-1);
   }

   rbuf.attach((unsigned char*)RoadMapDrawingBuffer->mem, RoadMapDrawingBuffer->width, RoadMapDrawingBuffer->height, RoadMapDrawingBuffer->bpl);
   
   renb.attach(pixf);
   ras.clip_box(0, 0, renb.width(), renb.height());

   agg::glyph_rendering gren = gren = agg::glyph_ren_outline; 

   roadmap_config_declare
       ("preferences", &RoadMapConfigFont, "font.ttf");

   char font_file[255];
   
   snprintf(font_file, sizeof(font_file), "%s/%s", roadmap_path_user(),
      roadmap_config_get (&RoadMapConfigFont));

   if (!RoadMapCanvasFontLoaded) {

      if(m_feng.load_font(font_file, 0, gren)) {
         m_feng.hinting(true);
         m_feng.height(15);
         m_feng.width(15);
         m_feng.flip_y(true);

         RoadMapCanvasFontLoaded = 1;
      } else {
         RoadMapCanvasFontLoaded = -1;
         char message[300];
         snprintf(message, sizeof(message), "Can't load font: %s\n", font_file);
         roadmap_messagebox("Error", message);
      }
   }

   (*RoadMapCanvasConfigure) ();

   return TRUE;
}


void roadmap_canvas_register_configure_handler
                    (RoadMapCanvasConfigureHandler handler) {

   RoadMapCanvasConfigure = handler;
}


static gint roadmap_canvas_expose (GtkWidget *widget, GdkEventExpose *event) {

   gdk_draw_image (widget->window,
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

   point.x = (short)event->x;
   point.y = (short)event->y;

   switch ((int) data) {
      case 1: (*RoadMapCanvasMouseButtonPressed) (&point);  break;
      case 2: (*RoadMapCanvasMouseButtonReleased) (&point); break;
      case 3: (*RoadMapCanvasMouseMoved) (&point);          break;
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

   gtk_widget_set_double_buffered (RoadMapDrawingArea, FALSE);

   gtk_widget_set_events
      (RoadMapDrawingArea,
       GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_POINTER_MOTION_MASK);


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
                     (GCallback) roadmap_canvas_mouse_event,
                     (gpointer)1);

   g_signal_connect (RoadMapDrawingArea,
                     "button_release_event",
                     (GCallback) roadmap_canvas_mouse_event,
                     (gpointer)2);

   g_signal_connect (RoadMapDrawingArea,
                     "motion_notify_event",
                     (GCallback) roadmap_canvas_mouse_event,
                     (gpointer)3);

   //RoadMapGc = RoadMapDrawingArea->style->black_gc;

   return RoadMapDrawingArea;
}


void roadmap_canvas_save_screenshot (const char* filename) {

#if 0
   gint width,height;
   GdkColormap *colormap = gdk_colormap_get_system();
   GdkPixbuf *pixbuf;
   GError *error = NULL;


   gdk_drawable_get_size(RoadMapDrawingBuffer, &width, &height);

   pixbuf = gdk_pixbuf_get_from_drawable(NULL, // Create a new pixbuf.
                                         RoadMapDrawingBuffer,
                                         colormap,
                                         0,0,         // source
                                         0,0,         // destination
                                         width, height);  // size


   if (gdk_pixbuf_save(pixbuf, filename, "png", &error, NULL) == FALSE) {

      roadmap_log(ROADMAP_ERROR, "Failed to save image %s\n",filename);
   }

   gdk_pixbuf_unref(pixbuf);
#endif   
}

/*
** Use FRIBIDI to encode the string.
** The return value must be freed by the caller.
*/

#ifdef USE_FRIBIDI
static wchar_t* bidi_string(wchar_t *logical) {
   
   FriBidiCharType base = FRIBIDI_TYPE_ON;
   size_t len;
   
   len = wcslen(logical);
   
   FriBidiChar *visual;
   
   FriBidiStrIndex *ltov, *vtol;
   FriBidiLevel *levels;
   FriBidiStrIndex new_len;
   fribidi_boolean log2vis;
   
   visual = (FriBidiChar *) malloc (sizeof (FriBidiChar) * (len + 1));
   ltov = NULL;
   vtol = NULL;
   levels = NULL;
   
   /* Create a bidi string. */
   log2vis = fribidi_log2vis ((FriBidiChar *)logical, len, &base,
      /* output */
      visual, ltov, vtol, levels);
   
   if (!log2vis) {
      //msSetError(MS_IDENTERR, "Failed to create bidi string.", 
      //"msGetFriBidiEncodedString()");
      return NULL;
   }
   
   new_len = len;
   
   return (wchar_t *)visual;
   
}
#endif

void roadmap_canvas_draw_string_angle (RoadMapGuiPoint *position,
                                       RoadMapGuiPoint *center,
                                       int angle, const char *text)
{
   
   if (roadmap_screen_is_dragging()) return;
   if (RoadMapCanvasFontLoaded != 1) return;
   
   ren_solid.color(agg::rgba8(0, 0, 0));
   
   double x  = 0;
   double y  = 0;
   wchar_t wstr[255];
   
   //mbstate_t ps;
   //int length = mbrtowc(wstr, text, 10, mbstate_t *ps);
   int length = mbstowcs(wstr, text, 254);

   if (length <=0) return;
   wstr[length] = 0;

#ifdef USE_FRIBIDI
   wchar_t *bidi_text = bidi_string(wstr);
   const wchar_t* p = bidi_text;
#else   
   const wchar_t* p = wstr;
#endif
   
   while(*p) {
      const agg::glyph_cache* glyph = m_fman.glyph(*p);

      if(glyph) {
         m_fman.init_embedded_adaptors(glyph, x, y);
         
         //agg::conv_curve<font_manager_type::path_adaptor_type> stroke(m_fman.path_adaptor());
         
         agg::trans_affine mtx;
         if (abs(angle) > 5) {
            mtx *= agg::trans_affine_rotation(agg::deg2rad(angle));
         }
         mtx *= agg::trans_affine_translation(position->x, position->y);
         
         agg::conv_transform<font_manager_type::path_adaptor_type> tr(m_fman.path_adaptor(), mtx);

         agg::conv_curve<agg::conv_transform<font_manager_type::path_adaptor_type> > fill(tr);

         //agg::conv_stroke<
            //agg::conv_curve<agg::conv_transform<font_manager_type::path_adaptor_type> > > 
         //stroke(fill);

         agg::conv_stroke< agg::conv_transform<font_manager_type::path_adaptor_type> >
         stroke(tr);

         ras.add_path(tr);
         //ren_solid.color(agg::rgba8(0, 0, 0));
         agg::render_scanlines(ras, sl, ren_solid);
         //ras.add_path(stroke);
         //ren_solid.color(agg::rgba8(255, 255, 255));
         //agg::render_scanlines(ras, sl, ren_solid);

         // increment pen position
         x += glyph->advance_x;
         y += glyph->advance_y;
      }
      ++p;
   }

#ifdef USE_FRIBIDI
   free(bidi_text);
#endif
}
