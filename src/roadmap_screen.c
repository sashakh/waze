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
#include "roadmap_main.h"
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
#include "roadmap_county.h"

#include "roadmap_sprite.h"
#include "roadmap_screen_obj.h"
#include "roadmap_object.h"
#include "roadmap_gpx.h"
#include "roadmap_trip.h"
#include "roadmap_track.h"
#include "roadmap_landmark.h"
#include "roadmap_features.h"
#include "roadmap_canvas.h"
#include "roadmap_state.h"
#include "roadmap_pointer.h"
#include "roadmap_display.h"
#include "roadmap_label.h"
#include "roadmap_linefont.h"
#include "roadmap_plugin.h"
#include "roadmap_factory.h"
#include "roadmap_time.h"
#include "roadmap_spawn.h"

#include "roadmap_screen.h"


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

static RoadMapConfigDescriptor RoadMapConfigEventLongClick =
                        ROADMAP_CONFIG_ITEM("Events", "Long Click");

static RoadMapConfigDescriptor RoadMapConfigScrollZoomEnable =
                        ROADMAP_CONFIG_ITEM("Events", "ScrollZoom");

static RoadMapConfigDescriptor RoadMapConfigStylePrettyDrag =
                  ROADMAP_CONFIG_ITEM("Style", "Pretty Lines when Dragging");

static RoadMapConfigDescriptor RoadMapConfigStyleObjects =
                  ROADMAP_CONFIG_ITEM("Style", "Show Objects when Dragging");

static RoadMapConfigDescriptor RoadMapConfigMapLabels =
                        ROADMAP_CONFIG_ITEM("Map", "Labels");

static RoadMapConfigDescriptor RoadMapConfigMapDynamicOrientation =
                        ROADMAP_CONFIG_ITEM("Map", "DynamicOrientation");

static RoadMapConfigDescriptor RoadMapConfigLinefontSelector =
                        ROADMAP_CONFIG_ITEM("Map", "Use Linefont");

static int RoadMapScreenInitialized = 0;
static int RoadMapScreenFrozen = 0;
static int RoadMapScreenDragging = 0;

static RoadMapGuiPoint RoadMapScreenPointerLocation;
static RoadMapPosition RoadMapScreenCenter;

static int RoadMapScreenViewMode = VIEW_MODE_2D;
static int RoadMapScreenOrientationDynamic = 1;
static int RoadMapScreen3dHorizon;
static int RoadMapScreenLabels;
static int RoadMapScreenRotation;
static int RoadMapScreenWidth;
static int RoadMapScreenHeight;

static int RoadMapScreenRepaintNeeded;

static int RoadMapLineFontSelect;

static int RoadMapScreenDeltaX;
static int RoadMapScreenDeltaY;

static char *SquareOnScreen;
static int   SquareOnScreenCount;

static RoadMapPen RoadMapScreenLastPen = NULL;



/* Define the buffers used to group all actual drawings. */

#define ROADMAP_SCREEN_BULK  4096

/* This is a default definition, because we might want to set this smaller
 * for some memory-starved targets.
 */
#ifndef ROADMAP_MAX_VISIBLE
#define ROADMAP_MAX_VISIBLE  30000
#endif

static struct {

   int *cursor;
   int *end;
   int data[ROADMAP_SCREEN_BULK];

} RoadMapScreenObjects;

struct roadmap_screen_point_buffer {

   RoadMapGuiPoint *cursor;
   RoadMapGuiPoint *end;
   RoadMapGuiPoint *data;
   int size;
};

static struct roadmap_screen_point_buffer RoadMapScreenLinePoints;
static struct roadmap_screen_point_buffer RoadMapScreenPoints;

static int RoadMapPolygonGeoPoints[ROADMAP_SCREEN_BULK];


static RoadMapPen RoadMapBackground = NULL;
static RoadMapPen RoadMapPenEdges = NULL;


static void roadmap_screen_setfont(void) {

   const char *p;

   RoadMapLineFontSelect = 0;
   p = roadmap_config_get(&RoadMapConfigLinefontSelector);
   if (p == NULL) {
      RoadMapLineFontSelect |= ROADMAP_TEXT_LABELS;
   } else {
      if (*p == 'l') { // "labels"
         RoadMapLineFontSelect |= ROADMAP_TEXT_LABELS;
      } else if (*p == 's') { // "signs"
         RoadMapLineFontSelect |= ROADMAP_TEXT_SIGNS;
      } else if (*p == 'a') { // "all"
         RoadMapLineFontSelect |= ROADMAP_TEXT_SIGNS|ROADMAP_TEXT_LABELS;
      }
   }
}


static void roadmap_screen_point_buffer_initialize
            (struct roadmap_screen_point_buffer *pb, int size) {

    if (pb->data) free(pb->data);

    if (size)
        pb->size = size;
    else
        pb->size *= 2;

    pb->data = calloc (pb->size, sizeof(*pb->data));
    roadmap_check_allocated(pb->data);

    pb->cursor = pb->data;
    pb->end = pb->data + pb->size;
}


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
       RoadMapScreenLinePoints.data, RoadMapScreenDragging);

   RoadMapScreenObjects.cursor = RoadMapScreenObjects.data;
   RoadMapScreenLinePoints.cursor  = RoadMapScreenLinePoints.data;
}


