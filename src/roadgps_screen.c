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
#include <time.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_gui.h"
#include "roadmap_gps.h"

#include "roadmap_math.h"
#include "roadmap_main.h"
#include "roadmap_start.h"
#include "roadmap_canvas.h"
#include "roadgps_screen.h"

static RoadMapConfigDescriptor RoadMapConfigGPSBackground =
                        ROADMAP_CONFIG_ITEM("GPS", "Background");

static RoadMapConfigDescriptor RoadMapConfigGPSForeground =
                        ROADMAP_CONFIG_ITEM("GPS", "Foreground");

static RoadMapConfigDescriptor RoadMapConfigGPSActive =
                        ROADMAP_CONFIG_ITEM("GPS", "Active");

static RoadMapConfigDescriptor RoadMapConfigGPSActiveFill =
                        ROADMAP_CONFIG_ITEM("GPS", "ActiveFill");

static RoadMapConfigDescriptor RoadMapConfigGPSActiveLabels =
                        ROADMAP_CONFIG_ITEM("GPS", "ActiveLabels");

static RoadMapConfigDescriptor RoadMapConfigGPSInactive =
                        ROADMAP_CONFIG_ITEM("GPS", "Inactive");

static RoadMapConfigDescriptor RoadMapConfigGPSInactiveFill =
                        ROADMAP_CONFIG_ITEM("GPS", "InactiveFill");

static RoadMapConfigDescriptor RoadMapConfigGPSInactiveLabels =
                        ROADMAP_CONFIG_ITEM("GPS", "InactiveLabels");

static RoadMapConfigDescriptor RoadMapConfigGPSLabels =
                        ROADMAP_CONFIG_ITEM("GPS", "Labels");

static RoadMapConfigDescriptor RoadMapConfigGPSValues =
                        ROADMAP_CONFIG_ITEM("GPS", "Values");

static RoadMapConfigDescriptor RoadMapConfigGPSFontSize =
                        ROADMAP_CONFIG_ITEM("GPS", "Font Size");


typedef struct {

   char  label[4];
   char  id;
   char  status;
   short azimuth;
   short elevation;
   short strength;

   RoadMapGuiPoint position;

} RoadGpsObject;


static RoadGpsObject *RoadGpsSatellites = NULL;
static int            RoadGpsSatelliteSize  = 0;
static int            RoadGpsSatelliteCount = 0;

static RoadMapGpsPosition RoadGpsPosition;
static RoadMapGpsPrecision RoadGpsPrecision;

static time_t RoadMapGpsReceivedTime = 0;

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

   int longitude_offset_x;
   int longitude_offset_y;
   int latitude_offset_x;
   int latitude_offset_y;

} RoadGpsFrame;

RoadMapPen RoadGpsEraser;
RoadMapPen RoadGpsFramePen;
RoadMapPen RoadGpsForeground;
RoadMapPen RoadGpsActive;
RoadMapPen RoadGpsActiveFill;
RoadMapPen RoadGpsActiveLabels;
RoadMapPen RoadGpsInactive;
RoadMapPen RoadGpsInactiveFill;
RoadMapPen RoadGpsInactiveLabels;
RoadMapPen RoadGpsLabels;
RoadMapPen RoadGpsValues;

int RoadMapGPSFontSize;


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
   RoadMapGuiPoint centers[1];
   int radius[1];


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
     radius[0] = RoadGpsFrame.label_width + 4 / 2;
   } else {
     radius[0] = strength / 2;
   }
   centers[0].x = satellite->position.x;
   centers[0].y = satellite->position.y;

   count = 4;

   if (strength > 0) {
     radius[0]=radius[0]-1;
     roadmap_canvas_select_pen (reverse?RoadGpsInactiveFill:RoadGpsActiveFill);
     roadmap_canvas_draw_multiple_circles (1, centers, radius, 1, 0);
     radius[0]=radius[0]+1;
     roadmap_canvas_select_pen (reverse?RoadGpsInactive:RoadGpsActive);
     roadmap_canvas_draw_multiple_circles (1, centers, radius, 0, 0);
   }

}


