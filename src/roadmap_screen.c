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
#include "roadmap_list.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_square.h"
#include "roadmap_street.h"
#include "roadmap_line.h"
#include "roadmap_shape.h"
#include "roadmap_point.h"
#include "roadmap_polygon.h"
#include "roadmap_dictionary.h"
#include "roadmap_county.h"
#include "roadmap_locator.h"

#include "roadmap_sprite.h"
#include "roadmap_trip.h"
#include "roadmap_canvas.h"
#include "roadmap_display.h"
#include "roadmap_screen.h"


static RoadMapPosition RoadMapScreenCenter;

static int RoadMapScreenRotation;
static int RoadMapScreenWidth;
static int RoadMapScreenHeight;
static int RoadMapScreenShapesVisible = 0;


static char *SquareOnScreen;
static int   SquareOnScreenCount;


/* CATEGORIES.
 * A category represents a group of map objects that are represented
 * using the same pen (i.e. same color, thickness) and the same
 * graphical primitive (line, polygon, etc..).
 */
static struct roadmap_canvas_category {

   char *name;
   int   declutter;
   int   thickness;

   RoadMapPen pen;

} *RoadMapCategory;


/* CLASSES.
 * A class represent a group of categories that have the same basic
 * properties. For example, the "Road" class can be searched for an
 * address.
 */
typedef struct {

   char *name;

   int   count;
   char  category[128];

} RoadMapClass;

static RoadMapClass RoadMapLineClass[] = {
   {"Road", 0, {0}},
   {"Feature", 0, {0}},
   {NULL, 0, {0}}
};

static RoadMapClass *RoadMapRoadClass = &(RoadMapLineClass[0]);


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


static struct {

   char  *name;
   time_t deadline;
   RoadMapPosition position;
   RoadMapPosition endpoint[2];

} RoadMapStreetTip = {NULL, 0, {0, 0}};

RoadMapPen RoadMapHighlightForeground = NULL;
RoadMapPen RoadMapHighlightBackground = NULL;
RoadMapPen RoadMapBackground = NULL;
RoadMapPen RoadMapEdges = NULL;


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


static void roadmap_screen_draw_line_no_shape (int line, int fully_visible) {

   RoadMapPosition position0;
   RoadMapPosition position1;


   roadmap_line_from (line, &position0);

   roadmap_line_to   (line, &position1);

   /* Optimization: do not draw lines that are obviously not visible. */

   if (! fully_visible)
   {
      if (! roadmap_math_line_is_visible (&position0, &position1)) return;
   }

   if (RoadMapScreenLinePoints.cursor + 2 >= RoadMapScreenLinePoints.end) {
      roadmap_screen_flush_lines ();
   }

   roadmap_math_coordinate (&position0, RoadMapScreenLinePoints.cursor);
   roadmap_math_coordinate (&position1, RoadMapScreenLinePoints.cursor+1);

   if ((RoadMapScreenLinePoints.cursor[0].x ==
           RoadMapScreenLinePoints.cursor[1].x) &&
       (RoadMapScreenLinePoints.cursor[0].y ==
           RoadMapScreenLinePoints.cursor[1].y)) {

      if (RoadMapScreenPoints.cursor >= RoadMapScreenPoints.end) {
         roadmap_screen_flush_points ();
      }
      RoadMapScreenPoints.cursor[0] = RoadMapScreenLinePoints.cursor[0];

      RoadMapScreenPoints.cursor += 1;

   } else {

      *RoadMapScreenObjects.cursor = 2;

      RoadMapScreenLinePoints.cursor  += 2;
      RoadMapScreenObjects.cursor += 1;
   }
}


