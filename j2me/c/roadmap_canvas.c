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
#include <math.h>

#include <java/lang.h>
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

struct roadmap_canvas_image {
   NOPH_Image_t image;
};

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

static void bidi_string(char *bidi_text, const char *text) {
   char rtl[500];
   int rtl_len = 0;
   int bidi_len = 0;

   while (*text) {
      switch (*text) {
      case -41:
         if (bidi_len) {
            memcpy (rtl + sizeof(rtl) - rtl_len - bidi_len, bidi_text, bidi_len);
            rtl_len += bidi_len;
            bidi_len = 0;
         }
         rtl[sizeof(rtl) - ++rtl_len] = *(text+1);
         rtl[sizeof(rtl) - ++rtl_len] = *text;
         text += 2;
         break;

      case ' ':
      case '-':
      case ',':
      case ':':
      case '(':
      case ')':
      case '_':
      case '/':
      case '\\':
      case '"':
      case '?':
      case '.':
         if (rtl_len) {
            rtl[sizeof(rtl) - ++rtl_len] = *text++;
         } else {
            bidi_text[bidi_len++] = *text++;
         }
         break;

      default:
/*         if (rtl_len) {
            memcpy (bidi_text + bidi_len, rtl + sizeof(rtl) - rtl_len, rtl_len);
            bidi_len += rtl_len;
            rtl_len = 0;
         }*/
         bidi_text[bidi_len++] = *text++;
      }
   }

   /*
   if (rtl_len) {
      memcpy (bidi_text + bidi_len, rtl + sizeof(rtl) - rtl_len, rtl_len);
      bidi_len += rtl_len;
   }
   */

   if (rtl_len) memcpy(bidi_text+bidi_len, rtl + sizeof(rtl) - rtl_len, rtl_len);
   bidi_text[bidi_len + rtl_len] = '\0';
}


void roadmap_canvas_get_text_extents 
        (const char *text, int size, int *width,
            int *ascent, int *descent, int *can_tilt) {

   static NOPH_Font_t font;
   static int _ascent;
   static int _descent;

   if (!font) {
      font = NOPH_Font_getFont(NOPH_Font_FACE_SYSTEM, NOPH_Font_STYLE_BOLD, NOPH_Font_SIZE_MEDIUM);
      NOPH_Graphics_setFont(graphicsBuffer, font);
      _ascent = NOPH_Font_getBaselinePosition(font);
      _descent = NOPH_Font_getHeight(font) - _ascent;
   }

   *ascent = _ascent;
   *descent = _descent;
   *can_tilt = 0;

   *width = NOPH_Font_stringWidth(font, text);
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
   int thickness = CurrentPen->thickness;
   if (!(thickness % 2)) thickness++;
   
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
                               
         if (thickness == 1) {
            NOPH_Graphics_drawLine
               (graphicsBuffer, points->x, points->y, to_point->x, to_point->y);
         } else {
            double angle;
            double halfWidth = ((double)thickness)/2.0 + 1;
            double deltaX = (double)(to_point->x - points->x);
            double deltaY = (double)(to_point->y - points->y);
            if (points->x == to_point->x)
               angle=M_PI;
            else
               angle=atan(deltaY/deltaX)+M_PI/2;
            int xOffset = (int)(halfWidth*cos(angle));
            int yOffset = (int)(halfWidth*sin(angle));
            int xCorners[] = { points->x-xOffset, to_point->x-xOffset+1,
               to_point->x+xOffset+1, points->x+xOffset };
            int yCorners[] = { points->y-yOffset, to_point->y-yOffset,
               to_point->y+yOffset+1, points->y+yOffset+1 };
            NOPH_PolygonGraphics_fillPolygon
                (graphicsBuffer, xCorners, yCorners, 4);
            //g.fillPolygon(xCorners, yCorners, 4);
         }              
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
   
   char bidi_text[500];

   bidi_string(bidi_text, text);
   NOPH_Graphics_drawString (graphicsBuffer, bidi_text,
                             position->x, position->y,
                             NOPH_Graphics_BASELINE|NOPH_Graphics_LEFT);
}


RoadMapImage roadmap_canvas_load_image (const char *path,
                                        const char *file_name) {

   char *full_name = roadmap_path_join (path, file_name);
   RoadMapImage image = NULL;
   printf ("Loading image: %s\n", full_name);
   NOPH_Image_t i = NOPH_Image_createImage_string(full_name);

   if (i) {
      image = (RoadMapImage)malloc(sizeof(*image));
      image->image = i;
   }

   free (full_name);

   return image;
}


void roadmap_canvas_free_image (RoadMapImage image) {

   NOPH_delete (image->image);
   free(image);
}


void roadmap_canvas_draw_image (RoadMapImage image, const RoadMapGuiPoint *pos,
                                int opacity, int mode) {

   NOPH_Graphics_drawImage (graphicsBuffer, image->image, pos->x, pos->y,
                            NOPH_Graphics_TOP|NOPH_Graphics_LEFT);
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
   if (!image) return 0;

   return NOPH_Image_getWidth (image->image);
}


int  roadmap_canvas_image_height (const RoadMapImage image) {
   if (!image) return 0;
   
   return NOPH_Image_getHeight (image->image);
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


