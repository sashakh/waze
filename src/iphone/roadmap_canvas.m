/* roadmap_canvas.c - manage the canvas that is used to draw the map.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
 *   Copyright 2008 Avi Romano
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
#import <Foundation/Foundation.h>
#import <GraphicsServices/GraphicsServices.h>
#import <UIKit/UITouch.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"

#include "roadmap_path.h"
#include "roadmap_canvas.h"
#include "roadmap_iphonecanvas.h"
#include "colors.h"

#define FONT_INC_FACTOR 1.3 //Trying to compensate the high PPI of iPhone screen
#define FONT_DEFAULT_SIZE 12
extern int CGFontGetAscent(CGFontRef font);
extern int CGFontGetDescent(CGFontRef font);
extern int CGFontGetUnitsPerEm(CGFontRef font);
extern CGFontRef CGContextGetFont(CGContextRef context);
extern void CGFontGetGlyphsForUnichars (CGFontRef, const UniChar[], const CGGlyph[], size_t);
extern CGFontRef CGFontCreateWithFontName (CFStringRef name);


struct RoadMapColor {
   float r;
   float g;
   float b;
   float a;
};

struct roadmap_canvas_pen {

   struct roadmap_canvas_pen *next;

   char  *name;
   int style;
   float lineWidth;
   struct RoadMapColor stroke;
};

#define ROADMAP_CANVAS_LAYER_COUNT 2
#define BEZIER_SMOOTH_VALUE 0.5

static struct roadmap_canvas_pen *RoadMapPenList = NULL;

RoadMapCanvasView  *RoadMapDrawingArea;
static CGContextRef RoadMapGc;
static int CanvasWidth, CanvasHeight;

static RoadMapPen CurrentPen;

static CGPoint ChordingPoints[MAX_CHORDING_POINTS]; //Number of chording fingers. We actually use max of 2
static CGPoint touch1point, touch2point;
static UITouch *touch1, *touch2;

/* The canvas callbacks: all callbacks are initialized to do-nothing
 * functions, so that we don't care checking if one has been setup.
 */
static void roadmap_canvas_ignore_mouse (int button, RoadMapGuiPoint *point) {}

static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonPressed =
                                     roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler RoadMapCanvasMouseButtonReleased =
                                     roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler RoadMapCanvasMouseMoved =
                                     roadmap_canvas_ignore_mouse;

static RoadMapCanvasMouseHandler RoadMapCanvasMouseScroll =
                                     roadmap_canvas_ignore_mouse;


static void roadmap_canvas_ignore_configure (void) {}

static RoadMapCanvasConfigureHandler RoadMapCanvasConfigure =
                                     roadmap_canvas_ignore_configure;

static void roadmap_canvas_convert_points
                (CGPoint *cgpoints, RoadMapGuiPoint *points, int count) {

    RoadMapGuiPoint *end = points + count;

    while (points < end) {
        cgpoints->x = points->x;
        cgpoints->y = points->y;
        cgpoints += 1;
        points += 1;
    }
}

static void roadmap_canvas_convert_rects
                (CGRect *rects, RoadMapGuiPoint *points, int count) {

    RoadMapGuiPoint *end = points + count;

    while (points < end) {
        rects->origin.x = points->x;
        rects->origin.y = points->y;
        rects->size.width = 1.0;
        rects->size.height = 1.0;
        rects += 1;
        points += 1;
    }
}

void set_fast_draw (int fast_draw) {
    if (fast_draw) {
      CGContextSetAllowsAntialiasing (RoadMapGc, NO);
      CGContextSetInterpolationQuality  (RoadMapGc, kCGInterpolationNone);
      CGContextSetLineCap(RoadMapGc, kCGLineCapButt);
      CGContextSetLineJoin(RoadMapGc, kCGLineJoinMiter);
   }
}

void end_fast_draw (int fast_draw) {
    if (fast_draw) {
      CGContextSetAllowsAntialiasing (RoadMapGc, YES);
      CGContextSetInterpolationQuality  (RoadMapGc, kCGInterpolationDefault);
      CGContextSetLineCap(RoadMapGc, kCGLineCapRound);
      CGContextSetLineJoin(RoadMapGc, kCGLineJoinRound);
   }
}

