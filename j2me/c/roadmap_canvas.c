/* roadmap_canvas.c - manage the canvas that is used to draw the map with j2me
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <javax/microedition/lcdui.h>
#include <javax/microedition/lcdui/game.h>
#include <jmicropolygon.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_screen.h"
#include "roadmap_canvas.h"
#include "roadmap_messagebox.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_path.h"

struct roadmap_canvas_pen {   
   struct roadmap_canvas_pen *next;
   char  *name;
   unsigned char color[3];
   int thickness;
};

static struct roadmap_canvas_pen *RoadMapPenList = NULL;
static RoadMapPen CurrentPen = 0;
static NOPH_GameCanvas_t canvas = 0;
static NOPH_Graphics_t graphics = 0;
static NOPH_Graphics_t graphicsBuffer = 0;

/* The canvas callbacks: all callbacks are initialized to do-nothing
 * functions, so that we don't care checking if one has been setup.
 */
static void roadmap_canvas_ignore_mouse (RoadMapGuiPoint *point) {}

RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonPressed =
                                     roadmap_canvas_ignore_mouse;

RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonReleased =
                                     roadmap_canvas_ignore_mouse;

RoadMapCanvasMouseHandler RoadMapCanvasMouseMoved =
                                     roadmap_canvas_ignore_mouse;


static void roadmap_canvas_ignore_configure (void) {}

RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
                                     roadmap_canvas_ignore_configure;


void roadmap_canvas_get_text_extents 
        (const char *text, int size, int *width,
            int *ascent, int *descent, int *can_tilt) {

   *width = 0;
   *ascent = 0;
   *descent = 0;
   *can_tilt = 0;
   return;

#if 0   
   *ascent = 0;
   *descent = 0;
   if (can_tilt) *can_tilt = 1;

   wchar_t wstr[255];
   int length = roadmap_canvas_agg_to_wchar (text, wstr, 255);

   if (length <=0) {
      *width = 0;
      return;
   }

   double x  = 0;
   double y  = 0;
   const wchar_t* p = wstr;

   font_manager_type *fman;

   if (size == -1) {
      /* Use the regular font */
      *descent = abs((int)m_feng.descender()) + 1;
      *ascent = (int)m_feng.ascender() + 1;
      fman = &m_fman;
   } else {

      m_image_feng.height(size);
      m_image_feng.width(size);
      *descent = abs((int)m_image_feng.descender()) + 1;
      *ascent = (int)m_image_feng.ascender() + 1;
      fman = &m_image_fman;
   }

   while(*p) {
      const agg::glyph_cache* glyph = fman->glyph(*p);

      if(glyph) {
         x += glyph->advance_x;
         y += glyph->advance_y;
         //if (-glyph->bounds.y1 > *descent) *descent=-glyph->bounds.y1 - 1;
      }
      ++p;
   }

   *width = (int)x;
   #endif
}


RoadMapPen roadmap_canvas_select_pen (RoadMapPen pen)
{
   RoadMapPen old_pen = CurrentPen;
   dbg_time_start(DBG_TIME_SELECT_PEN);

   CurrentPen = pen;

   if (graphicsBuffer)
         NOPH_Graphics_setColor
            (graphicsBuffer, pen->color[0], pen->color[1], pen->color[2]);

   dbg_time_end(DBG_TIME_SELECT_PEN);

   return old_pen;
}


RoadMapPen roadmap_canvas_create_pen (const char *name)
{
   struct roadmap_canvas_pen *pen;
   
   for (pen = RoadMapPenList; pen != NULL; pen = pen->next) {
      if (strcmp(pen->name, name) == 0) break;
   }
   
   if (pen == NULL) {
      
      pen = (struct roadmap_canvas_pen *)
         malloc (sizeof(struct roadmap_canvas_pen));
      roadmap_check_allocated(pen);
      
      pen->name = strdup (name);
      memset (&pen->color, 0, sizeof(pen->color));
      pen->thickness = 1;
      pen->next = RoadMapPenList;
      
      RoadMapPenList = pen;
   }
   
   roadmap_canvas_select_pen (pen);
   
   return pen;
}


void roadmap_canvas_set_foreground (const char *color) {

   int i;
   if (!CurrentPen) return;

   if (*color != '#') return;
   color++;

   for (i=0; i<3; i++) {
      int val = 0;
      int j;
      for (j=0; j<2; j++) {
         val = val * 16;
         if (isdigit(color[j])) val += color[j] - 48;
         else val += (tolower(color[j]) - 87);
      }

      CurrentPen->color[i] = val;
      color += 2;
   }

   roadmap_canvas_select_pen(CurrentPen);
}


int  roadmap_canvas_get_thickness  (RoadMapPen pen) {

   if (pen == NULL) return 0;

   return pen->thickness;
}


