/* roadmap_canvas.cpp - manage the canvas that is used to draw the map with agg
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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

#include <windows.h>

#define USE_FRIBIDI

#ifdef WIN32_PROFILE
#include <C:\Program Files\Windows CE Tools\Common\Platman\sdk\wce500\include\cecap.h>
#endif

#include "agg_rendering_buffer.h"
#include "agg_curves.h"
#include "agg_conv_stroke.h"
#include "agg_conv_contour.h"
#include "agg_conv_transform.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_rasterizer_outline_aa.h"
#include "agg_rasterizer_outline.h"
#include "agg_renderer_primitives.h"
#include "agg_ellipse.h"
#include "agg_renderer_scanline.h"
#include "agg_scanline_p.h"
#include "agg_renderer_outline_aa.h"
#include "agg_pixfmt_rgb_packed.h"
#include "agg_path_storage.h"

#include "agg_font_freetype.h"

#ifdef USE_FRIBIDI
#include <fribidi/fribidi.h>
#define MAX_STR_LEN 65000
#endif

extern "C" {
#include "../roadmap.h"
#include "../roadmap_types.h"
#include "../roadmap_gui.h"
#include "../roadmap_screen.h"
#include "../roadmap_canvas.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_math.h"
#include "../roadmap_config.h"
#include "../roadmap_path.h"
#include "roadmap_wincecanvas.h"
#include "colors.h"

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

//static agg::conv_curve<font_manager_type::path_adaptor_type> font_curves(m_fman.path_adaptor());
//static agg::conv_stroke<agg::conv_curve<font_manager_type::path_adaptor_type> > font_stroke(font_curves);

struct roadmap_canvas_pen {	
	struct roadmap_canvas_pen *next;
	char  *name;
	COLORREF color;
	int thickness;
};


static struct roadmap_canvas_pen *RoadMapPenList = NULL;

static HWND			RoadMapDrawingArea;
static HDC			RoadMapDrawingBuffer;
static RoadMapPen	CurrentPen;
static RECT			ClientRect;
static HBITMAP		OldBitmap = NULL;
static int        RoadMapCanvasFontLoaded = 0;

static void roadmap_canvas_ignore_button (RoadMapGuiPoint *point) {}
static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonPressed =
	roadmap_canvas_ignore_button;
static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonReleased =
	roadmap_canvas_ignore_button;
static RoadMapCanvasMouseHandler RoadMapCanvasMouseMove =
	roadmap_canvas_ignore_button;

static void roadmap_canvas_ignore_configure (void) {}

static RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
	roadmap_canvas_ignore_configure;


void roadmap_canvas_get_text_extents (const char *text, int *width,
			int *ascent, int *descent)
{
	*ascent = 0;
	*descent = 0;

	LPWSTR text_unicode = ConvertToWideChar(text, CP_UTF8);

	double x  = 0;
	double y  = 0;
	const wchar_t* p = text_unicode;

	while(*p) {
		const agg::glyph_cache* glyph = m_fman.glyph(*p);

		if(glyph) {
			x += glyph->advance_x;
			y += glyph->advance_y;
			if (-glyph->bounds.y1 > *descent) *descent=-glyph->bounds.y1 - 1;
		}
		++p;
	}

	*width = (int)x;

   	free(text_unicode);
}


void roadmap_canvas_select_pen (RoadMapPen pen)
{
   dbg_time_start(DBG_TIME_SELECT_PEN);
   if (!CurrentPen || (pen->thickness != CurrentPen->thickness)) {
      profile.width(pen->thickness);
   }
	CurrentPen = pen;

   reno.color(agg::rgba8(GetRValue(pen->color), GetGValue(pen->color), GetBValue(pen->color)));

   dbg_time_end(DBG_TIME_SELECT_PEN);

   return;	
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
		pen->color = RGB(0, 0, 0);
		pen->thickness = 1;
		pen->next = RoadMapPenList;
		
		RoadMapPenList = pen;
	}
	
	roadmap_canvas_select_pen (pen);
	
	return pen;
}


void roadmap_canvas_set_foreground (const char *color)
{
	int high, i, low;
	COLORREF c;
	
	if (*color == '#') {
		int r, g, b;
		sscanf(color, "#%2x%2x%2x", &r, &g, &b);
		c = RGB(r, g, b);
	} else {
		/* Do binary search on color table */
		for (low=(-1), high=sizeof(color_table)/sizeof(color_table[0]);
		high-low > 1;) {
			i = (high+low) / 2;
			if (strcmp(color, color_table[i].name) <= 0) high = i;
			else low = i;
		}
		
		if (!strcmp(color, color_table[high].name)) {
			c = RGB(color_table[high].r, color_table[high].g, color_table[high].b);
		} else {
			c = RGB(0, 0, 0);
		}
	}
	
	CurrentPen->color = c;
	roadmap_canvas_select_pen(CurrentPen);
}


