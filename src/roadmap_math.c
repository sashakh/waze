/* roadmap_math.c - Manage the little math required to place points on a map.
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
 *   See roadmap_math.h.
 *
 * These functions are used to compute the position of points on the map,
 * or the distance between two points, given their Tiger position.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_square.h"
#include "roadmap_config.h"

#include "roadmap_trigonometry.h"

#define ROADMAP_BASE_IMPERIAL 0
#define ROADMAP_BASE_METRIC   1


static RoadMapConfigDescriptor RoadMapConfigGeneralDefaultZoom =
                        ROADMAP_CONFIG_ITEM("General", "Default Zoom");

static RoadMapConfigDescriptor RoadMapConfigGeneralZoom =
                        ROADMAP_CONFIG_ITEM("General", "Zoom");


#define ROADMAP_REFERENCE_ZOOM 20


typedef struct {
    
    double unit_per_latitude;
    double unit_per_longitude;
    double speed_per_knot;
    double cm_to_unit;
    int    to_trip_unit;
    
    char  *length;
    char  *trip_distance;
    char  *speed;
    
} RoadMapUnits;


static RoadMapUnits RoadMapMetricSystem = {

    0.11112, /* Meters per latitude. */
    0.0,     /* Meters per longitude (dynamic). */
    1.852,   /* Kmh per knot. */
    0.01,    /* centimeters to meters. */
    1000,    /* meters per kilometer. */
    "m",
    "Km",
    "Kmh"
};

static RoadMapUnits RoadMapImperialSystem = {

    0.36464, /* Feet per latitude. */
    0.0,     /* Feet per longitude (dynamic). */
    1.151,   /* Mph per knot. */
    0.03281, /* centimeters to feet. */
    5280,    /* Feet per mile. */
    "ft",
    "Mi",
    "Mph",
};
    

static struct {

   unsigned short zoom;

   /* The current position shown on the map: */
   RoadMapPosition center;

   /* The center point (current position), in pixel: */
   int center_x;
   int center_y;

   /* The size of the area shown (pixels): */
   int width;
   int height;

   /* The conversion ratio from position to pixels: */
   int zoom_x;
   int zoom_y;


   RoadMapArea focus;
   RoadMapArea upright_screen;
   RoadMapArea current_screen;


   /* Map orientation (0: north, 90: east): */

   int orientation; /* angle in degrees. */

   int sin_orientation; /* Multiplied by 32768. */
   int cos_orientation; /* Multiplied by 32768. */

   RoadMapUnits *units;
   
} RoadMapContext;



static void roadmap_math_trigonometry (int angle, int *sine_p, int *cosine_p) {

   int i = angle % 90;
   int sine;
   int cosine;

   while (i < 0) {
      i += 90;
   }

   if (i <= 45) {
      sine   = RoadMapTrigonometricTable[i].x;
      cosine = RoadMapTrigonometricTable[i].y;
   } else {
      i = 90 - i;
      sine   = RoadMapTrigonometricTable[i].y;
      cosine = RoadMapTrigonometricTable[i].x;
   }

   while (angle < 0) {
       angle += 360;
   }
   i = (angle / 90) % 4;
   if (i < 0) {
      i += 4;
   }

   switch (i) {
   case 0:   *sine_p = sine;       *cosine_p = cosine;     break;
   case 1:   *sine_p = cosine;     *cosine_p = 0 - sine;   break;
   case 2:   *sine_p = 0 - sine;   *cosine_p = 0 - cosine; break;
   case 3:   *sine_p = 0 - cosine; *cosine_p = sine;       break;
   }
}