void roadmap_canvas_get_text_extents 
        (const char *text, int size, int *width,
         int *ascent, int *descent, int *can_tilt) 
{
   if (size == -1)
   {
      size = FONT_DEFAULT_SIZE;
   }
   float size_f = (size * 1.0);
    UniChar buf[2048];
    CGGlyph glyphs[2048];
    CFStringRef  string;
    CGPoint point;

    CGFontRef font = CGContextGetFont(RoadMapGc);
    CGContextSetFontSize(RoadMapGc, size_f);

    string = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
    CFIndex length = CFStringGetLength(string);
    if (length >= 2048)
   {
      CFRelease (string);
        return;
   }
    CFStringGetCharacters (string, CFRangeMake(0, length), buf);
    CFRelease (string);

    CGFontGetGlyphsForUnichars(font, buf, glyphs, length);

    CGContextSetTextDrawingMode (RoadMapGc, kCGTextInvisible);
    CGContextSetTextMatrix(RoadMapGc, CGAffineTransformIdentity);
    CGContextShowGlyphs(RoadMapGc, glyphs, length);
    point = CGContextGetTextPosition(RoadMapGc);
    CGContextSetTextDrawingMode (RoadMapGc, kCGTextFill);

    int unitsperem = CGFontGetUnitsPerEm(font);
    float unitsperpixel = unitsperem / size_f;

    *width = point.x + 0.5;
    *ascent = CGFontGetAscent(font) / unitsperpixel;
    *descent = -(CGFontGetDescent(font) / unitsperpixel);
    if (can_tilt) *can_tilt = 1;
}

RoadMapPen roadmap_canvas_select_pen (RoadMapPen pen) {

   float lengths[2] = {3.0f, 3.0f};
   RoadMapPen old_pen = CurrentPen;
   CurrentPen = pen;

   if (pen->style == 2) { /* dashed */
       CGContextSetLineDash(RoadMapGc, 0.0f,  lengths, 2);
   } else {
       CGContextSetLineDash(RoadMapGc, 0.0f,  NULL, 0);
   }
   CGContextSetLineWidth(RoadMapGc, pen->lineWidth);
   CGContextSetRGBStrokeColor( RoadMapGc, pen->stroke.r, pen->stroke.g, pen->stroke.b, pen->stroke.a);
   CGContextSetRGBFillColor( RoadMapGc, pen->stroke.r, pen->stroke.g, pen->stroke.b, pen->stroke.a);

   return old_pen;
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
      pen->style = 1; //solid
      pen->stroke.r = 0.0;
      pen->stroke.g = 0.0;
      pen->stroke.b = 0.0;
      pen->stroke.a = 1.0;
      pen->lineWidth = 1.0;
      pen->next = RoadMapPenList;

      RoadMapPenList = pen;
   }

   roadmap_canvas_select_pen (pen);

   return pen;
}

void roadmap_canvas_set_foreground (const char *color) {


    float r, g, b, a; 
    a = 1.0;
    int high, i, low;

    if (*color == '#') {
        int ir, ig, ib, ia;
        int count;
        count = sscanf(color, "#%2x%2x%2x%2x", &ir, &ig, &ib, &ia);
        r = ir/255.0;
        g = ig/255.0;
        b = ib/255.0;
        
        if (count == 4)
        {
            a = ia/255.0;
        }
    } else {
        /* Do binary search on color table */
        for (low=(-1), high=sizeof(color_table)/sizeof(color_table[0]);
                high-low > 1;) {
            i = (high+low) / 2;
            if (strcmp(color, color_table[i].name) <= 0) high = i;
            else low = i;
        }

        if (!strcmp(color, color_table[high].name)) {
            r = color_table[high].r / 255.0;
            g = color_table[high].g / 255.0;
            b = color_table[high].b / 255.0;
        } else {
            r = 0.0;
            g = 0.0;
            b = 0.0;
        }
    }

    CurrentPen->stroke.r = r;
    CurrentPen->stroke.g = g;
    CurrentPen->stroke.b = b;
    CurrentPen->stroke.a = a;  

	roadmap_canvas_select_pen(CurrentPen);
}