void roadmap_canvas_set_thickness(int thickness)
{
   if (CurrentPen && (CurrentPen->thickness != thickness)) {
		CurrentPen->thickness = thickness;
      profile.width(thickness);
	}
	roadmap_canvas_select_pen(CurrentPen);
}


void roadmap_canvas_erase (void)
{
   renb.clear(agg::rgba8(GetRValue(CurrentPen->color),
                         GetGValue(CurrentPen->color),
                         GetBValue(CurrentPen->color)));

}


void roadmap_canvas_draw_string (RoadMapGuiPoint *position,
                                 int corner,
                                 const char *text)
{
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


void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points)
{
   int i;

   for (i=0; i<count; i++) {

      if (points[i].x < ClientRect.left) continue;
      else if (points[i].x > ClientRect.right) continue;
      else if (points[i].y < ClientRect.bottom) continue;
      else if (points[i].y > ClientRect.top) continue;

      pixf.copy_pixel(points[i].x, points[i].y,
                      agg::rgba8(GetRValue(CurrentPen->color),
                        GetGValue(CurrentPen->color),
                        GetBValue(CurrentPen->color)));
   }
}


void roadmap_canvas_draw_multiple_lines (int count, int *lines,
                                         RoadMapGuiPoint *points)
{
   int i;
   int count_of_points;
   
   dbg_time_start(DBG_TIME_DRAW_LINES);
#ifdef WIN32_PROFILE
   ResumeCAPAll();
#endif
   
   raso.round_cap(true);
   raso.accurate_join(true);

   agg::path_storage path;
   
   for (i = 0; i < count; ++i) {
      
      int first = 1;
      
      count_of_points = *lines;
      
      if (count_of_points < 2) continue;
      
      dbg_time_start(DBG_TIME_CREATE_PATH);

      for (int j=0; j<count_of_points; j++) {

         if (first) {
            first = 0;
            path.move_to(points->x, points->y);            
         } else {
            path.line_to(points->x, points->y);
         }

         points++;
      }

      dbg_time_end(DBG_TIME_CREATE_PATH);
      dbg_time_start(DBG_TIME_ADD_PATH);
      
      if (roadmap_screen_is_dragging()) {
      //if (true) {
         renderer_pr ren_pr(renb);
         agg::rasterizer_outline<renderer_pr> ras_line(ren_pr);
         ren_pr.line_color(agg::rgba8(GetRValue(CurrentPen->color), GetGValue(CurrentPen->color), GetBValue(CurrentPen->color)));
         ras_line.add_path(path);
         
      } else {
         
         raso.add_path(path);
      }
      
      path.remove_all ();
      dbg_time_end(DBG_TIME_ADD_PATH);
      
      lines += 1;
   }
   
     
#ifdef WIN32_PROFILE
   SuspendCAPAll();
#endif
   
   dbg_time_end(DBG_TIME_DRAW_LINES);
}