static int roadmap_math_arccosine (int cosine, int sign) {
    
    int i;
    int low;
    int high;
    int result;
    int cosine_negative = 0;
    
    if (cosine < 0) {
        cosine = 0 - cosine;
        cosine_negative = 1;
    }
    if (cosine >= 32768) {
        if (cosine > 32768) {
            roadmap_log (ROADMAP_ERROR, "invalid cosine value %d", cosine);
            return 0;
        }
        cosine = 32767;
    }
    
    high = 45;
    low  = 0;
    
    if (cosine >= RoadMapTrigonometricTable[45].y) {
        
        while (high > low + 1) {
            
            i = (high + low) / 2;
            
            if (cosine > RoadMapTrigonometricTable[i-1].y) {
                high = i - 1;
            } else if (cosine < RoadMapTrigonometricTable[i].y) {
                low = i;
            } else {
                high = i;
                break;
            }
        }
        
        result = high;
        
    } else {
        
        while (high > low + 1) {
            
            i = (high + low) / 2;
            
            if (cosine >= RoadMapTrigonometricTable[i].x) {
                low = i;
            } else if (cosine < RoadMapTrigonometricTable[i-1].y) {
                high = i - 1;
            } else {
                high = i;
                break;
            }
        }
        
        result = 90 - high;
    }
    
    result = sign * result;
    
    if (cosine_negative) {
        result = 180 - result;
        if (result > 180) {
            result = result - 360;
        }
    }
    return result;
}


static void roadmap_math_compute_scale (void) {

   int orientation;

   int sine;
   int cosine;

   RoadMapGuiPoint point;
   RoadMapPosition position;


   if (RoadMapContext.zoom == 0) {
       RoadMapContext.zoom = ROADMAP_REFERENCE_ZOOM;
   }
   
   RoadMapContext.center_x = RoadMapContext.width / 2;
   RoadMapContext.center_y = RoadMapContext.height / 2;

   RoadMapContext.zoom_x = RoadMapContext.zoom;
   RoadMapContext.zoom_y = RoadMapContext.zoom;

   /* The horizontal ratio is derived from the vertical one,
    * with a scaling depending on the latitude. The goal is to
    * compute a map projection and avoid an horizontal distortion
    * when getting close to the poles.
    */

   roadmap_math_trigonometry (RoadMapContext.center.latitude / 1000000,
                                &sine,
                                &cosine);

   RoadMapMetricSystem.unit_per_longitude =
      (RoadMapMetricSystem.unit_per_latitude * cosine) / 32768;
      
   RoadMapImperialSystem.unit_per_longitude =
      (RoadMapImperialSystem.unit_per_latitude * cosine) / 32768;
      
   RoadMapContext.zoom_y =
      (int) ((RoadMapContext.zoom_y * cosine / 32768) + 0.5);

   RoadMapContext.upright_screen.west =
      RoadMapContext.center.longitude
         - (RoadMapContext.center_x * RoadMapContext.zoom_x);

   RoadMapContext.upright_screen.north =
      RoadMapContext.center.latitude
         + (RoadMapContext.center_y * RoadMapContext.zoom_y);

   orientation = RoadMapContext.orientation;
   roadmap_math_set_orientation (0);

   point.x = RoadMapContext.width;
   point.y = RoadMapContext.height;
   roadmap_math_to_position (&point, &position);
   RoadMapContext.upright_screen.south = position.latitude;
   RoadMapContext.upright_screen.east  = position.longitude;

   roadmap_math_set_orientation (orientation);
}


static int roadmap_math_check_point_in_segment (const RoadMapPosition *from,
                                                const RoadMapPosition *to,
                                                const RoadMapPosition *point,
                                                int count,
                                                RoadMapPosition intersections[]) {

   if (!roadmap_math_point_is_visible (point)) return count;

   if ( (((from->longitude >= point->longitude) &&
           (point->longitude >= to->longitude))    ||
         ((from->longitude <= point->longitude) &&
          (point->longitude <= to->longitude)))       &&
        (((from->latitude >= point->latitude)   &&
          (point->latitude >= to->latitude))       ||
         ((from->latitude <= point->latitude)   &&
          (point->latitude <= to->latitude))) ) {

      intersections[count] = *point;
      count++;
   }

   return count;
}


