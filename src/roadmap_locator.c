/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2008, Danny Backx.
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
 */

/**
 * @file
 * @brief Locate the map to which a specific place belongs.
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
#include "roadmap_scan.h"
#include "roadmap_dbread.h"
#include "roadmap_dictionary.h"
#include "roadmap_point.h"
#include "roadmap_square.h"
#include "roadmap_shape.h"
#include "roadmap_line.h"
#include "roadmap_street.h"
#include "roadmap_polygon.h"
#include "roadmap_osm.h"
#include "roadmap_math.h"
#include "roadmap_county.h"
#include "roadmap_config.h"

#include "roadmap_locator.h"


#define ROADMAP_CACHE_SIZE 8

struct roadmap_cache_entry {

   int          fips;
   const char  *path;
   unsigned int last_access;
};

static struct roadmap_cache_entry *RoadMapCountyCache = NULL;

static int RoadMapCountyCacheSize = 0;

static int RoadMapActiveCounty;


static roadmap_db_model *RoadMapUsModel;
static roadmap_db_model *RoadMapCountyModel;

RoadMapDictionary RoadMapUsCityDictionary = NULL;
static RoadMapDictionary RoadMapUsStateDictionary = NULL;


static int roadmap_locator_no_download (int fips) {return 0;}

static RoadMapInstaller  RoadMapDownload = roadmap_locator_no_download;


/*
 * @brief
 */
static void roadmap_locator_configure (void) {

   const char *path;

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
            (RoadMapCountyModel, "polygons", &RoadMapPolygonHandler);
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

      RoadMapCountyCacheSize = roadmap_option_cache ();
      if (RoadMapCountyCacheSize < ROADMAP_CACHE_SIZE) {
         RoadMapCountyCacheSize = ROADMAP_CACHE_SIZE;
      }
      RoadMapCountyCache = (struct roadmap_cache_entry *)
         calloc (RoadMapCountyCacheSize, sizeof(struct roadmap_cache_entry));
      roadmap_check_allocated (RoadMapCountyCache);

      for (path = roadmap_scan ("maps", "usdir.rdm");
           path != NULL;
           path = roadmap_scan_next ("maps", "usdir.rdm", path)) {

         if (roadmap_db_open (path, "usdir.rdm", RoadMapUsModel)) {
            break;
         }
         roadmap_log (ROADMAP_ERROR,
                      "cannot open database directory usdir in %s", path);
      }

      if (path == NULL) {
         roadmap_log (ROADMAP_WARNING,
                      "Cannot open map index file.  Only OpenStreetMap"
                      " maps will be available.  Otherwise,"
                      " the Map.Path preferences item must point at"
                      " map files and the master index file.");
         return;
      }

      RoadMapUsCityDictionary   = roadmap_dictionary_open ("city");
      RoadMapUsStateDictionary  = roadmap_dictionary_open ("state");

   }
}

/**
 * @brief
 * @param buf
 * @param fips
 * @return
 */
char *roadmap_locator_filename(char *buf, int fips) {

   static char filename[64];

   if (buf == NULL) buf = filename;

   if (fips < 0) {
      roadmap_osm_filename(buf, 1, fips);
   } else {
      roadmap_county_filename(buf, fips);
   }
   return buf;
}

/**
 * @brief
 * @param index
 */
static void roadmap_locator_remove (int index) {

   roadmap_db_close (RoadMapCountyCache[index].path,
        roadmap_locator_filename(NULL, RoadMapCountyCache[index].fips));

   RoadMapCountyCache[index].fips = 0;
   RoadMapCountyCache[index].path = NULL;
   RoadMapCountyCache[index].last_access = 0;
}

/**
 * @brief
 * @return
 */
static unsigned int roadmap_locator_new_access (void) {

   static unsigned int RoadMapLocatorAccessCount = 0;

   int i;


   RoadMapLocatorAccessCount += 1;

   if (RoadMapLocatorAccessCount == 0) { /* Rollover. */

      for (i = 0; i < RoadMapCountyCacheSize; i++) {

         if (RoadMapCountyCache[i].fips != 0) {
            roadmap_locator_remove (i);
         }
      }
      RoadMapActiveCounty = 0;

      RoadMapLocatorAccessCount += 1;
   }

   return RoadMapLocatorAccessCount;
}

/**
 * @brief
 * @param fips
 * @return
 */
