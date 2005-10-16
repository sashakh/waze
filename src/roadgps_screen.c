/* roadgps_screen.c - draw a graphical gps console on the screen.
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
 *   See roadgps_screen.h.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_gui.h"
#include "roadmap_gps.h"

#include "roadmap_canvas.h"
#include "roadgps_screen.h"


static RoadMapConfigDescriptor RoadMapConfigGPSBackground =
                        ROADMAP_CONFIG_ITEM("GPS", "Background");

static RoadMapConfigDescriptor RoadMapConfigGPSForeground =
                        ROADMAP_CONFIG_ITEM("GPS", "Foreground");


typedef struct {

   char  label[4];
   char  id;
   char  status;
   short azimuth;
   short elevation;
   short strength;

   RoadMapGuiPoint position;
   RoadMapGuiPoint points[4];

} RoadGpsObject;


static RoadGpsObject *RoadGpsSatellites = NULL;
static int            RoadGpsSatelliteSize  = 0;
static int            RoadGpsSatelliteCount = 0;


struct {

   int scale_line_counts[5];
   RoadMapGuiPoint scale_line_points[10];

   int radius[3];
   RoadMapGuiPoint centers[3];

   int position_scale;

   int label_width;
   int label_height;
   int label_ascent;
   int label_descent;

   int label_bottom;

   int label_offset_x;
   int label_offset_y;

   int level_bar_height;
   int level_bar_bottom;

} RoadGpsFrame;

RoadMapPen RoadGpsEraser;
RoadMapPen RoadGpsFramePen;
RoadMapPen RoadGpsForeground;


#define DEGRE2RADIAN (3.141592653589793116/180.0)

#define ROADGPS_STRENGTH_DEFAULT_SCALE 50


static int RoadGpsStrengthScale;


static void roadgps_screen_set_rectangle
               (int x, int y, int width, int height, RoadMapGuiPoint *points) {

   points[0].x = x;
   points[0].y = y;
   points[1].x = x + width;
   points[1].y = y;
   points[2].x = x + width;
   points[2].y = y + height;
   points[3].x = x;
   points[3].y = y + height;
}


static void roadgps_screen_draw_satellite_position
               (RoadGpsObject *satellite, int reverse) {

   double   scale = 0.0;
   double   azimuth;

   int count;
   int strength;


   if (!satellite->elevation) return;

   /* transforme the strength into a relative value. */
   strength = (satellite->strength * 10) / RoadGpsStrengthScale;

   /* Transform the relative value into a rectangle width (linear
    * transformation into the interval label_width to 2*label_width).
    */
   strength = (RoadGpsFrame.label_width * strength) / 10
                   + RoadGpsFrame.label_width + 6;

   scale = RoadGpsFrame.position_scale * (90 - satellite->elevation) / 90;

   azimuth = satellite->azimuth * DEGRE2RADIAN;

   satellite->position.x =
      RoadGpsFrame.centers[0].x + (short) (sin(azimuth) * scale);
   satellite->position.y =
      RoadGpsFrame.centers[0].y - (short) (cos(azimuth) * scale);

   if (strength == 0) {

      roadgps_screen_set_rectangle
         (satellite->position.x - RoadGpsFrame.label_offset_x - 2,
          satellite->position.y - RoadGpsFrame.label_offset_y - 3,
          RoadGpsFrame.label_width + 4,
          RoadGpsFrame.label_ascent + RoadGpsFrame.label_descent + 6,
          satellite->points);
   } else {
      roadgps_screen_set_rectangle
         (satellite->position.x - (strength / 2),
          satellite->position.y - (strength / 2),
          strength,
          strength,
          satellite->points);
   }
   count = 4;

   if (strength > 0) {
      roadmap_canvas_select_pen (RoadGpsForeground);
      roadmap_canvas_draw_multiple_polygons
         (1, &count, satellite->points, !!reverse);
   }
}


