/* roadmap_screen.c - Draw the map on the screen.
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
 *   See roadmap_screen.h.
 */

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_layer.h"
#include "roadmap_square.h"
#include "roadmap_line.h"
#include "roadmap_shape.h"
#include "roadmap_point.h"
#include "roadmap_polygon.h"
#include "roadmap_locator.h"
#include "roadmap_navigate.h"

#include "roadmap_sprite.h"
#include "roadmap_object.h"
#include "roadmap_trip.h"
#include "roadmap_canvas.h"
#include "roadmap_pointer.h"
#include "roadmap_display.h"
#include "roadmap_plugin.h"
#include "roadmap_factory.h"

#include "roadmap_screen.h"


static RoadMapConfigDescriptor RoadMapConfigDeltaX =
                        ROADMAP_CONFIG_ITEM("Delta", "X");

static RoadMapConfigDescriptor RoadMapConfigDeltaY =
                        ROADMAP_CONFIG_ITEM("Delta", "Y");

static RoadMapConfigDescriptor RoadMapConfigDeltaRotate =
                        ROADMAP_CONFIG_ITEM("Delta", "Rotate");

static RoadMapConfigDescriptor RoadMapConfigAccuracyMouse =
                        ROADMAP_CONFIG_ITEM("Accuracy", "Mouse");

static RoadMapConfigDescriptor RoadMapConfigMapBackground =
                        ROADMAP_CONFIG_ITEM("Map", "Background");

static RoadMapConfigDescriptor RoadMapConfigMapSigns =
                        ROADMAP_CONFIG_ITEM("Map", "Signs");

static RoadMapConfigDescriptor RoadMapConfigMapRefresh =
                        ROADMAP_CONFIG_ITEM("Map", "Refresh");

static RoadMapConfigDescriptor RoadMapConfigEventRightClick =
                        ROADMAP_CONFIG_ITEM("Events", "Right Click");

static RoadMapConfigDescriptor RoadMapConfigStylePrettyDrag =
                  ROADMAP_CONFIG_ITEM("Style", "Pretty Lines when Dragging");

static RoadMapConfigDescriptor RoadMapConfigStyleObjects =
                  ROADMAP_CONFIG_ITEM("Style", "Show Objects when Dragging");


static int RoadMapScreenInitialized = 0;
static int RoadMapScreenFrozen = 0;
static int RoadMapScreenDragging = 0;

static RoadMapGuiPoint RoadMapScreenPointerLocation;
static RoadMapPosition RoadMapScreenCenter;

static int RoadMapScreenRotation;
static int RoadMapScreenWidth;
static int RoadMapScreenHeight;

static int RoadMapScreenDeltaX;
static int RoadMapScreenDeltaY;

static char *SquareOnScreen;
static int   SquareOnScreenCount;

static RoadMapPen RoadMapScreenLastPen = NULL;


static void roadmap_screen_after_refresh (void) {}

static RoadMapScreenSubscriber RoadMapScreenAfterRefresh =
                                       roadmap_screen_after_refresh;


/* Define the buffers used to group all actual drawings. */

#define ROADMAP_SCREEN_BULK  4096

/* This is a default definition, because we might want to set this smaller
 * for some memory-starved targets.
 */
#ifndef ROADMAP_MAX_VISIBLE
#define ROADMAP_MAX_VISIBLE  20000
#endif

static struct {

   int *cursor;
   int *end;
   int data[ROADMAP_SCREEN_BULK];

} RoadMapScreenObjects;

struct roadmap_screen_point_buffer {

   RoadMapGuiPoint *cursor;
   RoadMapGuiPoint *end;
   RoadMapGuiPoint data[ROADMAP_SCREEN_BULK];
};

static struct roadmap_screen_point_buffer RoadMapScreenLinePoints;
static struct roadmap_screen_point_buffer RoadMapScreenPoints;

static int RoadMapPolygonGeoPoints[ROADMAP_SCREEN_BULK];


static RoadMapPen RoadMapBackground = NULL;
static RoadMapPen RoadMapPenEdges = NULL;


static void roadmap_screen_flush_points (void) {

   if (RoadMapScreenPoints.cursor == RoadMapScreenPoints.data) return;

   roadmap_math_rotate_coordinates
       (RoadMapScreenPoints.cursor - RoadMapScreenPoints.data,
        RoadMapScreenPoints.data);

   roadmap_canvas_draw_multiple_points
       (RoadMapScreenPoints.cursor - RoadMapScreenPoints.data,
        RoadMapScreenPoints.data);

   RoadMapScreenPoints.cursor  = RoadMapScreenPoints.data;
}


static void roadmap_screen_flush_lines (void) {

   if (RoadMapScreenObjects.cursor == RoadMapScreenObjects.data) return;

   roadmap_math_rotate_coordinates
       (RoadMapScreenLinePoints.cursor - RoadMapScreenLinePoints.data,
        RoadMapScreenLinePoints.data);

   roadmap_canvas_draw_multiple_lines
      (RoadMapScreenObjects.cursor - RoadMapScreenObjects.data,
       RoadMapScreenObjects.data,
       RoadMapScreenLinePoints.data);

   RoadMapScreenObjects.cursor = RoadMapScreenObjects.data;
   RoadMapScreenLinePoints.cursor  = RoadMapScreenLinePoints.data;
}


