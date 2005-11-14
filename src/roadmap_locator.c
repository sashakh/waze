/* roadmap_locator.c - Locate the map to which a specific place belongs.
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
 *  int roadmap_locator_by_position
 *         (RoadMapPosition *position, int *fips, int count);
 *  int  roadmap_locator_by_city     (const char *city, const char *state);
 *  int  roadmap_locator_activate    (int fips);
 *
 * These functions are used to retrieve which map the given entity belongs to.
 */

#include <stdio.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "roadmap_dictionary.h"
#include "roadmap_point.h"
#include "roadmap_square.h"
#include "roadmap_shape.h"
#include "roadmap_line.h"
#include "roadmap_street.h"
#include "roadmap_polygon.h"
#include "roadmap_county.h"

#include "roadmap_locator.h"


#define ROADMAP_CACHE_SIZE 8

struct roadmap_cache_entry {

   int          fips;
   unsigned int last_access;
};

static struct roadmap_cache_entry *RoadMapCountyCache = NULL;

static int RoadMapCountyCacheSize = 0;

static int RoadMapActiveCounty;


static roadmap_db_model *RoadMapUsModel;
static roadmap_db_model *RoadMapCountyModel;

static RoadMapDictionary RoadMapUsCityDictionary = NULL;
static RoadMapDictionary RoadMapUsStateDictionary = NULL;


static int roadmap_locator_no_download (int fips) {return 0;}

static RoadMapInstaller  RoadMapDownload = roadmap_locator_no_download;


static void roadmap_locator_configure (void) {

   if (RoadMapCountyCache == NULL) {

      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "zip", &RoadMapZipHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "street", &RoadMapStreetHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "range", &RoadMapRangeHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "polygon", &RoadMapPolygonHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "shape", &RoadMapShapeHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "line", &RoadMapLineHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "point", &RoadMapPointHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "square", &RoadMapSquareHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "string", &RoadMapDictionaryHandler);

      RoadMapUsModel =
         roadmap_db_register
            (RoadMapUsModel, "county", &RoadMapCountyHandler);
      RoadMapUsModel =
         roadmap_db_register
            (RoadMapUsModel, "string", &RoadMapDictionaryHandler);

      if (! roadmap_db_open ("usdir", RoadMapUsModel, "r")) {
         roadmap_log (ROADMAP_FATAL, "cannot open directory database (usdir)");
      }

      RoadMapUsCityDictionary   = roadmap_dictionary_open ("city");
      RoadMapUsStateDictionary  = roadmap_dictionary_open ("state");


      RoadMapCountyCacheSize = roadmap_option_cache ();
      if (RoadMapCountyCacheSize < ROADMAP_CACHE_SIZE) {
         RoadMapCountyCacheSize = ROADMAP_CACHE_SIZE;
      }
      RoadMapCountyCache = (struct roadmap_cache_entry *)
         calloc (RoadMapCountyCacheSize, sizeof(struct roadmap_cache_entry));
      roadmap_check_allocated (RoadMapCountyCache);
   }
}


static void roadmap_locator_remove (int index) {

   char map_name[64];

   snprintf (map_name, sizeof(map_name),
             "usc%05d", RoadMapCountyCache[index].fips);
   roadmap_db_close (map_name);

   RoadMapCountyCache[index].fips = 0;
   RoadMapCountyCache[index].last_access = 0;
}


static unsigned int roadmap_locator_new_access (void) {

   static unsigned int RoadMapLocatorAccessCount = 0;

   int i;


   RoadMapLocatorAccessCount += 1;

   if (RoadMapLocatorAccessCount == 0) { /* Rollover. */

      for (i = 0; i < RoadMapCountyCacheSize; i++) {

         if (RoadMapCountyCache[i].fips > 0) {
            roadmap_locator_remove (i);
         }
      }
      RoadMapActiveCounty = 0;

      RoadMapLocatorAccessCount += 1;
   }

   return RoadMapLocatorAccessCount;
}