static void roadgps_screen_draw_satellite_label
               (RoadGpsObject *satellite, int reverse) {

   roadmap_canvas_select_pen (reverse ?
      RoadGpsInactiveLabels : RoadGpsActiveLabels);

   roadmap_canvas_draw_string
      (&satellite->position, ROADMAP_CANVAS_CENTER,
        RoadMapGPSFontSize, satellite->label);
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

   roadmap_canvas_select_pen (filled ?
      RoadGpsActiveLabels : RoadGpsInactiveLabels);
   roadmap_canvas_draw_string
      (&position, ROADMAP_CANVAS_CENTER,
         RoadMapGPSFontSize, RoadGpsSatellites[satellite].label);

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
   roadmap_canvas_select_pen (filled?RoadGpsActiveFill:RoadGpsInactiveFill);
   roadmap_canvas_draw_multiple_polygons (1, &count, points, 1, 0);
   roadmap_canvas_select_pen (filled?RoadGpsActive:RoadGpsInactive);
   roadmap_canvas_draw_multiple_polygons (1, &count, points, 0, 0);
}

void roadgps_screen_to_coord(char data[], int islatitude, int value) {

  data[0] = "EWNS"[(islatitude * 2)  + (value < 0)];
  
  roadmap_math_to_floatstring(data + 1, value, MILLIONTHS);
}

static void roadgps_screen_draw_position (void) {

  char data[100];
  char timebuf[100];

  RoadMapGuiPoint point;
  int satcount,i;

  satcount = 0;

  for (i = 0; i < RoadGpsSatelliteCount; ++i) 
     if (RoadGpsSatellites[i].status == 'A') 
        satcount++;


  point.x = RoadGpsFrame.latitude_offset_x;
  point.y = RoadGpsFrame.latitude_offset_y;
  roadmap_canvas_select_pen (RoadGpsLabels);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, "Latitude:");
  point.y = point.y+RoadGpsFrame.label_height;

  roadgps_screen_to_coord(data,1,RoadGpsPosition.latitude);
  roadmap_canvas_select_pen (RoadGpsValues);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, data);

  point.x = RoadGpsFrame.longitude_offset_x;
  point.y = RoadGpsFrame.longitude_offset_y;

  roadmap_canvas_select_pen (RoadGpsLabels);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, "Longitude:");
  point.y = point.y+RoadGpsFrame.label_height;

  roadgps_screen_to_coord(data,0,RoadGpsPosition.longitude);
  roadmap_canvas_select_pen (RoadGpsValues);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, data);

  point.y = point.y+RoadGpsFrame.label_height;
  roadmap_canvas_select_pen (RoadGpsLabels);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, "Altitude:");
  sprintf(data,"%d%s",RoadGpsPosition.altitude,roadmap_math_distance_unit());
  point.y = point.y+RoadGpsFrame.label_height;
  roadmap_canvas_select_pen (RoadGpsValues);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, data);

  point.y = point.y+RoadGpsFrame.label_height;
  roadmap_canvas_select_pen (RoadGpsLabels);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, "Correction:");
  if (RoadGpsPrecision.dimension<2) {
    sprintf(data,"%s","None");
  } else {
    sprintf(data,"%s",RoadGpsPrecision.dimension==2?"2D fix":"3D fix");
  }
  point.y = point.y+RoadGpsFrame.label_height;
  roadmap_canvas_select_pen (RoadGpsValues);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, data);

  point.y = point.y+RoadGpsFrame.label_height;
  roadmap_canvas_select_pen (RoadGpsLabels);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, "Speed:");
  sprintf(data,"%d%s",
     roadmap_math_knots_to_speed_unit(RoadGpsPosition.speed),
     roadmap_math_speed_unit());
  point.y = point.y+RoadGpsFrame.label_height;
  roadmap_canvas_select_pen (RoadGpsValues);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, data);

  point.x = RoadGpsFrame.centers[0].x-RoadGpsFrame.radius[0];
  point.y = RoadGpsFrame.centers[0].y+RoadGpsFrame.radius[0]+
            RoadGpsFrame.label_height-10;
  roadmap_canvas_select_pen (RoadGpsLabels);
  sprintf(data,"Used satellites: %d",satcount);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, data);

  point.y = point.y+RoadGpsFrame.label_height;
  roadmap_canvas_select_pen (RoadGpsLabels);
  if (RoadMapGpsReceivedTime != 0) {
     strftime(timebuf, sizeof(timebuf),
                "%Y/%m/%d %H:%M:%S", localtime(&RoadMapGpsReceivedTime));
  } else {
     sprintf(timebuf, "N/A");
  }
  sprintf(data,"Time: %s", timebuf);
  roadmap_canvas_draw_string
      (&point, ROADMAP_CANVAS_LEFT, RoadMapGPSFontSize, data);

}