void roadmap_screen_draw_line (const RoadMapPosition *from,
                               const RoadMapPosition *to,
                               int fully_visible,
                               const RoadMapPosition *shape_start,
                               int first_shape,
                               int last_shape,
                               RoadMapShapeIterator shape_next,
                               RoadMapPen pen) {

   RoadMapGuiPoint point0;
   RoadMapGuiPoint point1;

   int i;
   RoadMapGuiPoint *points;

   int last_point_visible = 0;


   if (RoadMapScreenLastPen != pen) {

      /* The pen has changed: flush the previous lines and points. */

      roadmap_screen_flush_lines ();
      roadmap_screen_flush_points ();

      roadmap_canvas_select_pen (pen);
      RoadMapScreenLastPen = pen;
   }

   if (first_shape >= 0) {

      /* Draw a shaped line. */

      RoadMapPosition midposition;
      RoadMapPosition last_midposition;

      if (last_shape - first_shape + 3 >=
            RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.cursor) {

         if (last_shape - first_shape + 3 >=
               (RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.data)) {

            roadmap_log (ROADMAP_ERROR,
                  "cannot show all shape points (%d entries needed).",
                  last_shape - first_shape + 3);

            last_shape =
               first_shape
               + (RoadMapScreenLinePoints.data - RoadMapScreenLinePoints.cursor)
               - 3;
         }

         roadmap_screen_flush_lines ();
      }

      points = RoadMapScreenLinePoints.cursor;

      /* All the shape positions are relative: we need an absolute position
       * to start with.
       */
      last_midposition = *from;
      midposition = *shape_start;

      last_point_visible = 0; /* We have drawn nothing yet. */

      for (i = first_shape; i <= last_shape; ++i) {

         (*shape_next) (i, &midposition);

         if (roadmap_math_line_is_visible (&last_midposition, &midposition) && 
             roadmap_math_get_visible_coordinates
                        (&last_midposition, &midposition, &point0, &point1)) {

            if ((point0.x == point1.x) &&
                (point0.y == point1.y) &&
                last_point_visible) {

               /* This segment is very short, we can skip it */
               last_midposition = midposition; 

               continue;
            }

            /* Show this line: add 2 points if this is the start of a new
             * complete line (i.e. the first visible line), or just add
             * one more point to the current complete line.
             */
            if (!last_point_visible) {
               *(points++) = point0; /* Show the start of this segment. */
            }
            *(points++) = point1;

            last_point_visible = roadmap_math_point_is_visible (&midposition);

            if (!last_point_visible) {

               /* Show the previous segment as the end of a complete line.
                * The remaining part of the shaped line, if any, will be
                * drawn as a new complete line.
                */
               *RoadMapScreenObjects.cursor =
                  points - RoadMapScreenLinePoints.cursor;
               RoadMapScreenLinePoints.cursor = points;
               RoadMapScreenObjects.cursor += 1;

               if (last_shape - i + 3 >=
                  RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.cursor) {
                  roadmap_screen_flush_lines ();
                  points = RoadMapScreenLinePoints.cursor;
               }
            }
         }

         /* The latest position is our new start. */
         last_midposition = midposition;
      }

      if (roadmap_math_line_is_visible (&last_midposition, to) && 
             roadmap_math_get_visible_coordinates
                        (&last_midposition, to, &point0, &point1)) {

         if (!last_point_visible) {
             *(points++) = point0; /* Show the start of this segment. */
         }
         *(points++) = point1;

         /* set last point as visible to force line completion at the next
          * statement.
          */
         last_point_visible = 1;
      }

      if (last_point_visible) {

         /* End the current complete line. */

         *RoadMapScreenObjects.cursor =
            points - RoadMapScreenLinePoints.cursor;

         RoadMapScreenLinePoints.cursor = points;
         RoadMapScreenObjects.cursor += 1;
      }

   } else {

      /* Draw a line with no shape. */

      /* Optimization: do not draw a line that is obviously not visible. */
      if (! fully_visible) {
         if (! roadmap_math_line_is_visible (from, to)) return;
      }

      /* Optimization: adjust the edges of the line so they do not go out
       * of the screen.
       */
      if (!roadmap_math_get_visible_coordinates (from, to, &point0, &point1)) {
         return;
      }

      if ((point0.x == point1.x) && (point0.y == point1.y)) {
         /* draw a point instead of a line */
         if (RoadMapScreenPoints.cursor >= RoadMapScreenPoints.end) {
            roadmap_screen_flush_points ();
         }
         RoadMapScreenPoints.cursor[0] = point0;
         RoadMapScreenPoints.cursor += 1;

         return;
      }

      if (RoadMapScreenLinePoints.cursor + 2 >= RoadMapScreenLinePoints.end) {
         roadmap_screen_flush_lines ();
      }

      RoadMapScreenLinePoints.cursor[0] = point0;
      RoadMapScreenLinePoints.cursor[1] = point1;

      *RoadMapScreenObjects.cursor = 2;

      RoadMapScreenLinePoints.cursor  += 2;
      RoadMapScreenObjects.cursor += 1;

   }
}


