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
#include "roadmap_trip.h"
#include "roadmap_canvas.h"
#include "roadmap_display.h"

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


static int RoadMapScreenInitialized = 0;
static int RoadMapScreenFrozen = 0;

static RoadMapPosition RoadMapScreenCenter;

static int RoadMapScreenRotation;
static int RoadMapScreenWidth;
static int RoadMapScreenHeight;

static int RoadMapScreenDeltaX;
static int RoadMapScreenDeltaY;

static char *SquareOnScreen;
static int   SquareOnScreenCount;


static void roadmap_screen_after_refresh (void) {}

static RoadMapScreenSubscriber RoadMapScreenAfterRefresh =
                                       roadmap_screen_after_refresh;


/* Define the buffers used to group all actual drawings. */

#define ROADMAP_SCREEN_BULK  4096

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
static RoadMapPen RoadMapEdges = NULL;


static void roadmap_screen_flush_points (void) {

   roadmap_math_rotate_coordinates
       (RoadMapScreenPoints.cursor - RoadMapScreenPoints.data,
        RoadMapScreenPoints.data);

   roadmap_canvas_draw_multiple_points
       (RoadMapScreenPoints.cursor - RoadMapScreenPoints.data,
        RoadMapScreenPoints.data);

   RoadMapScreenPoints.cursor  = RoadMapScreenPoints.data;
}


