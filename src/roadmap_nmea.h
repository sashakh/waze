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

#ifndef _ROADMAP_NMEA__H_
#define _ROADMAP_NMEA__H_

#include "sys/time.h"

typedef union {

   struct {
      time_t fix;
      int    status;
      int    latitude;
      int    longitude;
      int    speed;
      int    steering;
   } gprmc;

   struct {
      time_t fix;
      int    latitude;
      int    longitude;
      int    quality;
      int    count;
      int    dilution;
      int    altitude;
      int    dgps_update;
      char  *dgps_id;
   } gpgga;

   struct {
      char  automatic;
      char  dimension;
      short reserved0;
      char  satellite[12];
      float dilution_position;
      float dilution_horizontal;
      float dilution_vertical;
   } gpgsa;

   struct {
      char  total;
      char  index;
      char  count;
      char  reserved0;
      char  satellite[4];
      char  elevation[4];
      short azimuth[4];
      short strength[4];
   } gpgsv;

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

} RoadMapNmeaFields;

typedef void (*RoadMapNmeaFilter)   (RoadMapNmeaFields *fields);
typedef void (*RoadMapNmeaListener) (void *context,
                                     const RoadMapNmeaFields *fields);


RoadMapNmeaFilter
    roadmap_nmea_add_filter (char *name, RoadMapNmeaFilter filter);

RoadMapNmeaListener
    roadmap_nmea_subscribe (char *name, RoadMapNmeaListener listener);


int roadmap_nmea_decode (void *context, char *sentence);

#endif // _ROADMAP_NMEA__H_