static void roadgps_screen_draw_frame (void) {

   RoadMapGuiPoint points[4];  
   int count = 4;
   int radius[1];

   roadmap_canvas_select_pen (RoadGpsForeground);

   radius[0] = RoadGpsFrame.radius[0];
   roadmap_canvas_draw_multiple_circles
      (1, RoadGpsFrame.centers, radius, 1, 0);


   roadmap_canvas_select_pen (RoadGpsFramePen);

   roadmap_canvas_draw_multiple_circles
      (3, RoadGpsFrame.centers, RoadGpsFrame.radius, 0, 0);

   roadmap_canvas_select_pen (RoadGpsForeground);
   roadmap_canvas_set_foreground
       (roadmap_config_get (&RoadMapConfigGPSForeground));


   roadgps_screen_set_rectangle(RoadGpsFrame.scale_line_points[0].x,
                                  RoadGpsFrame.scale_line_points[9].y,
                                  RoadGpsFrame.scale_line_points[1].x,
                                  RoadGpsFrame.scale_line_points[0].y,
                                  points);
   roadmap_canvas_draw_multiple_polygons
                     (1, &count, points, 1, 0);

   roadmap_canvas_select_pen (RoadGpsFramePen);

   roadmap_canvas_draw_multiple_lines
      (5, RoadGpsFrame.scale_line_counts, RoadGpsFrame.scale_line_points, 0);
}


void roadgps_screen_draw (void) {

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

         roadgps_screen_draw_satellite_position (RoadGpsSatellites+i, 1);
         roadgps_screen_draw_satellite_signal (i, 0);
         break;

      case 'A':

         roadgps_screen_draw_satellite_position (RoadGpsSatellites+i, 0);
         roadgps_screen_draw_satellite_signal (i, 1);
         break;

      default:

         break; /* Do not show. */

      }
   }

   for (i = 0; i < RoadGpsSatelliteCount; ++i) {

      switch (RoadGpsSatellites[i].status) {

      case 'F':

         roadgps_screen_draw_satellite_label (RoadGpsSatellites+i, 1);
         break;

      case 'A':

         roadgps_screen_draw_satellite_label (RoadGpsSatellites+i, 0);
         break;

      default:

         break; /* Do not show. */

      }
   }

   roadgps_screen_draw_position();
   roadmap_canvas_refresh ();
}

static void roadgps_screen_listener
                  (int reception,
                   int gps_time,
                   const RoadMapGpsPrecision *dilution,
                   const RoadMapGpsPosition *position) {

  
  RoadGpsPosition.latitude = position->latitude;
  RoadGpsPosition.longitude = position->longitude;
  RoadGpsPosition.altitude = position->altitude;    // meters
  RoadGpsPosition.speed = position->speed;          // knots
  RoadGpsPosition.steering = position->steering;

  RoadGpsPrecision.dimension = dilution->dimension;
  RoadGpsPrecision.dilution_position = dilution->dilution_position;
  RoadGpsPrecision.dilution_horizontal = dilution->dilution_horizontal;
  RoadGpsPrecision.dilution_vertical = dilution->dilution_vertical;

  RoadMapGpsReceivedTime = gps_time;

  roadmap_start_request_repaint();

}