static int roadmap_math_find_screen_intersection (const RoadMapPosition *from,
                                                  const RoadMapPosition *to,
                                                  double a,
                                                  double b,
                                                  RoadMapPosition intersections[],
                                                  int max_intersections) {

   int count = 0;
   RoadMapPosition point;


   if ((from->longitude - to->longitude > -1.0) &&
         (from->longitude - to->longitude < 1.0)) {

      /* The two points are very close, or the line is quasi vertical:
       * approximating the line to a vertical one is good enough.
       */
      point.longitude = from->longitude;
      point.latitude = RoadMapContext.upright_screen.north;
      count =
         roadmap_math_check_point_in_segment
               (from, to, &point, count, intersections);

      if (count == max_intersections) return count;

      point.latitude = RoadMapContext.upright_screen.south;
      count =
         roadmap_math_check_point_in_segment
               (from, to, &point, count, intersections);

      return count;

   } else if ((from->latitude - to->latitude > -1.0) &&
               (from->latitude - to->latitude < 1.0)) {

      /* The two points are very close, or the line is quasi horizontal:
       * approximating the line to a horizontal one is good enough.
       */

      point.latitude = from->latitude;
      point.longitude = RoadMapContext.upright_screen.west;
      count =
         roadmap_math_check_point_in_segment
             (from, to, &point, count, intersections);

      if (count == max_intersections) return count;

      point.longitude = RoadMapContext.upright_screen.east;
      count =
         roadmap_math_check_point_in_segment
             (from, to, &point, count, intersections);

      return count;

   } else {
      
      point.latitude = RoadMapContext.upright_screen.north;
      point.longitude = (int) ((point.latitude - b) / a);
      count =
         roadmap_math_check_point_in_segment
               (from, to, &point, count, intersections);
      if (count == max_intersections) return count;

      point.latitude = RoadMapContext.upright_screen.south;
      point.longitude = (int) ((point.latitude - b) / a);
      count =
         roadmap_math_check_point_in_segment
               (from, to, &point, count, intersections);
      if (count == max_intersections) return count;

      point.longitude = RoadMapContext.upright_screen.west;
      point.latitude = (int) (b + a * point.longitude);
      count =
         roadmap_math_check_point_in_segment
               (from, to, &point, count, intersections);
      if (count == max_intersections) return count;

      point.longitude = RoadMapContext.upright_screen.east;
      point.latitude = (int) (b + a * point.longitude);
      count =
         roadmap_math_check_point_in_segment
               (from, to, &point, count, intersections);
      if (count == max_intersections) return count;

   }

   return count;
}


static void roadmap_math_counter_rotate_coordinate (RoadMapGuiPoint *point) {

   int x = point->x - RoadMapContext.center_x;
   int y = RoadMapContext.center_y - point->y;

   point->x =
       RoadMapContext.center_x +
           (((x * RoadMapContext.cos_orientation)
                - (y * RoadMapContext.sin_orientation) + 16383) / 32768);

   point->y =
       RoadMapContext.center_y -
           (((x * RoadMapContext.sin_orientation)
                + (y * RoadMapContext.cos_orientation) + 16383) / 32768);
}

/* Rotation of the screen:
 * rotate the coordinates of a point on the screen, the center of
 * the rotation being the center of the screen.
 */
void roadmap_math_rotate_coordinates (int count, RoadMapGuiPoint *points) {

   int i;
   int x;
   int y;


   if (RoadMapContext.orientation == 0) return;

   for (i = count; i > 0; --i) {

      x = points->x - RoadMapContext.center_x;
      y = RoadMapContext.center_y - points->y;

      points->x =
          RoadMapContext.center_x +
              (((x * RoadMapContext.cos_orientation)
                  + (y * RoadMapContext.sin_orientation) + 16383) / 32768);

      points->y =
          RoadMapContext.center_y -
              (((y * RoadMapContext.cos_orientation)
                   - (x * RoadMapContext.sin_orientation) + 16383) / 32768);

      points += 1;
   }
}


/* Rotate a specific object:
 * rotate the coordinates of the object's points according to the provided
 * rotation.
 */
void roadmap_math_rotate_object
         (int count, RoadMapGuiPoint *points,
          RoadMapGuiPoint *center, int orientation) {

   int i;
   int x;
   int y;
   int sin_o;
   int cos_o;
   int total = (RoadMapContext.orientation + orientation) % 360;


   if (total == 0) return;

   roadmap_math_trigonometry (total, &sin_o, &cos_o);

   for (i = count; i > 0; --i) {

      x = points->x - center->x;
      y = center->y - points->y;

      points->x = center->x + (((x * cos_o) + (y * sin_o) + 16383) / 32768);
      points->y = center->y - (((y * cos_o) - (x * sin_o) + 16383) / 32768);

      points += 1;
   }
}