/* TODO: ugly hack (both the hack and the drawing itself are ugly!).
 * This should be rewritten to allow specifying a sprite for
 * the direction mark.
 */
static void roadmap_screen_draw_direction (RoadMapGuiPoint *point0,
                                           RoadMapGuiPoint *point1,
                                           int width,
                                           int direction) {
   RoadMapGuiPoint from;
   RoadMapGuiPoint to;

   double delta_x = point1->x - point0->x;
   double delta_y = point1->y - point0->y;
   double ll = (1.0 * (abs((int)(delta_x)) + abs((int)(delta_y))) / 15);

   if (ll >= 1) {

      static int i=0;
      
      double step_x = delta_x / ll;
      double step_y = delta_y / ll;
      double x = point0->x + step_x;
      double y = point0->y + step_y;

      while (abs((int)(x + step_x - point0->x)) < abs((int)(delta_x))) {
         i++;

         from.x = (int)x;
         from.y = (int)y;
         to.x = (int) (x + step_x);
         to.y = (int) (y + step_y);

         if (RoadMapScreenLinePoints.cursor + 5 >=
               RoadMapScreenLinePoints.end) {
            roadmap_screen_flush_lines ();
         }

         /* main line */
         RoadMapScreenLinePoints.cursor[0] = from;
         RoadMapScreenLinePoints.cursor[1] = to;
         *RoadMapScreenObjects.cursor = 2;
         RoadMapScreenLinePoints.cursor  += 2;
         RoadMapScreenObjects.cursor += 1;

         /* head */
         if ((direction == 1) ||
               ((direction == 3) && (i % 2))) {
            RoadMapGuiPoint head;
            double dir=atan2(from.x-to.x, from.y-to.y);
            int i1=9;
            head.x = (short)(to.x + i1*sin(dir+0.5));
            head.y = (short)(to.y + i1*cos(dir+0.5));
            RoadMapScreenLinePoints.cursor[0] = head;
            head.x = (short)(to.x + i1*sin(dir-0.5));
            head.y = (short)(to.y + i1*cos(dir-0.5));
            RoadMapScreenLinePoints.cursor[2] = head;
            RoadMapScreenLinePoints.cursor[1] = to;
            *RoadMapScreenObjects.cursor = 3;
            RoadMapScreenLinePoints.cursor  += 3;
            RoadMapScreenObjects.cursor += 1;
         }

         if ((direction == 2) ||
               ((direction == 3) && !(i % 2))) {
            /* second head */
            RoadMapGuiPoint head;
            double dir=atan2(to.x-from.x, to.y-from.y);
            int i1=9;
            head.x = (short)(from.x + i1*sin(dir+0.5));
            head.y = (short)(from.y + i1*cos(dir+0.5));
            RoadMapScreenLinePoints.cursor[0] = head;
            head.x = (short)(from.x + i1*sin(dir-0.5));
            head.y = (short)(from.y + i1*cos(dir-0.5));
            RoadMapScreenLinePoints.cursor[2] = head;
            RoadMapScreenLinePoints.cursor[1] = from;
            *RoadMapScreenObjects.cursor = 3;
            RoadMapScreenLinePoints.cursor  += 3;
            RoadMapScreenObjects.cursor += 1;
         }

         x += step_x*3;
         y += step_y*3;
      }
   }
}


/* TODO: this function should either be implemented in
 * roadmap_screen_draw_one_line(), or common parts should be extracted.
 */
void roadmap_screen_draw_line_direction (const RoadMapPosition *from,
                                         const RoadMapPosition *to,
                                         const RoadMapPosition *shape_start,
                                         int first_shape,
                                         int last_shape,
                                         RoadMapShapeIterator shape_next,
                                         int width,
                                         int direction) {

   static RoadMapPen direction_pen = NULL;

   RoadMapGuiPoint point0;
   RoadMapGuiPoint point1;

   /* These are used when the line has a shape: */
   RoadMapPosition midposition;
   RoadMapPosition last_midposition;

   int i;

   roadmap_screen_flush_lines ();
   roadmap_screen_flush_points ();

   if (direction_pen == NULL) {
      direction_pen = roadmap_canvas_create_pen ("direction_mark");
      roadmap_canvas_set_thickness (1);
      roadmap_canvas_set_foreground ("black");
   } else {
     roadmap_canvas_select_pen (direction_pen);
   }

   if (first_shape >= 0) {

      last_midposition = *from;
      midposition = *shape_start;

      for (i = first_shape; i <= last_shape; ++i) {

         (*shape_next) (i, &midposition);

         if (roadmap_math_line_is_visible (&last_midposition, &midposition) && 
             roadmap_math_get_visible_coordinates
                        (&last_midposition, &midposition, &point0, &point1)) {

            roadmap_screen_draw_direction (&point0, &point1, width, direction);

         }
         last_midposition = midposition;
      }

      if (roadmap_math_line_is_visible (&last_midposition, to) && 
             roadmap_math_get_visible_coordinates
                        (&last_midposition, to, &point0, &point1)) {

         roadmap_screen_draw_direction (&point0, &point1, width, direction);

      }

   } else {

      if (roadmap_math_get_visible_coordinates (from, to, &point0, &point1)) {
         roadmap_screen_draw_direction (&point0, &point1, width, direction);
      }
   }

   roadmap_screen_flush_lines ();
   roadmap_screen_flush_points ();
}