void roadmap_canvas_set_linestyle (const char *style) {
   if (strcasecmp (style, "dashed") == 0) {
       CurrentPen->style = 2;
   } else {
       CurrentPen->style = 1;
   }
}

void roadmap_canvas_set_thickness  (int thickness) {
   if (thickness < 0) {
      //printf ("Warning: thickness %d changed to 0\n",thickness);
      thickness = 0;
   }
   
   CurrentPen->lineWidth = thickness * 1.0;
}

#if defined(ROADMAP_ADVANCED_STYLE)
/* this are stubs */
void roadmap_canvas_set_opacity (int opacity) {}

void roadmap_canvas_set_linejoin(const char *join) {}
void roadmap_canvas_set_linecap(const char *cap) {}

void roadmap_canvas_set_brush_color(const char *color) {}
void roadmap_canvas_set_brush_style(const char *style) {}
void roadmap_canvas_set_brush_isbackground(int isbackground) {}

void roadmap_canvas_set_label_font_name(const char *name) {}
void roadmap_canvas_set_label_font_color(const char *color) {}
void roadmap_canvas_set_label_font_size(int size) {}
void roadmap_canvas_set_label_font_spacing(int spacing) {}
void roadmap_canvas_set_label_font_weight(const char *weight) {}
void roadmap_canvas_set_label_font_style(int style) {}

void roadmap_canvas_set_label_buffer_color(const char *color) {}
void roadmap_canvas_set_label_buffer_size(int size) {}
#endif /* ROADMAP_ADVANCED_STYLE */

void roadmap_canvas_erase (void) {
   /* 'erase' means fill the canvas with the foreground color */
   CGContextFillRect(RoadMapGc, [RoadMapDrawingArea bounds]);
}

void roadmap_canvas_erase_area (const RoadMapGuiRect *rect)
{
	CGRect imageRect = CGRectMake(rect->minx * 1.0f, rect->miny * 1.0f, (rect->maxx - rect->minx) * 1.0f, (rect->maxy - rect->miny) * 1.0f);
	CGContextFillRect(RoadMapGc, imageRect);
}


void roadmap_canvas_draw_string  (RoadMapGuiPoint *position,
                                  int corner,
                                  int size,
                                  const char *text) {

   int text_width;
   int text_ascent;
   int text_descent;
   int text_height;
   RoadMapGuiPoint start[1];
   roadmap_canvas_get_text_extents 
        (text, size, &text_width, &text_ascent, &text_descent, NULL);
   text_height = text_ascent + text_descent;
   start->x = position->x;
   start->y = position->y;
   if (corner & ROADMAP_CANVAS_RIGHT)
      start->x -= text_width;
   else if (corner & ROADMAP_CANVAS_CENTER_X)
      start->x -= text_width / 2;

   if (corner & ROADMAP_CANVAS_BOTTOM)
      start->y -= text_descent;
   else if (corner & ROADMAP_CANVAS_CENTER_Y)
      start->y = start->y - text_descent + ((text_descent + text_ascent) / 2);
   else 
      start->y += text_ascent;

   roadmap_canvas_draw_string_angle (start, size, 0, text);
}

void roadmap_canvas_draw_string_angle (RoadMapGuiPoint *center,
                                       int size, int angle, const char *text)
{
    CGAffineTransform trans;
    float angle_f = (angle - 180.0f) * M_PI / 180.0f;
    float x = center->x * 1.0f;
    float y = center->y * 1.0f;
    float size_f = (size * 1.0f);
    UniChar buf[2048];
    CGGlyph glyphs[2048];
    CFStringRef  string;
    CGFontRef font = CGContextGetFont(RoadMapGc);

    CGContextSetFontSize(RoadMapGc, size_f);

    string = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
    CFIndex length = CFStringGetLength(string);
    if (length >= 2048)
        return;
    CFStringGetCharacters (string, CFRangeMake(0, length), buf);

    CGFontGetGlyphsForUnichars(font, buf, glyphs, length);

    trans = CGAffineTransformMakeRotation(angle_f);
    trans = CGAffineTransformScale (trans, -1.0, 1.0);

    CGContextSetTextMatrix(RoadMapGc, trans);

    CGContextShowGlyphsAtPoint(RoadMapGc, x, y, glyphs, length);
}

