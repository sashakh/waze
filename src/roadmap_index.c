/* roadmap_index.c - Access the directory index used to select a map.
 *
 * LICENSE:
 *
 *   Copyright 2005 Pascal F. Martin
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
 *   int roadmap_index_by_position (RoadMapPosition *position,
 *                                  int *wtid,
 *                                  int count);
 *
 *   int roadmap_index_by_city (const char *city, const char *territory);
 *
 *   extern roadmap_db_handler RoadMapIndexHandler;
 *
 * These functions are used to retrieve a map given a location or city.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_hash.h"
#include "roadmap_dbread.h"
#include "roadmap_db_index.h"
#include "roadmap_dictionary.h"

#include "roadmap_index.h"


static char RoadMapIndexType[] = "RoadMapIndexType";

typedef struct {

   char *type;

   RoadMapAuthority *authority;
   int               authority_count;

   RoadMapTerritory *territory;
   int               territory_count;

   RoadMapMap       *map;
   int               map_count;

   RoadMapString    *name;
   int               name_count;

   RoadMapString    *city;
   int               city_count;

   RoadMapHash *hash;

   RoadMapDictionary classes;
   RoadMapDictionary names;
   RoadMapDictionary cities;
   RoadMapDictionary files;

   char *territory_is_covered;

} RoadMapIndexContext;

static RoadMapIndexContext *RoadMapIndexActive = NULL;


static void *roadmap_index_map (roadmap_db *root) {

   roadmap_db *authority_table;
   roadmap_db *territory_table;
   roadmap_db *map_table;
   roadmap_db *name_table;
   roadmap_db *city_table;

   RoadMapIndexContext *context;


   context = malloc (sizeof(RoadMapIndexContext));
   roadmap_check_allocated(context);

   context->type = RoadMapIndexType;

   authority_table = roadmap_db_get_subsection (root, "authority");
   territory_table = roadmap_db_get_subsection (root, "territory");
   map_table       = roadmap_db_get_subsection (root, "map");
   name_table      = roadmap_db_get_subsection (root, "name");
   city_table      = roadmap_db_get_subsection (root, "city");

   context->authority =
      (RoadMapAuthority *) roadmap_db_get_data (authority_table);
   context->territory =
      (RoadMapTerritory *) roadmap_db_get_data (territory_table);
   context->map  = (RoadMapMap *)    roadmap_db_get_data (map_table);
   context->name = (RoadMapString *) roadmap_db_get_data (name_table);
   context->city = (RoadMapString *) roadmap_db_get_data (city_table);

   context->authority_count = roadmap_db_get_count (authority_table);
   context->territory_count = roadmap_db_get_count (territory_table);
   context->map_count       = roadmap_db_get_count (map_table);
   context->name_count      = roadmap_db_get_count (name_table);
   context->city_count      = roadmap_db_get_count (city_table);

   if (roadmap_db_get_size(authority_table) !=
          context->authority_count * sizeof(RoadMapAuthority)) {
      roadmap_log (ROADMAP_FATAL, "invalid index/authority structure");
   }
   if (roadmap_db_get_size(territory_table) !=
          context->territory_count * sizeof(RoadMapTerritory)) {
      roadmap_log (ROADMAP_FATAL, "invalid index/territory structure");
   }
   if (roadmap_db_get_size(map_table) !=
          context->map_count * sizeof(RoadMapMap)) {
      roadmap_log (ROADMAP_FATAL, "invalid index/map structure");
   }
   if (roadmap_db_get_size(name_table) !=
          context->name_count * sizeof(RoadMapString)) {
      roadmap_log (ROADMAP_FATAL, "invalid index/name structure");
   }
   if (roadmap_db_get_size(city_table) !=
          context->city_count * sizeof(RoadMapString)) {
      roadmap_log (ROADMAP_FATAL, "invalid index/city structure");
   }

   context->names = roadmap_dictionary_open ("name");
   context->cities = roadmap_dictionary_open ("city");
   context->classes = roadmap_dictionary_open ("class");
   context->files = roadmap_dictionary_open ("file");

   context->territory_is_covered = (char *) malloc(context->territory_count);

   return context;
}

static void roadmap_index_activate (void *context) {

   RoadMapIndexContext *index_context = (RoadMapIndexContext *) context;

   if (index_context != NULL) {

      if (index_context->type != RoadMapIndexType) {
         roadmap_log (ROADMAP_FATAL, "cannot activate (invalid context type)");
      }

      index_context->hash =
         roadmap_hash_new ("territoryIndex", index_context->territory_count);
   }
   RoadMapIndexActive = index_context;
}

static void roadmap_index_unmap (void *context) {

   RoadMapIndexContext *index_context = (RoadMapIndexContext *) context;

   if (index_context->type != RoadMapIndexType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (invalid context type)");
   }

   if (index_context == RoadMapIndexActive) {
      RoadMapIndexActive = NULL;
   }

   roadmap_hash_free (index_context->hash);

   free (index_context->territory_is_covered);
   free (index_context);
}

roadmap_db_handler RoadMapIndexHandler = {
   "index",
   roadmap_index_map,
   roadmap_index_activate,
   roadmap_index_unmap
};


static int roadmap_index_search_territory (int wtid) {

   int i;

   for (i = roadmap_hash_get_first (RoadMapIndexActive->hash, wtid);
        i >= 0;
        i = roadmap_hash_get_next (RoadMapIndexActive->hash, i)) {

      if (wtid == RoadMapIndexActive->territory[i].wtid) return i;
   }

   for (i = RoadMapIndexActive->territory_count - 1; i >= 0; --i) {

      if (wtid == RoadMapIndexActive->territory[i].wtid) {
         roadmap_hash_add (RoadMapIndexActive->hash, wtid, i);
         return i;
      }
   }

   return -1;
}


static RoadMapAuthority *roadmap_index_search_authority (const char *name) {

   int i;
   int j;
   int names_end;
   RoadMapString index;
   RoadMapAuthority *this_authority;


   index = roadmap_dictionary_locate (RoadMapIndexActive->names, name);
   if (index <= 0) return NULL;

   for (i = RoadMapIndexActive->authority_count - 1; i >= 0; --i) {

      this_authority = RoadMapIndexActive->authority + i;

      if (this_authority->symbol == index) return this_authority;

      names_end = this_authority->name_first + this_authority->name_count;

      for (j = this_authority->name_first; j < names_end; ++j) {
         if (RoadMapIndexActive->name[j] == index) return this_authority;
      }
   }

   return NULL;
}


int roadmap_index_by_position
       (const RoadMapPosition *position, int *wtid, int count) {

   int i;
   int j;
   int territory_end;
   int found;
   RoadMapAuthority *this_authority;
   RoadMapTerritory *this_territory;


   if (RoadMapIndexActive == NULL) {

      /* There is no point of looking for a territory if none is available. */
      return 0;
   }

   found = 0;

   /* First find the territories that might "cover" the given location.
    * these territories have priority.
    */
   for (i = RoadMapIndexActive->authority_count - 1; i >= 0; --i) {

      this_authority = RoadMapIndexActive->authority + i;

      if (this_authority->symbol == 0) continue; /* Unused entry. */

      if (position->longitude > this_authority->edges.east) continue;
      if (position->longitude < this_authority->edges.west) continue;
      if (position->latitude  > this_authority->edges.north)  continue;
      if (position->latitude  < this_authority->edges.south)  continue;

      if (this_authority->edges.south == this_authority->edges.north) continue;

      territory_end =
         this_authority->territory_first + this_authority->territory_count;

      for (j = this_authority->territory_first; j < territory_end; ++j) {

         this_territory = RoadMapIndexActive->territory + j;

         if (position->longitude > this_territory->edges.east) continue;
         if (position->longitude < this_territory->edges.west) continue;
         if (position->latitude  > this_territory->edges.north)  continue;
         if (position->latitude  < this_territory->edges.south)  continue;

         wtid[found++] = j;
         RoadMapIndexActive->territory_is_covered[j] = 1;

         if (found >= count) goto search_complete;
      }
   }


   /* Then find the territories that are merely visible.
    */
   for (i = RoadMapIndexActive->authority_count - 1; i >= 0; --i) {

      this_authority = RoadMapIndexActive->authority + i;

      if (this_authority->symbol == 0) continue; /* Unused entry. */

      if (! roadmap_math_is_visible (&(this_authority->edges))) continue;

      if (this_authority->edges.south == this_authority->edges.north) continue;

      territory_end =
         this_authority->territory_first + this_authority->territory_count;

      for (j = this_authority->territory_first; j < territory_end; ++j) {

         this_territory = RoadMapIndexActive->territory + j;

         if (roadmap_math_is_visible (&(this_territory->edges))) {

            /* Do not register the same territory more than once. */
            if (RoadMapIndexActive->territory_is_covered[j]) continue;

            wtid[found++] = j;
            if (found >= count) goto search_complete;
         }
      }
   }