static void roadmap_screen_flush_polygons (void) {

   int count = RoadMapScreenObjects.cursor - RoadMapScreenObjects.data;
    
   if (count == 0) {
       return;
   }
   
   roadmap_math_rotate_coordinates
       (RoadMapScreenLinePoints.cursor - RoadMapScreenLinePoints.data,
        RoadMapScreenLinePoints.data);

   roadmap_canvas_draw_multiple_polygons
      (count, RoadMapScreenObjects.data, RoadMapScreenLinePoints.data, 1);

   RoadMapScreenObjects.cursor = RoadMapScreenObjects.data;
   RoadMapScreenLinePoints.cursor  = RoadMapScreenLinePoints.data;
}


static void roadmap_screen_draw_polygons (void) {

   static RoadMapGuiPoint null_point = {0, 0};

   int i;
   int j;
   int size;
   int *geo_point;
   RoadMapPosition position;
   RoadMapGuiPoint *graphic_point;
   RoadMapGuiPoint *previous_point;

   RoadMapGuiPoint upper_left;
   RoadMapGuiPoint lower_right;

   RoadMapArea edges;
   RoadMapPen pen = NULL;


   RoadMapScreenLastPen = NULL;

   if (! roadmap_is_visible (ROADMAP_SHOW_AREA)) return;


   for (i = roadmap_polygon_count() - 1; i >= 0; --i) {

      pen = roadmap_layer_get_pen (roadmap_polygon_category (i), 0);

      if (pen == NULL) continue;

      if (RoadMapScreenLastPen != pen) {
         roadmap_screen_flush_polygons ();
         roadmap_canvas_select_pen (pen);
         RoadMapScreenLastPen = pen;
      }

      roadmap_polygon_edges (i, &edges);

      if (! roadmap_math_is_visible (&edges)) continue;

      /* Declutter logic: do not show the polygon when it has been
       * reduced (by the zoom) to a quasi-point.
       */
      position.longitude = edges.west;
      position.latitude  = edges.north;
      roadmap_math_coordinate (&position, &upper_left);

      position.longitude = edges.east;
      position.latitude  = edges.south;
      roadmap_math_coordinate (&position, &lower_right);

      if (abs(upper_left.x - lower_right.x) < 5 &&
          abs(upper_left.y - lower_right.y) < 5) {
         continue;
      }

      size = roadmap_polygon_points
                (i,
                 RoadMapPolygonGeoPoints,
                 RoadMapScreenLinePoints.end
                    - RoadMapScreenLinePoints.cursor - 1);

      if (size <= 0) {

         roadmap_screen_flush_polygons ();

         size = roadmap_polygon_points
                   (i,
                    RoadMapPolygonGeoPoints,
                    RoadMapScreenLinePoints.end
                       - RoadMapScreenLinePoints.cursor - 1);
      }
      geo_point = RoadMapPolygonGeoPoints;
      graphic_point = RoadMapScreenLinePoints.cursor;
      previous_point = &null_point;

      for (j = size; j > 0; --j) {

         roadmap_point_position  (*geo_point, &position);
         roadmap_math_coordinate (&position, graphic_point);

         if ((graphic_point->x != previous_point->x) ||
             (graphic_point->y != previous_point->y)) {

            previous_point = graphic_point;
            graphic_point += 1;
         }
         geo_point += 1;
      }

      /* Do not show polygons that have been reduced to a single
       * graphical point because of the zoom factor (natural declutter).
       */
      if (graphic_point != RoadMapScreenLinePoints.cursor) {

         *(graphic_point++) = *RoadMapScreenLinePoints.cursor;

         *(RoadMapScreenObjects.cursor++) =
             graphic_point - RoadMapScreenLinePoints.cursor;

         RoadMapScreenLinePoints.cursor = graphic_point;
      }
   }

   roadmap_screen_flush_polygons ();
}


static void roadmap_screen_draw_square_edges (int square) {

   int count;
   RoadMapArea edges;
   RoadMapPosition topleft;
   RoadMapPosition bottomright;
   RoadMapPosition position;

   RoadMapGuiPoint points[6];


   if (! roadmap_is_visible (ROADMAP_SHOW_SQUARE)) return;

   roadmap_square_edges (square, &edges);
   
   topleft.longitude     = edges.west;
   topleft.latitude      = edges.north;
   bottomright.longitude = edges.east;
   bottomright.latitude  = edges.south;

   roadmap_math_coordinate (&topleft, points);
   points[4] = points[0];

   position.longitude = bottomright.longitude;
   position.latitude  = topleft.latitude;
   roadmap_math_coordinate (&position, points+1);
   points[5] = points[1];

   roadmap_math_coordinate (&bottomright, points+2);

   position.longitude = topleft.longitude;
   position.latitude  = bottomright.latitude;
   roadmap_math_coordinate (&position, points+3);

   roadmap_canvas_select_pen (RoadMapPenEdges);
   count = 6;
   roadmap_math_rotate_coordinates (count, points);
   roadmap_canvas_draw_multiple_lines (1, &count, points);
   RoadMapScreenLastPen = NULL;
}