static void roadmap_screen_draw_line_with_shape
               (int line, int first_shape, int last_shape) {

   int i;
   RoadMapPosition position_end;
   RoadMapPosition position;

   RoadMapGuiPoint  point_end;
   RoadMapGuiPoint *points;


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

   roadmap_line_to (line, &position_end);

   points = RoadMapScreenLinePoints.cursor;

   roadmap_line_from (line, &position);
   roadmap_math_coordinate (&position, points);

   roadmap_math_coordinate (&position_end, &point_end);

   if ((points->x == point_end.x) && (points->y == point_end.y)) {

      if (RoadMapScreenPoints.cursor >= RoadMapScreenPoints.end) {
         roadmap_screen_flush_points ();
      }
      *RoadMapScreenPoints.cursor = *points;

      RoadMapScreenPoints.cursor += 1;

      return;
   }

   points += 1;

   for (i = first_shape; i <= last_shape; ++i) {

      roadmap_shape_get_position (i, &position);

      roadmap_math_coordinate (&position, points);
      points += 1;
   }

   *points = point_end;
   points += 1;


   *RoadMapScreenObjects.cursor = points - RoadMapScreenLinePoints.cursor;

   RoadMapScreenLinePoints.cursor   = points;
   RoadMapScreenObjects.cursor += 1;
}


void roadmap_screen_draw_line (int line) {

   int square;
   int first_shape_line;
   int last_shape_line;
   int first_shape;
   int last_shape;

   RoadMapPosition position;


   roadmap_line_from (line, &position);
   square = roadmap_square_search (&position);

   if (roadmap_shape_in_square (square, &first_shape_line,
                                        &last_shape_line) > 0) {

      if (roadmap_shape_of_line (line, first_shape_line,
                                       last_shape_line,
                                       &first_shape, &last_shape) > 0) {

         roadmap_screen_draw_line_with_shape (line, first_shape, last_shape);
         return;
      }
   }
   roadmap_screen_draw_line_no_shape (line, 1);
}


static void roadmap_screen_flush_polygons (void) {

   roadmap_math_rotate_coordinates
       (RoadMapScreenLinePoints.cursor - RoadMapScreenLinePoints.data,
        RoadMapScreenLinePoints.data);

   roadmap_canvas_draw_multiple_polygons
      (RoadMapScreenObjects.cursor - RoadMapScreenObjects.data,
       RoadMapScreenObjects.data,
       RoadMapScreenLinePoints.data,
       1);

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


   if (! roadmap_is_visible (ROADMAP_SHOW_AREA)) return;

   if (! roadmap_math_declutter
            (roadmap_config_get_integer("Polygons", "Declutter"))) return;


   for (i = roadmap_polygon_count() - 1; i >= 0; --i) {

      category = roadmap_polygon_category (i);

      if (category != current_category) {
         if (RoadMapScreenObjects.cursor != RoadMapScreenObjects.data) {
            roadmap_screen_flush_polygons ();
         }
         roadmap_canvas_select_pen (RoadMapCategory[category].pen);
         current_category = category;
      }

      roadmap_polygon_edges (i, &west, &east, &north, &south);

      if (! roadmap_math_is_visible (west, east, north, south)) {
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

   if (RoadMapScreenObjects.cursor != RoadMapScreenObjects.data) {
      roadmap_screen_flush_polygons ();
   }
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
   int first_shape;
   int last_shape;

   int drawn = 0;


   /* Draw each line that belongs to this square. */

   if (roadmap_line_in_square (square, cfcc, &first_line, &last_line) > 0) {

      if (RoadMapScreenShapesVisible &&
          (roadmap_shape_in_square (square, &first_shape_line,
                                            &last_shape_line) > 0)) {

         for (line = first_line; line <= last_line; ++line) {

            if (roadmap_shape_of_line (line, first_shape_line,
                                             last_shape_line,
                                             &first_shape, &last_shape) > 0) {
               roadmap_screen_draw_line_with_shape
                        (line, first_shape, last_shape);
            } else {
               roadmap_screen_draw_line_no_shape (line, fully_visible);
            }
            drawn += 1;
         }

      } else {

         for (line = first_line; line <= last_line; ++line) {

            roadmap_screen_draw_line_no_shape (line, fully_visible);
            drawn += 1;
         }
      }
   }

   /* Draw each line that intersects with this square (but belongs
    * to another square--the crossing lines).
    */
   if (roadmap_line_in_square2 (square, cfcc, &first_line, &last_line) > 0) {

      int last_real_square = -1;
      int real_square  = 0;
      int check_shapes = 0;
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

            if (RoadMapScreenShapesVisible) {
               check_shapes = roadmap_shape_in_square
                                 (real_square,
                                  &first_shape_line, &last_shape_line);
            } else {
               check_shapes = 0;
            }
         }

         if (on_canvas[real_square]) {
            /* Either it has already been drawn, or it will be soon. */
            continue;
         }

         if (check_shapes) {
            if (roadmap_shape_of_line (real_line,
                                       first_shape_line,
                                       last_shape_line,
                                       &first_shape, &last_shape) > 0) {
               roadmap_screen_draw_line_with_shape
                          (real_line, first_shape, last_shape);
            } else {
               roadmap_screen_draw_line_no_shape (real_line, fully_visible);
            }
         } else {
            roadmap_screen_draw_line_no_shape (real_line, fully_visible);
         }
         drawn += 1;
      }

      free (on_canvas);
   }

   return drawn;
}