static void roadmap_screen_flush_lines (void) {

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


void roadmap_screen_draw_one_line (int line,
                                   int fully_visible,
                                   int first_shape_line,
                                   int last_shape_line) {

   int line_style;
#define ROADMAP_LINE_STYLE_POINT   0
#define ROADMAP_LINE_STYLE_SEGMENT 1
#define ROADMAP_LINE_STYLE_SHAPE   2

   int first_shape;
   int last_shape;

   RoadMapPosition position0;
   RoadMapPosition position1;

   RoadMapGuiPoint point0;
   RoadMapGuiPoint point1;

   int i;
   RoadMapGuiPoint *points;


   /* Retrieve the position and "size" of the line and decide
    * what details to draw.
    */
   roadmap_line_from (line, &position0);
   roadmap_line_to   (line, &position1);

   roadmap_math_coordinate (&position0, &point0);
   roadmap_math_coordinate (&position1, &point1);

   if (abs(point0.x - point1.x) < 5 && abs(point0.y - point1.y) < 5) {

      /* Decluttering logic: show less details if the line is too small
       * for these details to matter.
       */
      if ((point0.x == point1.x) && (point0.y == point1.y)) {

         line_style = ROADMAP_LINE_STYLE_POINT;

      } else {

         /* This line is too small for us to consider a shape. */
         line_style = ROADMAP_LINE_STYLE_SEGMENT;
      }

   } else if (first_shape_line < 0) {

      line_style = ROADMAP_LINE_STYLE_SEGMENT;

   } else {
      
      if (roadmap_shape_of_line (line,
                                 first_shape_line, last_shape_line,
                                 &first_shape, &last_shape) > 0) {

         line_style = ROADMAP_LINE_STYLE_SHAPE;

      } else {

         line_style = ROADMAP_LINE_STYLE_SEGMENT;
      }
   }


   /* Now that we know how much to draw, draw it. */

   switch (line_style) {

   case ROADMAP_LINE_STYLE_SHAPE: /* Draw a shaped line. */

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

      *(points++) = point0;

      for (i = first_shape; i <= last_shape; ++i) {

         roadmap_shape_get_position (i, &position0);

         roadmap_math_coordinate (&position0, points);
         points += 1;
      }

      *(points++) = point1;

      *RoadMapScreenObjects.cursor = points - RoadMapScreenLinePoints.cursor;

      RoadMapScreenLinePoints.cursor   = points;
      RoadMapScreenObjects.cursor += 1;

      break;

   case ROADMAP_LINE_STYLE_SEGMENT: /* Draw a line with no shape. */

      /* Optimization: do not draw a line that is obviously not visible. */
      if (! fully_visible)
      {
         if (! roadmap_math_line_is_visible (&position0, &position1)) return;
      }

      if (RoadMapScreenLinePoints.cursor + 2 >= RoadMapScreenLinePoints.end) {
         roadmap_screen_flush_lines ();
      }

      RoadMapScreenLinePoints.cursor[0] = point0;
      RoadMapScreenLinePoints.cursor[1] = point1;

      *RoadMapScreenObjects.cursor = 2;

      RoadMapScreenLinePoints.cursor  += 2;
      RoadMapScreenObjects.cursor += 1;

      break;

   case ROADMAP_LINE_STYLE_POINT: /* Draw a point. */

      /* Optimization: do not draw a line that is obviously not visible. */
      if (! fully_visible)
      {
         if (! roadmap_math_point_is_visible (&position0)) return;
      }

      if (RoadMapScreenPoints.cursor >= RoadMapScreenPoints.end) {
         roadmap_screen_flush_points ();
      }
      RoadMapScreenPoints.cursor[0] = point0;

      RoadMapScreenPoints.cursor += 1;

      break;
   }
}


void roadmap_screen_draw_line (int line) {

   int square;
   int first_shape_line;
   int last_shape_line;

   RoadMapPosition position;


   roadmap_line_from (line, &position);
   square = roadmap_square_search (&position);

   if (roadmap_shape_in_square (square, &first_shape_line,
                                        &last_shape_line) > 0) {

      roadmap_screen_draw_one_line
         (line, 1, first_shape_line, last_shape_line);

   } else {

      roadmap_screen_draw_one_line (line, 1, -1, -1);
   }
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
   int category;
   int current_category = -1;
   int west;
   int east;
   int north;
   int south;
   int *geo_point;
   RoadMapPosition position;
   RoadMapGuiPoint *graphic_point;
   RoadMapGuiPoint *previous_point;

   RoadMapGuiPoint upper_left;
   RoadMapGuiPoint lower_right;


   if (! roadmap_is_visible (ROADMAP_SHOW_AREA)) return;


   for (i = roadmap_polygon_count() - 1; i >= 0; --i) {

      category = roadmap_polygon_category (i);

      if (category != current_category) {
         roadmap_screen_flush_polygons ();
         roadmap_layer_select (category);
         current_category = category;
      }

      roadmap_polygon_edges (i, &west, &east, &north, &south);

      if (! roadmap_math_is_visible (west, east, north, south)) {
         continue;
      }

      /* Declutter logic: do not show the polygon when it has been
       * reduced (by the zoom) to a quasi-point.
       */
      position.longitude = west;
      position.latitude  = north;
      roadmap_math_coordinate (&position, &upper_left);

      position.longitude = east;
      position.latitude  = south;
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
   RoadMapPosition topleft;
   RoadMapPosition bottomright;
   RoadMapPosition position;

   RoadMapGuiPoint points[6];


   if (! roadmap_is_visible (ROADMAP_SHOW_SQUARE)) return;

   roadmap_square_edges (square, &(topleft.longitude),
                                 &(bottomright.longitude),
                                 &(topleft.latitude),
                                 &(bottomright.latitude));

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

   roadmap_canvas_select_pen (RoadMapEdges);
   count = 6;
   roadmap_math_rotate_coordinates (count, points);
   roadmap_canvas_draw_multiple_lines (1, &count, points);
}


static int roadmap_screen_draw_square
              (int square, int cfcc, int fully_visible) {

   int line;
   int first_line;
   int last_line;
   int first_shape_line;
   int last_shape_line;

   int drawn = 0;


   roadmap_log_push ("roadmap_screen_draw_square");

   /* Draw each line that belongs to this square. */

   if (roadmap_line_in_square (square, cfcc, &first_line, &last_line) > 0) {

      if (roadmap_shape_in_square (square, &first_shape_line,
                                            &last_shape_line) <= 0) {
         first_shape_line = last_shape_line = -1;
      }

      for (line = first_line; line <= last_line; ++line) {

         roadmap_screen_draw_one_line
            (line, fully_visible, first_shape_line, last_shape_line);
         drawn += 1;
      }
   }

   /* Draw each line that intersects with this square (but belongs
    * to another square--the crossing lines).
    */
   if (roadmap_line_in_square2 (square, cfcc, &first_line, &last_line) > 0) {

      int last_real_square = -1;
      int real_square  = 0;
      int west = 0;
      int east = 0;
      int north = 0;
      int south = 0;

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
         if ((position.longitude <  west) ||
             (position.longitude >= east) ||
             (position.latitude <  south) ||
             (position.latitude >= north)) {
            real_square = roadmap_square_search (&position);
         }

         if (real_square != last_real_square) {

            if (real_square < 0 || real_square >= square_count) {
               roadmap_log (ROADMAP_ERROR,
                            "Invalid square index %d", real_square);
               continue;
            }

            roadmap_square_edges (real_square, &west, &east, &north, &south);

            last_real_square = real_square;

            if (on_canvas[real_square]) {
               /* Either it has already been drawn, or it will be soon. */
               continue;
            }

            if (roadmap_math_is_visible (west, east, north, south)) {

               on_canvas[real_square] = 1;
               continue;
            }

            if (roadmap_shape_in_square
                                 (real_square,
                                  &first_shape_line, &last_shape_line) <= 0) {

               first_shape_line = last_shape_line = -1;
            }
         }

         if (on_canvas[real_square]) {
            /* Either it has already been drawn, or it will be soon. */
            continue;
         }

         roadmap_screen_draw_one_line
            (real_line, fully_visible, first_shape_line, last_shape_line);
         drawn += 1;
      }

      free (on_canvas);
   }

   roadmap_log_pop ();
   return drawn;
}


static void roadmap_screen_reset_square_mask (void) {

   if (SquareOnScreen != NULL) {
      free(SquareOnScreen);
   }
   SquareOnScreenCount = roadmap_square_count();
   SquareOnScreen = calloc (SquareOnScreenCount, sizeof(char));
   roadmap_check_allocated(SquareOnScreen);
}


static int roadmap_screen_repaint_square (int square) {

   int i;
   int count;
   int layers[256];

   int west;
   int east;
   int north;
   int south;

   int drawn = 0;
   int category;
   int fully_visible;


   if (SquareOnScreen[square]) {
      return 0;
   }
   SquareOnScreen[square] = 1;

   roadmap_log_push ("roadmap_screen_repaint_square");

   roadmap_square_edges (square, &west, &east, &north, &south);

   switch (roadmap_math_is_visible (west, east, north, south)) {
   case 0:
      roadmap_log_pop ();
      return 0;
   case 1:
      fully_visible = 1;
      break;
   default:
      fully_visible = 0;
   }

   roadmap_screen_draw_square_edges (square);

   count = roadmap_layer_visible_lines (layers, 256);
   
   for (i = 0; i < count; ++i) {

        category = layers[i];

        roadmap_layer_select (category);

        drawn += roadmap_screen_draw_square
                    (square, category, fully_visible);

        if (RoadMapScreenObjects.cursor != RoadMapScreenObjects.data) {
            roadmap_screen_flush_lines();
        }
        if (RoadMapScreenPoints.cursor != RoadMapScreenPoints.data) {
            roadmap_screen_flush_points();
        }
   }

   roadmap_log_pop ();
   return drawn;
}


static void roadmap_screen_repaint (void) {

    static RoadMapGuiPoint CompassPoint = {20, 20};
    static int *fips = NULL;

    int i;
    int j;
    int count;
    int in_view[4096];
    

    if (RoadMapScreenFrozen) return;


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

        count = roadmap_square_view (in_view, 4096);

        if (count > 0) {

            roadmap_screen_draw_polygons ();
            roadmap_screen_reset_square_mask();

            for (j = count - 1; j >= 0; --j) {
                roadmap_screen_repaint_square (in_view[j]);
            }
        }
    }

    roadmap_trip_format_messages ();
    
    if (strcasecmp
           (roadmap_config_get (&RoadMapConfigMapSigns), "yes") == 0) {

       roadmap_sprite_draw ("Compass", &CompassPoint, 0);


       roadmap_display_signs ();

       roadmap_trip_display ();
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



static void roadmap_screen_button_pressed (RoadMapGuiPoint *point) {
    
    int line;
    int distance;
    RoadMapPosition position;

    
    roadmap_math_to_position (point, &position);
   
    line = roadmap_navigate_retrieve_line
                (&position,
                 roadmap_config_get_integer (&RoadMapConfigAccuracyMouse),
                 &distance);

    if (line >= 0) {

        roadmap_trip_set_point ("Selection", &position);
        roadmap_display_activate ("Selected Street", line, &position);
        roadmap_screen_repaint ();
    }
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


void roadmap_screen_refresh (void) {

    int refresh = 0;

    roadmap_log_push ("roadmap_screen_refresh");

    if (roadmap_trip_is_focus_changed()) {

        roadmap_screen_reset_delta ();
        RoadMapScreenCenter = *roadmap_trip_get_focus_position ();
        roadmap_math_set_center (&RoadMapScreenCenter);
        refresh = 1;
        
    } else if (roadmap_trip_is_focus_moved()) {

        RoadMapScreenCenter = *roadmap_trip_get_focus_position ();
        roadmap_math_set_center (&RoadMapScreenCenter);

        if (RoadMapScreenDeltaX || RoadMapScreenDeltaY) {

           int dx = RoadMapScreenDeltaX;
           int dy = RoadMapScreenDeltaY;

           refresh |=
               roadmap_math_set_orientation
                   (roadmap_trip_get_orientation() + RoadMapScreenRotation);

           RoadMapScreenDeltaX = RoadMapScreenDeltaY = 0;
           roadmap_screen_record_move (dx, dy);
        }
    }
    
    refresh |=
        roadmap_math_set_orientation
            (roadmap_trip_get_orientation() + RoadMapScreenRotation);

    if (strcasecmp
           (roadmap_config_get (&RoadMapConfigMapRefresh), "forced") == 0) {

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

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigMapSigns, "yes", "no", NULL);

    roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigMapRefresh, "normal", "forced", NULL);

   roadmap_canvas_register_button_handler (&roadmap_screen_button_pressed);
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
    
    roadmap_layer_initialize();

    RoadMapScreenDeltaX = roadmap_config_get_integer (&RoadMapConfigDeltaX);
    RoadMapScreenDeltaY = roadmap_config_get_integer (&RoadMapConfigDeltaY);
    RoadMapScreenRotation =
       roadmap_config_get_integer (&RoadMapConfigDeltaRotate);

    RoadMapBackground = roadmap_canvas_create_pen ("Map.Background");
    roadmap_canvas_set_foreground
        (roadmap_config_get (&RoadMapConfigMapBackground));

    RoadMapEdges = roadmap_canvas_create_pen ("Map.Edges");
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

