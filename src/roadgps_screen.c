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
#include "roadmap_nmea.h"

#include "roadmap_canvas.h"
#include "roadgps_screen.h"


#define ROADGPS_STATUS_OFF    0
#define ROADGPS_STATUS_FIXING 1
#define ROADGPS_STATUS_OK     2

typedef struct {

   char label[4];
   int  id;
   int  azimuth;
   int  elevation;
   int  strength;
   int  status;

} RoadMapObject;

#define ROADGPS_SATELLITE_COUNT  12

static RoadMapObject RoadGpsSatellites[ROADGPS_SATELLITE_COUNT];
static char          RoadGpsActiveSatellites[ROADGPS_SATELLITE_COUNT];
static int           RoadGpsSatelliteCount = 0;

struct {

   int scale_line_counts[5];
   RoadMapGuiPoint scale_line_points[10];

   int radius[2];
   RoadMapGuiPoint centers[2];

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


static RoadMapNmeaListener RoadGpsNextGpgsv = NULL;
static RoadMapNmeaListener RoadGpsNextGpgsa = NULL;


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
               (RoadMapObject *satellite, int reverse) {

   RoadMapPen foreground;
   RoadMapPen background;

   double   scale = 0.0;
   double   azimuth;

   RoadMapGuiPoint position;

   int count;
   RoadMapGuiPoint points[4];


   if (satellite->strength <= 0) return;

   scale = RoadGpsFrame.position_scale
                * sin((90 - satellite->elevation) * DEGRE2RADIAN);

   azimuth = satellite->azimuth * DEGRE2RADIAN;

   position.x = RoadGpsFrame.centers[0].x + (short) (sin(azimuth) * scale);
   position.y = RoadGpsFrame.centers[0].y - (short) (cos(azimuth) * scale);

   roadgps_screen_set_rectangle
      (position.x - RoadGpsFrame.label_offset_x - 2,
       position.y - RoadGpsFrame.label_offset_y - 3,
       RoadGpsFrame.label_width + 4,
       RoadGpsFrame.label_ascent + 6,
       points);
   count = 4;

   if (reverse) {

      foreground = RoadGpsEraser;
      background = RoadGpsForeground;

   } else {

      background = RoadGpsEraser;
      foreground = RoadGpsForeground;
   }

   roadmap_canvas_select_pen (background);
   roadmap_canvas_draw_multiple_polygons (1, &count, points, 1);

   roadmap_canvas_select_pen (foreground);
   roadmap_canvas_draw_multiple_polygons (1, &count, points, 0);

   roadmap_canvas_draw_string
      (&position, ROADMAP_CANVAS_CENTER, satellite->label);
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
      (2, RoadGpsFrame.centers, RoadGpsFrame.radius, 0);

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

   for (i = 0; i < RoadGpsSatelliteCount; ++i) {

      switch (RoadGpsSatellites[i].status) {

      case ROADGPS_STATUS_FIXING:

         roadgps_screen_draw_satellite_position (RoadGpsSatellites+i, 0);
         roadgps_screen_draw_satellite_signal (i, 0);
         break;

      case ROADGPS_STATUS_OK:

         roadgps_screen_draw_satellite_position (RoadGpsSatellites+i, 1);
         roadgps_screen_draw_satellite_signal (i, 1);
         break;

      default:

         break; /* Do not show. */

      }
   }

   roadmap_canvas_refresh ();
}


static void roadgps_screen_gpgsa
               (void *context, const RoadMapNmeaFields *fields) {

   int i;

   for (i = 0; i < ROADGPS_SATELLITE_COUNT; i += 1) {

      RoadGpsActiveSatellites[i] = fields->gpgsa.satellite[i];
   }

   (*RoadGpsNextGpgsa) (context, fields);
}


static void roadgps_screen_gpgsv
               (void *context, const RoadMapNmeaFields *fields) {

   int i;
   int index;

   for (i = 0, index = (fields->gpgsv.index - 1) * 4;
        i < 4 && index < fields->gpgsv.count;
        i += 1, index += 1) {

      sprintf (RoadGpsSatellites[index].label,
               "%02d", fields->gpgsv.satellite[i]);

      RoadGpsSatellites[index].id        = fields->gpgsv.satellite[i];
      RoadGpsSatellites[index].elevation = fields->gpgsv.elevation[i];
      RoadGpsSatellites[index].azimuth   = fields->gpgsv.azimuth[i];
      RoadGpsSatellites[index].strength  = fields->gpgsv.strength[i];

      RoadGpsSatellites[index].status  = ROADGPS_STATUS_FIXING;
   }

   if (fields->gpgsv.index == fields->gpgsv.total) {

      RoadGpsSatelliteCount = fields->gpgsv.count;

      for (index = 0; index < RoadGpsSatelliteCount; index += 1) {

         for (i = 0; i < ROADGPS_SATELLITE_COUNT; i += 1) {

            if (RoadGpsActiveSatellites[i] == RoadGpsSatellites[index].id) {
               RoadGpsSatellites[index].status = ROADGPS_STATUS_OK;
               break;
            }
         }
      }

      for (index = RoadGpsSatelliteCount;
           index < ROADGPS_SATELLITE_COUNT;
           index += 1) {
         RoadGpsSatellites[index].status = ROADGPS_STATUS_OFF;
      }

      roadgps_screen_draw ();
   }

   (*RoadGpsNextGpgsv) (context, fields);
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

   RoadGpsFrame.centers[0].x = RoadGpsFrame.centers[1].x = canvas_width / 2;
   RoadGpsFrame.centers[0].y = RoadGpsFrame.centers[1].y = size / 2;

   RoadGpsFrame.radius[0] = (size * 2) / 6;
   RoadGpsFrame.radius[1] = size / 6;

   RoadGpsFrame.position_scale = ((size * 3) / 8);


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
   RoadGpsFrame.label_offset_y = text_ascent / 2;


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
      roadmap_canvas_set_foreground (roadmap_config_get ("GPS", "Background"));

      RoadGpsForeground = roadmap_canvas_create_pen ("gps/foreground");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground (roadmap_config_get ("GPS", "Foreground"));

      RoadGpsFramePen = roadmap_canvas_create_pen ("gps/frame");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground ("grey");
   }
   roadgps_screen_format_frame ();
   roadgps_screen_draw ();
}


void roadgps_screen_initialize (void) {

   roadmap_config_declare ("preferences", "GPS", "Background", "LightYellow");
   roadmap_config_declare ("preferences", "GPS", "Foreground", "black");

   if (RoadGpsNextGpgsa == NULL) {
      RoadGpsNextGpgsa =
         roadmap_nmea_subscribe ("GPGSA", roadgps_screen_gpgsa);
   }

   if (RoadGpsNextGpgsv == NULL) {
      RoadGpsNextGpgsv =
         roadmap_nmea_subscribe ("GPGSV", roadgps_screen_gpgsv);
   }

   roadmap_canvas_register_configure_handler (&roadgps_screen_configure);
}