static void roadmap_screen_reset_square_mask (void) {

   if (SquareOnScreen != NULL) {
      free(SquareOnScreen);
   }
   SquareOnScreenCount = roadmap_square_count();
   SquareOnScreen = calloc (SquareOnScreenCount, sizeof(char));
   if (SquareOnScreen == NULL) {
      roadmap_log (ROADMAP_FATAL, "no more memory");
   }
}


static int roadmap_screen_repaint_square (int square) {

   int i;
   int j;
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

   roadmap_square_edges (square, &west, &east, &north, &south);

   switch (roadmap_math_is_visible (west, east, north, south)) {
   case 0:
      return 0;
   case 1:
      fully_visible = 1;
      break;
   default:
      fully_visible = 0;
   }

   roadmap_screen_draw_square_edges (square);

   for (i = 0; RoadMapLineClass[i].name != NULL; ++i) {

      RoadMapClass *class = RoadMapLineClass + i;

      for (j = class->count - 1; j >= 0; --j) {

         category = class->category[j];

         if (roadmap_math_declutter (RoadMapCategory[category].declutter)) {

            roadmap_canvas_select_pen (RoadMapCategory[category].pen);

            drawn += roadmap_screen_draw_square
                        (square, category, fully_visible);

            if (RoadMapScreenObjects.cursor != RoadMapScreenObjects.data) {
               roadmap_screen_flush_lines();
            }
            if (RoadMapScreenPoints.cursor != RoadMapScreenPoints.data) {
               roadmap_screen_flush_points();
            }
         }
      }
   }

   return drawn;
}


static void roadmap_screen_repaint_map (void) {

   static int *fips = NULL;

   int i;
   int j;
   int count;
   int in_view[4096];


   roadmap_canvas_select_pen (RoadMapBackground);
   roadmap_canvas_erase ();


   /* Repaint the drawing buffer. */

   roadmap_math_set_center (&RoadMapScreenCenter);


   /* - Identifies the candidate counties. */

   count = roadmap_locator_by_position (&RoadMapScreenCenter, &fips);


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
}


static void roadmap_screen_repaint_sprites (void) {

    RoadMapGuiPoint point;
    
    point.x = 20;
    point.y = 20;
    roadmap_sprite_draw ("Compass", &point, 0);
    
    roadmap_trip_display ();

    roadmap_display_show ();
}


static int roadmap_screen_is_street_tip_active (void) {
    
    if (RoadMapStreetTip.name == NULL) {
        return 0;
    }
    
    if ((time(NULL) > RoadMapStreetTip.deadline) ||
        (! roadmap_math_point_is_visible (&RoadMapStreetTip.position))) {

        free (RoadMapStreetTip.name);
        RoadMapStreetTip.name = NULL;
            
        return 0;
   }

   return 1;
}