void roadmap_canvas_draw_multiple_polygons (int count, int *polygons,
                                            RoadMapGuiPoint *points, int filled)
{
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


void roadmap_canvas_draw_multiple_circles (int count,
                                           RoadMapGuiPoint *centers, int *radius, int filled)
{
   
   int i;
   
   agg::path_storage path;
   
   for (i = 0; i < count; ++i) {
      
      int r = radius[i];
      
      int x = centers[i].x;
      int y = centers[i].y;
      
      ras.reset ();
      
      path.add_path (agg::ellipse( x, y, r, r));
      
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


void roadmap_canvas_register_configure_handler (
                                                RoadMapCanvasConfigureHandler handler)
{
   RoadMapCanvasConfigure = handler;
}


void roadmap_canvas_button_pressed(POINT *data)
{
   RoadMapGuiPoint point;
   
   point.x = (short)data->x;
   point.y = (short)data->y;
   
   (*RoadMapCanvasMouseButtonPressed) (&point);
   
}


void roadmap_canvas_button_released(POINT *data)
{
   RoadMapGuiPoint point;
   
   point.x = (short)data->x;
   point.y = (short)data->y;
   
   (*RoadMapCanvasMouseButtonReleased) (&point);
   
}


void roadmap_canvas_mouse_move(POINT *data)
{
   RoadMapGuiPoint point;
   
   point.x = (short)data->x;
   point.y = (short)data->y;
   
   (*RoadMapCanvasMouseMove) (&point);
   
}


void roadmap_canvas_register_button_pressed_handler (
                                                     RoadMapCanvasMouseHandler handler)
{
   RoadMapCanvasMouseButtonPressed = handler;
}


void roadmap_canvas_register_button_released_handler (
                                                      RoadMapCanvasMouseHandler handler)
{
   RoadMapCanvasMouseButtonReleased = handler;
}


void roadmap_canvas_register_mouse_move_handler (
                                                 RoadMapCanvasMouseHandler handler)
{
   RoadMapCanvasMouseMove = handler;
}


int roadmap_canvas_width (void)
{
   if (RoadMapDrawingArea == NULL) {
      return 0;
   }
   return ClientRect.right - ClientRect.left + 1;
}


int roadmap_canvas_height (void)
{
   if (RoadMapDrawingArea == NULL) {
      return 0;
   }
   
   return ClientRect.bottom - ClientRect.top + 1;
}


void roadmap_canvas_refresh (void)
{
   HDC hdc;
   
   if (RoadMapDrawingArea == NULL) return;
   
   dbg_time_start(DBG_TIME_FLIP);
   
   hdc = GetDC(RoadMapDrawingArea);
   BitBlt(hdc, ClientRect.left, ClientRect.top,
      ClientRect.right - ClientRect.left + 1,
      ClientRect.bottom - ClientRect.top + 1,
      RoadMapDrawingBuffer, 0, 0, SRCCOPY); 
   
   DeleteDC(hdc);
   dbg_time_end(DBG_TIME_FLIP);
}


HWND roadmap_canvas_new (HWND hWnd, HWND tool_bar)
{
   HDC hdc;
   BITMAPINFO bmp_info; 
   HBITMAP bmp;
   void* buf = 0;
   
   if ((RoadMapDrawingBuffer != NULL) && IsWindowVisible(tool_bar)) {
      
      DeleteObject(SelectObject(RoadMapDrawingBuffer, OldBitmap));
      DeleteDC(RoadMapDrawingBuffer);
   }
   
   hdc = GetDC(RoadMapDrawingArea);
   
   RoadMapDrawingArea = hWnd;
   GetClientRect(hWnd, &ClientRect);
   if (tool_bar != NULL) {
      RECT tb_rect;
      GetClientRect(tool_bar, &tb_rect);
      ClientRect.top += tb_rect.bottom;
   }
   
   
   RoadMapDrawingBuffer = CreateCompatibleDC(hdc);
   
   bmp_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
   bmp_info.bmiHeader.biWidth = ClientRect.right - ClientRect.left + 1;
   bmp_info.bmiHeader.biHeight = ClientRect.bottom - ClientRect.top + 1; 
   bmp_info.bmiHeader.biPlanes = 1; 
   bmp_info.bmiHeader.biBitCount = 16; 
   bmp_info.bmiHeader.biCompression = BI_BITFIELDS; 
   bmp_info.bmiHeader.biSizeImage = 0; 
   bmp_info.bmiHeader.biXPelsPerMeter = 0; 
   bmp_info.bmiHeader.biYPelsPerMeter = 0; 
   bmp_info.bmiHeader.biClrUsed = 0; 
   bmp_info.bmiHeader.biClrImportant = 0; 
   ((DWORD*)bmp_info.bmiColors)[0] = 0xF800;
   ((DWORD*)bmp_info.bmiColors)[1] = 0x07E0;
   ((DWORD*)bmp_info.bmiColors)[2] = 0x001F; 
   
   bmp = CreateDIBSection( 
      RoadMapDrawingBuffer, 
      &bmp_info, 
      DIB_RGB_COLORS, 
      &buf, 
      0, 
      0 
      ); 
   
   int stride = (((ClientRect.right - ClientRect.left + 1) * 2 + 3) >> 2) << 2;
   rbuf.attach((unsigned char*)buf, ClientRect.right - ClientRect.left + 1, ClientRect.bottom - ClientRect.top + 1, -stride);
   
   renb.attach(pixf);
   ras.clip_box(0, 0, renb.width(), renb.height());

   agg::glyph_rendering gren = gren = agg::glyph_ren_outline; 

   roadmap_config_declare
       ("preferences", &RoadMapConfigFont, "font.ttf");

   char font_file[255];
   
   snprintf(font_file, sizeof(font_file), "%s\\%s", roadmap_path_user(),
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

   
   OldBitmap = (HBITMAP)SelectObject(RoadMapDrawingBuffer, bmp);
   
   DeleteDC(hdc);
   (*RoadMapCanvasConfigure) ();
   
   return RoadMapDrawingArea;
}


void roadmap_canvas_save_screenshot (const char* filename) {
   /* NOT IMPLEMENTED. */
}

}


/*
** Use FRIBIDI to encode the string.
** The return value must be freed by the caller.
*/

#ifdef USE_FRIBIDI
static wchar_t* bidi_string(FriBidiChar *logical) {
   
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
   log2vis = fribidi_log2vis (logical, len, &base,
      /* output */
      visual, ltov, vtol, levels);
   
   if (!log2vis) {
      //msSetError(MS_IDENTERR, "Failed to create bidi string.", 
      //"msGetFriBidiEncodedString()");
      return NULL;
   }
   
   new_len = len;
   
   return visual;
   
}
#endif


void roadmap_canvas_draw_string_angle (RoadMapGuiPoint *position,
                                       RoadMapGuiPoint *center,
                                       int angle, const char *text)
{
   
   if (roadmap_screen_is_dragging()) return;
   if (RoadMapCanvasFontLoaded != 1) return;
   
   dbg_time_start(DBG_TIME_TEXT_FULL);      
   dbg_time_start(DBG_TIME_TEXT_CNV);
   
   LPWSTR text_unicode = ConvertToWideChar(text, CP_UTF8);
   
   ren_solid.color(agg::rgba8(0, 0, 0));
#ifdef USE_FRIBIDI
   LPWSTR logical = text_unicode;
   text_unicode = bidi_string(logical);
   free(logical);
#endif
   
   dbg_time_end(DBG_TIME_TEXT_CNV);
   
   dbg_time_start(DBG_TIME_TEXT_LOAD);
   
   double x  = 0;
   double y  = 0;
   const wchar_t* p = text_unicode;
   
   dbg_time_end(DBG_TIME_TEXT_LOAD);
   while(*p) {
      dbg_time_start(DBG_TIME_TEXT_ONE_LETTER);
      dbg_time_start(DBG_TIME_TEXT_GET_GLYPH);
      const agg::glyph_cache* glyph = m_fman.glyph(*p);
      dbg_time_end(DBG_TIME_TEXT_GET_GLYPH);

      if(glyph) {
         m_fman.init_embedded_adaptors(glyph, x, y);
         
         //agg::conv_curve<font_manager_type::path_adaptor_type> stroke(m_fman.path_adaptor());
         
         agg::trans_affine mtx;
         mtx *= agg::trans_affine_rotation(agg::deg2rad(angle));
         mtx *= agg::trans_affine_translation(position->x, position->y);
         
         agg::conv_transform<font_manager_type::path_adaptor_type> tr(m_fman.path_adaptor(), mtx);

         agg::conv_curve<agg::conv_transform<font_manager_type::path_adaptor_type> > fill(tr);

         //agg::conv_stroke<
            //agg::conv_curve<agg::conv_transform<font_manager_type::path_adaptor_type> > > 
         //stroke(fill);

         agg::conv_stroke< agg::conv_transform<font_manager_type::path_adaptor_type> >
            stroke(tr);

         dbg_time_start(DBG_TIME_TEXT_ONE_RAS);
         
#ifdef WIN32_PROFILE
         ResumeCAPAll();
#endif
         ras.add_path(tr);
         agg::render_scanlines(ras, sl, ren_solid);
         //ras.add_path(fill);
         //ren_solid.color(agg::rgba8(0, 0, 0));
         //agg::render_scanlines(ras, sl, ren_solid);
         //ras.add_path(stroke);
         //ren_solid.color(agg::rgba8(255, 255, 255));
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
   free(text_unicode);

   dbg_time_end(DBG_TIME_TEXT_FULL);
}