static void roadgps_screen_monitor
               (int reception,
                const RoadMapGpsPrecision *precision,
                const RoadMapGpsSatellite *satellites,
                int activecount,
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

   roadmap_start_request_repaint();
}


static void roadgps_screen_format_frame (void) {

   int i;
   int size;
   int step;
   int sizew;

   int canvas_width;
   int canvas_height;

   int text_width;
   int text_ascent;
   int text_descent;
   int text_height;


   canvas_width  = roadmap_canvas_width();
   canvas_height = roadmap_canvas_height();

   roadmap_canvas_get_text_extents
       ("09", RoadMapGPSFontSize, &text_width,
         &text_ascent, &text_descent, NULL);

   text_height = text_ascent + text_descent + 2;

   /* vertical space is 2/3 screen minus two lines of text */
   size = (canvas_height * 2) / 3 - (text_height * 2);
   /* horizontal space is room for 12 characters */
   sizew = canvas_width - text_width*6;

   if (size > sizew) {
      size = sizew;
   }

   /* Format the two circles used as frame for the satellites' position. */

   RoadGpsFrame.centers[0].x =
      RoadGpsFrame.centers[1].x =
      RoadGpsFrame.centers[2].x = sizew / 2;
   RoadGpsFrame.centers[0].y =
      RoadGpsFrame.centers[1].y =
      RoadGpsFrame.centers[2].y = size / 2;

   RoadGpsFrame.radius[0] = (size * 3) / 6;
   RoadGpsFrame.radius[1] = (size * 2) / 6;
   RoadGpsFrame.radius[2] = size / 6;

   RoadGpsFrame.position_scale = ((size * 4) / 8);

   RoadGpsFrame.latitude_offset_x = sizew + text_width /2;
   RoadGpsFrame.longitude_offset_x = sizew + text_width/2;
   RoadGpsFrame.latitude_offset_y = 5;
   RoadGpsFrame.longitude_offset_y = 5+text_height*2;

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


void roadgps_screen_configure (void) {

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

      RoadGpsActive = roadmap_canvas_create_pen ("gps/active");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSActive));

      RoadGpsInactive = roadmap_canvas_create_pen ("gps/inactive");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSInactive));

      RoadGpsActiveFill = roadmap_canvas_create_pen ("gps/activefill");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSActiveFill));

      RoadGpsInactiveFill = roadmap_canvas_create_pen ("gps/inactivefill");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSInactiveFill));

      RoadGpsActiveLabels = roadmap_canvas_create_pen ("gps/activelabels");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSActiveLabels));

      RoadGpsInactiveLabels = roadmap_canvas_create_pen ("gps/inactivelabels");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSInactiveLabels));

      RoadGpsLabels = roadmap_canvas_create_pen ("gps/labels");
      roadmap_canvas_set_thickness (2);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSLabels));

     RoadGpsValues = roadmap_canvas_create_pen ("gps/values");
      roadmap_canvas_set_thickness (3);
      roadmap_canvas_set_foreground
          (roadmap_config_get (&RoadMapConfigGPSValues));

     RoadMapGPSFontSize =
      roadmap_config_get_integer (&RoadMapConfigGPSFontSize);
   }
   roadgps_screen_format_frame ();

}


void roadgps_screen_initialize (void) {

   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSBackground, "LightYellow");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSForeground, "#606c9e");

   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSActive, "#6aa85b");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSActiveLabels, "#ffffff");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSActiveFill, "#97c740");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSInactive, "#ff4128");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSInactiveFill, "#ff8c29");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSInactiveLabels, "#000000");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSLabels, "#000000");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSValues, "#03079a");

   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSFontSize, "15");

   roadmap_gps_register_monitor (roadgps_screen_monitor);
   roadmap_gps_register_listener (roadgps_screen_listener);

}