static void roadmap_screen_repaint_street_tip (void) {

   int i;
   RoadMapGuiPoint point;

   for (i = 1; i >= 0; --i) {
      roadmap_math_coordinate (RoadMapStreetTip.endpoint+i, &point);
      roadmap_math_rotate_coordinates (1, &point);
      roadmap_sprite_draw ("Highlight", &point, 0);
   }
   roadmap_display_details
       (&RoadMapStreetTip.position, RoadMapStreetTip.name);
}


static void roadmap_screen_repaint (void) {

   roadmap_math_set_center (&RoadMapScreenCenter);

   RoadMapScreenShapesVisible =
      roadmap_math_declutter
         (roadmap_config_get_integer("Shapes", "Declutter"));

   roadmap_screen_repaint_map ();
   roadmap_screen_repaint_sprites ();

   if (roadmap_screen_is_street_tip_active()) {
      roadmap_screen_repaint_street_tip ();
   }

   roadmap_canvas_refresh ();
}


static void roadmap_screen_configure (void) {

   RoadMapScreenWidth = roadmap_canvas_width();
   RoadMapScreenHeight = roadmap_canvas_height();

   roadmap_math_set_size (RoadMapScreenWidth, RoadMapScreenHeight);
   if (RoadMapCategory != NULL) {
      roadmap_screen_repaint ();
   }
}



static int roadmap_screen_activate_street_tip (int line,
                                               RoadMapPosition *position) {

   char *name;

   name = roadmap_street_get_name_from_line (line);

   if (name == NULL || name[0] == 0) {
       name = "(this street has no name)";
   }

   if (RoadMapStreetTip.name != NULL) {
      free (RoadMapStreetTip.name);
   }
   RoadMapStreetTip.name = strdup (name);

   roadmap_line_from (line, &RoadMapStreetTip.endpoint[0]);
   roadmap_line_to   (line, &RoadMapStreetTip.endpoint[1]);

   RoadMapStreetTip.deadline =
      time(NULL) + roadmap_config_get_integer ("Highlight", "Duration");
   RoadMapStreetTip.position = *position;

   roadmap_canvas_select_pen (RoadMapHighlightForeground);
   roadmap_screen_draw_line (line);
   roadmap_canvas_refresh ();

   return 1;
}


static void roadmap_screen_button_pressed (RoadMapGuiPoint *point) {

   int i;
   int count = 0;
   int detail[128];

   int west, east, north, south;

   int line;
   int distance;
   int accuracy = atoi (roadmap_config_get ("Accuracy", "Mouse"));

   RoadMapPosition position;
   RoadMapGuiPoint focus_point;
   RoadMapPosition focus_position;

   RoadMapPosition focus_topleft;
   RoadMapPosition focus_bottomright;


   ROADMAP_POINT_SET_X(&focus_point, ROADMAP_POINT_GET_X(point) + accuracy);
   ROADMAP_POINT_SET_Y(&focus_point, ROADMAP_POINT_GET_Y(point) + accuracy);
   roadmap_math_to_position (&focus_point, &focus_position);

   west = focus_position.longitude;
   east = focus_position.longitude;
   north = focus_position.latitude;
   south = focus_position.latitude;

   focus_bottomright = focus_position;

   ROADMAP_POINT_SET_X(&focus_point, ROADMAP_POINT_GET_X(point) - accuracy);
   roadmap_math_to_position (&focus_point, &focus_position);

   if (focus_position.longitude < west) {
      west = focus_position.longitude;
   }
   if (focus_position.longitude > east) {
      east = focus_position.longitude;
   }
   if (focus_position.latitude < south) {
      south = focus_position.latitude;
   }
   if (focus_position.latitude > north) {
      north = focus_position.latitude;
   }

   ROADMAP_POINT_SET_Y(&focus_point, ROADMAP_POINT_GET_Y(point) - accuracy);
   roadmap_math_to_position (&focus_point, &focus_position);

   if (focus_position.longitude < west) {
      west = focus_position.longitude;
   }
   if (focus_position.longitude > east) {
      east = focus_position.longitude;
   }
   if (focus_position.latitude < south) {
      south = focus_position.latitude;
   }
   if (focus_position.latitude > north) {
      north = focus_position.latitude;
   }

   focus_topleft = focus_position;

   ROADMAP_POINT_SET_X(&focus_point, ROADMAP_POINT_GET_X(point) + accuracy);
   roadmap_math_to_position (&focus_point, &focus_position);

   if (focus_position.longitude < west) {
      west = focus_position.longitude;
   }
   if (focus_position.longitude > east) {
      east = focus_position.longitude;
   }
   if (focus_position.latitude < south) {
      south = focus_position.latitude;
   }
   if (focus_position.latitude > north) {
      north = focus_position.latitude;
   }

   roadmap_math_to_position (point, &position);
   
#ifdef DEBUG
printf ("Position: %d longitude, %d latitude\n",
        position.longitude,
        position.latitude);
fflush(stdout);
#endif

   for (i = RoadMapRoadClass->count - 1; i >= 0; --i) {

      int category = RoadMapRoadClass->category[i];

      if (roadmap_math_declutter (RoadMapCategory[category].declutter)) {

         detail[count++] = category;
      }
   }

   if (count > 0) {

      roadmap_math_set_focus (west, east, north, south);

      line = roadmap_street_get_closest (&position, count, detail, &distance);

      roadmap_math_release_focus ();

      if (line >= 0) {

         if (roadmap_screen_activate_street_tip (line, &position)) {
            roadmap_screen_repaint ();
         }
      }
   }
}


