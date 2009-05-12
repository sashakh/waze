/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2009 Danny Backx.
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
 * @brief Build a city table & index for BuildMap.
 *
 * These functions are used to build a table of cities.
 *
 * There is no city table for RoadMap, because the only attribute
 * of the city we do care about is its name: we just reference the
 * RoadMap dictionary table.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_city.h"


struct buildmap_city_struct {

   int fips;
   int year;
   RoadMapString name;
};

typedef struct buildmap_city_struct BuildMapCity;

static int CityCount = 0;
static BuildMapCity *City[BUILDMAP_BLOCK] = {NULL};

static RoadMapHash *CityByFips = NULL;


static void buildmap_city_register (void);

/**
 * @brief initialize the city module
 */
static void buildmap_city_initialize (void) {

   CityByFips = roadmap_hash_new ("CityByFips", BUILDMAP_BLOCK);

   CityCount = 0;

   buildmap_city_register();
}

/**
 * @brief add a city
 * @param fips a region code (county or other country subdivision)
 * @param year (not sure why this is relevant)
 * @param name the city name
 */
void buildmap_city_add (int fips, int year, RoadMapString name) {

   int index;
   int block;
   int offset;
   BuildMapCity *this_city;


   if (CityByFips == NULL) buildmap_city_initialize();


   /* First search if that city is not yet known. */

   for (index = roadmap_hash_get_first (CityByFips, fips);
        index >= 0;
        index = roadmap_hash_get_next (CityByFips, index)) {

       this_city = City[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

       if (this_city->fips == fips) {

          if (this_city->year == year && this_city->name != name) {
             buildmap_fatal (0, "non unique city FIPS code %d (years %d and %d)",
                                fips, this_city->year, year);
          }

          if (year > this_city->year) {
             this_city->name = name;
             this_city->year = year;
          }
          return;
        }
   }

   /* This city was not known yet: create a new one. */

   block = CityCount / BUILDMAP_BLOCK;
   offset = CityCount % BUILDMAP_BLOCK;

   if (block >= BUILDMAP_BLOCK) {
      buildmap_fatal (0,
         "Underdimensioned city table (block %d, BUILDMAP_BLOCK %d)",
	 block, BUILDMAP_BLOCK);
   }

   if (City[block] == NULL) {

      /* We need to add a new block to the table. */

      City[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapCity));
      if (City[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
      roadmap_hash_resize (CityByFips, (block+1) * BUILDMAP_BLOCK);
   }

   this_city = City[block] + offset;

   this_city->name = name;
   this_city->fips = fips;
   this_city->year = year;

   roadmap_hash_add (CityByFips, fips, CityCount);

   CityCount += 1;
}

/**
 * @brief query the name of a city by fips
 * @param fips identifies the city
 * @return internal represenation of city name
 */
RoadMapString buildmap_city_get_name (int fips) {

   int index;
   BuildMapCity *this_city;

   if (CityByFips == NULL) return 0;

   for (index = roadmap_hash_get_first (CityByFips, fips);
        index >= 0;
        index = roadmap_hash_get_next (CityByFips, index)) {

       this_city = City[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

       if (this_city->fips == fips) {
          return this_city->name;
        }
   }

   return 0;
}

/**
 * @brief print buildmap summary for this module
 */
static void buildmap_city_summary (void) {

   fprintf (stderr,
            "-- city table statistics: %d cities\n", CityCount);
}

/**
 * @brief clear city module database
 */
static void buildmap_city_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; i++) {
      if (City[i] != NULL) {
         free (City[i]);
         City[i] = NULL;
      }
   }

   CityCount = 0;

   roadmap_hash_delete (CityByFips);
   CityByFips = NULL;
}

/**
 * @brief identify the city module
 */
static buildmap_db_module BuildMapCityModule = {
   "city",
   NULL,
   NULL,
   buildmap_city_summary,
   buildmap_city_reset
};

/**
 * @brief register the city module with the buildmap application
 */
static void buildmap_city_register (void) {
   buildmap_db_register (&BuildMapCityModule);
}