search_complete:

   /* Translate the territory's index into it's wtid and reset the
    * coverage flag.
    */
   for (i = found - 1; i >= 0; --i) {
      j = wtid[i];
      wtid[i] = RoadMapIndexActive->territory[j].wtid;
      RoadMapIndexActive->territory_is_covered[j] = 0;
   }

   return found;
}


int roadmap_index_by_city (const char *authority, const char *city) {

   int i;
   int j;
   int city_end;
   int territory_end;
   RoadMapAuthority *this_authority;
   RoadMapTerritory *this_territory;

   RoadMapString city_index;


   if (RoadMapIndexActive == NULL) return 0;

   this_authority = roadmap_index_search_authority (authority);
   if (this_authority == NULL) {
      return 0; /* Entity is not in this map. */
   }

   city_index = roadmap_dictionary_locate (RoadMapIndexActive->cities, city);

   territory_end =
      this_authority->territory_first + this_authority->territory_count;

   for (j = this_authority->territory_first; j < territory_end; ++j) {

      this_territory = RoadMapIndexActive->territory + j;

      city_end = this_territory->city_first + this_territory->city_count;

      for (i = this_territory->city_first; i < city_end; ++i) {

         if (RoadMapIndexActive->city[i] == city_index) {
            return this_territory->wtid;
         }
      }
   }

   return 0; /* No such city in this state. */
}


