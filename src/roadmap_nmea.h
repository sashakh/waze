/* roadmap_nmea.h - Decode a NMEA sentence.
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
 */

#ifndef INCLUDED__ROADMAP_NMEA__H
#define INCLUDED__ROADMAP_NMEA__H

#include "sys/time.h"

#include "roadmap_string.h"


#define ROADMAP_NMEA_MAX_SATELLITE   16

#define ROADMAP_NMEA_QUALITY_INVALID   0
#define ROADMAP_NMEA_QUALITY_GPS       1
#define ROADMAP_NMEA_QUALITY_DGPS      2
#define ROADMAP_NMEA_QUALITY_PPS       3
#define ROADMAP_NMEA_QUALITY_OTHER     4

typedef union {

   struct {
      time_t fixtime;
      int    status;
      int    latitude;
      int    longitude;
      int    speed;
      int    steering;
   } rmc;

   struct {
      time_t fixtime;
      int    latitude;
      int    longitude;
      int    quality;
      int    count;
      int    dilution;
      int    altitude;
      char   altitude_unit[4];
   } gga;

   struct {
      int    status;
      int    mode;
      int    latitude;
      int    longitude;
   } gll;

   struct {
      char  automatic;
      char  dimension;
      short reserved0;
      char  satellite[ROADMAP_NMEA_MAX_SATELLITE];
      float dilution_position;
      float dilution_horizontal;
      float dilution_vertical;
   } gsa;

   struct {
      char  total;
      char  index;
      char  count;
      char  reserved0;
      char  satellite[4];
      char  elevation[4];
      short azimuth[4];
      short strength[4];
   } gsv;

   /* The following structures match Garmin extensions: */

   struct {
      char  datum[256];
   } pgrmm;

   struct {
      int   horizontal;
      char  horizontal_unit[4];
      int   vertical;
      char  vertical_unit[4];
      int   three_dimensions;
      char  three_dimensions_unit[4];
   } pgrme;

   /* RoadMap's own extensions: */

   struct {
      RoadMapDynamicString id;
      RoadMapDynamicString name;
      RoadMapDynamicString sprite;
   } pxrmadd;

   struct {
      RoadMapDynamicString id;
      int  latitude;
      int  longitude;
      int  speed;
      int  steering;
   } pxrmmov;

   struct {
      RoadMapDynamicString id;
   } pxrmdel;

   struct {
      int  count;
      struct {
         RoadMapDynamicString item;
      } subscribed[16];
   } pxrmsub;

   struct {
      RoadMapDynamicString category;
      RoadMapDynamicString name;
      RoadMapDynamicString value;
   } pxrmcfg;

} RoadMapNmeaFields;


typedef void (*RoadMapNmeaFilter)   (RoadMapNmeaFields *fields);
typedef void (*RoadMapNmeaListener) (void *context,
                                     const RoadMapNmeaFields *fields);


RoadMapNmeaFilter
    roadmap_nmea_add_filter (const char *vendor, /* NULL means standard. */
                             const char *sentence,
                             RoadMapNmeaFilter filter);

RoadMapNmeaListener
    roadmap_nmea_subscribe (const char *vendor, /* NULL means standard. */
                            const char *sentence,
                            RoadMapNmeaListener listener);


typedef void (*RoadMapNmeaLogger)  (const char *data);
typedef int  (*RoadMapNmeaReceive) (void *context, char *data, int size);

typedef struct roadmap_nmea_context {

   const char *title;
   void       *user_context;

   RoadMapNmeaLogger  logger;
   RoadMapNmeaReceive receive;

   char data[1024];
   int  cursor;

} RoadMapNmeaContext;

int roadmap_nmea_input (RoadMapNmeaContext *context);

#endif // INCLUDED__ROADMAP_NMEA__H