void roadmap_canvas_draw_multiple_points (int count, RoadMapGuiPoint *points) {

   CGRect rects[1024];

   while (count > 1024) {
       roadmap_canvas_convert_rects (rects, points, 1024);
       CGContextFillRects(RoadMapGc, rects, 1024);
       points += 1024;
       count -= 1024;
   }
   roadmap_canvas_convert_rects (rects, points, count);
   CGContextFillRects(RoadMapGc, rects, count);
}


static void roadmap_calc_bezier_controlpoints
   (CGPoint *points, float *c1x, float *c1y, float *c2x, float *c2y)
{
    float smooth = BEZIER_SMOOTH_VALUE;
    short x0 = points[0].x;
    short y0 = points[0].y;
    short x1 = points[1].x;
    short y1 = points[1].y;
    short x2 = points[2].x;
    short y2 = points[2].y;

    float angle;

    float xc1 = (x0 + x1) / 2.0;
    float yc1 = (y0 + y1) / 2.0;
    float xc2 = (x1 + x2) / 2.0;
    float yc2 = (y1 + y2) / 2.0;

    float len1 = sqrt((x1-x0) * (x1-x0) + (y1-y0) * (y1-y0));
    float len2 = sqrt((x2-x1) * (x2-x1) + (y2-y1) * (y2-y1));
    float dot = (x1 - x0) * (x2 - x1) + (y1 - y0) * (y2 - y1);

    angle = acos ( dot / (len1 * len2) );
    if (dot < 0)
    {
       if (angle > 0)
           angle += M_PI;
       else
           angle -= M_PI;
    }

    /* If the angle is less than a bit more than 90 degrees
       _or_ if two of the points coincide - return the
       mid point as control points (to cause a sharp
       corner) */
    if ((angle > 1.4 || angle < -1.4) ||
        (len1 < 0.0001 && len1 > -0.001) ||
        (len2 < 0.0001 && len2 > -0.001))
    {
        *c1x = x1;
        *c1y = y1;

        *c2x = x1;
        *c2y = y1;
        return;
 }

    float k = len1 / (len1 + len2);

    float xm = xc1 + (xc2 - xc1) * k;
    float ym = yc1 + (yc2 - yc1) * k;

    *c1x = (xc1 - xm) * smooth + x1;
    *c1y = (yc1 - ym) * smooth + y1;

    *c2x = (xc2 - xm) * smooth + x1;
    *c2y = (yc2 - ym) * smooth + y1;
}