void roadmap_math_initialize (void) {

    roadmap_config_declare ("session", &RoadMapConfigGeneralZoom, "0");
    roadmap_config_declare
        ("preferences", &RoadMapConfigGeneralDefaultZoom, "20");
    RoadMapContext.orientation = 0;

    roadmap_math_use_imperial ();
    roadmap_math_compute_scale ();
}

void roadmap_math_use_metric (void) {

    RoadMapContext.units = &RoadMapMetricSystem;

}


void roadmap_math_use_imperial (void) {

    RoadMapContext.units = &RoadMapImperialSystem;
}


void roadmap_math_set_focus (const RoadMapArea *focus) {

   RoadMapContext.focus = *focus;
}


void roadmap_math_release_focus (void) {

   RoadMapContext.focus = RoadMapContext.current_screen;
}


int roadmap_math_is_visible (const RoadMapArea *area) {

   if (area->west > RoadMapContext.focus.east ||
       area->east < RoadMapContext.focus.west ||
       area->south > RoadMapContext.focus.north ||
       area->north < RoadMapContext.focus.south)
   {
       return 0;
   }

   if (area->west >= RoadMapContext.focus.west &&
       area->east < RoadMapContext.focus.east &&
       area->south > RoadMapContext.focus.south &&
       area->north <= RoadMapContext.focus.north)
   {
       return 1;
   }

   return -1;
}


int roadmap_math_line_is_visible (const RoadMapPosition *point1,
                                  const RoadMapPosition *point2) {

   if ((point1->longitude > RoadMapContext.focus.east) &&
       (point2->longitude > RoadMapContext.focus.east)) {
      return 0;
   }

   if ((point1->longitude < RoadMapContext.focus.west) &&
       (point2->longitude < RoadMapContext.focus.west)) {
      return 0;
   }

   if ((point1->latitude > RoadMapContext.focus.north) &&
       (point2->latitude > RoadMapContext.focus.north)) {
      return 0;
   }

   if ((point1->latitude < RoadMapContext.focus.south) &&
       (point2->latitude < RoadMapContext.focus.south)) {
      return 0;
   }

   return 1; /* Do not bother checking for partial visibility yet. */
}


int roadmap_math_point_is_visible (const RoadMapPosition *point) {

   if ((point->longitude > RoadMapContext.focus.east) ||
       (point->longitude < RoadMapContext.focus.west) ||
       (point->latitude  > RoadMapContext.focus.north) ||
       (point->latitude  < RoadMapContext.focus.south)) {
      return 0;
   }

   return 1;
}


int roadmap_math_get_visible_coordinates (const RoadMapPosition *from,
                                          const RoadMapPosition *to,
                                          RoadMapGuiPoint *point0,
                                          RoadMapGuiPoint *point1) {

   int from_visible = roadmap_math_point_is_visible (from);
   int to_visible = roadmap_math_point_is_visible (to);
   int count;
   RoadMapPosition intersections[2];
   int max_intersections = 0;

   if (!from_visible || !to_visible) {

      double a;
      double b;

      if (from_visible || to_visible) {
         max_intersections = 1;
      } else {
         max_intersections = 2;
      }


      /* Equation of the line: */

      if ((from->longitude - to->longitude) == 0) {
         a = b = 0;
      } else {
         a = 1.0 * (from->latitude - to->latitude) /
                   (from->longitude - to->longitude);
         b = from->latitude - 1.0 * a * from->longitude;
      }

      if (roadmap_math_find_screen_intersection
            (from, to, a, b, intersections, max_intersections) !=
             max_intersections) {
         return 0;
      }
   }

   if (max_intersections == 2) {
      /* Make sure we didn't swap the from/to points */
      if (roadmap_math_distance (from, &intersections[0]) >
          roadmap_math_distance (from, &intersections[1])) {

         RoadMapPosition tmp = intersections[1];
         intersections[1] = intersections[0];
         intersections[0] = tmp;
      }
   }

   count = 0;

   if (from_visible) {
      roadmap_math_coordinate (from, point0);
   } else {
      roadmap_math_coordinate (&intersections[count], point0);
      count++;
   }

   if (to_visible) {
      roadmap_math_coordinate (to, point1);
   } else {
      roadmap_math_coordinate (&intersections[count], point1);
   }

   return 1;
}