static void roadgps_screen_draw_satellite_label
               (RoadGpsObject *satellite, int reverse) {

   int count = 4;

   roadmap_canvas_select_pen (reverse?RoadGpsEraser:RoadGpsForeground);

   roadmap_canvas_draw_string
      (&satellite->position, ROADMAP_CANVAS_CENTER, satellite->label);
}


static void roadgps_screen_draw_satellite_signal (int satellite, int filled) {

   int step;
   int height;
   int strength;

   int count;
   RoadMapGuiPoint position;
   RoadMapGuiPoint points[6];


   if (RoadGpsSatelliteCount <= 0) return;

   roadmap_canvas_select_pen (RoadGpsForeground);

   step = roadmap_canvas_width() / RoadGpsSatelliteCount;

   position.x = (satellite * step) + (step / 2);
   position.y = RoadGpsFrame.label_bottom;

   roadmap_canvas_draw_string
      (&position, ROADMAP_CANVAS_CENTER, RoadGpsSatellites[satellite].label);

   if (RoadGpsSatellites[satellite].strength == 0) return;


   strength =
      ((RoadGpsSatellites[satellite].strength * 100) / RoadGpsStrengthScale);

   height = (RoadGpsFrame.level_bar_height * strength) / 100;

   roadgps_screen_set_rectangle
      (position.x - RoadGpsFrame.label_offset_x,
       RoadGpsFrame.level_bar_bottom - height,
       RoadGpsFrame.label_width, height,
       points);

   count = 4;
   roadmap_canvas_draw_multiple_polygons (1, &count, points, filled);
}


static void roadgps_screen_draw_frame (void) {

   roadmap_canvas_select_pen (RoadGpsFramePen);

   roadmap_canvas_draw_multiple_circles
      (3, RoadGpsFrame.centers, RoadGpsFrame.radius, 0);

   roadmap_canvas_draw_multiple_lines
      (5, RoadGpsFrame.scale_line_counts, RoadGpsFrame.scale_line_points);
}


static void roadgps_screen_draw (void) {

   int i;
   

   /* Paint a "clean" screen. */

   roadmap_canvas_select_pen (RoadGpsEraser);
   roadmap_canvas_erase ();

   roadgps_screen_draw_frame ();


   /* Paint the satellites. */

   RoadGpsStrengthScale = ROADGPS_STRENGTH_DEFAULT_SCALE;

   for (i = 0; i < RoadGpsSatelliteCount; ++i) {

      if (RoadGpsSatellites[i].status > 0) {
         if (RoadGpsSatellites[i].strength > RoadGpsStrengthScale) {
            RoadGpsStrengthScale =
               ((RoadGpsSatellites[i].strength / 10) + 1) * 10;
         }
      }
   }

   /* First show all the polygons (background of the satellites),
    * then all the labels (foreground of the satellites).
    */
   for (i = 0; i < RoadGpsSatelliteCount; ++i) {

      switch (RoadGpsSatellites[i].status) {

      case 'F':

         roadgps_screen_draw_satellite_position (RoadGpsSatellites+i, 0);
         roadgps_screen_draw_satellite_signal (i, 0);
         break;

      case 'A':

         roadgps_screen_draw_satellite_position (RoadGpsSatellites+i, 1);
         roadgps_screen_draw_satellite_signal (i, 1);
         break;

      default:

         break; /* Do not show. */

      }
   }

   for (i = 0; i < RoadGpsSatelliteCount; ++i) {

      switch (RoadGpsSatellites[i].status) {

      case 'F':

         roadgps_screen_draw_satellite_label (RoadGpsSatellites+i, 0);
         break;

      case 'A':

         roadgps_screen_draw_satellite_label (RoadGpsSatellites+i, 1);
         break;

      default:

         break; /* Do not show. */

      }
   }

   roadmap_canvas_refresh ();
}