static int roadmap_screen_draw_square
              (int square, int layer, int fully_visible, int pen_index) {

   int line;
   int first_line;
   int last_line;
   int first_shape_line;
   int last_shape_line;
   int first_shape;
   int last_shape;
   RoadMapPen layer_pen;
   int fips = roadmap_locator_active ();

   int drawn = 0;


   roadmap_log_push ("roadmap_screen_draw_square");

   layer_pen = roadmap_layer_get_pen (layer, pen_index);

   if (layer_pen == NULL) return 0;


   /* Draw each line that belongs to this square. */

   if (roadmap_line_in_square (square, layer, &first_line, &last_line) > 0) {

      if (roadmap_shape_in_square (square, &first_shape_line,
                                            &last_shape_line) <= 0) {
         first_shape_line = last_shape_line = -1;
         first_shape = last_shape = -1;
      }

      for (line = first_line; line <= last_line; ++line) {

         /* A plugin may override a line: it can change the pen or
          * decide not to draw the line.
          */
         if (!roadmap_plugin_override_line (line, layer, fips)) {

            RoadMapPosition from;
            RoadMapPosition to;
            RoadMapPen pen;

            if (first_shape_line >= 0) {
               if (roadmap_shape_of_line (line,
                                          first_shape_line, last_shape_line,
                                          &first_shape, &last_shape) == 0) {
                  first_shape = -1;
               }
            }

            /* Check if the plugin wants to override the pen. */
            if (! roadmap_plugin_override_pen
                     (line, layer, fips, pen_index, &pen)) {
               pen = layer_pen;
            }

            if (pen == NULL) continue;

            roadmap_line_from (line, &from);
            roadmap_line_to (line, &to);

            roadmap_screen_draw_line
               (&from, &to, fully_visible, &from, first_shape, last_shape,
                roadmap_shape_get_position, layer_pen);
            drawn += 1;
         }
      }
   }

   /* Draw each line that intersects with this square (but belongs
    * to another square--the crossing lines).
    */
   if (roadmap_line_in_square2 (square, layer, &first_line, &last_line) > 0) {

      int last_real_square = -1;
      int real_square  = 0;
      RoadMapArea edges = {0, 0, 0, 0};

      int real_line;
      int square_count = roadmap_square_count();
      char *on_canvas  = calloc (square_count, sizeof(char));

      for (line = first_line; line <= last_line; ++line) {

         RoadMapPosition position;


         real_line = roadmap_line_get_from_index2 (line);

         roadmap_line_from (real_line, &position);

         /* Optimization: search for the square only if the new line does not
          * belong to the same square as the line before.
          */
         if ((position.longitude <  edges.west) ||
             (position.longitude >= edges.east) ||
             (position.latitude <  edges.south) ||
             (position.latitude >= edges.north)) {
            real_square = roadmap_square_search (&position);
         }

         if (real_square != last_real_square) {

            if (real_square < 0 || real_square >= square_count) {
               roadmap_log (ROADMAP_ERROR,
                            "Invalid square index %d", real_square);
               continue;
            }

            roadmap_square_edges (real_square, &edges);

            last_real_square = real_square;

            if (on_canvas[real_square]) {
               /* Either it has already been drawn, or it will be soon. */
               continue;
            }

            if (roadmap_math_is_visible (&edges)) {

               on_canvas[real_square] = 1;
               continue;
            }

            if (roadmap_shape_in_square
                                 (real_square,
                                  &first_shape_line, &last_shape_line) <= 0) {

               first_shape_line = last_shape_line = -1;
               first_shape = last_shape = -1;
            }
         }

         if (on_canvas[real_square]) {
            /* Either it has already been drawn, or it will be soon. */
            continue;
         }

         if (! roadmap_plugin_override_line (real_line, layer, fips)) {

            RoadMapPosition from;
            RoadMapPosition to;
            RoadMapPen pen;

            if (first_shape_line >= 0) {
               roadmap_shape_of_line (real_line,
                                      first_shape_line, last_shape_line,
                                      &first_shape, &last_shape);
            } else {
               first_shape = last_shape = -1;
            }

            /* Check if a plugin wants to override the pen */
            if (! roadmap_plugin_override_pen
                      (real_line, layer, fips, pen_index, &pen)) {
               pen = layer_pen;
            }
            if (pen == NULL) continue;

            roadmap_line_from (real_line, &from);
            roadmap_line_to (real_line, &to);

            roadmap_screen_draw_line
               (&from, &to, fully_visible, &from, first_shape, last_shape,
                roadmap_shape_get_position, layer_pen);

            drawn += 1;
         }
      }

      free (on_canvas);
   }

   roadmap_log_pop ();
   return drawn;
}