static void roadmap_screen_draw_line (const RoadMapPosition *from,
                               const RoadMapPosition *to,
                               int fully_visible,
                               const RoadMapPosition *shape_start,
                               int first_shape,
                               int last_shape,
                               RoadMapShapeIterator shape_next,
                               RoadMapPen pen,
                               int *total_length_ptr,
                               RoadMapGuiPoint *middle,
                               int *angle) {

   RoadMapGuiPoint point0;
   RoadMapGuiPoint point1;

   /* These are used when the line has a shape: */
   RoadMapPosition midposition;
   RoadMapPosition last_midposition;

   int i;
   RoadMapGuiPoint *points;

   int last_point_visible = 0;
   int longest = -1;

   dbg_time_start(DBG_TIME_DRAW_ONE_LINE);

   if (total_length_ptr) *total_length_ptr = 0;

   /* if the pen has changed, we need to flush the previous lines and points
    */

   if (RoadMapScreenLastPen != pen) {

      roadmap_screen_flush_lines ();
      roadmap_screen_flush_points ();
      roadmap_canvas_select_pen (pen);
      RoadMapScreenLastPen = pen;
   }

   if (first_shape >= 0) {
      /* Draw a shaped line. */

      if (last_shape - first_shape + 3 >=
            RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.cursor) {

         roadmap_screen_flush_lines ();
         while (last_shape - first_shape + 3 >=
               (RoadMapScreenLinePoints.end - RoadMapScreenLinePoints.data)) {

            roadmap_screen_point_buffer_initialize
                    (&RoadMapScreenLinePoints, 0);

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

            if ((point0.x == point1.x) && (point0.y == point1.y)) {

               if (last_point_visible) {

                  /* This segment is very short, we can skip it */
                  last_midposition = midposition; 

                  continue;
               }

            } else {

               if (total_length_ptr) {

                  int length_sq = roadmap_math_screen_distance
                                   (&point1, &point0, MATH_DIST_SQUARED);

                  /* bad math, but it's for a labelling heuristic anyway */
                  *total_length_ptr += length_sq;

                  if (length_sq > longest) {
                     longest = length_sq;
                     if (angle) {
                        *angle = roadmap_math_azymuth(&last_midposition, &midposition);
                     }
                     middle->x = (point1.x + point0.x) / 2;
                     middle->y = (point1.y + point0.y) / 2;
                  }
               }
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
         last_midposition = midposition; /* The latest position is our new start. */
      }

      if (roadmap_math_line_is_visible (&last_midposition, to) && 
             roadmap_math_get_visible_coordinates
                        (&last_midposition, to, &point0, &point1)) {

         if (total_length_ptr) {

            int length_sq = roadmap_math_screen_distance
                                (&point1, &point0, MATH_DIST_SQUARED);
            if (length_sq) {
               /* bad math, but it's for a labelling heuristic anyway */
               *total_length_ptr += length_sq;

               if (length_sq > longest) {
                  longest = length_sq;
                  if (angle) {
                     *angle = roadmap_math_azymuth(&last_midposition, to);
                  }
                  middle->x = (point1.x + point0.x) / 2;
                  middle->y = (point1.y + point0.y) / 2;
               }
            }
         }

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
         if (! roadmap_math_line_is_visible (from, to)) {
            dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
            return;
         }
      }

      /* Optimization: adjust the edges of the line so
       * they do not go out of the screen. */
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

         dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
         return;

      } else {
      
         if (total_length_ptr) {

            *total_length_ptr = roadmap_math_screen_distance
                                   (&point1, &point0, MATH_DIST_SQUARED);

            if (angle) {
               *angle = roadmap_math_azymuth(from, to);
            }
            middle->x = (point1.x + point0.x) / 2;
            middle->y = (point1.y + point0.y) / 2;
         }
      }
      
      if (RoadMapScreenLinePoints.cursor + 2 >= RoadMapScreenLinePoints.end) {
         roadmap_screen_flush_lines ();
      }

      RoadMapScreenLinePoints.cursor[0] = point0;
      RoadMapScreenLinePoints.cursor[1] = point1;

      *RoadMapScreenObjects.cursor = 2;

      RoadMapScreenLinePoints.cursor += 2;
      RoadMapScreenObjects.cursor += 1;

   }

   dbg_time_end(DBG_TIME_DRAW_ONE_LINE);
}



static int roadmap_screen_flush_polygons (void) {

   int count = RoadMapScreenObjects.cursor - RoadMapScreenObjects.data;
    
   if (count == 0) {
       return 0;
   }
   
   roadmap_math_rotate_coordinates
       (RoadMapScreenLinePoints.cursor - RoadMapScreenLinePoints.data,
        RoadMapScreenLinePoints.data);

   roadmap_canvas_draw_multiple_polygons
      (count, RoadMapScreenObjects.data, RoadMapScreenLinePoints.data, 1,
       RoadMapScreenDragging);

   RoadMapScreenObjects.cursor = RoadMapScreenObjects.data;
   RoadMapScreenLinePoints.cursor  = RoadMapScreenLinePoints.data;
   return 1;
}


static int roadmap_screen_draw_polygons (void) {

   static RoadMapGuiPoint null_point = {0, 0};

   int i;
   int j;
   int size;
   int category;
   int *geo_point;
   int drew = 0;
   RoadMapPosition position;
   RoadMapGuiPoint *graphic_point;
   RoadMapGuiPoint *previous_point;

   RoadMapGuiPoint upper_left;
   RoadMapGuiPoint lower_right;

   RoadMapArea edges;
   RoadMapPen pen = NULL;

   RoadMapScreenLastPen = NULL;

   if (! roadmap_is_visible (ROADMAP_SHOW_AREA)) return 0;


   for (i = roadmap_polygon_count() - 1; i >= 0; --i) {

      category = roadmap_polygon_category (i);
      pen = roadmap_layer_get_pen (category, 0);
      
      if (pen == NULL) continue;

      if (RoadMapScreenLastPen != pen) {
         drew |= roadmap_screen_flush_polygons ();
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

         drew |= roadmap_screen_flush_polygons ();

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

   return drew + roadmap_screen_flush_polygons ();
}


static void roadmap_screen_draw_square_edges (int square) {

   int count;
   RoadMapArea edges;
   RoadMapPosition topleft;
   RoadMapPosition bottomright;
   RoadMapPosition position;

   RoadMapGuiPoint points[6];

   if (! roadmap_is_visible (ROADMAP_SHOW_SQUARE)) return;

   roadmap_log_push ("roadmap_screen_draw_square_edges");

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
   roadmap_canvas_draw_multiple_lines (1, &count, points,
                                       RoadMapScreenDragging);
   RoadMapScreenLastPen = NULL;
   roadmap_log_pop ();
}

/* arrange to not do labels further than 3/4 up the screen */
static int roadmap_screen_label_cutoff(RoadMapGuiPoint *loweredge) {

   RoadMapGuiPoint label_cutoff[1];

   label_cutoff->y = roadmap_canvas_height() / 4;
   label_cutoff->x = roadmap_canvas_width() / 2;

   loweredge->x = roadmap_canvas_width() / 2;
   loweredge->y = roadmap_canvas_height();

   roadmap_math_unproject(label_cutoff);
   roadmap_math_unproject(loweredge);

   return roadmap_math_screen_distance
        (label_cutoff, loweredge, MATH_DIST_SQUARED);
}

static int roadmap_screen_draw_one_line(int line, 
    int layer, int pen_index, RoadMapPen layer_pen, int fips,
    int first_shape_line, int last_shape_line,
    int fully_visible) {

    int total_length;
    int *total_length_ptr = 0;
    int angle = 90;
    int *angle_ptr = 0;
    RoadMapGuiPoint loweredge;
    int cutoff_dist = 0;
    RoadMapPosition from;
    RoadMapPosition to;
    RoadMapPen pen = NULL;
    int first_shape = -1;
    int last_shape = -1;
    RoadMapGuiPoint seg_middle;

    if ((pen_index == 0) &&       /* we do labels only for the first pen */
          !RoadMapScreenDragging &&
          RoadMapScreenLabels) {
       total_length_ptr = &total_length;
       if (RoadMapScreen3dHorizon != 0) {
          cutoff_dist = roadmap_screen_label_cutoff(&loweredge);
       } else {
          angle_ptr = &angle;
       }
    }

    
    if (first_shape_line >= 0) {
       roadmap_shape_of_line (line,
                           first_shape_line, last_shape_line,
                          &first_shape, &last_shape);
    }

    /* Check if a plugin wants to override the pen. */
    if (! roadmap_plugin_override_pen
             (line, layer, pen_index, fips, &pen)) {
       pen = layer_pen;
    }

    if (pen == NULL) return 0;

    roadmap_line_from (line, &from);
    roadmap_line_to (line, &to);

    roadmap_screen_draw_line
       (&from, &to, fully_visible, &from, first_shape, last_shape,
        roadmap_shape_get_position, pen, total_length_ptr,
        &seg_middle, angle_ptr);
                 
    if (total_length_ptr && *total_length_ptr && (cutoff_dist == 0 ||
            cutoff_dist > roadmap_math_screen_distance
                    (&seg_middle, &loweredge, MATH_DIST_SQUARED)) ) {
       PluginLine l = {ROADMAP_PLUGIN_ID, line, layer, fips};
       roadmap_label_add
            (&seg_middle, angle_ptr ? *angle_ptr : 90, *total_length_ptr, &l);
    }

    return 1;
}

static int roadmap_screen_draw_square
              (int square, int layer, int fully_visible, int pen_index) {

   int line;
   int first_line;
   int last_line;
   int first_shape_line;
   int last_shape_line;
   RoadMapPen layer_pen;
   int fips;
   int drawn = 0;


   roadmap_log_push ("roadmap_screen_draw_square");

   layer_pen = roadmap_layer_get_pen (layer, pen_index);
   if (layer_pen == NULL) return 0;
   
   fips = roadmap_locator_active ();

   /* Draw each line that belongs to this square. */

   if (roadmap_line_in_square (square, layer, &first_line, &last_line) > 0) {

      if (roadmap_shape_in_square (square, &first_shape_line,
                                            &last_shape_line) <= 0) {
         first_shape_line = last_shape_line = -1;
      }

      for (line = first_line; line <= last_line; ++line) {

         /* A plugin may override a line: it can change the pen or
          * decide not to draw the line.
          */
         if (!roadmap_plugin_override_line (line, layer, fips)) {

            if (roadmap_screen_draw_one_line(line, layer,
                    pen_index, layer_pen, fips,
                    first_shape_line, last_shape_line,
                    fully_visible)) {
                drawn += 1;
            }
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

      int iline;
      int square_count = roadmap_square_count();
      char *on_canvas  = calloc ((square_count+7)/8, sizeof(char));

      for (iline = first_line; iline <= last_line; ++iline) {

         RoadMapPosition position;


         line = roadmap_line_get_from_index2 (iline);

         roadmap_line_from (line, &position);

         /* Optimization: search for the square only if the new line does not
          * belong to the same square as the line before.
          */
         if ((position.longitude <  edges.west) ||
             (position.longitude >= edges.east) ||
             (position.latitude <  edges.south) ||
             (position.latitude >= edges.north)) {

            real_square = roadmap_square_search (&position);
            if (real_square < 0) continue;

         }

         if (real_square != last_real_square) {

            roadmap_square_edges (real_square, &edges);

            last_real_square = real_square;

            if (on_canvas[real_square / 8] & (1 << (real_square % 8))) {
               /* Either it has already been drawn, or it will be soon. */
               continue;
            }

            if (roadmap_math_is_visible (&edges)) {

               on_canvas[real_square / 8] |= (1 << (real_square % 8));
               continue;
            }

            if (roadmap_shape_in_square
                                 (real_square,
                                  &first_shape_line, &last_shape_line) <= 0) {

               first_shape_line = last_shape_line = -1;
            }
         }

         if (on_canvas[real_square / 8] & (1 << (real_square % 8))) {
            /* Either it has already been drawn, or it will be soon. */
            continue;
         }

         if (!roadmap_plugin_override_line (line, layer, fips)) {

            if (roadmap_screen_draw_one_line(line, layer,
                    pen_index, layer_pen, fips,
                    first_shape_line, last_shape_line,
                    fully_visible)) {
                drawn += 1;
            }
         }
      }

      free (on_canvas);
   }

   roadmap_log_pop ();
   return drawn;
}

static int roadmap_screen_draw_long_lines (int pen_index) {

   int last_layer = -1;
   int index = 0;
   int line;
   int real_square = -2;
   int layer;
   int first_shape_line;
   int last_shape_line;
   RoadMapPen layer_pen = NULL;
   int fips = roadmap_locator_active ();
   RoadMapArea area;
   int last_real_square = -1;
   RoadMapArea edges = {0, 0, 0, 0};

   int drawn = 0;

   dbg_time_start(DBG_TIME_DRAW_LONG_LINES);

   roadmap_log_push ("roadmap_screen_draw_long_lines");

   while (roadmap_line_long (index++, &line, &area, &layer)) {
         RoadMapPosition position;
         RoadMapPosition to_position;

         if (!roadmap_math_is_visible (&area)) {
            continue;
         }

         if (last_layer != layer) {
            layer_pen = roadmap_layer_get_pen (layer, pen_index);
            last_layer = layer;
         }

         if (layer_pen == NULL) continue;

         roadmap_line_to (line, &to_position);

         if (roadmap_math_point_is_visible (&to_position)) {
            continue;

         } else {
            int to_square;
            RoadMapArea to_edges;

            to_square = roadmap_square_search (&to_position);
            if (to_square < 0) continue;

            roadmap_square_edges (to_square, &to_edges);

            if (roadmap_math_is_visible (&to_edges)) {
               continue;
            }
         }

         roadmap_line_from (line, &position);

         /* Optimization: search for the square only if the new line does not
          * belong to the same square as the line before.
          */
         if ((position.longitude <  edges.west) ||
             (position.longitude >= edges.east) ||
             (position.latitude <  edges.south) ||
             (position.latitude >= edges.north)) {

            real_square = roadmap_square_search (&position);
            if (real_square < 0) continue;

         }

         if (real_square != last_real_square) {

            roadmap_square_edges (real_square, &edges);

            last_real_square = real_square;

            if (roadmap_math_is_visible (&edges)) {

               continue;
            }

            if (roadmap_shape_in_square
                                 (real_square,
                                  &first_shape_line, &last_shape_line) <= 0) {

               first_shape_line = last_shape_line = -1;
            }

         }


         if (roadmap_math_is_visible (&edges)) {
            continue;
         }

         if (!roadmap_plugin_override_line (line, layer, fips)) {

            if (roadmap_screen_draw_one_line(line, layer,
                    pen_index, layer_pen, fips,
                    first_shape_line, last_shape_line,
                    0)) {
                drawn += 1;
            }
         }
   }

   roadmap_log_pop ();
   dbg_time_end(DBG_TIME_DRAW_LONG_LINES);

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


static int roadmap_screen_repaint_square (int square, int pen_type, 
                                          int layer_count, int *layers) {

   int i;

   RoadMapArea edges;

   int drawn = 0;
   int category;
   int fully_visible;


   if (SquareOnScreen[square]) {
      return 0;
   }

   dbg_time_start(DBG_TIME_DRAW_SQUARE);

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

   /* draw global square outline (only with "--square") */
   if (pen_type == 0) roadmap_screen_draw_square_edges (square);
   
   RoadMapScreenLastPen = NULL;

   for (i = 0; i < layer_count; ++i) {

        category = layers[i];

        drawn += roadmap_screen_draw_square
                    (square, category, fully_visible, pen_type);

   }

   roadmap_screen_flush_lines();
   roadmap_screen_flush_points();

   roadmap_log_pop ();
   
   dbg_time_end(DBG_TIME_DRAW_SQUARE);
   return drawn;
}


static void roadmap_screen_repaint (void) {

    static int *fips = NULL;
    static int *in_view = NULL;

    int i;
    int j;
    int k;
    int count;
    int drawn;
    int max_pen = roadmap_layer_max_pen();
    static int nomap;
    

    if (!RoadMapScreenDragging && RoadMapScreenFrozen) return;

    dbg_time_start(DBG_TIME_FULL);
    if (RoadMapScreenDragging &&
        (! roadmap_config_match(&RoadMapConfigStylePrettyDrag, "yes"))) {
       max_pen = 1;
    }

    roadmap_screen_setfont();

    roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT_WITH_DELAY);

    if (in_view == NULL) {
       in_view = calloc (ROADMAP_MAX_VISIBLE, sizeof(int));
       roadmap_check_allocated(in_view);
    }

    roadmap_log_push ("roadmap_screen_repaint");

    /* Clean the drawing buffer. */

    roadmap_canvas_select_pen (RoadMapBackground);
    roadmap_canvas_erase ();
    RoadMapScreenLastPen = NULL;

    /* Repaint the drawing buffer. */
    
    /* - Identifies the candidate counties. */

    count = roadmap_locator_by_position (&RoadMapScreenCenter, &fips);

    if (count == 0) {
       RoadMapPosition pos;
       char lon[32], lat[32];
       roadmap_math_get_context (&pos, NULL, NULL);
       roadmap_display_text("Info", "No map available: %s, %s",
             roadmap_math_to_floatstring(lon, pos.longitude, MILLIONTHS),
             roadmap_math_to_floatstring(lat, pos.latitude, MILLIONTHS));
       nomap = 1;
    } else if (nomap) {
       roadmap_display_hide("Info");
       nomap = 0;
    }

    roadmap_label_start();

    /* - For each candidate county: */

    for (i = count-1; i >= 0; --i) {

        /* -- nothing to draw at this zoom? -- */
        if (roadmap_county_get_decluttered(fips[i]))
            continue;

        roadmap_main_busy_check();

        /* -- Access the county's database. */

        if (roadmap_locator_activate (fips[i]) != ROADMAP_US_OK) continue;

        drawn = 0;
        drawn += roadmap_screen_draw_polygons ();

        /* -- Look for the squares that are currently visible. */

        count = roadmap_square_view (in_view, ROADMAP_MAX_VISIBLE);

        for (k = 0; k < max_pen; ++k) {

            if (count > 0) {
               static int *layers = NULL;
               static int  layers_size = 0;
               int layer_count;
               int pen_type = k;

               roadmap_screen_reset_square_mask();

               if (layers == NULL) {
                  layers_size = roadmap_layer_max_defined();
                  layers = (int *)calloc (layers_size, sizeof(int));
                  roadmap_check_allocated(layers);
               }
               layer_count = roadmap_layer_visible_lines
                                (layers, layers_size, k);

               if (!layer_count) continue;

               for (j = count - 1; j >= 0; --j) {
                  drawn += roadmap_screen_repaint_square (in_view[j], pen_type,
                    layer_count, layers);
               }

               roadmap_main_busy_check();

            }

            drawn += roadmap_screen_draw_long_lines (k);

            if (k == 0) {
                /* draw global square outline (only with "--square") */
                roadmap_screen_draw_square_edges (ROADMAP_SQUARE_GLOBAL);
            }
        }


        roadmap_plugin_screen_repaint (max_pen);
        roadmap_screen_flush_lines ();
        roadmap_screen_flush_points ();

        roadmap_main_busy_check();
        
        if (drawn && !RoadMapScreenDragging) {
            roadmap_label_draw_cache (RoadMapScreen3dHorizon == 0);
        }

        if (!drawn)
            roadmap_county_set_decluttered(fips[i]);
    }

    if (!RoadMapScreenDragging ||
       roadmap_config_match(&RoadMapConfigStyleObjects, "yes")) {

       /* we could probably use a callback registration system
        * for this, but order is important.
        */
       roadmap_object_iterate (roadmap_screen_draw_object);
       roadmap_trip_format_messages ();
       roadmap_landmark_display ();
       roadmap_features_display ();
       roadmap_trip_display ();
       roadmap_track_display ();
       roadmap_screen_obj_draw ();

       if (roadmap_config_match (&RoadMapConfigMapSigns, "yes")) {

          roadmap_display_signs ();
       }

    }


    roadmap_canvas_refresh ();

    roadmap_main_set_cursor (ROADMAP_CURSOR_NORMAL);

    RoadMapScreenRepaintNeeded = 0;

    roadmap_log_pop ();
    dbg_time_end(DBG_TIME_FULL);
}


static void roadmap_screen_configure (void) {

   RoadMapScreenWidth = roadmap_canvas_width();
   RoadMapScreenHeight = roadmap_canvas_height();

   RoadMapScreenLabels = ! roadmap_config_match(&RoadMapConfigMapLabels, "off");
   
   RoadMapScreenOrientationDynamic = 
        roadmap_config_match(&RoadMapConfigMapDynamicOrientation, "on");

   roadmap_math_set_size (RoadMapScreenWidth, RoadMapScreenHeight);

   if (RoadMapScreenInitialized) {
      roadmap_screen_repaint ();
   }

}

static int roadmap_screen_short_click (RoadMapGuiPoint *point) {
    
   PluginLine line;
   int distance;
   int accuracy;
   RoadMapPosition position;
   RoadMapArea area;

   accuracy =  roadmap_config_get_integer (&RoadMapConfigAccuracyMouse);
    
   roadmap_math_to_position (point, &position, 1);
   roadmap_math_focus_area (&area, &position, accuracy);

   if (roadmap_trip_retrieve_area_points(&area, &position)) {
       ;
   } else if (roadmap_navigate_retrieve_line
               (&area, &position, &line, &distance) != -1) {

       PluginStreet street;

       roadmap_display_activate ("Selected Street", &line, &position, &street);
       RoadMapScreenRepaintNeeded = 1;
   }

   roadmap_trip_set_point ("Selection", &position);
   roadmap_screen_refresh ();

   return 1;
}


static int roadmap_screen_right_click (RoadMapGuiPoint *point) {

   roadmap_factory_popup
      (roadmap_config_get(&RoadMapConfigEventRightClick), point);
   return 1;
}

static int roadmap_screen_long_click (RoadMapGuiPoint *point) {

   roadmap_factory_popup
      (roadmap_config_get(&RoadMapConfigEventLongClick), point);
   return 1;
}


static void roadmap_screen_reset_delta (void) {

   RoadMapScreenDeltaX = 0;
   RoadMapScreenDeltaY = 0;
   RoadMapScreenRotation = 0;
}


static void roadmap_screen_record_move (int dx, int dy) {

   RoadMapGuiPoint center;

   RoadMapScreenDeltaX += dx;
   RoadMapScreenDeltaY += dy;

   center.x = (RoadMapScreenWidth / 2) + dx;
   center.y = (RoadMapScreenHeight / 2) + dy;

   roadmap_math_to_position (&center, &RoadMapScreenCenter, 0);
   roadmap_math_set_center (&RoadMapScreenCenter);
}


static int roadmap_screen_drag_start (RoadMapGuiPoint *point) {

   RoadMapScreenDragging = 1;
   RoadMapScreenPointerLocation = *point;
   roadmap_screen_hold (); /* We don't want to move with the GPS position. */

   return 1;
}

static int roadmap_screen_drag_end (RoadMapGuiPoint *point) {

   roadmap_screen_record_move
      (RoadMapScreenPointerLocation.x - point->x,
       RoadMapScreenPointerLocation.y - point->y);

   RoadMapScreenDragging = 0;
   RoadMapScreenPointerLocation = *point;
   roadmap_screen_hold ();
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();

   return 1;
}

static int roadmap_screen_drag_motion (RoadMapGuiPoint *point) {

   if (RoadMapScreenViewMode == VIEW_MODE_3D) {

      RoadMapGuiPoint p = *point;
      RoadMapGuiPoint p2 = RoadMapScreenPointerLocation;

      roadmap_math_unproject (&p);
      roadmap_math_unproject (&p2);

      roadmap_screen_record_move (p2.x - p.x, p2.y - p.y);
      
   } else {

      roadmap_screen_record_move
         (RoadMapScreenPointerLocation.x - point->x,
          RoadMapScreenPointerLocation.y - point->y);
   }

   roadmap_screen_repaint ();
   RoadMapScreenPointerLocation = *point;

   return 1;
}

static int roadmap_screen_get_view_mode (void) {
   
   return RoadMapScreenViewMode;
}

static int roadmap_screen_get_orientation_mode (void) {

   if (RoadMapScreenOrientationDynamic)
      return ORIENTATION_DYNAMIC;
   else
      return ORIENTATION_FIXED;
}

static int roadmap_screen_scroll_up (RoadMapGuiPoint *point) {
   if ( ! roadmap_config_match(&RoadMapConfigScrollZoomEnable, "off") ) {
      roadmap_screen_zoom_in ();
      return 1;
   }
   return 0;
}

static int roadmap_screen_scroll_down (RoadMapGuiPoint *point) {
   if ( ! roadmap_config_match(&RoadMapConfigScrollZoomEnable, "off") ) {
      roadmap_screen_zoom_out ();
      return 1;
   }
   return 0;
}


void roadmap_screen_refresh (void) {

    int refresh = 0;

    roadmap_log_push ("roadmap_screen_refresh");

    if (roadmap_trip_is_focus_changed()) {

        roadmap_screen_reset_delta ();
        RoadMapScreenCenter = *roadmap_trip_get_focus_position ();
        roadmap_math_set_center (&RoadMapScreenCenter);
        refresh = 1;
        
        if (RoadMapScreenOrientationDynamic) {
           roadmap_math_set_orientation (roadmap_trip_get_orientation());
        }

    } else if (roadmap_trip_is_focus_moved()) {

        RoadMapScreenCenter = *roadmap_trip_get_focus_position ();
        roadmap_math_set_center (&RoadMapScreenCenter);

        if (RoadMapScreenOrientationDynamic) {
           refresh |=
              roadmap_math_set_orientation
                 (roadmap_trip_get_orientation() + RoadMapScreenRotation);
        } else {
           refresh |=
              roadmap_math_set_orientation (RoadMapScreenRotation);
        }

        if (RoadMapScreenDeltaX || RoadMapScreenDeltaY) {

           /* Force recomputation. */

           int dx = RoadMapScreenDeltaX;
           int dy = RoadMapScreenDeltaY;

           RoadMapScreenDeltaX = RoadMapScreenDeltaY = 0;
           roadmap_screen_record_move (dx, dy);
        }
    }

    /* Be sure to fully evaluate all possible refresh causes
     * every time, since these routines maintain internal state
     * flags.  Otherwise some will be left un-"cleared", and will
     * trigger a second repaint next time around.  (We should
     * probably use a callback registration system for this.)
     */
    refresh |= roadmap_config_match (&RoadMapConfigMapRefresh, "forced");
    refresh |= roadmap_trip_is_refresh_needed();
    refresh |= roadmap_landmark_is_refresh_needed();
    refresh |= roadmap_track_is_refresh_needed();
    refresh |= roadmap_display_is_refresh_needed();

    if (RoadMapScreenRepaintNeeded || refresh) {
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

   if (!RoadMapScreenFrozen) return;

   RoadMapScreenFrozen = 0;
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();
}


void roadmap_screen_hold (void) {

   roadmap_trip_copy_focus ("Hold");
   roadmap_trip_set_focus ("Hold");
   roadmap_trip_set_focus_position (&RoadMapScreenCenter);
}


void roadmap_screen_rotate (int delta) {

   int rotation = RoadMapScreenRotation + delta;
   int calculated_rotation;

   while (rotation >= 360) {
      rotation -= 360;
   }
   while (rotation < 0) {
      rotation += 360;
   }

   if (RoadMapScreenOrientationDynamic) {
      calculated_rotation = roadmap_trip_get_orientation() + rotation;
   } else {
      calculated_rotation = rotation;
   }

   if (roadmap_math_set_orientation (calculated_rotation)) {
      RoadMapScreenRotation = rotation;
      RoadMapScreenRepaintNeeded = 1;
      roadmap_screen_refresh ();
   }
}


void roadmap_screen_toggle_view_mode (void) {
   
   if (RoadMapScreenViewMode == VIEW_MODE_2D) {
      RoadMapScreenViewMode = VIEW_MODE_3D;
      RoadMapScreen3dHorizon = -100;
   } else {
      RoadMapScreenViewMode = VIEW_MODE_2D;
      RoadMapScreen3dHorizon = 0;
   }

   roadmap_math_set_horizon (RoadMapScreen3dHorizon);
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();
}

void roadmap_screen_toggle_labels (void) {
   
   RoadMapScreenLabels = ! RoadMapScreenLabels;
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh();
}

void roadmap_screen_toggle_orientation_mode (void) {
   
   RoadMapScreenOrientationDynamic = ! RoadMapScreenOrientationDynamic;

   RoadMapScreenRotation = 0;
   roadmap_screen_rotate (0);
   roadmap_screen_refresh ();
}


void roadmap_screen_increase_horizon (void) {

   if (RoadMapScreenViewMode != VIEW_MODE_3D) return;

   if (RoadMapScreen3dHorizon >= -100) return;

   RoadMapScreen3dHorizon += 100;
   roadmap_math_set_horizon (RoadMapScreen3dHorizon);
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();
}


void roadmap_screen_decrease_horizon (void) {

   if (RoadMapScreenViewMode != VIEW_MODE_3D) return;

   RoadMapScreen3dHorizon -= 100;
   roadmap_math_set_horizon (RoadMapScreen3dHorizon);
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();
}

#define FRACMOVE 4
// #define FRACMOVE 100  // tiny moves:  useful for debug

void roadmap_screen_move_up (void) {

   roadmap_screen_record_move (0, 0 - (RoadMapScreenHeight / FRACMOVE));
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();
}


void roadmap_screen_move_down (void) {

   roadmap_screen_record_move (0, RoadMapScreenHeight / FRACMOVE);
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();
}


void roadmap_screen_move_right (void) {

   roadmap_screen_record_move (RoadMapScreenHeight / FRACMOVE, 0);
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();
}


void roadmap_screen_move_left (void) {

   roadmap_screen_record_move (0 - (RoadMapScreenHeight / FRACMOVE), 0);
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();
}


void roadmap_screen_zoom_in  (void) {

    roadmap_math_zoom_in ();

    roadmap_layer_adjust ();
    RoadMapScreenRepaintNeeded = 1;
    roadmap_screen_refresh ();
}


void roadmap_screen_zoom_out (void) {

    roadmap_math_zoom_out ();

    roadmap_layer_adjust ();
    RoadMapScreenRepaintNeeded = 1;
    roadmap_screen_refresh ();
}


void roadmap_screen_zoom_reset (void) {

   roadmap_math_zoom_reset ();

   roadmap_layer_adjust ();
   RoadMapScreenRepaintNeeded = 1;
   roadmap_screen_refresh ();
}


void roadmap_screen_text
     (int id, RoadMapGuiPoint *center, int where, int size, const char *text) {
    if (ROADMAP_USE_LINEFONT && (RoadMapLineFontSelect & id) != 0) {
        roadmap_linefont_text ( center, where, size, text);
    } else {
        roadmap_canvas_draw_string ( center, where, text);
    }
}
void roadmap_screen_text_angle 
        (int id, RoadMapGuiPoint *start, RoadMapGuiPoint *center,
                int theta, int size, const char *text) {
    if (ROADMAP_USE_LINEFONT && (RoadMapLineFontSelect & id) != 0) {
        roadmap_linefont_text_angle ( start, center, theta, size, text);
    } else {
        roadmap_canvas_draw_string_angle ( start, center, theta, text);
    }
}
void roadmap_screen_text_extents 
        (int id, const char *text, int size,
         int *width, int *ascent, int *descent, int *can_tilt) {
    if (ROADMAP_USE_LINEFONT && (RoadMapLineFontSelect & id) != 0) {
        roadmap_linefont_extents
                (text, size, width, ascent, descent, can_tilt);
    } else {
        roadmap_canvas_get_text_extents
                (text, -1, width, ascent, descent, can_tilt);
    }
}


void roadmap_screen_initialize (void) {

   roadmap_config_declare
       ("preferences", &RoadMapConfigAccuracyMouse,  "20");

   roadmap_config_declare
       ("preferences", &RoadMapConfigMapBackground, "LightYellow");

   roadmap_config_declare
       ("preferences", &RoadMapConfigEventRightClick,  "Right Click Popup");

   roadmap_config_declare
       ("preferences", &RoadMapConfigEventLongClick,  "Long Click Popup");

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigScrollZoomEnable, "on", "off", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigMapSigns, "yes", "no", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigMapRefresh, "normal", "forced", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStylePrettyDrag, "yes", "no", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStyleObjects, "yes", "no", NULL);

   roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigMapLabels, "on", "off", NULL);

   roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigMapDynamicOrientation, "on", "off", NULL);

   roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigLinefontSelector, "labels", "signs", "all", "off", NULL);



   roadmap_pointer_register_short_click (&roadmap_screen_short_click, POINTER_DEFAULT);
   roadmap_pointer_register_drag_start (&roadmap_screen_drag_start, POINTER_DEFAULT);
   roadmap_pointer_register_drag_end (&roadmap_screen_drag_end, POINTER_DEFAULT);
   roadmap_pointer_register_drag_motion (&roadmap_screen_drag_motion, POINTER_DEFAULT);

   roadmap_pointer_register_right_click (&roadmap_screen_right_click, POINTER_DEFAULT);
   roadmap_pointer_register_long_click (&roadmap_screen_long_click, POINTER_DEFAULT);
   roadmap_pointer_register_scroll_up (&roadmap_screen_scroll_up, POINTER_DEFAULT);
   roadmap_pointer_register_scroll_down (&roadmap_screen_scroll_down, POINTER_DEFAULT);

   roadmap_canvas_register_configure_handler (&roadmap_screen_configure);

   RoadMapScreenObjects.cursor = RoadMapScreenObjects.data;
   RoadMapScreenObjects.end = RoadMapScreenObjects.data + ROADMAP_SCREEN_BULK;

   roadmap_screen_point_buffer_initialize
        (&RoadMapScreenLinePoints, ROADMAP_SCREEN_BULK);

   roadmap_screen_point_buffer_initialize
        (&RoadMapScreenPoints, ROADMAP_SCREEN_BULK);
   
   RoadMapScreen3dHorizon = 0;

   roadmap_state_add ("view_mode", &roadmap_screen_get_view_mode);
   roadmap_state_add ("get_orientation",
            &roadmap_screen_get_orientation_mode);
   roadmap_state_monitor (&roadmap_screen_repaint);
}


void roadmap_screen_set_initial_position (void) {

    RoadMapScreenInitialized = 1;
    
    roadmap_layer_initialize();

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


#ifdef ROADMAP_DBG_TIME

static unsigned long dbg_time_rec[DBG_TIME_LAST_COUNTER];
static unsigned long dbg_time_tmp[DBG_TIME_LAST_COUNTER];

void dbg_time_start(int type) {
   dbg_time_tmp[type] = roadmap_time_get_millis();
}

void dbg_time_end(int type) {
   dbg_time_rec[type] += roadmap_time_get_millis() - dbg_time_tmp[type];
}

#endif // ROADMAP_DBG_TIME