void roadmap_math_restore_zoom (void) {

    RoadMapContext.zoom =
        roadmap_config_get_integer (&RoadMapConfigGeneralZoom);
    if (RoadMapContext.zoom == 0) {
         RoadMapContext.zoom =
            roadmap_config_get_integer (&RoadMapConfigGeneralDefaultZoom);
        if (RoadMapContext.zoom == 0) {
            RoadMapContext.zoom = ROADMAP_REFERENCE_ZOOM;
        }
    }
    roadmap_math_compute_scale ();
}

void roadmap_math_zoom_out (void) {

   int zoom;

   zoom = (3 * RoadMapContext.zoom) / 2;
   if (zoom < 0x10000) {
      RoadMapContext.zoom = (unsigned short) zoom;
   }
   roadmap_config_set_integer (&RoadMapConfigGeneralZoom, RoadMapContext.zoom);
   roadmap_math_compute_scale ();
}


void roadmap_math_zoom_in (void) {

   RoadMapContext.zoom = (2 * RoadMapContext.zoom) / 3;
   if (RoadMapContext.zoom <= 1) {
      RoadMapContext.zoom = 2;
   }
   roadmap_config_set_integer (&RoadMapConfigGeneralZoom, RoadMapContext.zoom);
   roadmap_math_compute_scale ();
}


void roadmap_math_zoom_reset (void) {

   RoadMapContext.zoom =
        roadmap_config_get_integer (&RoadMapConfigGeneralDefaultZoom);

   roadmap_config_set_integer (&RoadMapConfigGeneralZoom, RoadMapContext.zoom);

   roadmap_math_compute_scale ();
}


int roadmap_math_declutter (int level) {

   return (RoadMapContext.zoom < level);
}


int roadmap_math_thickness (int base, int declutter, int use_multiple_pens) {

   double ratio;

   ratio = ((2.5 * ROADMAP_REFERENCE_ZOOM) * base) / RoadMapContext.zoom;

   if (ratio < 0.1 / base) {
      return 1;
   }

   if (declutter > (ROADMAP_REFERENCE_ZOOM*100)) {
      declutter = ROADMAP_REFERENCE_ZOOM*100;
   }

   if (ratio < 1.0 * base) {

      /* Use the declutter value to decide how fast should a line shrink.
       * This way, a street shrinks faster than a freeway when we zoom out.
       */
      ratio += (base-ratio) * (0.30*declutter/RoadMapContext.zoom);
      if (ratio > base) {
         ratio = base;
      }
   }

   /* if this is a multi-pen object, try to force a minimum thickness of 3
    * so we'll get a pretty drawing. Otherwise set it to 1.
    */
   if (use_multiple_pens && (ratio < 3)) {
      if ((1.0 * RoadMapContext.zoom / declutter) < 0.50) {
         ratio = 3;
      } else {
         ratio = 1;
      }
   }

   return (int) ratio;
}


void roadmap_math_set_size (int width, int height) {

   RoadMapContext.width = width;
   RoadMapContext.height = height;

   roadmap_math_compute_scale ();
}


void roadmap_math_set_center (RoadMapPosition *position) {

   RoadMapContext.center = *position;

   roadmap_math_compute_scale ();
}