static void roadgps_screen_monitor
               (const RoadMapGpsPrecision *precision,
                const RoadMapGpsSatellite *satellites,
                int count) {

   int i;

   if (RoadGpsSatelliteSize < count) {

      RoadGpsSatellites = (RoadGpsObject *)
         realloc (RoadGpsSatellites, sizeof(RoadGpsObject) * count);
      roadmap_check_allocated(RoadGpsSatellites);

      RoadGpsSatelliteSize = count;
   }

   for (i = 0; i < count; ++i) {

      sprintf (RoadGpsSatellites[i].label, "%02d", satellites[i].id);

      RoadGpsSatellites[i].id        = satellites[i].id;
      RoadGpsSatellites[i].elevation = satellites[i].elevation;
      RoadGpsSatellites[i].azimuth   = satellites[i].azimuth;
      RoadGpsSatellites[i].strength  = satellites[i].strength;
      RoadGpsSatellites[i].status    = satellites[i].status;
   }

   RoadGpsSatelliteCount = count;

   roadgps_screen_draw ();
}


static void roadgps_screen_format_frame (void) {

   int i;
   int size;
   int step;

   int canvas_width;
   int canvas_height;

   int text_width;
   int text_ascent;
   int text_descent;
   int text_height;


   canvas_width  = roadmap_canvas_width();
   canvas_height = roadmap_canvas_height();

   roadmap_canvas_get_text_extents
       ("09", &text_width, &text_ascent, &text_descent);

   text_height = text_ascent + text_descent + 2;

   size = (canvas_height * 2) / 3;

   if (size > canvas_width) {
      size = canvas_width;
   }

   /* Format the two circles used as frame for the satellites' position. */

   RoadGpsFrame.centers[0].x =
      RoadGpsFrame.centers[1].x =
      RoadGpsFrame.centers[2].x = canvas_width / 2;
   RoadGpsFrame.centers[0].y =
      RoadGpsFrame.centers[1].y =
      RoadGpsFrame.centers[2].y = size / 2;

   RoadGpsFrame.radius[0] = (size * 3) / 6;
   RoadGpsFrame.radius[1] = (size * 2) / 6;
   RoadGpsFrame.radius[2] = size / 6;

   RoadGpsFrame.position_scale = ((size * 4) / 8);


   /* Pre-compute the size and position of the satellite labels. */

   RoadGpsFrame.label_width   = text_width;
   RoadGpsFrame.label_height  = text_height;
   RoadGpsFrame.label_ascent  = text_ascent;
   RoadGpsFrame.label_descent = text_descent;

   RoadGpsFrame.label_bottom = canvas_height - (text_height / 2);

   RoadGpsFrame.level_bar_height = (canvas_height / 3) - text_height - 2; 
   RoadGpsFrame.level_bar_bottom = 
      ((canvas_height * 2) / 3) + RoadGpsFrame.level_bar_height;

   RoadGpsFrame.label_offset_x = text_width / 2;
   RoadGpsFrame.label_offset_y = (text_descent + text_ascent) / 2;


   /* Format the signal strength scale lines. */

   step = ((canvas_height / 3) - text_height) / 5;

   for (i = 0; i < 5; ++i) {
      RoadGpsFrame.scale_line_counts[i] = 2;
      RoadGpsFrame.scale_line_points[2*i].x = 2;
      RoadGpsFrame.scale_line_points[2*i+1].x = canvas_width - 4;
      RoadGpsFrame.scale_line_points[2*i].y =
         RoadGpsFrame.scale_line_points[2*i+1].y =
             canvas_height - (text_height + (i * step)) - 2;
   }
}


static void roadgps_screen_configure (void) {

   static int Initialized = 0;

   if (! Initialized) {

      RoadGpsEraser = roadmap_canvas_create_pen ("eraser");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSBackground));

      RoadGpsForeground = roadmap_canvas_create_pen ("gps/foreground");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSForeground));

      RoadGpsFramePen = roadmap_canvas_create_pen ("gps/frame");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground ("grey");
   }
   roadgps_screen_format_frame ();
   roadgps_screen_draw ();
}


void roadgps_screen_initialize (void) {

   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSBackground, "LightYellow");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSForeground, "black");

   roadmap_gps_register_monitor (roadgps_screen_monitor);

   roadmap_canvas_register_configure_handler (&roadgps_screen_configure);
}