static void roadmap_draw_bezier_path(CGPoint *p, int count)
{
    int i = 0;
    float c1x, c1y, c2x, c2y, p1x, p1y, p2x, p2y;
    CGContextBeginPath(RoadMapGc);
    CGContextMoveToPoint(RoadMapGc, p[0].x, p[0].y);
    c1x = (p[1].x - p[0].x) / 2.0 + p[0].x;
    c1y = (p[1].y - p[0].y) / 2.0 + p[0].y;
    roadmap_calc_bezier_controlpoints(&(p[0]), &p1x, &p1y, &p2x, &p2y);
    c2x = p1x; c2y = p1y;
    CGContextAddCurveToPoint(RoadMapGc, c1x, c1y, c2x, c2y, p[i+1].x, p[i+1].y);
    i = 0;
    for (i = 1; i + 3 <= count; i++)
    {
        c1x = p2x; c1y = p2y;
        roadmap_calc_bezier_controlpoints(&(p[i]), &p1x, &p1y, &p2x, &p2y);
        c2x = p1x; c2y = p1y;
        CGContextAddCurveToPoint(RoadMapGc, c1x, c1y, c2x, c2y, p[i+1].x, p[i+1].y);
    }
    if (count > 2)
    {
        c1x = p2x; c1y = p2y;

        c2x = (p[i + 1].x - p[i].x) / 2.0 + p[i].x;
        c2y = (p[i + 1].y - p[i].y) / 2.0 + p[i].y;
        CGContextAddCurveToPoint(RoadMapGc, c1x, c1y, c2x, c2y, p[i+1].x, p[i+1].y);
    }
    CGContextStrokePath(RoadMapGc);
}
/*
   Known graphic artifacts of the following function:
   When drawing bezier lines - every 1024th point will
   have a sharp corner.
   Lines that are moving out of the screen will suddenly
   change from being drawn with one control point to
   two... To fix this, an extra line segment outside the
   screen must always be included in the line paths to
   be drawn.
*/
void roadmap_canvas_draw_multiple_lines 
(int count, int *lines, RoadMapGuiPoint *points, int fast_draw) {

    int i;
    int bezier = !fast_draw;
    int count_of_points;
    CGPoint cgpoints[1024];
   set_fast_draw (fast_draw);

    for (i = 0; i < count; ++i) {

        count_of_points = *lines;
        if (count_of_points < 2)
           continue;

        while (count_of_points > 1024) {
            roadmap_canvas_convert_points (cgpoints, points, 1024);
            CGContextBeginPath(RoadMapGc);
            if (bezier)
               roadmap_draw_bezier_path(cgpoints, 1024);
            else
               CGContextAddLines(RoadMapGc, cgpoints, 1024);

            CGContextStrokePath(RoadMapGc);
            points += 1023;
            count_of_points -= 1023;
        }

        roadmap_canvas_convert_points (cgpoints, points, count_of_points);
        CGContextBeginPath(RoadMapGc);
        if (bezier)
           roadmap_draw_bezier_path(cgpoints, count_of_points);
        else
           CGContextAddLines(RoadMapGc, cgpoints, count_of_points);

        CGContextStrokePath(RoadMapGc);
        points += count_of_points;
        lines += 1;
    }
   
   end_fast_draw (fast_draw);
}


void roadmap_canvas_draw_multiple_polygons
         (int count, int *polygons, RoadMapGuiPoint *points, int filled,
            int fast_draw) {

    int i;
    int count_of_points;
    CGPoint cgpoints[1024];
    
   set_fast_draw (fast_draw);
   
	for (i = 0; i < count; ++i) {

        count_of_points = *polygons;

        while (count_of_points > 1024) {
            roadmap_canvas_convert_points (cgpoints, points, 1024);
            CGContextBeginPath(RoadMapGc);
            CGContextAddLines(RoadMapGc, cgpoints, 1024);
            CGContextClosePath(RoadMapGc);
            if (filled)
                CGContextFillPath(RoadMapGc);
            else
                CGContextStrokePath(RoadMapGc);
            points += 1023;
            count_of_points -= 1023;
        }

        roadmap_canvas_convert_points (cgpoints, points, count_of_points);

        CGContextBeginPath(RoadMapGc);
        CGContextAddLines(RoadMapGc, cgpoints, count_of_points);
        CGContextClosePath(RoadMapGc);
        if (filled)
            CGContextFillPath(RoadMapGc);
        else
            CGContextStrokePath(RoadMapGc);

        points += count_of_points;
        polygons += 1;
    }
    
   end_fast_draw (fast_draw);
}

void roadmap_canvas_draw_multiple_circles
        (int count, RoadMapGuiPoint *centers, int *radius, int filled,
            int fast_draw) {

   int i;

   set_fast_draw (fast_draw);
   
   for (i = 0; i < count; ++i) {

       int r = radius[i];

       CGRect circle = CGRectMake(centers[i].x-r, (centers[i].y)-r, r*2, r*2);
       CGContextAddEllipseInRect(RoadMapGc, circle);
       if (filled)
           CGContextFillEllipseInRect(RoadMapGc, circle);
       else
           CGContextStrokeEllipseInRect(RoadMapGc, circle);

   }
   
   end_fast_draw (fast_draw);
}

