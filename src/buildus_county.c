#undef	VERBOSE
/*
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
 */

/**
 * @file
 * @brief Build a county table & index for RoadMap.
 *
 * SYNOPSYS:
 *
 *   int  buildus_county_add (int fips,
 *                            RoadMapString name, RoadMapString state_symbol);
 *   int  buildus_county_add_state (RoadMapString name, int code);
 *   void buildus_county_add_city (int fips, RoadMapString city);
 *
 *   void buildus_county_set_position (int fips,
 *                                     const RoadMapArea *bounding_box);
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

#include "roadmap_dictionary.h"

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


/*
 * This needs to be defined as a much larger number than 99 :
 * - there's not only the US states but the ISO countries too,
 * - there's code there that relies on the 'fips' (now 1000 + country code)
 *   to fit in the array StateCode
 */
#define MAX_US_STATE   2000

/**
 * @brief
 */
typedef struct {
   RoadMapString name;		/**< */
   RoadMapString symbol;	/**< */
   RoadMapArea edges;		/**< */
} BuildUsState;

static BuildUsState State[MAX_US_STATE+1];
static int StateCode[MAX_US_STATE+1];

static int StateMaxCode = 0;
static int StateCount   = 0;


static void buildus_county_register (void);

/**
 * @brief
 */
static void buildus_county_initialize (void) {

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

   buildus_county_register();
}

/**
 * @brief
 * @param fips
 * @param name
 * @param state_symbol
 * @return
 */
