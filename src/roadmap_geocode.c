/* roadmap_geocode.c - Find the possible positions of a street address.
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
 *   See roadmap_geocode.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_street.h"
#include "roadmap_locator.h"
#include "roadmap_trip.h"
#include "roadmap_preferences.h"

#include "roadmap_geocode.h"


#define ROADMAP_MAX_STREETS  256


static const char *RoadMapGeocodeError = NULL;


int roadmap_geocode_address (RoadMapGeocode **selections,
                             const char *number_image,
                             const char *street_name,
                             const char *city_name,
                             const char *state_name) {

   int i, j, k;
   int fips;
   int count;

   int   street_number [ROADMAP_MAX_STREETS];
   RoadMapBlocks blocks[ROADMAP_MAX_STREETS];

   RoadMapGeocode *results;


   RoadMapGeocodeError = "No error";
   *selections = NULL;

   switch (roadmap_locator_by_city (city_name, state_name)) {

   case ROADMAP_US_OK:

      count = roadmap_street_blocks_by_city
                  (street_name, city_name, blocks, 256);
      break;

   case ROADMAP_US_NOSTATE:

      RoadMapGeocodeError = "No state with that name could be found";
      return 0;

   case ROADMAP_US_NOCITY:

      RoadMapGeocodeError = "No city with that name could be found";
      return 0;

   default:

      RoadMapGeocodeError = "No related map could not be found";
      return 0;
   }

   if (count <= 0) {

      switch (count) {
      case ROADMAP_STREET_NOADDRESS:
         RoadMapGeocodeError = "No such address could be found on that street";
         break;
      case ROADMAP_STREET_NOCITY:
         RoadMapGeocodeError = "No city with that name could be found";
         break;
      case ROADMAP_STREET_NOSTREET:
         RoadMapGeocodeError = "No street with that name could be found";
      default:
         RoadMapGeocodeError = "The address could not be found";
      }
      return 0;
   }

   if (count > ROADMAP_MAX_STREETS) {
      roadmap_log (ROADMAP_ERROR, "too many blocks");
      count = ROADMAP_MAX_STREETS;
   }

   /* If the street number was not provided, retrieve all street blocks to
    * expand the match list, else decode that street number and update the
    * match list.
    */
   if (number_image[0] == 0) {

      int range_count;
      RoadMapStreetRange ranges[ROADMAP_MAX_STREETS];

      for (i = 0, j = 0; i < count; ++i) {
         street_number[i] = -1;
      }

      for (i = 0, j = 0; i < count && street_number[i] < 0; ++i) {

         range_count =
            roadmap_street_get_ranges (blocks+i, ROADMAP_MAX_STREETS, ranges);

         if (range_count > 0) {

            street_number[i] = ranges[0].fradd;

            for (k = 1; k < range_count; ++k) {

               if (count >= ROADMAP_MAX_STREETS) {
                  roadmap_log (ROADMAP_WARNING,
                               "too many blocks, cannot expand");
                  break;
               }
               blocks[count] = blocks[i];
               ranges[count] = ranges[k];
               street_number[count] = ranges[count].fradd;
               count += 1;
            }
         }
      }

   } else {

      int number;

      number = roadmap_math_street_address (number_image, strlen(number_image));

      for (i = 0, j = 0; i < count; ++i) {
         street_number[i] = number;
      }
   }

   results = (RoadMapGeocode *)
       calloc (count, sizeof(RoadMapGeocode));

   fips = roadmap_locator_active();

   for (i = 0, j = 0; i< count; ++i) {

      int line;

      line = roadmap_street_get_position
                    (blocks+i, street_number[i], &results[j].position);

      for (k = 0; k < j; ++k) {
         if (results[k].line == line) {
            line = -1;
            break;
         }
      }

      if (line >= 0) {

        RoadMapStreetProperties properties;
          
        roadmap_street_get_properties (line, &properties);
        results[j].fips = fips;
        results[j].line = line;
        results[j].name = strdup (roadmap_street_get_full_name (&properties));
        j += 1;
      }
   }

   if (j <= 0) {

      free (results);
      RoadMapGeocodeError = "No valid street was found";

   } else {

      *selections = results;
   }

   return j;
}


const char *roadmap_geocode_last_error (void) {

   return RoadMapGeocodeError;
}