void roadmap_canvas_register_configure_handler
                    (RoadMapCanvasConfigureHandler handler) {

   RoadMapCanvasConfigure = handler;
}

/*
static gboolean roadmap_canvas_scroll_event
               (GtkWidget *w, GdkEventScroll *event, gpointer data) {

   int direction = 0;
   RoadMapGuiPoint point;

   point.x = event->x;
   point.y = event->y;

   switch (event->direction) {
      case GDK_SCROLL_UP:    direction = 1;  break;
      case GDK_SCROLL_DOWN:  direction = -1; break;
      case GDK_SCROLL_LEFT:  direction = 2;  break;
      case GDK_SCROLL_RIGHT: direction = -2; break;
   }

   (*RoadMapCanvasMouseScroll) (direction, &point);

   return FALSE;
}

*/
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
   RoadMapCanvasMouseScroll = handler;
}


int roadmap_canvas_width (void) {
   return CanvasWidth;
}

int roadmap_canvas_height (void) {
   return CanvasHeight;
}

void roadmap_canvas_refresh (void) {
   [RoadMapDrawingArea setNeedsDisplay];
}

void roadmap_canvas_save_screenshot (const char* filename) {
    if (RoadMapGc)
    {
        CGImageRef image = CGBitmapContextCreateImage (RoadMapGc);
        // TODO: save image
        CGImageRelease(image);
    }
}

int  roadmap_canvas_image_width  (const RoadMapImage image)
{
   UIImage *img = (UIImage*)image;
   if (img)
      return img.size.width;
   else
      return 0;
}

int  roadmap_canvas_image_height (const RoadMapImage image)
{
   UIImage *img = (UIImage*)image;
   if (img)
      return img.size.height;
   else
      return 0;
}

static char *roadmap_canvas_image_path (const char *name) {
    unsigned int i;

    if (name == NULL)
        return NULL;

    const char *image_file = roadmap_path_search_icon (name);
    if (image_file == NULL)
        return NULL;

    for (i = 0; i < strlen(image_file); i++)
    {
        if (strncmp (image_file + i, "resources", 9) == 0)
	{
            return (char *)(image_file + i);
}	
    }
    return NULL;
}

RoadMapImage roadmap_canvas_load_image ( const char* path, const char* file_name)
{
   char *imagePath;
   NSString *nsImagePath;
   imagePath = roadmap_canvas_image_path (file_name);

    if (imagePath)
    {
        nsImagePath = [[NSString alloc] initWithUTF8String:imagePath];
        UIImage *image = [UIImage imageNamed: nsImagePath];
        return (RoadMapImage)image;
    }
    return NULL;
}

void roadmap_canvas_draw_image (RoadMapImage image, RoadMapGuiPoint *pos,
                                int opacity, int mode)
{
   UIImage *img = (UIImage*)image;
   CGPoint point;
   point.x = pos->x;
   point.y = pos->y; 
   //NSLog (@"draw image with opacity: %i at %f %f", opacity, point.x, point.y);
   if (img)
 {
      UIGraphicsPushContext(RoadMapGc);
      [img drawAtPoint:point blendMode:kCGBlendModeNormal alpha:1.0f];
      UIGraphicsPopContext();
}
}

void roadmap_canvas_copy_image (RoadMapImage dst_image,
                                const RoadMapGuiPoint *pos,
                                const RoadMapGuiRect  *rect,
                                RoadMapImage src_image, int mode)
{
}

void roadmap_canvas_draw_image_text (RoadMapImage image,
                                     const RoadMapGuiPoint *position,
                                     int size, const char *text)
{
}

void roadmap_canvas_layer_select(int layer)
{
/* remove me */
}

void roadmap_canvas_get_chording_pt (RoadMapGuiPoint points[MAX_CHORDING_POINTS])
{
   points[0].x = ChordingPoints[0].x;
   points[0].y = ChordingPoints[0].y;
   
   points[1].x = ChordingPoints[1].x;
   points[1].y = ChordingPoints[1].y;
   
   points[2].x = ChordingPoints[2].x;
   points[2].y = ChordingPoints[2].y;
}