static void roadmap_screen_draw_object
               (const char *name,
                const char *sprite,
                const RoadMapGpsPosition *gps_position) {

   RoadMapPosition position;
   RoadMapGuiPoint screen_point;


   if (sprite == NULL) return; /* Not a visible object. */

   position.latitude = gps_position->latitude;
   position.longitude = gps_position->longitude;

   if (roadmap_math_point_is_visible(&position)) {

      roadmap_math_coordinate (&position, &screen_point);
      roadmap_math_rotate_coordinates (1, &screen_point);
      roadmap_sprite_draw (sprite, &screen_point, gps_position->steering);
   }
}


static void roadmap_screen_reset_square_mask (void) {

   if (SquareOnScreen != NULL) {
      free(SquareOnScreen);
   }
   SquareOnScreenCount = roadmap_square_count();
   SquareOnScreen = calloc (SquareOnScreenCount, sizeof(char));
   roadmap_check_allocated(SquareOnScreen);
}


static int roadmap_screen_repaint_square (int square, unsigned int pen_index) {

   int i;
   int count;
   int layers[1024];

   RoadMapArea edges;

   int drawn = 0;
   int layer;
   int fully_visible;


   if (SquareOnScreen[square]) {
      return 0;
   }
   SquareOnScreen[square] = 1;

   roadmap_log_push ("roadmap_screen_repaint_square");

   roadmap_square_edges (square, &edges);

   switch (roadmap_math_is_visible (&edges)) {
   case 0:
      roadmap_log_pop ();
      return 0;
   case 1:
      fully_visible = 1;
      break;
   default:
      fully_visible = 0;
   }

   if (pen_index == 0) roadmap_screen_draw_square_edges (square);

   count = roadmap_layer_visible_lines (layers, 1024, pen_index);
   
   RoadMapScreenLastPen = NULL;

   for (i = 0; i < count; ++i) {

        layer = layers[i];

        drawn += roadmap_screen_draw_square
                    (square, layer, fully_visible, pen_index);

        roadmap_screen_flush_lines();
        roadmap_screen_flush_points();
   }

   roadmap_log_pop ();
   return drawn;
}


static void roadmap_screen_repaint (void) {

    static RoadMapGuiPoint CompassPoint = {20, 20};
    static int *fips = NULL;
    static int *in_view = NULL;

    int i;
    int j;
    int count;
    unsigned int max_pen = roadmap_layer_max_pen();
    

    if (!RoadMapScreenDragging && RoadMapScreenFrozen) return;

    if (RoadMapScreenDragging &&
        (! roadmap_config_match(&RoadMapConfigStylePrettyDrag, "yes"))) {
       max_pen = 1;
    }

    if (in_view == NULL) {
       in_view = calloc (ROADMAP_MAX_VISIBLE, sizeof(int));
       roadmap_check_allocated(in_view);
    }

    roadmap_log_push ("roadmap_screen_repaint");

    /* Clean the drawing buffer. */

    roadmap_canvas_select_pen (RoadMapBackground);
    roadmap_canvas_erase ();


    /* Repaint the drawing buffer. */
    
    /* - Identifies the candidate counties. */

    count = roadmap_locator_by_position (&RoadMapScreenCenter, &fips);

    if (count == 0) {
       roadmap_display_text("Info", "No map available");
    }

    /* - For each candidate county: */

    for (i = count-1; i >= 0; --i) {

        /* -- Access the county's database. */

        if (roadmap_locator_activate (fips[i]) != ROADMAP_US_OK) continue;

        /* -- Look for the square that are currently visible. */

        count = roadmap_square_view (in_view, ROADMAP_MAX_VISIBLE);

        if (count > 0) {

            unsigned int pen_index;

            roadmap_screen_draw_polygons ();

            for (pen_index = 0; pen_index < max_pen; ++pen_index) {
               roadmap_screen_reset_square_mask();
               for (j = count - 1; j >= 0; --j) {
                  roadmap_screen_repaint_square (in_view[j], pen_index);
               }
            }
        }

        roadmap_plugin_screen_repaint (max_pen);
        roadmap_screen_flush_lines ();
        roadmap_screen_flush_points ();
    }

    if (!RoadMapScreenDragging ||
        roadmap_config_match(&RoadMapConfigStyleObjects, "yes")) {

       roadmap_object_iterate (roadmap_screen_draw_object);

       roadmap_trip_format_messages ();
    
       if (roadmap_config_match (&RoadMapConfigMapSigns, "yes")) {

          roadmap_sprite_draw ("Compass", &CompassPoint, 0);

          roadmap_trip_display ();
          roadmap_display_signs ();
       }
    }

    RoadMapScreenAfterRefresh();

    roadmap_canvas_refresh ();

    roadmap_log_pop ();
}


static void roadmap_screen_configure (void) {

   RoadMapScreenWidth = roadmap_canvas_width();
   RoadMapScreenHeight = roadmap_canvas_height();

   roadmap_math_set_size (RoadMapScreenWidth, RoadMapScreenHeight);
   if (RoadMapScreenInitialized) {
      roadmap_screen_repaint ();
   }
}