void roadmap_screen_set_waypoint (void) {
    
    if (roadmap_screen_is_street_tip_active()) {
        roadmap_trip_set_point (RoadMapStreetTip.name, &RoadMapStreetTip.position);
    }
}


void roadmap_screen_refresh (void) {

    int refresh = 0;
    
    if (roadmap_trip_is_focus_changed()) {
        
        RoadMapScreenCenter = *roadmap_trip_get_focus_position ();
        RoadMapScreenRotation = 0;
        refresh = 1;
        
    } else if (roadmap_trip_is_focus_moved()) {
        
        RoadMapScreenCenter = *roadmap_trip_get_focus_position ();

    }
    
    refresh |=
        roadmap_math_set_orientation
            (roadmap_trip_get_orientation() + RoadMapScreenRotation);
        
    refresh |= roadmap_trip_is_refresh_needed();
        
    if (refresh) {
        roadmap_screen_repaint ();
    }
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
      roadmap_screen_repaint ();
   }
}


void roadmap_screen_move_up (void) {

   RoadMapGuiPoint center;

   ROADMAP_POINT_SET_X(&center, RoadMapScreenWidth / 2);
   ROADMAP_POINT_SET_Y(&center, RoadMapScreenHeight / 4);

   roadmap_math_to_position (&center, &RoadMapScreenCenter);
   roadmap_screen_repaint ();
}


void roadmap_screen_move_down (void) {

   RoadMapGuiPoint center;

   ROADMAP_POINT_SET_X(&center, RoadMapScreenWidth / 2);
   ROADMAP_POINT_SET_Y(&center, (3 * RoadMapScreenHeight) / 4);

   roadmap_math_to_position (&center, &RoadMapScreenCenter);
   roadmap_screen_repaint ();
}


void roadmap_screen_move_right (void) {

   RoadMapGuiPoint center;

   ROADMAP_POINT_SET_X(&center, (3 * RoadMapScreenWidth) / 4);
   ROADMAP_POINT_SET_Y(&center, RoadMapScreenHeight / 2);

   roadmap_math_to_position (&center, &RoadMapScreenCenter);
   roadmap_screen_repaint ();
}


void roadmap_screen_move_left (void) {


   RoadMapGuiPoint center;

   ROADMAP_POINT_SET_X(&center, RoadMapScreenWidth / 4);
   ROADMAP_POINT_SET_Y(&center, RoadMapScreenHeight / 2);

   roadmap_math_to_position (&center, &RoadMapScreenCenter);
   roadmap_screen_repaint ();
}


