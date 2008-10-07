/* roadmap_canvas.c - manage the canvas that is used to draw the map.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
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
#import <UIKit/CDStructures.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"

#include "roadmap_canvas.h"
#include "roadmap_iphonecanvas.h"
#include "colors.h"

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


static struct roadmap_canvas_pen *RoadMapPenList = NULL;

static RoadMapCanvasView  *RoadMapDrawingArea;
static CGContextRef RoadMapGc;
static LKLayer *mainLayer;
static int CanvasWidth, CanvasHeight;

static RoadMapPen CurrentPen;

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
        cgpoints->y = CanvasHeight - points->y;
        cgpoints += 1;
        points += 1;
    }
}

static void roadmap_canvas_convert_rects
                (CGRect *rects, RoadMapGuiPoint *points, int count) {

    RoadMapGuiPoint *end = points + count;

    while (points < end) {
        rects->origin.x = points->x;
        rects->origin.y = CanvasHeight - points->y;
        rects->size.width = 1.0;
        rects->size.height = 1.0;
        rects += 1;
        points += 1;
    }
}

void roadmap_canvas_get_text_extents 
        (const char *text, int size, int *width,
         int *ascent, int *descent, int *can_tilt) 
{
    float size_f = (size * 1.0f);
    UniChar buf[2048];
    CGGlyph glyphs[2048];
    CFStringRef  string;
    CGPoint point;

    CGFontRef font = CGContextGetFont(RoadMapGc);
    CGContextSetFontSize(RoadMapGc, size_f);

    string = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
    CFIndex length = CFStringGetLength(string);
    if (length >= 2048)
        return;
    CFStringGetCharacters (string, CFRangeMake(0, length), buf);

    CGFontGetGlyphsForUnichars(font, buf, glyphs, length);

    CGContextSetTextDrawingMode (RoadMapGc, kCGTextInvisible);
    CGContextSetTextPosition(RoadMapGc, 0,0);
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
        int ir, ig, ib;
        sscanf(color, "#%2x%2x%2x", &ir, &ig, &ib);
        r = ir/255.0;
        g = ig/255.0;
        b = ib/255.0;
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

}

void roadmap_canvas_set_linestyle (const char *style) {
   if (strcasecmp (style, "dashed") == 0) {
       CurrentPen->style = 2;
   } else {
       CurrentPen->style = 1;
   }
}

void roadmap_canvas_set_thickness  (int thickness) {
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
   CGContextFillRect(RoadMapGc, [RoadMapDrawingArea frame]);
}

void roadmap_canvas_draw_string  (RoadMapGuiPoint *position,
                                  int corner,
                                  int size,
                                  const char *text) {

   int text_width;
   int text_ascent;
   int text_descent;
   RoadMapGuiPoint start[1];
   roadmap_canvas_get_text_extents 
        (text, size, &text_width, &text_ascent, &text_descent, NULL);
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
    float angle_f = (angle * -1.0f) * M_PI / 180.0f;
    float x = center->x * 1.0f;
    float y = CanvasHeight - center->y * 1.0f;
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


void roadmap_canvas_draw_multiple_lines 
(int count, int *lines, RoadMapGuiPoint *points, int fast_draw) {

    int i;
    int count_of_points;
    CGPoint cgpoints[1024];

    for (i = 0; i < count; ++i) {

        count_of_points = *lines;
        while (count_of_points > 1024) {
            roadmap_canvas_convert_points (cgpoints, points, 1024);
            CGContextBeginPath(RoadMapGc);
            CGContextAddLines(RoadMapGc, cgpoints, 1024);
            CGContextStrokePath(RoadMapGc);
            points += 1023;
            count_of_points -= 1023;
        }

        roadmap_canvas_convert_points (cgpoints, points, count_of_points);

        CGContextBeginPath(RoadMapGc);
        CGContextAddLines(RoadMapGc, cgpoints, count_of_points);
        CGContextStrokePath(RoadMapGc);

        points += count_of_points;
        lines += 1;
    }
}

void roadmap_canvas_draw_multiple_polygons
         (int count, int *polygons, RoadMapGuiPoint *points, int filled,
            int fast_draw) {
    int i;
    int count_of_points;
    CGPoint cgpoints[1024];
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
}

void roadmap_canvas_draw_multiple_circles
        (int count, RoadMapGuiPoint *centers, int *radius, int filled,
            int fast_draw) {

   int i;

   for (i = 0; i < count; ++i) {

       int r = radius[i];

       CGRect circle = CGRectMake(centers[i].x-r, (CanvasHeight - centers[i].y)-r, r*2, r*2);
       CGContextAddEllipseInRect(RoadMapGc, circle);
       if (filled)
           CGContextFillEllipseInRect(RoadMapGc, circle);
       else
           CGContextStrokeEllipseInRect(RoadMapGc, circle);

   }
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
        // save image
        CGImageRelease(image);
    }
}

@implementation RoadMapCanvasView

-(RoadMapCanvasView *) initWithFrame: (struct CGRect) rect
{
    CanvasWidth = rect.size.width;
    CanvasHeight = rect.size.height;
    self = [super initWithFrame: rect];

    RoadMapDrawingArea = self;

    CGColorSpaceRef    imageColorSpace = CGColorSpaceCreateDeviceRGB();       
    RoadMapGc = CGBitmapContextCreate(nil, CanvasWidth, CanvasHeight, 8, 0, imageColorSpace, kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(imageColorSpace);

    CGFontRef font = CGFontCreateWithFontName((CFStringRef)@"arialuni") ;
    CGContextSetFont(RoadMapGc, font);
    CGContextSetTextDrawingMode (RoadMapGc, kCGTextFill);
    CGContextSetLineCap(RoadMapGc, kCGLineCapRound);

    mainLayer = [[LKLayer layer] retain];
    [mainLayer setFrame: rect];

    [self._layer addSublayer: mainLayer];
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
        [mainLayer setContents: (id)ref];
        CGImageRelease(ref);
    }
}

- (void)mouseDown: (GSEvent*)event
{
    printf("mouseDown\n");
    CGPoint location = GSEventGetLocationInWindow(event);
    RoadMapGuiPoint point;
    point.x = location.x;
    point.y = location.y;
    (*RoadMapCanvasMouseButtonPressed) (1, &point);
}

- (void) mouseUp:(GSEvent*)event
{
    printf("mouseUp\n");
    CGPoint location = GSEventGetLocationInWindow(event);
    RoadMapGuiPoint point;
    point.x = location.x;
    point.y = location.y;
    (*RoadMapCanvasMouseButtonReleased) (1, &point);
}

- (void) mouseDragged:(GSEvent*)event
{
    printf("mouseMoved\n");
    CGPoint location = GSEventGetLocationInWindow(event);
    RoadMapGuiPoint point;
    point.x = location.x;
    point.y = location.y;
    (*RoadMapCanvasMouseMoved) (0, &point);
}

@end