int roadmap_index_by_authority (const char *name, int *wtid, int count) {

   int i;
   int found;
   int territory_end;
   RoadMapAuthority *this_authority;

   if (RoadMapIndexActive == NULL) return 0;

   this_authority = roadmap_index_search_authority (name);
   if (this_authority == NULL) {
      return 0; /* This authority is not listed in this map. */
   }

   /* Retrieve all the territories, but not more than what is expected by
    * the caller.
    */
   if (count < this_authority->territory_count) {
      territory_end = this_authority->territory_first + count;
   } else {
      territory_end =
         this_authority->territory_first + this_authority->territory_count;
   }

   found = 0;

   for (i = this_authority->territory_first; i < territory_end; ++i) {
      wtid[found++] = RoadMapIndexActive->territory[i].wtid;
   }

   return found;
}


const char *roadmap_index_get_territory_name (int wtid) {

   int i;

   if (RoadMapIndexActive == NULL) return "";

   i = roadmap_index_search_territory (wtid);
   if (i < 0) return "";

   return roadmap_dictionary_get (RoadMapIndexActive->names,
                                  RoadMapIndexActive->territory[i].name);
}


const char *roadmap_index_get_authority_symbol (int wtid) {

   int i;
   int index;
   RoadMapAuthority *this_authority;


   if (RoadMapIndexActive == NULL) return "";

   index = roadmap_index_search_territory (wtid);
   if (index < 0) return "";

   for (i = RoadMapIndexActive->authority_count - 1; i >= 0; --i) {

      this_authority = RoadMapIndexActive->authority + i;

      if (index >= this_authority->territory_first &&
          index <= this_authority->territory_first
                       + this_authority->territory_count) {
         return roadmap_dictionary_get
                   (RoadMapIndexActive->names, this_authority->symbol);
      }
   }
   return "";
}


const char *roadmap_index_get_authority_name (int wtid) {

   int i;
   int index;
   RoadMapAuthority *this_authority;


   if (RoadMapIndexActive == NULL) return "";

   index = roadmap_index_search_territory (wtid);
   if (index < 0) return "";

   for (i = RoadMapIndexActive->authority_count - 1; i >= 0; --i) {

      this_authority = RoadMapIndexActive->authority + i;

      if (index >= this_authority->territory_first &&
          index <= this_authority->territory_first
                       + this_authority->territory_count) {
         return roadmap_dictionary_get
                   (RoadMapIndexActive->names,
                    RoadMapIndexActive->name[this_authority->name_first]);
      }
   }
   return "";
}


int roadmap_index_get_territory_count (void) {

   if (RoadMapIndexActive == NULL) {
      return 0; /* None is available, anyway. */
   }
   return RoadMapIndexActive->territory_count;
}


const RoadMapArea *roadmap_index_get_territory_edges (int wtid) {

   int i;

   if (RoadMapIndexActive == NULL) return NULL;

   i = roadmap_index_search_territory (wtid);
   if (i < 0) return NULL;

   return &RoadMapIndexActive->territory[i].edges;
}