void roadmap_canvas_set_thickness (int thickness) {

   if (CurrentPen && (CurrentPen->thickness != thickness)) {
      CurrentPen->thickness = thickness;
   }
}


void roadmap_canvas_set_opacity (int opacity) {

   return;
#if 0
   if (!CurrentPen) return;
   CurrentPen->color.a = opacity;
   roadmap_canvas_select_pen(CurrentPen);
#endif   
}


void roadmap_canvas_erase (void) {

   roadmap_log (ROADMAP_DEBUG, "in roadmap_canvas_erase: canvas: 0x%x\n", canvas);
   NOPH_Graphics_fillRect (graphicsBuffer,
                           0, 0,
                           NOPH_GameCanvas_getWidth(canvas),
                           NOPH_GameCanvas_getHeight(canvas));
}


void roadmap_canvas_erase_area (const RoadMapGuiRect *rect) {

   NOPH_Graphics_fillRect (graphicsBuffer,
                           rect->minx, rect->miny, rect->maxx, rect->maxy);
}


void roadmap_canvas_draw_string (RoadMapGuiPoint *position,
                                 int corner,
                                 const char *text) {
   return;
#if 0   
   int x;
   int y;
   int text_width;
   int text_ascent;
   int text_descent;
   int text_height;
   
   roadmap_canvas_get_text_extents 
         (text, -1, &text_width, &text_ascent, &text_descent, NULL);
   
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
   roadmap_canvas_draw_string_angle (&start, position, 0, -1, text);
#endif   
}


void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points) {

   int i;

   for (i=0; i<count; i++) {

      NOPH_Graphics_drawLine
         (graphicsBuffer, points[i].x, points[i].y, points[i].x, points[i].y);
   }
}


void roadmap_canvas_draw_multiple_lines (int count, int *lines,
      RoadMapGuiPoint *points, int fast_draw) {

   int i;
   
   dbg_time_start(DBG_TIME_DRAW_LINES);

   for (i = 0; i < count; ++i) {
      
      int count_of_points = *lines;
      int j;
      
      if (count_of_points < 2) continue;
      
      roadmap_log (ROADMAP_DEBUG, "Drawing a line:\n");
      for (j=0; j<count_of_points-1; j++) {

         RoadMapGuiPoint *to_point = points+1;
         roadmap_log (ROADMAP_DEBUG, "From: %d,%d To %d,%d\n",
                      points->x, points->y, to_point->x, to_point->y);
                               
         NOPH_Graphics_drawLine
            (graphicsBuffer, points->x, points->y, to_point->x, to_point->y);
         points++;
      }
      points++;

      lines += 1;
   }
   
   dbg_time_end(DBG_TIME_DRAW_LINES);
}

void roadmap_canvas_draw_multiple_polygons
         (int count, int *polygons, RoadMapGuiPoint *points, int filled,
          int fast_draw) {

   int i;
   int count_of_points;
   int *x_points = NULL;
   int *y_points = NULL;
   int array_size = 0;
   
   for (i = 0; i < count; ++i) {
      int j;
      
      count_of_points = *polygons;
      if (array_size < count_of_points) {
         array_size = count_of_points;

         if (x_points) {
            free(x_points);
            free(y_points);
         }

         x_points = malloc(count_of_points * sizeof(int));
         y_points = malloc(count_of_points * sizeof(int));
      }
     
      for (j=0; j<count_of_points; j++) {
         
         x_points[j] = points->x;
         y_points[j] = points->y;
         points++;
      }
      
      if (filled) {
         NOPH_PolygonGraphics_fillPolygon
                (graphicsBuffer, x_points, y_points, count_of_points);
      } else {
         NOPH_PolygonGraphics_drawPolygon
                (graphicsBuffer, x_points, y_points, count_of_points);
      }
      
      polygons += 1;
   }

   if (x_points) {
      free(x_points);
      free(y_points);
   }
}