static int roadmap_locator_open (int fips) {

   int i;
   int access;
   int oldest = 0;

   char map_name[64];


   if (fips <= 0) {
      return ROADMAP_US_NOMAP;
   }
   snprintf (map_name, sizeof(map_name), "usc%05d", fips);

   /* Look for the oldest entry in the cache. */

   for (i = RoadMapCountyCacheSize-1; i >= 0; --i) {

      if (RoadMapCountyCache[i].fips == fips) {

         roadmap_db_activate (map_name);
         RoadMapActiveCounty = fips;

         return ROADMAP_US_OK;
      }

      if (RoadMapCountyCache[i].last_access
             < RoadMapCountyCache[oldest].last_access) {

         oldest = i;
      }
   }

   if (RoadMapCountyCache[oldest].fips > 0) {
       roadmap_locator_remove (oldest);
       if (RoadMapCountyCache[oldest].fips == RoadMapActiveCounty) {
           RoadMapActiveCounty = 0;
       }
   }

   access = roadmap_locator_new_access ();

   while (! roadmap_db_open (map_name, RoadMapCountyModel, "r")) {

      if (! RoadMapDownload (fips)) {
         return ROADMAP_US_NOMAP;
      }
   }

   RoadMapCountyCache[oldest].fips = fips;
   RoadMapCountyCache[oldest].last_access = access;

   RoadMapActiveCounty = fips;
   
   return ROADMAP_US_OK;
}


void roadmap_locator_close (int fips) {

   int i;

   for (i = RoadMapCountyCacheSize-1; i >= 0; --i) {

      if (RoadMapCountyCache[i].fips == fips) {
         roadmap_locator_remove (i);
      }
   }
}


static int roadmap_locator_allocate (int **fips) {

   int count;

   roadmap_locator_configure();

   count = roadmap_county_count();

   if (*fips == NULL) {
      *fips = calloc (count, sizeof(int));
      roadmap_check_allocated(*fips);
   }

   return count;
}


void roadmap_locator_declare (RoadMapInstaller download) {

   if (download == NULL) {
       RoadMapDownload = roadmap_locator_no_download;
   } else {
       RoadMapDownload = download;
   }
}


int roadmap_locator_by_position
        (const RoadMapPosition *position, int **fips) {

   int count;

   count = roadmap_locator_allocate (fips);
   return roadmap_county_by_position (position, *fips, count);
}


int roadmap_locator_by_state (const char *state_symbol, int **fips) {

   int count;
   RoadMapString state;

   count = roadmap_locator_allocate (fips);

   state = roadmap_dictionary_locate (RoadMapUsStateDictionary, state_symbol);
   if (state <= 0) {
       return 0;
   }
   return roadmap_county_by_state (state, *fips, count);
}


int roadmap_locator_by_city (const char *city_name, const char *state_symbol) {

   RoadMapString city;
   RoadMapString state;

   roadmap_locator_configure();

   state = roadmap_dictionary_locate (RoadMapUsStateDictionary, state_symbol);
   if (state <= 0) {
      return ROADMAP_US_NOSTATE;
   }

   while (city_name[0] == '?') {
      ++city_name;
      while (city_name[0] == ' ') ++city_name;
   }
   city = roadmap_dictionary_locate (RoadMapUsCityDictionary, city_name);
   if (city <= 0) {
      return ROADMAP_US_NOCITY;
   }

   return roadmap_locator_open (roadmap_county_by_city (city, state));
}


int roadmap_locator_activate (int fips) {

   if (RoadMapActiveCounty == fips) {
       return ROADMAP_US_OK;
   }

   roadmap_locator_configure();

   return roadmap_locator_open (fips);
}


int roadmap_locator_active (void) {
    return RoadMapActiveCounty;
}


RoadMapString roadmap_locator_get_state (const char *state) {

   return roadmap_dictionary_locate (RoadMapUsStateDictionary, state);
}