static void roadmap_screen_short_click (RoadMapGuiPoint *point) {
    
   PluginLine line;
   int distance;
   RoadMapPosition position;

    
   roadmap_math_to_position (point, &position);
   
   if (roadmap_navigate_retrieve_line
               (&position,
                roadmap_config_get_integer (&RoadMapConfigAccuracyMouse),
                &line,
                &distance) != -1) {

       PluginStreet street;

       roadmap_trip_set_point ("Selection", &position);
       roadmap_display_activate ("Selected Street", &line, &position, &street);
       roadmap_screen_repaint ();
   }
}


static void roadmap_screen_right_click (RoadMapGuiPoint *point) {

   roadmap_factory_popup (&RoadMapConfigEventRightClick, point);
}


static void roadmap_screen_reset_delta (void) {

   RoadMapScreenDeltaX = 0;
   RoadMapScreenDeltaY = 0;
   RoadMapScreenRotation = 0;

   roadmap_config_set_integer (&RoadMapConfigDeltaX, 0);
   roadmap_config_set_integer (&RoadMapConfigDeltaY, 0);
   roadmap_config_set_integer (&RoadMapConfigDeltaRotate, 0);
}


static void roadmap_screen_record_move (int dx, int dy) {

   RoadMapGuiPoint center;

   RoadMapScreenDeltaX += dx;
   RoadMapScreenDeltaY += dy;

   roadmap_config_set_integer (&RoadMapConfigDeltaX, RoadMapScreenDeltaX);
   roadmap_config_set_integer (&RoadMapConfigDeltaY, RoadMapScreenDeltaY);

   center.x = (RoadMapScreenWidth / 2) + dx;
   center.y = (RoadMapScreenHeight / 2) + dy;

   roadmap_math_to_position (&center, &RoadMapScreenCenter);
   roadmap_math_set_center (&RoadMapScreenCenter);
}


static void roadmap_screen_drag_start (RoadMapGuiPoint *point) {

   RoadMapScreenDragging = 1;
   RoadMapScreenPointerLocation = *point;
   roadmap_screen_hold (); /* We don't want to move with the GPS position. */
}

static void roadmap_screen_drag_end (RoadMapGuiPoint *point) {

   roadmap_screen_record_move
      (RoadMapScreenPointerLocation.x - point->x,
       RoadMapScreenPointerLocation.y - point->y);

   RoadMapScreenDragging = 0;
   RoadMapScreenPointerLocation = *point;
   roadmap_screen_repaint ();
}

static void roadmap_screen_drag_motion (RoadMapGuiPoint *point) {

   roadmap_screen_record_move
      (RoadMapScreenPointerLocation.x - point->x,
       RoadMapScreenPointerLocation.y - point->y);
   roadmap_screen_repaint ();
   RoadMapScreenPointerLocation = *point;
}


static void roadmap_screen_scroll_up (RoadMapGuiPoint *point) {
   roadmap_screen_zoom_in ();
}


static void roadmap_screen_scroll_down (RoadMapGuiPoint *point) {
   roadmap_screen_zoom_out ();
}


void roadmap_screen_refresh (void) {

    int refresh = 0;

    roadmap_log_push ("roadmap_screen_refresh");

    if (roadmap_trip_is_focus_changed()) {

        roadmap_screen_reset_delta ();
        RoadMapScreenCenter = *roadmap_trip_get_focus_position ();
        roadmap_math_set_center (&RoadMapScreenCenter);
        refresh = 1;
        
        roadmap_math_set_orientation (roadmap_trip_get_orientation());

    } else if (roadmap_trip_is_focus_moved()) {

        RoadMapScreenCenter = *roadmap_trip_get_focus_position ();
        roadmap_math_set_center (&RoadMapScreenCenter);

        refresh |=
           roadmap_math_set_orientation
              (roadmap_trip_get_orientation() + RoadMapScreenRotation);

        if (RoadMapScreenDeltaX || RoadMapScreenDeltaY) {

           /* Force recomputation. */

           int dx = RoadMapScreenDeltaX;
           int dy = RoadMapScreenDeltaY;

           RoadMapScreenDeltaX = RoadMapScreenDeltaY = 0;
           roadmap_screen_record_move (dx, dy);
        }
    }

    if (roadmap_config_match (&RoadMapConfigMapRefresh, "forced")) {

        roadmap_screen_repaint ();

    } else if (refresh || roadmap_trip_is_refresh_needed()) {

       roadmap_screen_repaint ();
    }

    roadmap_log_pop ();
}


void roadmap_screen_redraw (void) {

    roadmap_screen_repaint ();
}


void roadmap_screen_freeze (void) {

   RoadMapScreenFrozen = 1;
}

void roadmap_screen_unfreeze (void) {

   RoadMapScreenFrozen = 0;
   roadmap_screen_repaint ();
}


void roadmap_screen_hold (void) {

   roadmap_trip_copy_focus ("Hold");
   roadmap_trip_set_focus ("Hold");
}