void roadmap_canvas_draw_multiple_circles
        (int count, RoadMapGuiPoint *centers, int *radius, int filled,
         int fast_draw) {

   int i;
   
   for (i = 0; i < count; ++i) {
      
      int r = radius[i];
      
      int x = centers[i].x - r;
      int y = centers[i].y - r;

      r *= 2;
      
      roadmap_log (ROADMAP_DEBUG, "circle: x:%d, y:%d, r:%d, filled:%d\n",
                                  x, y, r, filled);
      if (filled) {
         
         NOPH_Graphics_fillArc (graphicsBuffer, x, y, r, r, 0, 360);
         
      } else {
         
         NOPH_Graphics_drawArc (graphicsBuffer, x, y, r, r, 0, 360);
      }
   }
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


void roadmap_canvas_register_configure_handler
                    (RoadMapCanvasConfigureHandler handler) {

   RoadMapCanvasConfigure = handler;
}


int roadmap_canvas_width (void) {
   return NOPH_GameCanvas_getWidth (canvas);
}


int roadmap_canvas_height (void) {
   return NOPH_GameCanvas_getHeight (canvas);
}


void roadmap_canvas_save_screenshot (const char* filename) {
   /* NOT IMPLEMENTED. */
}


void roadmap_canvas_draw_string_angle (const RoadMapGuiPoint *position,
                                       RoadMapGuiPoint *center,
                                       int angle, int size,
                                       const char *text)
{
   
   return;
#if 0   
   if (RoadMapCanvasFontLoaded != 1) return;
   
   dbg_time_start(DBG_TIME_TEXT_FULL);      
   dbg_time_start(DBG_TIME_TEXT_CNV);
   
   wchar_t wstr[255];
   int length = roadmap_canvas_agg_to_wchar (text, wstr, 255);
   if (length <=0) return;
   
#ifdef USE_FRIBIDI
   wchar_t *bidi_text = bidi_string(wstr);
   const wchar_t* p = bidi_text;
#else   
   const wchar_t* p = wstr;
#endif
   
   ren_solid.color(CurrentPen->color);
   dbg_time_end(DBG_TIME_TEXT_CNV);
   
   double x  = 0;
   double y  = 0;
   
   if ((angle > -5) && (angle < 5)) {

      if (size < 0) size = 15;

      /* Use faster drawing for text with no angle */
      x  = position->x;
      y  = position->y;

//      ren_solid.color(agg::rgba8(0, 0, 0));

      m_image_feng.height(size);
      m_image_feng.width(size);

      while(*p) {
         const agg::glyph_cache* glyph = m_image_fman.glyph(*p);

         if(glyph) {
            m_image_fman.init_embedded_adaptors(glyph, x, y);

            agg::render_scanlines(m_image_fman.gray8_adaptor(), 
                  m_image_fman.gray8_scanline(), 
                  ren_solid);      

            // increment pen position
            x += glyph->advance_x;
            y += glyph->advance_y;
         }
         ++p;
      }
   }

   while(*p) {
      dbg_time_start(DBG_TIME_TEXT_ONE_LETTER);
      dbg_time_start(DBG_TIME_TEXT_GET_GLYPH);
      const agg::glyph_cache* glyph = m_fman.glyph(*p);
      dbg_time_end(DBG_TIME_TEXT_GET_GLYPH);

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

         //agg::conv_contour<agg::conv_transform<font_manager_type::path_adaptor_type> >contour(tr);
         //contour.width(2);

         //agg::conv_stroke< agg::conv_contour<agg::conv_transform<font_manager_type::path_adaptor_type> > > stroke(contour);
         //agg::conv_stroke< agg::conv_transform<font_manager_type::path_adaptor_type> > stroke(tr);

         dbg_time_start(DBG_TIME_TEXT_ONE_RAS);
         
#ifdef WIN32_PROFILE
         ResumeCAPAll();
#endif
         ras.reset();
         ras.add_path(tr);
         agg::render_scanlines(ras, sl, ren_solid);
         //ras.add_path(fill);
         //ras.add_path(stroke);
         //ren_solid.color(agg::rgba8(255, 255, 255));
         //agg::render_scanlines(ras, sl, ren_solid);
         //ras.add_path(tr);
         //ren_solid.color(agg::rgba8(0, 0, 0));
         //agg::render_scanlines(ras, sl, ren_solid);

#ifdef WIN32_PROFILE
         SuspendCAPAll();
#endif
         
         dbg_time_end(DBG_TIME_TEXT_ONE_RAS);
         
         // increment pen position
         x += glyph->advance_x;
         y += glyph->advance_y;
         dbg_time_end(DBG_TIME_TEXT_ONE_LETTER);
      }
      ++p;
   }

#ifdef USE_FRIBIDI
   free(bidi_text);
#endif

   dbg_time_end(DBG_TIME_TEXT_FULL);
#endif
}


RoadMapImage roadmap_canvas_load_image (const char *path,
                                        const char* file_name) {
   return NULL;
}


void roadmap_canvas_free_image (RoadMapImage image) {

   return;
}