int buildus_county_add
       (int fips, RoadMapString name, RoadMapString state_symbol) {

   int index;
   int block;
   int offset;
   RoadMapCounty *this_county;

#ifdef	VERBOSE
   static BuildMapDictionary county_dictionary;
   static BuildMapDictionary BuildMapStateDictionary;

   county_dictionary = buildmap_dictionary_open ("county");
   BuildMapStateDictionary = buildmap_dictionary_open ("state");
   fprintf(stderr, "buildus_county_add(%d,", fips);
   fprintf(stderr, " %s, %s)\n",
		   buildmap_dictionary_get(county_dictionary, name),
		   buildmap_dictionary_get(BuildMapStateDictionary, state_symbol));
#endif

   if (CountyByFips == NULL) buildus_county_initialize ();


   /* First search if that county is not yet known. */

   for (index = roadmap_hash_get_first (CountyByFips, fips);
        index >= 0;
        index = roadmap_hash_get_next (CountyByFips, index)) {

       this_county = County[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

       if (this_county->fips == fips) {

          if (this_county->name != name) {
	     static BuildMapDictionary county_dictionary;
	     county_dictionary = buildmap_dictionary_open ("county");
             buildmap_fatal (0, "non unique county FIPS code %d (%s, %s)",
			     fips,
			     buildmap_dictionary_get(county_dictionary, name),
			     buildmap_dictionary_get(county_dictionary, this_county->name));
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
      static BuildMapDictionary BuildMapStateDictionary;
      BuildMapStateDictionary = buildmap_dictionary_open ("state");

      buildmap_fatal (0, "invalid state FIPS (fips %d, state %s), not %s",
		      fips,
		      buildmap_dictionary_get(BuildMapStateDictionary,
			      state_symbol),
		      buildmap_dictionary_get(BuildMapStateDictionary,
			      State[StateCode[fips]].symbol));
   }

   return CountyCount++;
}

/**
 * @brief
 * @param name
 * @param symbol
 */
void buildus_county_add_state (RoadMapString name, RoadMapString symbol) {

   int i;
   RoadMapArea area_reset = {0, 0, 0, 0};

#ifdef	VERBOSE
   BuildMapDictionary state_dictionary = buildmap_dictionary_open ("state");
   fprintf(stderr, "buildus_county_add_state(%s, %s)\n",
		   buildmap_dictionary_get(state_dictionary, name),
		   buildmap_dictionary_get(state_dictionary, symbol));
#endif

   if (CountyByFips == NULL) buildus_county_initialize ();

   /* Search if that state is not yet known. */

   for (i = StateCount; i > 0; --i) {

       if (State[i].symbol == symbol) {

           if (State[i].name != name) {
		   static BuildMapDictionary state_dictionary;

		   state_dictionary = buildmap_dictionary_open ("state");
               buildmap_fatal (0, "state symbol conflict (%s %s %s)",
			       buildmap_dictionary_get(state_dictionary, symbol),
			       buildmap_dictionary_get(state_dictionary, State[i].name),
			       buildmap_dictionary_get(state_dictionary, name));
           }
           return;
       }
   }


   if (StateCount == MAX_US_STATE - 1) {
	   buildmap_fatal(0, "Cannot add more than %d states", MAX_US_STATE);
   }

   StateCount += 1;
   State[StateCount].name   = name;
   State[StateCount].symbol = symbol;

   State[StateCount].edges = area_reset;
}

/**
 * @brief
 * @param fips
 * @param city
 */
void buildus_county_add_city (int fips, RoadMapString city) {

   int index;
   int block;
   int offset;
   RoadMapCounty *this_county;
   struct RoadMapCity *this_city;

#ifdef	VERBOSE

   RoadMapDictionary cities;
   cities = buildmap_dictionary_open ("city");
   fprintf(stderr, "buildus_county_add_city (%d, %d - %s)\n",
		  fips, city, buildmap_dictionary_get(cities, city));
#endif

   if (CountyByFips == NULL) buildus_county_initialize ();

   /* First retrieve the county. */

   for (index = roadmap_hash_get_first (CountyByFips, fips);
        index >= 0;
        index = roadmap_hash_get_next (CountyByFips, index)) {

       this_county = County[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

       if (this_county->fips == fips) break;
   }

   if (index < 0) {
      buildmap_fatal (0, "unknown county FIPS code %05d (add_city)", fips);
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

/**
 * @brief
 * @param fips
 * @param bounding_box
 */
void buildus_county_set_position (int fips,
                                  const RoadMapArea *bounding_box) {

   int index;
   RoadMapCounty *this_county;

   buildmap_info ( "County %d geometry n %d s %d w %d e %d",
		   fips,
		   bounding_box->north,
		   bounding_box->south,
		   bounding_box->west,
		   bounding_box->east);

   for (index = roadmap_hash_get_first (CountyByFips, fips);
        index >= 0;
        index = roadmap_hash_get_next (CountyByFips, index)) {

      this_county = County[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      if (this_county->fips == fips) {

         this_county->edges = *bounding_box;

         index = StateCode[fips / 1000];

         if (index <= 0 || index > MAX_US_STATE) {
            buildmap_fatal (0, "invalid state code");
         }

         if ((State[index].edges.east == 0) ||
             (State[index].edges.east < bounding_box->east)) {
            State[index].edges.east = bounding_box->east;
         }
         if ((State[index].edges.west == 0) ||
             (State[index].edges.west > bounding_box->west)) {
            State[index].edges.west = bounding_box->west;
         }
         if ((State[index].edges.north == 0) ||
             (State[index].edges.north < bounding_box->north)) {
            State[index].edges.north = bounding_box->north;
         }
         if ((State[index].edges.south == 0) ||
             (State[index].edges.south >  bounding_box->south)) {
            State[index].edges.south =  bounding_box->south;
         }

         return;
      }
   }

   if (index < 0) {
      buildmap_fatal (0, "unknown county FIPS code %05d (set_position)", fips);
   }
}

/**
 * @brief
 * @param r1
 * @param r2
 * @return
 */
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

/**
 * @brief
 * @param r1
 * @param r2
 * @return
 */
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

/**
 * @brief
 */
static void buildus_county_sort (void) {

   int i;

   if (CountyCount == 0) return;

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

/**
 * @brief
 */
static void buildus_county_save (void) {

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

#if 1
   if (state_max != StateMaxCode) {
      buildmap_fatal
         (0, "abnormal state count: counties know %d, state lists %d",
             state_max, StateMaxCode);
   }
#else
   state_max = StateMaxCode;
#endif

   root = buildmap_db_add_section (NULL, "county");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   data_table = buildmap_db_add_section (root, "data");
   if (data_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (data_table, CountyCount, sizeof(RoadMapCounty));

   city_table = buildmap_db_add_section (root, "city2county");
   if (city_table == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data
      (city_table, CountyCityCount, sizeof(RoadMapCountyCity));

   bystate_table = buildmap_db_add_section (root, "bystate");
   if (bystate_table == NULL) buildmap_fatal (0, "Can't add a new section");
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

          db_state[i].edges = State[state].edges;
      }
   }
}

/**
 * @brief
 */
static buildmap_db_module BuildUsCountyModule = {
   "county",
   buildus_county_sort,
   buildus_county_save,
   NULL,
   NULL
};

/**
 * @brief
 */
static void buildus_county_register (void) {
   buildmap_db_register (&BuildUsCountyModule);
}