int roadmap_math_set_orientation (int direction) {

   /* FIXME: this function, which primary purpose was to
    * compute the span of the visible map area when rotated,
    * has become THE way for setting the visible map area
    * (i.e. RoadMapContext.current_screen). Therefore, one
    * must execute it to the end.
    */
   int status = 1; /* Force a redraw by default. */

   direction = direction % 360;
   while (direction < 0) {
      direction += 360;
   }

   if (direction == RoadMapContext.orientation) {

      status = 0; /* Not modified at all. */

   } else if ((direction != 0) &&
              (abs(direction - RoadMapContext.orientation) <= 5)) {

      /* We do not force a redraw for every small move, except
       * when it is a back-to-zero event, which might be a reset.
       */
      status = 0; /* Not modified enough. */

   } else {

      RoadMapContext.orientation = direction;
   }

   roadmap_math_trigonometry (direction,
                              &RoadMapContext.sin_orientation,
                              &RoadMapContext.cos_orientation);

   RoadMapContext.current_screen = RoadMapContext.upright_screen;

   if (direction != 0) {

      int i;
      RoadMapGuiPoint point;
      RoadMapPosition position[4];

      point.x = 0;
      point.y = 0;
      roadmap_math_to_position (&point, position);

      point.x = RoadMapContext.width;
      roadmap_math_to_position (&point, position+1);

      point.y = RoadMapContext.height;
      roadmap_math_to_position (&point, position+2);

      point.x = 0;
      roadmap_math_to_position (&point, position+3);

      for (i = 0; i < 4; ++i) {

         if (position[i].longitude > RoadMapContext.current_screen.east) {

            RoadMapContext.current_screen.east = position[i].longitude;
         }
         if (position[i].longitude < RoadMapContext.current_screen.west) {

            RoadMapContext.current_screen.west = position[i].longitude;
         }
         if (position[i].latitude < RoadMapContext.current_screen.south) {

            RoadMapContext.current_screen.south = position[i].latitude;
         }
         if (position[i].latitude > RoadMapContext.current_screen.north) {

            RoadMapContext.current_screen.north = position[i].latitude;
         }
      }
   }

   roadmap_math_release_focus ();
#ifdef DEBUG
roadmap_log (ROADMAP_ERROR, "visibility: north=%d south=%d east=%d west=%d\n",
        RoadMapContext.current_screen.north,
        RoadMapContext.current_screen.south,
        RoadMapContext.current_screen.east,
        RoadMapContext.current_screen.east);
#endif
   return status;
}


int  roadmap_math_get_orientation (void) {

   return RoadMapContext.orientation;
}


void roadmap_math_to_position (const RoadMapGuiPoint *point,
                               RoadMapPosition *position) {

   RoadMapGuiPoint point2;

   if (RoadMapContext.orientation) {

      point2 = *point;
      roadmap_math_counter_rotate_coordinate (&point2);
      point = &point2;
   }

   position->longitude =
      RoadMapContext.upright_screen.west
         + (point->x * RoadMapContext.zoom_x);

   position->latitude =
      RoadMapContext.upright_screen.north
         - (point->y * RoadMapContext.zoom_y);
}


void roadmap_math_coordinate (const RoadMapPosition *position,
                              RoadMapGuiPoint *point) {

   point->x =
      ((position->longitude - RoadMapContext.upright_screen.west)
             / RoadMapContext.zoom_x);

   point->y =
      ((RoadMapContext.upright_screen.north - position->latitude)
             / RoadMapContext.zoom_y);
}


int roadmap_math_azymuth
       (const RoadMapPosition *point1, const RoadMapPosition *point2) {

    int result;
    double x;
    double y;
    double d;


    x = RoadMapContext.units->unit_per_longitude
            * (point2->longitude - point1->longitude);
    y = RoadMapContext.units->unit_per_latitude
            * (point2->latitude  - point1->latitude);

    d = sqrt ((x * x) + (y * y));
    
    if (d > 0.0001 || d < -0.0001) {
        result = roadmap_math_arccosine
                    ((int) ((32768 * y) / d), (x > 0)?1:-1);
    } else {
        result = 0;
    }
    
    roadmap_log (ROADMAP_DEBUG,
                    "azymuth for (x=%f, y=%f): %d",
                    x, y, result);
    return result;
}


int roadmap_math_distance
       (const RoadMapPosition *position1, const RoadMapPosition *position2) {

   double x;
   double y;


   x = RoadMapContext.units->unit_per_longitude
          * (position1->longitude - position2->longitude);
   y = RoadMapContext.units->unit_per_latitude
          * (position1->latitude  - position2->latitude);

   return (int) sqrt ((x * x) + (y * y));
}


char *roadmap_math_distance_unit (void) {
    
    return RoadMapContext.units->length;
}


char *roadmap_math_trip_unit (void) {
    
    return RoadMapContext.units->trip_distance;
}


char *roadmap_math_speed_unit (void) {
    
    return RoadMapContext.units->speed;
}


int roadmap_math_to_trip_distance (int distance) {
    
    return distance / RoadMapContext.units->to_trip_unit;
}