void roadmap_canvas_draw_image (RoadMapImage image, const RoadMapGuiPoint *pos,
                                int opacity, int mode) {

   return;

#if 0   

   if ((mode == IMAGE_SELECTED) || (opacity <= 0) || (opacity >= 255)) {
      opacity = 255;
   }

   agg_renb.blend_from(image->pixfmt, 0, pos->x, pos->y, opacity);

   if (mode == IMAGE_SELECTED) {
      static RoadMapPen selection;

      if (!selection) {
         selection = roadmap_canvas_create_pen("selection");
         roadmap_canvas_set_foreground ("#000000");
      }

      RoadMapGuiPoint points[5] = {
         {pos->x, pos->y},
         {pos->x + image->rbuf.width(), pos->y},
         {pos->x + image->rbuf.width(), pos->y + image->rbuf.height()},
         {pos->x, pos->y + image->rbuf.height()},
         {pos->x, pos->y}};

      int num_points = 5;

      RoadMapPen current = roadmap_canvas_select_pen (selection);
      roadmap_canvas_draw_multiple_lines (1, &num_points, points, 0);
      roadmap_canvas_select_pen (current);
   }
#endif
}


void roadmap_canvas_copy_image (RoadMapImage dst_image,
                                const RoadMapGuiPoint *pos,
                                const RoadMapGuiRect  *rect,
                                RoadMapImage src_image, int mode) {

   return;
#if 0
   agg::renderer_base<agg::pixfmt_rgba32> renb(dst_image->pixfmt);

   agg::rect_i agg_rect;
   agg::rect_i *agg_rect_p = NULL;

   if (rect) {
      agg_rect.x1 = rect->minx;
      agg_rect.y1 = rect->miny;
      agg_rect.x2 = rect->maxx;
      agg_rect.y2 = rect->maxy;

      agg_rect_p = &agg_rect;
   }

   if (mode == CANVAS_COPY_NORMAL) {
      renb.copy_from(src_image->rbuf, agg_rect_p, pos->x, pos->y);
   } else {
      renb.blend_from(src_image->pixfmt, agg_rect_p, pos->x, pos->y, 255);
   }
#endif   
}


int  roadmap_canvas_image_width  (const RoadMapImage image) {

   return 0;
}


int  roadmap_canvas_image_height (const RoadMapImage image) {
   
   return 0;
}


void roadmap_canvas_draw_image_text (RoadMapImage image,
                                     const RoadMapGuiPoint *position,
                                     int size, const char *text) {
   
   return;
#if 0
   if (RoadMapCanvasFontLoaded != 1) return;
   
   wchar_t wstr[255];
   int length = roadmap_canvas_agg_to_wchar (text, wstr, 255);
   if (length <=0) return;
   
#ifdef USE_FRIBIDI
   wchar_t *bidi_text = bidi_string(wstr);
   const wchar_t* p = bidi_text;
#else   
   const wchar_t* p = wstr;
#endif
   
   double x  = position->x;
   double y  = position->y + size - 7;

   agg::renderer_base<agg::pixfmt_rgba32> renb(image->pixfmt);
   agg::renderer_scanline_aa_solid< agg::renderer_base<agg::pixfmt_rgba32> > ren_solid (renb);

   ren_solid.color(agg::rgba8(0, 0, 0));

   m_image_feng.height(size);
   m_image_feng.width(size);

   while(*p) {
      const agg::glyph_cache* glyph = m_image_fman.glyph(*p);

      if(glyph) {
         m_image_fman.init_embedded_adaptors(glyph, x, y);
         
         agg::render_scanlines(m_image_fman.gray8_adaptor(), 
               m_image_fman.gray8_scanline(), 
               ren_solid);      

         // increment pen position
         x += glyph->advance_x;
         y += glyph->advance_y;
      }
      ++p;
   }

#ifdef USE_FRIBIDI
   free(bidi_text);
#endif
#endif
}

#if 0
void roadmap_canvas_button_pressed(POINT *data) {
   RoadMapGuiPoint point;
   
   point.x = (short)data->x;
   point.y = (short)data->y;
   
   (*RoadMapCanvasMouseButtonPressed) (&point);
   
}


void roadmap_canvas_button_released(POINT *data) {
   RoadMapGuiPoint point;
   
   point.x = (short)data->x;
   point.y = (short)data->y;
   
   (*RoadMapCanvasMouseButtonReleased) (&point);
   
}


void roadmap_canvas_mouse_moved(POINT *data) {
   RoadMapGuiPoint point;
   
   point.x = (short)data->x;
   point.y = (short)data->y;
   
   (*RoadMapCanvasMouseMoved) (&point);
   
}

#endif

void roadmap_canvas_refresh (void) {
   
   dbg_time_start(DBG_TIME_FLIP);
   
   if (graphics != graphicsBuffer) {
   }

   NOPH_GameCanvas_flushGraphics(canvas);

   dbg_time_end(DBG_TIME_FLIP);
}


void roadmap_canvas_configure (void) {

   roadmap_log (ROADMAP_DEBUG, "***** In roadmap_canvas_configure *****\n");
   canvas = NOPH_GameCanvas_get();
   graphics = NOPH_GameCanvas_getGraphics(canvas);
   graphicsBuffer = graphics;

   (*RoadMapCanvasConfigure) ();
}