static void roadmap_screen_after_zoom (void) {

   int i;

   /* Adjust the thickness of the drawing pen for all categories. */
    
   for (i = roadmap_locator_category_count(); i > 0; --i) {

      if (roadmap_math_declutter (RoadMapCategory[i].declutter)) {

         roadmap_canvas_select_pen (RoadMapCategory[i].pen);

         roadmap_canvas_set_thickness
            (roadmap_math_thickness (RoadMapCategory[i].thickness));
      }
   }
   
   roadmap_screen_repaint ();
}


void roadmap_screen_zoom_in  (void) {

   roadmap_math_zoom_in ();
   roadmap_screen_after_zoom ();
}


void roadmap_screen_zoom_out (void) {

   roadmap_math_zoom_out ();
   roadmap_screen_after_zoom ();
}


void roadmap_screen_zoom_reset (void) {

   roadmap_math_zoom_reset ();
   roadmap_screen_repaint ();
}


void roadmap_screen_initialize (void) {

   roadmap_config_declare ("preferences", "Polygons", "Declutter", "1300");
   roadmap_config_declare ("preferences", "Shapes", "Declutter", "1300");

   roadmap_config_declare ("preferences", "Accuracy", "GPS", "1000");
   roadmap_config_declare ("preferences", "Accuracy", "Mouse", "20");

   roadmap_config_declare ("preferences", "Map", "Background", "LightYellow");

   roadmap_config_declare ("preferences", "Highlight", "Background", "yellow");
   roadmap_config_declare ("preferences", "Highlight", "Thickness", "4");
   roadmap_config_declare ("preferences", "Highlight", "Duration", "10");

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

   int i;
   int category_count;


   category_count = roadmap_locator_category_count();

   RoadMapCategory =
      calloc (category_count + 1, sizeof(*RoadMapCategory));

   for (i = 1; i < category_count; ++i) {

      char *name = roadmap_locator_category_name(i);
      char *class;
      char *color;

      RoadMapClass *p;


      roadmap_config_declare ("schema", name, "Class", "");

      roadmap_config_declare_color ("schema", name, "Color", "black");
      roadmap_config_declare ("schema", name, "Thickness", "1");
      roadmap_config_declare ("schema", name, "Declutter", "20248000000");

      class = roadmap_config_get (name, "Class");

      for (p = RoadMapLineClass; p->name != NULL; ++p) {

         if (strcasecmp (class, p->name) == 0) {
            p->category[p->count++] = i;
            break;
         }
      }

      RoadMapCategory[i].name = name;
      RoadMapCategory[i].declutter =
         roadmap_config_get_integer (name, "Declutter");
      RoadMapCategory[i].thickness =
         roadmap_config_get_integer (name, "Thickness");

      RoadMapCategory[i].pen = roadmap_canvas_create_pen (name);

      roadmap_canvas_set_thickness (RoadMapCategory[i].thickness);

      color = roadmap_config_get (name, "Color");
      if (color != NULL && *color > ' ') {
         roadmap_canvas_set_foreground (color);
      }
   }

   RoadMapBackground = roadmap_canvas_create_pen ("Map.Background");
   roadmap_canvas_set_foreground (roadmap_config_get ("Map", "Background"));

   RoadMapEdges = roadmap_canvas_create_pen ("Map.Edges");
   roadmap_canvas_set_thickness (4);
   roadmap_canvas_set_foreground ("grey");


   RoadMapHighlightBackground =
      roadmap_canvas_create_pen ("Highlight.Background");

   roadmap_canvas_set_foreground
      (roadmap_config_get ("Highlight", "Background"));

   roadmap_canvas_set_thickness
      (roadmap_config_get_integer ("Highlight", "Thickness"));

   RoadMapHighlightForeground =
      roadmap_canvas_create_pen ("Highlight.Foreground");
   
   roadmap_display_colors (RoadMapHighlightForeground,
                           RoadMapHighlightBackground);
}

