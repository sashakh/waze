/* buildus_county.c - Build a county table & index for RoadMap.
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
 *   void buildus_county_initialize (void);
 *   int  buildus_county_add (int fips,
 *                            RoadMapString name, RoadMapString state_symbol);
 *   int  buildus_county_add_state (RoadMapString name, int code);
 *   void buildus_county_add_city (int fips, RoadMapString city);
 *   void buildus_county_set_position (int fips,
 *                                     int max_longitude, int max_latitude,
 *                                     int min_longitude, int min_latitude);
 *   void buildus_county_sort (void);
 *   void buildus_county_save (void);
 *   void buildus_county_summary (void);
 *
 * These functions are used to build a table of counties from
 * the census bureau list of FIPS code.
 * The goal is to help localize the specific map for a given address
 * or position.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_db_county.h"

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildus_county.h"


struct RoadMapCity {
   int fips;
   RoadMapString name;
};

static int CountyCount = 0;
static RoadMapCounty *County[BUILDMAP_BLOCK] = {NULL};

static int CountyCityCount = 0;
static struct RoadMapCity *CountyCity[BUILDMAP_BLOCK] = {NULL};

static RoadMapHash *CountyByFips = NULL;

static int *SortedCounty = NULL;
static int *SortedCountyCity = NULL;
static int *SortedInverseCounty = NULL;


#define MAX_US_STATE   100

typedef struct {

   RoadMapString name;
   RoadMapString symbol;

   int max_longitude;
   int max_latitude;

   int min_longitude;
   int min_latitude;

} BuildUsState;

static BuildUsState State[MAX_US_STATE+1];
static int          StateCode[MAX_US_STATE+1];

static int StateMaxCode = 0;
static int StateCount   = 0;


void buildus_county_initialize (void) {

   int i;

   CountyByFips = roadmap_hash_new ("CountyByFips", BUILDMAP_BLOCK);

   for (i = MAX_US_STATE; i >= 0; i--) {
      StateCode[i]    = 0;
      State[i].name   = 0;
      State[i].symbol = 0;
   }

   CountyCount = 0;
   CountyCityCount = 0;
   StateMaxCode = 0;
   StateCount = 0;
}


int buildus_county_add
       (int fips, RoadMapString name, RoadMapString state_symbol) {

   int index;
   int block;
   int offset;
   RoadMapCounty *this_county;


   /* First search if that county is not yet known. */

   for (index = roadmap_hash_get_first (CountyByFips, fips);
        index >= 0;
        index = roadmap_hash_get_next (CountyByFips, index)) {

       this_county = County[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

       if (this_county->fips == fips) {

          if (this_county->name != name) {
             buildmap_fatal (0, "non unique county FIPS code");
          }
          return index;
        }
   }

   /* This county was not known yet: create a new one. */

   block = CountyCount / BUILDMAP_BLOCK;
   offset = CountyCount % BUILDMAP_BLOCK;

   if (County[block] == NULL) {

      /* We need to add a new block to the table. */

      County[block] = calloc (BUILDMAP_BLOCK, sizeof(RoadMapCounty));
      if (County[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
      roadmap_hash_resize (CountyByFips, (block+1) * BUILDMAP_BLOCK);
   }

   this_county = County[block] + offset;

   this_county->fips = fips;
   this_county->name = name;

   roadmap_hash_add (CountyByFips, fips, CountyCount);

   if (CountyCount > 0xffff) {
      buildmap_fatal (0, "too many counties");
   }

   fips /= 1000; /* State code. */

   if (StateCode[fips] <= 0) {

      for (index = StateCount; index > 0; --index) {
          if (State[index].symbol == state_symbol) {
              break;
          }
      }

      if (index <= 0) {
          buildmap_fatal (0, "invalid state symbol");
      }
      StateCode[fips] = index;

      if (fips > StateMaxCode) {
         StateMaxCode = fips;
      }

   } else if (State[StateCode[fips]].symbol != state_symbol) {

      buildmap_fatal (0, "invalid state FIPS");
   }

   return CountyCount++;
}


void buildus_county_add_state (RoadMapString name, RoadMapString symbol) {

   int i;

   /* Search if that state is not yet known. */

   for (i = StateCount; i > 0; --i) {

       if (State[i].symbol == symbol) {

           if (State[i].name != name) {

               buildmap_fatal (0, "state symbol conflict");
           }
           return;
       }
   }

   StateCount += 1;

   State[StateCount].name   = name;
   State[StateCount].symbol = symbol;

   State[StateCount].max_longitude = 0;
   State[StateCount].max_latitude  = 0;
   State[StateCount].min_longitude = 0;
   State[StateCount].min_latitude  = 0;
}


void buildus_county_add_city (int fips, RoadMapString city) {

   int index;
   int block;
   int offset;
   RoadMapCounty *this_county;
   struct RoadMapCity *this_city;


   /* First retrieve the county. */

   for (index = roadmap_hash_get_first (CountyByFips, fips);
        index >= 0;
        index = roadmap_hash_get_next (CountyByFips, index)) {

       this_county = County[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

       if (this_county->fips == fips) break;
   }

   if (index < 0) {
      buildmap_fatal (0, "unknown county FIPS code %05d", fips);
   }

   block = CountyCityCount / BUILDMAP_BLOCK;
   offset = CountyCityCount % BUILDMAP_BLOCK;

   if (CountyCity[block] == NULL) {

      /* We need to add a new block to the table. */

      CountyCity[block] = calloc (BUILDMAP_BLOCK, sizeof(struct RoadMapCity));
      if (CountyCity[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
   }

   this_city = CountyCity[block] + offset;

   this_city->fips = fips;
   this_city->name = city;

   CountyCityCount += 1;
}


void buildus_county_set_position (int fips,
                                  int max_longitude, int max_latitude,
                                  int min_longitude, int min_latitude) {

   int index;
   RoadMapCounty *this_county;


   for (index = roadmap_hash_get_first (CountyByFips, fips);
        index >= 0;
        index = roadmap_hash_get_next (CountyByFips, index)) {

      this_county = County[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      if (this_county->fips == fips) {

         this_county->max_longitude = max_longitude;
         this_county->max_latitude  = max_latitude;
         this_county->min_longitude = min_longitude;
         this_county->min_latitude  = min_latitude;

         index = StateCode[fips / 1000];

         if (index <= 0 || index > MAX_US_STATE) {
            buildmap_fatal (0, "invalid state code");
         }

         if ((State[index].max_longitude == 0) ||
             (State[index].max_longitude < max_longitude)) {
            State[index].max_longitude = max_longitude;
         }
         if ((State[index].min_longitude == 0) ||
             (State[index].min_longitude > min_longitude)) {
            State[index].min_longitude = min_longitude;
         }
         if ((State[index].max_latitude == 0) ||
             (State[index].max_latitude < max_latitude)) {
            State[index].max_latitude = max_latitude;
         }
         if ((State[index].min_latitude == 0) ||
             (State[index].min_latitude > min_latitude)) {
            State[index].min_latitude = min_latitude;
         }

         return;
      }
   }

   if (index < 0) {
      buildmap_fatal (0, "unknown county FIPS code %05d", fips);
   }
}


static int buildmap_county_compare (const void *r1, const void *r2) {

   int index1 = *((int *)r1);
   int index2 = *((int *)r2);

   RoadMapCounty *record1;
   RoadMapCounty *record2;

   record1 = County[index1/BUILDMAP_BLOCK] + (index1 % BUILDMAP_BLOCK);
   record2 = County[index2/BUILDMAP_BLOCK] + (index2 % BUILDMAP_BLOCK);


   /* The lines are sorted by FIPS code, which sorts them by state too. */

   return record1->fips - record2->fips;
}

static int buildmap_county_compare_city (const void *r1, const void *r2) {

   int state1;
   int state2;

   int index1 = *((int *)r1);
   int index2 = *((int *)r2);

   struct RoadMapCity *record1;
   struct RoadMapCity *record2;

   record1 = CountyCity[index1/BUILDMAP_BLOCK] + (index1 % BUILDMAP_BLOCK);
   record2 = CountyCity[index2/BUILDMAP_BLOCK] + (index2 % BUILDMAP_BLOCK);


   /* The lines are sorted by state, then by string index. */

   state1 = record1->fips / 1000;
   state2 = record2->fips / 1000;

   if (state1 != state2) {
      return state1 - state2;
   }

   return record1->name - record2->name;
}

void buildus_county_sort (void) {

   int i;

   if (SortedCounty != NULL) return; /* Sort was already performed. */

   buildmap_info ("sorting counties...");

   SortedCounty = malloc (CountyCount * sizeof(int));
   if (SortedCounty == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   for (i = 0; i < CountyCount; i++) {
      SortedCounty[i] = i;
   }

   qsort (SortedCounty, CountyCount, sizeof(int), buildmap_county_compare);

   SortedInverseCounty = malloc (CountyCount * sizeof(int));
   if (SortedInverseCounty == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   for (i = 0; i < CountyCount; i++) {
      SortedInverseCounty[SortedCounty[i]] = i;
   }

   SortedCountyCity = malloc (CountyCityCount * sizeof(int));
   if (SortedCountyCity == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   for (i = 0; i < CountyCityCount; i++) {
      SortedCountyCity[i] = i;
   }

   qsort (SortedCountyCity, CountyCityCount, sizeof(int),
          buildmap_county_compare_city);
}


void buildus_county_save (void) {

   int i;
   int j;
   int k;
   int state;
   int state_current;
   int state_max;

   RoadMapCounty *one_county;
   struct RoadMapCity   *one_city;

   RoadMapCounty *db_county;
   RoadMapCountyCity *db_city;
   RoadMapCountyByState *db_state;

   buildmap_db *root;
   buildmap_db *data_table;
   buildmap_db *city_table;
   buildmap_db *bystate_table;


   buildmap_info ("saving counties...");

   one_county = County[(CountyCount-1)/BUILDMAP_BLOCK]
                    + ((CountyCount-1) % BUILDMAP_BLOCK);
   state_max = one_county->fips / 1000;

   if (state_max != StateMaxCode) {
      buildmap_fatal
         (0, "abnormal state count: counties know %d, state lists %d",
             state_max, StateMaxCode);
   }

   root = buildmap_db_add_section (NULL, "county");

   data_table = buildmap_db_add_section (root, "data");
   buildmap_db_add_data (data_table, CountyCount, sizeof(RoadMapCounty));

   city_table = buildmap_db_add_section (root, "city2county");
   buildmap_db_add_data
      (city_table, CountyCityCount, sizeof(RoadMapCountyCity));

   bystate_table = buildmap_db_add_section (root, "bystate");
   buildmap_db_add_data (bystate_table,
                         state_max + 1, sizeof(RoadMapCountyByState));

   db_county = (RoadMapCounty *) buildmap_db_get_data (data_table);
   db_city   = (RoadMapCountyCity *) buildmap_db_get_data (city_table);
   db_state  = (RoadMapCountyByState *) buildmap_db_get_data (bystate_table);

   state_current = -1;

   for (i = 0; i < CountyCount; i++) {

      j = SortedCounty[i];

      one_county = County[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      db_county[i] = *one_county;

      state = one_county->fips / 1000;

      if (state != state_current) {

         if (state > state_max) {
            buildmap_fatal
               (0, "abnormal state order: %d is out of bound", state);
         }
         if (state < state_current) {
            buildmap_fatal (0, "abnormal state order: %d following %d",
                               state, state_current);
         }
         if (state_current >= 0) {
            db_state[state_current].last_county = (unsigned short) (i - 1);
         }

         state_current = state;
         db_state[state_current].first_county = (unsigned short) i;
      }
   }
   db_state[state_current].last_county = CountyCount - 1;


   state_current = -1;

   for (i = 0; i < CountyCityCount; i++) {

      j = SortedCountyCity[i];

      one_city = CountyCity[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      state = one_city->fips / 1000;

      if (state != state_current) {

         if (state > state_max) {
            buildmap_fatal
               (0, "abnormal state order: %d is out of bound", state);
         }
         if (state < state_current) {
            buildmap_fatal (0, "abnormal state order: %d following %d",
                               state, state_current);
         }
         if (state_current >= 0) {
            db_state[state_current].last_city = (unsigned short) (i - 1);
         }

         state_current = state;
         db_state[state_current].first_city = (unsigned short) i;
      }

      for (k = db_state[state].first_county;
           k <= db_state[state].last_county; ++k) {
         if (one_city->fips == db_county[k].fips) {
            db_city[i].county = k;
            break;
         }
      }
      if (k > db_state[state].last_county) {
         buildmap_fatal (0, "cannot find county with FIPS %d for city %d",
                         one_city->fips, one_city->name);
      }

      db_city[i].city = one_city->name;

   }
   db_state[state_current].last_city = CountyCityCount - 1;

   for (i = 1; i <= StateMaxCode; i++) {

      int state = StateCode[i];

      if (state > 0) {

          db_state[i].name   = State[state].name;
          db_state[i].symbol = State[state].symbol;

          db_state[i].max_longitude = State[state].max_longitude;
          db_state[i].min_longitude = State[state].min_longitude;
          db_state[i].max_latitude  = State[state].max_latitude;
          db_state[i].min_latitude  = State[state].min_latitude;
      }
   }
}


void buildus_county_summary (void) {

}