int  roadmap_math_get_distance_from_segment
        (const RoadMapPosition *position,
         const RoadMapPosition *position1,
         const RoadMapPosition *position2,
               RoadMapPosition *intersection) {

   int distance;
   int minimum;

   double x1;
   double y1;
   double x2;
   double y2;
   double x3;
   double y3;


   /* Compute the coordinates relative to the "position" point. */

   x1 = RoadMapContext.units->unit_per_longitude
           * (position->longitude - position1->longitude);
   y1 = RoadMapContext.units->unit_per_latitude
           * (position->latitude  - position1->latitude);

   x2 = RoadMapContext.units->unit_per_longitude
           * (position->longitude - position2->longitude);
   y2 = RoadMapContext.units->unit_per_latitude
           * (position->latitude  - position2->latitude);


   /* Compute the coordinates of the intersection with the perpendicular. */

   if ((x1 - x2 > -1.0) && (x1 - x2 < 1.0)) {

      /* The two points are very close, or the line is quasi vertical:
       * approximating the line to a vertical one is good enough.
       */
      x3 = (x1 + x2) / 2;
      y3 = 0.0;

      intersection->longitude =
         (position1->longitude + position2->longitude) / 2;

   } else {
      
      /* Equation of the line: */

      double a = (y1 - y2) / (x1 - x2);
      double b = y1 - a * x1;

      /* The equation of the perpendicular is: y = - (x / a). */

      x3 = 0.0 - (b / (a + (1.0 / a)));
      y3 = b / ((a * a) + 1.0);

      intersection->longitude =
         position1->longitude
            + (int)(((x1 - x3)
                 * (position2->longitude - position1->longitude)) / (x1 - x2));
   }


   if ((((x1 >= x3) && (x3 >= x2)) || ((x1 <= x3) && (x3 <= x2))) &&
       (((y1 >= y3) && (y3 >= y2)) || ((y1 <= y3) && (y3 <= y2)))) {

      /* The intersection point is in the segment. */

      if ((y1 - y2 > -1.0) && (y1 - y2 < 1.0)) {

         intersection->latitude =
            (position1->latitude + position2->latitude) / 2;

      } else {

         intersection->latitude =
            position1->latitude
               + (int)(((y1 - y3)
                   * (position2->latitude - position1->latitude)) / (y1 - y2));
      }

      return (int) sqrt ((x3 * x3) + (y3 * y3));
   }

   /* The intersection point is not in the segment: use the distance to
    * the closest segment's endpoint.
    */
   minimum  = (int) sqrt ((x1 * x1) + (y1 * y1));
   distance = (int) sqrt ((x2 * x2) + (y2 * y2));

   if (distance < minimum) {
      *intersection = *position2;
      return distance;
   }

   *intersection = *position1;
   return minimum;
}


int roadmap_math_to_speed_unit (int knots) {
    
    return (int) (knots * RoadMapContext.units->speed_per_knot);
}


int roadmap_math_to_current_unit (int value, const char *unit) {

    if (strcasecmp (unit, "cm") == 0) {

        static int PreviousValue = 0;
        static int PreviousResult = 0;

        if (value != PreviousValue) {
            PreviousResult = (int) (RoadMapContext.units->cm_to_unit * value);
            PreviousValue = value;
        }
        return PreviousResult;
    }

    roadmap_log (ROADMAP_ERROR, "unsupported unit '%s'", unit);
    return value;
}


void roadmap_math_screen_edges (RoadMapArea *area) {

   *area = RoadMapContext.current_screen;
}


int  roadmap_math_street_address (const char *image, int length) {

   int i;
   int digit;
   int result = 0;

   if ((length > 8) &&
       ((image[0] == 'W') || (image[0] == 'E') ||
        (image[0] == 'S') || (image[0] == 'N'))) {

       /* This is a very special case: the street number is organized
        * like a geographical position (longitude/latitude). Skip
        * The west/north flags and format the number accordingly.
        */
       int separator = 0;
       int part1 = 0;
       int part2 = 0;
       int multiplier = 10;


       for (i = length - 1; i > 0; --i) {

          if ((image[i] == 'W') || (image[i] == 'E') ||
              (image[i] == 'S') || (image[i] == 'N') || (image[i] == '-')) {

             separator = i;
          }
       }

       for (i = 1; i < separator; ++i) {
          if (image[i] < '0' || image[i] > '9') {
             roadmap_log
                (ROADMAP_ERROR, "bad numerical character %c", image[i]);
          }
          part1 = (part1 * 10) + (image[i] - '0');
       }

       for (i = separator + 1; i < length; ++i) {
          if (image[i] < '0' || image[i] > '9') {
             roadmap_log
                (ROADMAP_ERROR, "bad numerical character %c", image[i]);
          }
          part2 = (part2 * 10) + (image[i] - '0');
          multiplier *= 10;
       }

       return (part1 * multiplier) + part2;
   }

   for (i = 0; i < length; i++) {

      digit = image[i];

      if ((digit < '0') || (digit > '9')) {

         if (length >= 9) {
            continue;
         } else if (digit == '-' || digit == ' ') {
            continue;
         } else if (digit >= 'A' && digit <= 'Z') {
            digit = '1' + digit - 'A';
         } else if (digit >= 'a' && digit <= 'z') {
            digit = '1' + digit - 'a';
         } else  {
            roadmap_log (ROADMAP_ERROR, "bad numerical character %c", digit);
            continue;
         }
      }
      result = (result * 10) + (digit - '0');
   }

   return result;
}