int roadmap_canvas_is_chording()
{
   return (ChordingPoints[0].x > -1);
}




@implementation RoadMapCanvasView

-(RoadMapCanvasView *) initWithFrame: (struct CGRect) rect
{
    CanvasWidth = rect.size.width;
    CanvasHeight = rect.size.height;
    self = [super initWithFrame: rect];


    touch1 = nil;
    touch2 = nil;

    RoadMapDrawingArea = self;
    self.multipleTouchEnabled = TRUE;

    CGColorSpaceRef    imageColorSpace = CGColorSpaceCreateDeviceRGB();       
    RoadMapGc = CGBitmapContextCreate(nil, CanvasWidth, CanvasHeight, 8, 0, imageColorSpace, kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(imageColorSpace);

    CGFontRef font = CGFontCreateWithFontName((CFStringRef)@"ArialUnicodeMS") ;
    CGContextSetFont(RoadMapGc, font);
    CGContextSetTextDrawingMode (RoadMapGc, kCGTextFill);
    CGContextSetLineCap(RoadMapGc, kCGLineCapRound);

    (*RoadMapCanvasConfigure) ();

    return self;
}

-(void) drawRect: (CGRect) rect
{
    if (RoadMapGc)
    {
        /* CGBitmapContextCreateImage might use copy-on-change
           so if we're lucky no copy is actually made! */
        CGImageRef ref = CGBitmapContextCreateImage(RoadMapGc);           
        CGContextDrawImage(UIGraphicsGetCurrentContext(), rect, ref);
        CGImageRelease(ref);
    }
}

- (void)touchesBegan: (NSSet *) touches withEvent: (UIEvent*)event
{
   UITouch *touch = [touches anyObject];
   CGPoint location =  [touch locationInView: touch.view];
   RoadMapGuiPoint point;
   point.x = location.x;
   point.y = location.y;
   for (UITouch *touch in touches) {
      location =  [touch locationInView: touch.view];
      if (touch1 == nil)
      {
         touch1 = touch;
         touch1point = location;
      }
      else if (touch2 == nil)
      {
         touch2 = touch;
         touch2point = location;
      }
   }	
   if (touch1 != nil && touch2 != nil) {
      ChordingPoints[0] = touch1point;
      ChordingPoints[1] = touch2point;
   } else {
      ChordingPoints[0].x = -1;  
   }
         ChordingPoints[2].x = -1;
   
   (*RoadMapCanvasMouseButtonPressed) (1, &point);
}

- (void)touchesEnded: (NSSet *) touches withEvent: (UIEvent*)event
{
   UITouch *touch = [touches anyObject];
   CGPoint location =  [touch locationInView: touch.view];
   RoadMapGuiPoint point;
   point.x = location.x;
   point.y = location.y;
   for (UITouch *touch in touches) {
      if (touch == touch1)
         touch1 = nil;
      else if (touch == touch2)
         touch2 = nil;
   }	
   
   if (touch1 != nil && touch2 != nil) {
      NSLog(@"chording !");
      
      ChordingPoints[0] = touch1point;
      ChordingPoints[1] = touch2point;
      
      ChordingPoints[2].x = -1;
   } else {
      ChordingPoints[0].x = -1;
   }
   
   (*RoadMapCanvasMouseButtonReleased) (1, &point);
}

- (void)touchesMoved: (NSSet *) touches withEvent: (UIEvent*)event
{
   UITouch *touch = [touches anyObject];
   CGPoint location =  [touch locationInView: touch.view];
   RoadMapGuiPoint point;
   point.x = location.x;
   point.y = location.y;
   for (UITouch *touch in touches) {
      location =  [touch locationInView: touch.view];
      if (touch1 == touch)
         touch1point = location;
      else if (touch2 == touch)
         touch2point = location;
   }	
   
   if (touch1 != nil && touch2 != nil) {
      
      ChordingPoints[0] = touch1point;
      ChordingPoints[1] = touch2point;
            
      ChordingPoints[2].x = -1;
   } else {
      ChordingPoints[0].x = -1;
   }
   
   (*RoadMapCanvasMouseMoved) (0, &point);
}

@end

