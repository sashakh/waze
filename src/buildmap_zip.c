/* buildmap_zip.c - Build a zip code table for RoadMap.
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
 *   void buildmap_zip_initialize (void);
 *   void buildmap_zip_set_county (int county_code);
 *   RoadMapZip buildmap_zip_add (int zip, int longitude, int latitude);
 *   int  buildmap_zip_get_zip_code  (RoadMapZip index);
 *   int  buildmap_zip_get_longitude (RoadMapZip index);
 *   int  buildmap_zip_get_latitude  (RoadMapZip index);
 *   RoadMapZip buildmap_zip_locate (int zip);
 *   void buildmap_zip_save (void);
 *   void buildmap_zip_summary (void);
 *   void buildmap_zip_reset   (void);
 *
 * These functions are used to build a table of zip codes from
 * the Tiger maps. The objective is double: (1) reduce the size of
 * the Tiger data by sharing all duplicated information and
 * (2) produce the index data to serve as the basis for a fast
 * search mechanism for zip code in roadmap. The zip code will
 * be used to fast identify which county map to use (RoadMap maps
 * are following the Tiger county-based organization) and where
 * the zip code area lies (median point).
 *
 * Because the zip table is used to locate the county, it is managed
 * globally, in permanent storage (file).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "buildmap.h"
#include "buildmap_zip.h"


struct zip_code_record {

   int zip_code;
   int county_code;
   int max_longitude;
   int min_longitude;
   int max_latitude;
   int min_latitude;
};

static int ThisMapCounty = 0;


#define BUILDMAP_MAX_ZIP 1024

static int ZipCodeCount = 0;
static int ZipCodeAddCount = 0;
static struct zip_code_record ZipCode[BUILDMAP_MAX_ZIP] = {{0}};



void buildmap_zip_initialize (void) {

   /* TBD: Load the existing ZIP table. */
}


void  buildmap_zip_set_county (int county_code) {
   ThisMapCounty = county_code;
}


RoadMapZip  buildmap_zip_add (int zip, int longitude, int latitude) {

   int i;

   for (i = 1; i < ZipCodeCount; i++) {

      if (ZipCode[i].zip_code == zip) {

         if (ZipCode[i].max_longitude < longitude) {
            ZipCode[i].max_longitude = longitude;
         }
         if (ZipCode[i].min_longitude >= longitude) {
            ZipCode[i].min_longitude = longitude;
         }

         if (ZipCode[i].max_latitude < latitude) {
            ZipCode[i].max_latitude = latitude;
         }
         if (ZipCode[i].min_latitude >= latitude) {
            ZipCode[i].min_latitude = latitude;
         }
         return (RoadMapZip)i;
      }
   }

   if (ZipCodeCount >= BUILDMAP_MAX_ZIP) {
       buildmap_fatal (0, "too many zip codes");
   }

   ZipCodeCount += 1;

   i = ZipCodeCount;

   ZipCode[i].zip_code = zip;
   ZipCode[i].county_code = ThisMapCounty;
   ZipCode[i].max_longitude = - 0x7fffffff;
   ZipCode[i].min_longitude = + 0x7fffffff;
   ZipCode[i].max_latitude  = - 0x7fffffff;
   ZipCode[i].min_latitude  = + 0x7fffffff;

   return (RoadMapZip)i;
}


int   buildmap_zip_get_zip_code (RoadMapZip index) {

   if (index <= 0 || index >= ZipCodeCount) {
      buildmap_fatal (0, "invalid zip index");
   }

   return ZipCode[index].zip_code;
}


int   buildmap_zip_get_longitude (RoadMapZip index) {

   if (index <= 0 || index >= ZipCodeCount) {
      buildmap_fatal (0, "invalid zip index");
   }

   return (ZipCode[index].max_longitude + ZipCode[index].min_longitude) / 2;
}


int   buildmap_zip_get_latitude  (RoadMapZip index) {

   if (index <= 0 || index >= ZipCodeCount) {
      buildmap_fatal (0, "invalid zip index");
   }

   return (ZipCode[index].max_latitude + ZipCode[index].min_latitude) / 2;
}


RoadMapZip  buildmap_zip_locate (int zip) {

   int i;

   for (i = 1; i < ZipCodeCount; i++) {

      if (ZipCode[i].zip_code == zip) {
         return (RoadMapZip)i;
      }
   }

   return 0;
}


void buildmap_zip_save (void) {

   int i;
   int *db_zip;
   buildmap_db *root  = buildmap_db_add_section (NULL, "zip");

   buildmap_db_add_data (root, ZipCodeCount, sizeof(int));

   db_zip = (int *) buildmap_db_get_data (root);

   for (i = 1; i < ZipCodeCount; i++) {
      db_zip[i] = ZipCode[i].zip_code;
   }
}


void buildmap_zip_summary (void) {

   fprintf (stderr, "-- zip code table: %d items, %d add\n",
                    ZipCodeCount, ZipCodeAddCount);
}


void buildmap_zip_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_MAX_ZIP; i++) {

      ZipCode[i].zip_code = 0;
      ZipCode[i].county_code = 0;
      ZipCode[i].max_longitude = 0;
      ZipCode[i].min_longitude = 0;
      ZipCode[i].max_latitude = 0;
      ZipCode[i].min_latitude = 0;
   }

   ThisMapCounty = 0;
   ZipCodeCount = 0;
   ZipCodeAddCount = 0;
}

