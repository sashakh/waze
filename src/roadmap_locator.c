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
 *  int  roadmap_locator_by_city     (char *city, char *state);
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
#include "roadmap_gui.h"
#include "roadmap_county.h"
#include "roadmap_locator.h"


#define ROADMAP_US_CACHE_SIZE 8

static struct {

   int          fips;
   unsigned int last_access;

} RoadMapCountyCache [ROADMAP_US_CACHE_SIZE] = {{0, 0}};

static int RoadMapActiveCounty;


static roadmap_db_model *RoadMapUsModel;
static roadmap_db_model *RoadMapCountyModel;

static RoadMapDictionary RoadMapUsCityDictionary = NULL;
static RoadMapDictionary RoadMapUsCountyDictionary = NULL;
static RoadMapDictionary RoadMapUsStateDictionary = NULL;
static RoadMapDictionary RoadMapUsCategoryDictionary = NULL;


/* The following table is a hardcoded default when the
 * "category" dictionary is not found.
 */
static char *RoadMapDefaultCategoryTable[] = {
   "Freeways",
   "Ramps",
   "Highways",
   "Streets",
   "Trails",
   "Parks",
   "Hospitals",
   "Airports",
   "Stations",
   "Malls",
   "Shore",
   "Rivers",
   "Lakes",
   "Sea"
};

static void roadmap_locator_initialize (void) {

   static int Initialized = 0;

   if (! Initialized) {

      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "string", &RoadMapDictionaryHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "point", &RoadMapPointHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "square", &RoadMapSquareHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "shape", &RoadMapShapeHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "zip", &RoadMapZipHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "line", &RoadMapLineHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "street", &RoadMapStreetHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "range", &RoadMapRangeHandler);
      RoadMapCountyModel =
         roadmap_db_register
            (RoadMapCountyModel, "polygon", &RoadMapPolygonHandler);

      RoadMapUsModel =
         roadmap_db_register
            (RoadMapUsModel, "county", &RoadMapCountyHandler);
      RoadMapUsModel =
         roadmap_db_register
            (RoadMapUsModel, "string", &RoadMapDictionaryHandler);

      if (! roadmap_db_open ("usdir", RoadMapUsModel)) {
         roadmap_log (ROADMAP_FATAL, "cannot open directory database (usdir)");
      }
      roadmap_db_activate ("usdir");

      RoadMapUsCityDictionary   = roadmap_dictionary_open ("city");
      RoadMapUsCountyDictionary = roadmap_dictionary_open ("county");
      RoadMapUsStateDictionary  = roadmap_dictionary_open ("state");
      RoadMapUsCategoryDictionary  = roadmap_dictionary_open ("category");

      Initialized = 1;
   }
}


static void roadmap_locator_close (int index) {

   char map_name[64];

   snprintf (map_name, sizeof(map_name),
             "usc%05d", RoadMapCountyCache[index].fips);
   roadmap_db_close (map_name);

   RoadMapCountyCache[index].fips = 0;
   RoadMapCountyCache[index].last_access = 0;
}


static unsigned int roadmap_locator_new_access (void) {

   static int RoadMapLocatorAccessCount = 0;

   int i;


   RoadMapLocatorAccessCount += 1;

   if (RoadMapLocatorAccessCount == 0) { /* Rollover. */

      for (i = 0; i < ROADMAP_US_CACHE_SIZE; i++) {

         if (RoadMapCountyCache[i].fips > 0) {
            roadmap_locator_close (i);
         }
      }

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

   for (i = ROADMAP_US_CACHE_SIZE-1; i >= 0; i--) {

      if (RoadMapCountyCache[i].fips == fips) {

         roadmap_db_activate (map_name);
         return ROADMAP_US_OK;
      }

      if (RoadMapCountyCache[i].last_access
             < RoadMapCountyCache[oldest].last_access) {

         oldest = i;
      }
   }

   if (RoadMapCountyCache[oldest].fips > 0) {
      roadmap_locator_close (oldest);
   }

   access = roadmap_locator_new_access ();

   if (! roadmap_db_open (map_name, RoadMapCountyModel)) {

      roadmap_log (ROADMAP_ERROR, "cannot open map database %s", map_name);

      return ROADMAP_US_NOMAP;
   }
   roadmap_db_activate (map_name);

   RoadMapCountyCache[oldest].fips = fips;
   RoadMapCountyCache[oldest].last_access = access;

   RoadMapActiveCounty = fips;
   
   return ROADMAP_US_OK;
}


int roadmap_locator_by_position
        (const RoadMapPosition *position, int **fips) {

   int i;
   int count;
   int missing;
   int usable;

   int *counties;


   roadmap_locator_initialize();

   count = roadmap_county_count();

   if (*fips == NULL) {
      *fips = calloc (count, sizeof(int));
      roadmap_check_allocated(*fips);
   }

   counties = *fips;
   count = roadmap_county_by_position (position, counties, count);

   missing = 0;
   for (i = count - 1; i >= 0; i--) {

      if (roadmap_locator_open (counties[i]) != ROADMAP_US_OK) {
         counties[i] = 0;
         missing = 1;
      }
   }

   if (! missing) {
      return count;
   }

   for (usable = 0, i = 0; i < count; i++) {

      if (counties[i] != 0) {
         counties[usable++] = counties[i];
      }
   }

   return usable;
}


int roadmap_locator_by_city (char *city_name, char *state_symbol) {

   int city;
   int state;

   roadmap_locator_initialize();

   state = roadmap_dictionary_locate (RoadMapUsStateDictionary, state_symbol);
   if (state <= 0) {
      return ROADMAP_US_NOSTATE;
   }

   while (city_name[0] == '?') {
      city_name += 1;
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

   roadmap_locator_initialize();

   return roadmap_locator_open (fips);
}


int roadmap_locator_active (void) {
    return RoadMapActiveCounty;
}


int roadmap_locator_category_count (void) {

   roadmap_locator_initialize ();

   if (RoadMapUsCategoryDictionary == NULL) {
      return sizeof(RoadMapDefaultCategoryTable) / sizeof(char *);
   }
   return roadmap_dictionary_count (RoadMapUsCategoryDictionary);
}

char *roadmap_locator_category_name (int index) {

   if (RoadMapUsCategoryDictionary == NULL) {
      return RoadMapDefaultCategoryTable[index-1];
   }
   return roadmap_dictionary_get (RoadMapUsCategoryDictionary, index);
}