void roadmap_screen_rotate (int delta) {

   int rotation = RoadMapScreenRotation + delta;

   while (rotation >= 360) {
      rotation -= 360;
   }
   while (rotation < 0) {
      rotation += 360;
   }

   if (roadmap_math_set_orientation
           (roadmap_trip_get_orientation() + rotation)) {
      RoadMapScreenRotation = rotation;
      roadmap_config_set_integer (&RoadMapConfigDeltaRotate, rotation);
      roadmap_screen_repaint ();
   }
}


void roadmap_screen_move_up (void) {

   roadmap_screen_record_move (0, 0 - (RoadMapScreenHeight / 4));
   roadmap_screen_repaint ();
}


void roadmap_screen_move_down (void) {

   roadmap_screen_record_move (0, RoadMapScreenHeight / 4);
   roadmap_screen_repaint ();
}


void roadmap_screen_move_right (void) {

   roadmap_screen_record_move (RoadMapScreenHeight / 4, 0);
   roadmap_screen_repaint ();
}


void roadmap_screen_move_left (void) {

   roadmap_screen_record_move (0 - (RoadMapScreenHeight / 4), 0);
   roadmap_screen_repaint ();
}


void roadmap_screen_zoom_in  (void) {

    roadmap_math_zoom_in ();

    roadmap_layer_adjust ();
    roadmap_screen_repaint ();
}


void roadmap_screen_zoom_out (void) {

    roadmap_math_zoom_out ();

    roadmap_layer_adjust ();
    roadmap_screen_repaint ();
}


void roadmap_screen_zoom_reset (void) {

   roadmap_math_zoom_reset ();

   roadmap_layer_adjust ();
   roadmap_screen_repaint ();
}


void roadmap_screen_initialize (void) {

   roadmap_config_declare ("session", &RoadMapConfigDeltaX, "0");
   roadmap_config_declare ("session", &RoadMapConfigDeltaY, "0");
   roadmap_config_declare ("session", &RoadMapConfigDeltaRotate, "0");

   roadmap_config_declare
       ("preferences", &RoadMapConfigAccuracyMouse,  "20");

   roadmap_config_declare
       ("preferences", &RoadMapConfigMapBackground, "LightYellow");

   roadmap_config_declare
       ("preferences", &RoadMapConfigEventRightClick,  "Map Menu");

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigMapSigns, "yes", "no", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigMapRefresh, "normal", "forced", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStylePrettyDrag, "yes", "no", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStyleObjects, "yes", "no", NULL);

   roadmap_pointer_register_short_click (&roadmap_screen_short_click);
   roadmap_pointer_register_drag_start (&roadmap_screen_drag_start);
   roadmap_pointer_register_drag_end (&roadmap_screen_drag_end);
   roadmap_pointer_register_drag_motion (&roadmap_screen_drag_motion);

   roadmap_pointer_register_right_click (&roadmap_screen_right_click);
   roadmap_pointer_register_scroll_up (&roadmap_screen_scroll_up);
   roadmap_pointer_register_scroll_down (&roadmap_screen_scroll_down);

   roadmap_canvas_register_configure_handler (&roadmap_screen_configure);

   RoadMapScreenObjects.cursor = RoadMapScreenObjects.data;
   RoadMapScreenObjects.end = RoadMapScreenObjects.data + ROADMAP_SCREEN_BULK;

   RoadMapScreenLinePoints.cursor = RoadMapScreenLinePoints.data;
   RoadMapScreenLinePoints.end = RoadMapScreenLinePoints.data + ROADMAP_SCREEN_BULK;

   RoadMapScreenPoints.cursor = RoadMapScreenPoints.data;
   RoadMapScreenPoints.end = RoadMapScreenPoints.data + ROADMAP_SCREEN_BULK;
}


void roadmap_screen_set_initial_position (void) {

    RoadMapScreenInitialized = 1;
    
    RoadMapScreenDeltaX = roadmap_config_get_integer (&RoadMapConfigDeltaX);
    RoadMapScreenDeltaY = roadmap_config_get_integer (&RoadMapConfigDeltaY);
    RoadMapScreenRotation =
       roadmap_config_get_integer (&RoadMapConfigDeltaRotate);

    RoadMapBackground = roadmap_canvas_create_pen ("Map.Background");
    roadmap_canvas_set_foreground
        (roadmap_config_get (&RoadMapConfigMapBackground));

    RoadMapPenEdges = roadmap_canvas_create_pen ("Map.Edges");
    roadmap_canvas_set_thickness (4);
    roadmap_canvas_set_foreground ("grey");

    roadmap_layer_adjust ();
}


void roadmap_screen_get_center (RoadMapPosition *center) {

   if (center != NULL) {
      *center = RoadMapScreenCenter;
   }
}


void roadmap_screen_subscribe_after_refresh (RoadMapScreenSubscriber handler) {

   if (handler == NULL) {
      RoadMapScreenAfterRefresh = roadmap_screen_after_refresh;
   } else {
      RoadMapScreenAfterRefresh = handler;
   }
}

