/* roadmap_county.c - Manage the county directory used to select a map.
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
 *   int  roadmap_county_by_position
 *           (RoadMapPosition *position, int *fips, int count);
 *
 *   int  roadmap_county_by_city (RoadMapString city, RoadMapString state);
 *
 *   extern roadmap_db_handler RoadMapCountyHandler;
 *
 * These functions are used to retrieve a map given a location.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_dbread.h"
#include "roadmap_db_county.h"

#include "roadmap_county.h"


static char RoadMapCountyType[] = "RoadMapCountyType";

typedef struct {

   char *type;

   RoadMapCounty *county;
   int            county_count;

   RoadMapCountyCity *city;
   int                city_count;

   RoadMapCountyByState *state;
   int                   state_count;

} RoadMapCountyContext;

static RoadMapCountyContext *RoadMapCountyActive = NULL;


static void *roadmap_county_map (roadmap_db *root) {

   roadmap_db *county_table;
   roadmap_db *state_table;
   roadmap_db *city_table;

   RoadMapCountyContext *context;

   context = malloc (sizeof(RoadMapCountyContext));
   roadmap_check_allocated(context);

   context->type = RoadMapCountyType;

   state_table  = roadmap_db_get_subsection (root, "bystate");
   city_table   = roadmap_db_get_subsection (root, "city");
   county_table = roadmap_db_get_subsection (root, "data");

   context->county =
      (RoadMapCounty *) roadmap_db_get_data (county_table);
   context->city = (RoadMapCountyCity *) roadmap_db_get_data (city_table);
   context->state = (RoadMapCountyByState *) roadmap_db_get_data (state_table);

   context->county_count = roadmap_db_get_count (county_table);
   context->city_count   = roadmap_db_get_count (city_table);
   context->state_count  = roadmap_db_get_count (state_table);

   if (roadmap_db_get_size(state_table) !=
          context->state_count * sizeof(RoadMapCountyByState)) {
      roadmap_log (ROADMAP_FATAL, "invalid county/bystate structure");
   }
   if (roadmap_db_get_size(city_table) !=
          context->city_count * sizeof(RoadMapCountyCity)) {
      roadmap_log (ROADMAP_FATAL, "invalid county/city structure");
   }

   if (roadmap_db_get_size(county_table) !=
          context->county_count * sizeof(RoadMapCounty)) {
      roadmap_log (ROADMAP_FATAL, "invalid county/data structure");
   }

   return context;
}

static void roadmap_county_activate (void *context) {

   RoadMapCountyContext *county_context = (RoadMapCountyContext *) context;

   if (county_context->type != RoadMapCountyType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (invalid context type)");
   }
   RoadMapCountyActive = county_context;
}

static void roadmap_county_unmap (void *context) {

   RoadMapCountyContext *county_context = (RoadMapCountyContext *) context;

   if (county_context->type != RoadMapCountyType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (invalid context type)");
   }

   if (county_context == RoadMapCountyActive) {
      RoadMapCountyActive = NULL;
   }

   free (county_context);
}

roadmap_db_handler RoadMapCountyHandler = {
   "county",
   roadmap_county_map,
   roadmap_county_activate,
   roadmap_county_unmap
};


int roadmap_county_by_position
       (const RoadMapPosition *position, int *fips, int count) {

   int i;
   int j;
   int k;
   int found;
   RoadMapCounty *this_county;
   RoadMapCountyByState *this_state;


   found = 0;

   /* First find the counties that might "cover" the given location.
    * these counties have priority.
    */
   for (i = 1; i <= RoadMapCountyActive->state_count; i++) {

      this_state = RoadMapCountyActive->state + i;

      if (this_state->symbol == 0) continue; /* Unused FIPS. */

      if (position->longitude > this_state->max_longitude) continue;
      if (position->longitude < this_state->min_longitude) continue;
      if (position->latitude  > this_state->max_latitude)  continue;
      if (position->latitude  < this_state->min_latitude)  continue;

      if (this_state->min_latitude == this_state->max_latitude) continue;

      for (j = this_state->first_county; j <= this_state->last_county; j++) {

         this_county = RoadMapCountyActive->county + j;

         if (position->longitude > this_county->max_longitude) continue;
         if (position->longitude < this_county->min_longitude) continue;
         if (position->latitude  > this_county->max_latitude)  continue;
         if (position->latitude  < this_county->min_latitude)  continue;

         fips[found++] = this_county->fips;

         if (found >= count) return found;
      }
   }

   /* Then find the counties that are merely visible.
    */
   for (i = 1; i <= RoadMapCountyActive->state_count; i++) {

      this_state = RoadMapCountyActive->state + i;

      if (this_state->symbol == 0) continue; /* Unused FIPS. */

      if (! roadmap_math_is_visible (this_state->min_longitude,
                                     this_state->max_longitude,
                                     this_state->max_latitude,
                                     this_state->min_latitude)) {
         continue;
      }

      if (this_state->min_latitude == this_state->max_latitude) continue;

      for (j = this_state->first_county; j <= this_state->last_county; j++) {

         this_county = RoadMapCountyActive->county + j;

         if (roadmap_math_is_visible (this_county->min_longitude,
                                      this_county->max_longitude,
                                      this_county->max_latitude,
                                      this_county->min_latitude)) {

            /* Do not register the same county more than once. */
            for (k = 0; k < found; k++) {
               if (fips[k] == this_county->fips) break;
            }

            if (k >= found) {
               fips[found++] = this_county->fips;
               if (found >= count) return found;
            }
         }
      }
   }

   return found;
}


int roadmap_county_by_city (RoadMapString city, RoadMapString state) {

   int i;
   int j;
   RoadMapCountyCity *this_city;
   RoadMapCountyByState *this_state;

   for (i = 1; i <= RoadMapCountyActive->state_count; i++) {

      this_state = RoadMapCountyActive->state + i;

      if ((this_state->symbol != state) && (this_state->name != state)) {
         continue;
      }

      for (j = this_state->first_city; j <= this_state->last_city; j++) {

         this_city = RoadMapCountyActive->city + j;

         if (this_city->city == city) {
            return this_city->fips;
         }
      }
   }

   return 0;
}


int  roadmap_county_count (void) {

   return RoadMapCountyActive->county_count;
}