static int roadmap_locator_open (int fips) {

   int i;
   int access;
   int oldest = 0;

   const char *path;
   char map_name[64];


   if (fips == 0) {
      return ROADMAP_US_NOMAP;
   }

   roadmap_locator_filename(map_name, fips);

   /* Look for the oldest entry in the cache. */

   for (i = RoadMapCountyCacheSize-1; i >= 0; --i) {

      if (RoadMapCountyCache[i].fips == fips) {

         roadmap_db_activate (RoadMapCountyCache[i].path, map_name);
         RoadMapActiveCounty = fips;

         return ROADMAP_US_OK;
      }

      if (RoadMapCountyCache[i].last_access
             < RoadMapCountyCache[oldest].last_access) {

         oldest = i;
      }
   }

   if (RoadMapCountyCache[oldest].fips != 0) {
       if (RoadMapCountyCache[oldest].fips == RoadMapActiveCounty) {
           RoadMapActiveCounty = 0;
       }
       roadmap_locator_remove (oldest);
   }

   access = roadmap_locator_new_access ();

   do {

      for (path = roadmap_scan ("maps", map_name);
           path != NULL;
           path = roadmap_scan_next ("maps", map_name, path)) {

         if (roadmap_db_open (path, map_name, RoadMapCountyModel)) {

            RoadMapCountyCache[oldest].fips = fips;
            RoadMapCountyCache[oldest].path = path;
            RoadMapCountyCache[oldest].last_access = access;

            RoadMapActiveCounty = fips;

            return ROADMAP_US_OK;
         }
      }

   } while (RoadMapDownload (fips));

   return ROADMAP_US_NOMAP;
}

/**
 * @brief
 * @param fips
 */
void roadmap_locator_close (int fips) {

   int i;

   if (RoadMapCountyCache != NULL) {

      for (i = RoadMapCountyCacheSize-1; i >= 0; --i) {

         if (RoadMapCountyCache[i].fips == fips) {
            roadmap_locator_remove (i);
         }
      }
   }
}

/**
 * @brief
 * @param fipslistp
 * @return
 */
static int roadmap_locator_allocate (int **fipslistp) {

   int count;

   roadmap_locator_configure();
   if (RoadMapCountyCache == NULL) return 0;

   count = roadmap_county_count();

   /* note that this can also be resized during tile splitting in
    * roadmap_osm.c -- see usage of roadmap_osm_tilelist in that file.
    */
   *fipslistp = realloc (*fipslistp, count * sizeof(int));
   roadmap_check_allocated(*fipslistp);

   return count;
}

/**
 * @brief
 * @param download
 */
void roadmap_locator_declare_downloader (RoadMapInstaller download) {

   if (download == NULL) {
       RoadMapDownload = roadmap_locator_no_download;
   } else {
       RoadMapDownload = download;
   }
}

/**
 * @brief
 * @param position
 * @param fipslistp
 * @return
 */
int roadmap_locator_by_position
        (const RoadMapPosition *position, int **fipslistp) {

   int count;
   RoadMapArea focus;

   count = roadmap_locator_allocate (fipslistp);
   if (count < 0) return 0;
   
   count = roadmap_county_by_position (position, *fipslistp, count);

   roadmap_math_get_focus (&focus);

   return roadmap_osm_by_position (position, &focus, fipslistp, count);
}

/**
 * @brief
 * @param state_symbol
 * @param fipslistp
 * @return
 */
int roadmap_locator_by_state (const char *state_symbol, int **fipslistp) {

   int count;
   RoadMapString state;

   count = roadmap_locator_allocate (fipslistp);
   if (count <= 0) return 0;

   state = roadmap_dictionary_locate (RoadMapUsStateDictionary, state_symbol);
   if (state <= 0) {
       return 0;
   }
   return roadmap_county_by_state (state, *fipslistp, count);
}

/**
 * @brief
 * @param city_name
 * @param state_symbol
 * @param fipslistp
 * @return
 */
int roadmap_locator_by_city
       (const char *city_name, const char *state_symbol, int **fipslistp) {

   int count;

   RoadMapString city;
   RoadMapString state;

   count = roadmap_locator_allocate (fipslistp);
   if (count <= 0) return ROADMAP_US_NOMAP;

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

   return roadmap_county_by_city (city, state, *fipslistp, count);
}

/**
 * @brief
 * @param fips
 * @return
 */
int roadmap_locator_activate (int fips) {

   if (RoadMapActiveCounty == fips) {
       return ROADMAP_US_OK;
   }

   roadmap_locator_configure();
   if (RoadMapCountyCache == NULL) return ROADMAP_US_NOMAP;

   return roadmap_locator_open (fips);
}

/**
 * @brief
 * @return
 */
int roadmap_locator_active (void) {
    return RoadMapActiveCounty;
}

/**
 * @brief
 * @param state
 * @return
 */
RoadMapString roadmap_locator_get_state (const char *state) {

   if (RoadMapCountyCache == NULL) return 0;
   return roadmap_dictionary_locate (RoadMapUsStateDictionary, state);
}

/**
 * @brief
 * @param fips
 * @return
 */
int roadmap_locator_get_decluttered(int fips) {

   if (fips > 0)
      return roadmap_county_get_decluttered(fips);
   else
      return 0;  /* unimplemented for geographic maps as yet */
}

/**
 * @brief
 * @param fips
 * @return
 */
void roadmap_locator_set_decluttered(int fips) {

   /* unimplemented for geographic maps as yet */
   if (fips > 0)
      roadmap_county_set_decluttered(fips);

}