int roadmap_math_intersection (RoadMapPosition *from1,
                               RoadMapPosition *to1,
                               RoadMapPosition *from2,
                               RoadMapPosition *to2,
                               RoadMapPosition *intersection) {

   double a1,b1;
   double a2,b2;

   if (from1->longitude == to1->longitude) {

      a1 = 0;
      b1 = from1->latitude;
   } else {
      a1 = 1.0 * (from1->latitude - to1->latitude) /
         (from1->longitude - to1->longitude);
      b1 = from1->latitude - 1.0 * a1 * from1->longitude;
   }

   if ((from2->longitude - to2->longitude) == 0) {

      a2 = 0;
      b2 = from2->latitude;
   } else {
      a2 = 1.0 * (from2->latitude - to2->latitude) /
         (from2->longitude - to2->longitude);
      b2 = from2->latitude - 1.0 * a2 * from2->longitude;
   }

   if (a1 == a2) return 0;

   intersection->longitude = (int) ((b1 - b2) / (a2 - a1));
   intersection->latitude = (int) (b1 + intersection->longitude * a1);

   return 1;
}


int roadmap_math_compare_points (const RoadMapPosition *p1,
                                 const RoadMapPosition *p2) {

   if ((p1->longitude == p2->longitude) &&
         (p1->latitude == p2->latitude))
      return 0;

   if (p1->longitude < p2->longitude) return -1;
   if (p2->longitude > p1->longitude) return 1;

   if (p1->latitude < p2->latitude) {
      return -1;
   } else {
      return 1;
   }
}


int roadmap_math_delta_direction (int direction1, int direction2) {

    int delta = direction2 - direction1;

    while (delta > 180)  delta -= 360;
    while (delta < -180) delta += 360;

    if (delta < 0) delta = 0 - delta;

    while (delta >= 360) delta -= 360;

    return delta;
}


/* Take a number followed by ft/mi/m/km, and converts it to current units. */
int roadmap_math_distance_convert(const char *string, int *explicit)
{
    char *suffix;
    double distance;
    RoadMapUnits *my_units, *other_units;
    int had_units = 1;

    my_units = RoadMapContext.units;
    if (my_units == &RoadMapMetricSystem) {
        other_units = &RoadMapImperialSystem;
    } else {
        other_units = &RoadMapMetricSystem;
    }

    distance = strtol (string, &suffix, 10);

    while (*suffix && isspace(*suffix)) suffix++;

    if (*suffix) {
        if (0 == strcasecmp(suffix, my_units->length)) {
            /* Nothing to do, hopefully this is the usual case. */
        } else if (0 == strcasecmp(suffix, my_units->trip_distance)) {
            distance *= my_units->to_trip_unit;
        } else if (0 == strcasecmp(suffix, other_units->length)) {
            distance /= other_units->cm_to_unit;
            distance *= my_units->cm_to_unit;
        } else if (0 == strcasecmp(suffix, other_units->trip_distance)) {
            distance *= other_units->to_trip_unit;
            distance /= other_units->cm_to_unit;
            distance *= my_units->cm_to_unit;
        } else {
            roadmap_log (ROADMAP_WARNING, 
                "dropping unknown units '%s' from '%s'", suffix, string);
            had_units = 0;
        }
    } else {
        had_units = 0;
    }

    if (explicit) *explicit = had_units;

    return (int)distance;
}

