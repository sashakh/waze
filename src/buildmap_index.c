/* buildmap_index.c - Build the index table for RoadMap.
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
 *   void buildmap_index_initialize (const char *path);
 *
 *   void buildmap_index_add_map (int wtid,
 *                                const char *class,
 *                                const char *authority,
 *                                const char *filename);
 *   void buildmap_index_set_territory_name (const char *name);
 *   void buildmap_index_add_authority_name (const char *name);
 *   void buildmap_index_set_map_edges      (const RoadMapArea *edges);
 *   void buildmap_index_add_city           (const char *city);
 *   void buildmap_index_add_postal_code    (unsigned int code);
 *
 *   void buildmap_index_sort (void);
 *   void buildmap_index_save (void);
 *   void buildmap_index_summary (void);
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

#include "roadmap_db_index.h"

#include "roadmap_path.h"
#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_index.h"


#define BUILDMAP_MAX_NAME    16   // Arbitrary value...


static BuildMapDictionary MapClasses;
static BuildMapDictionary MapCities;
static BuildMapDictionary MapNames;
static BuildMapDictionary MapFiles;


typedef struct buildmap_city_record {

   struct buildmap_city_record *next;

   RoadMapString name;

} BuildMapCity;

typedef struct buildmap_file_record {

   struct buildmap_file_record *next;

   char *filename;

   RoadMapString class;
   RoadMapString filename_index;

} BuildMapFile;

typedef struct buildmap_territory_record {

   struct buildmap_territory_record *next;

   char *pathname;

   int wtid;

   RoadMapString authority;
   RoadMapString pathname_index;
   RoadMapString name;

   RoadMapArea edges;

   BuildMapFile *maps;
   BuildMapCity *cities;

   unsigned int city_count;
   unsigned int postal_low;
   unsigned int postal_high;

} BuildMapTerritory;

typedef struct {

   char *pathname;

   RoadMapString symbol;
   RoadMapString pathname_index;

   int name_count;
   RoadMapString names[BUILDMAP_MAX_NAME];

   RoadMapArea edges;

   BuildMapTerritory *territories;

} BuildMapAuthority;


static int AuthorityCount = 0;
static BuildMapAuthority *MapAuthority[BUILDMAP_BLOCK] = {NULL};

static BuildMapAuthority *BuildMapCurrentAuthority = NULL;

static int TerritoryCount = 0;
static BuildMapTerritory *Territory[BUILDMAP_BLOCK] = {NULL};

static BuildMapTerritory *BuildMapCurrentTerritory = NULL;

static int MapCount = 0;
static BuildMapFile *Map[BUILDMAP_BLOCK] = {NULL};

static int CityCount = 0;
static BuildMapCity *MapCity[BUILDMAP_BLOCK] = {NULL};

static int NameCount = 0;

static RoadMapHash *TerritoryByWtid = NULL;
static RoadMapHash *AuthorityBySymbol = NULL;

static const char *BuildMapBasePath = NULL;
static       int   BuildMapBaseLength = 0;


void buildmap_index_initialize (const char *path) {

   TerritoryByWtid = roadmap_hash_new ("TerritoryByWtid", BUILDMAP_BLOCK);
   AuthorityBySymbol = roadmap_hash_new ("AuthorityBySymbol", BUILDMAP_BLOCK);

   MapClasses = buildmap_dictionary_open ("class");
   MapCities = buildmap_dictionary_open ("city");
   MapNames = buildmap_dictionary_open ("name");
   MapFiles = buildmap_dictionary_open ("file");

   MapCount = 0;
   CityCount = 0;
   TerritoryCount = 0;
   AuthorityCount = 0;

   BuildMapBasePath = strdup(path);
   BuildMapBaseLength = strlen(BuildMapBasePath);
}


static BuildMapAuthority *
            buildmap_index_search_authority (RoadMapString authority) {

   int i;
   int index;
   BuildMapAuthority *this_authority;


   for (index = roadmap_hash_get_first (AuthorityBySymbol, authority);
        index >= 0;
        index = roadmap_hash_get_next (AuthorityBySymbol, index)) {

         this_authority =
            MapAuthority[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

         if (this_authority->symbol == authority) return this_authority;

         for (i = 0; i < this_authority->name_count; ++i) {
            if (this_authority->names[i] == authority) return this_authority;
         }
   }

   return NULL;
}


static BuildMapAuthority *buildmap_index_new_authority (RoadMapString symbol) {

   int block = AuthorityCount / BUILDMAP_BLOCK;
   int offset = AuthorityCount % BUILDMAP_BLOCK;

   BuildMapAuthority *this_authority;


   if (MapAuthority[block] == NULL) {

      /* We need to add a new block to the table. */

      MapAuthority[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapAuthority));
      if (MapAuthority[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
   }

   this_authority = MapAuthority[block] + offset;

   this_authority->symbol = symbol;
   this_authority->name_count = 0;
   this_authority->territories = NULL;

   if (++AuthorityCount > 0xffff) {
      buildmap_fatal (0, "too many authorities for a single index");
   }

   BuildMapCurrentAuthority = this_authority;

   roadmap_hash_add (AuthorityBySymbol, symbol, AuthorityCount);

   return this_authority;
}

static BuildMapTerritory *buildmap_index_new_territory
                              (int wtid, BuildMapAuthority *this_authority) {

   int block = TerritoryCount / BUILDMAP_BLOCK;
   int offset = TerritoryCount % BUILDMAP_BLOCK;

   BuildMapTerritory *this_territory;


   if (Territory[block] == NULL) {

      /* We need to add a new block to the table. */

      Territory[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapTerritory));
      if (Territory[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
      roadmap_hash_resize (TerritoryByWtid, (block+1) * BUILDMAP_BLOCK);
   }

   this_territory = Territory[block] + offset;

   this_territory->wtid = wtid;
   this_territory->authority = this_authority->symbol;

   this_territory->maps = NULL;
   this_territory->cities = NULL;
   this_territory->city_count = 0;

   this_territory->postal_low = 0;
   this_territory->postal_high = 0;

   roadmap_hash_add (TerritoryByWtid, wtid, TerritoryCount);

   this_territory->next = this_authority->territories;
   this_authority->territories = this_territory;

   if (++TerritoryCount > 0xffff) {
      buildmap_fatal (0, "too many territories for a single index");
   }

   BuildMapCurrentTerritory = this_territory;

   return this_territory;
}


static void buildmap_index_new_map (BuildMapTerritory *territory,
                                    RoadMapString class,
                                    const char *filename) {

   int block = MapCount / BUILDMAP_BLOCK;
   int offset = MapCount % BUILDMAP_BLOCK;

   BuildMapFile *this_map;


   if (Map[block] == NULL) {

      /* We need to add a new block to the table. */

      Map[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapFile));
      if (Map[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
   }

   this_map = Map[block] + offset;

   this_map->class = class;

   /* If the map file is in a subdirectory of the index we are creating,
    * record only the relative path. This way the whole set can be moved
    * or installed elsewhere without having to rebuild the index.
    */
   if (strncmp (BuildMapBasePath, filename, BuildMapBaseLength) == 0) {

      const char *relative =
         roadmap_path_skip_separator (filename+BuildMapBaseLength);

      if (relative != NULL) filename = relative;
   }
   this_map->filename = strdup(filename);

   this_map->next = territory->maps;
   territory->maps = this_map;

   if (++MapCount > 0xffff) {
      buildmap_fatal (0, "too many maps for a single index");
   }
}


static void buildmap_index_include_area (RoadMapArea *outer,
                                         const RoadMapArea *inner) {

   if ((outer->east == 0) || (outer->east < inner->east)) {
      outer->east = inner->east;
   }
   if ((outer->west == 0) || (outer->west > inner->west)) {
      outer->west = inner->west;
   }
   if ((outer->north == 0) || (outer->north < inner->north)) {
      outer->north = inner->north;
   }
   if ((outer->south == 0) || (outer->south >  inner->south)) {
      outer->south = inner->south;
   }
}


void buildmap_index_add_map (int wtid,
                             const char *class,
                             const char *authority,
                             const char *filename) {

   int index;
   BuildMapFile *this_map;
   BuildMapTerritory *this_territory;
   BuildMapAuthority *this_authority;

   RoadMapString class_index;
   RoadMapString authority_index;


   class_index = buildmap_dictionary_add (MapClasses, class, strlen(class));

   authority_index =
      buildmap_dictionary_add (MapNames, authority, strlen(authority));

   /* First search if that map is not yet known. */

   for (index = roadmap_hash_get_first (TerritoryByWtid, wtid);
        index >= 0;
        index = roadmap_hash_get_next (TerritoryByWtid, index)) {

       this_territory =
          Territory[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

       if (this_territory->wtid == wtid) {

          /* Run some coherency checks. */

          if (this_territory->authority != authority_index) {
             buildmap_fatal
                (0, "territory %d is managed by multiple authorities", wtid);
          }

          for (this_map = this_territory->maps;
               this_map != NULL; this_map = this_map->next) {

             if (this_map->class == class_index) {
                buildmap_fatal
                   (0, "class %s appears more than once for territory %d",
                    class, wtid);
             }
          }

          /* The territory was known: just create the map entry. */
          buildmap_index_new_map (this_territory, class_index, filename);

          BuildMapCurrentTerritory = this_territory;
          BuildMapCurrentAuthority =
             buildmap_index_search_authority (authority_index);
          return;
        }
   }


   /* Now search the authority, or create a new one if necessary. */

   this_authority = buildmap_index_search_authority (authority_index);

   if (this_authority == NULL) {
      this_authority = buildmap_index_new_authority (authority_index);
   } else {
      BuildMapCurrentAuthority = this_authority;
   }

   /* This territory was not known yet: create a new record. */

   this_territory = buildmap_index_new_territory (wtid, this_authority);


   buildmap_index_new_map (this_territory, class_index, filename);
}


void buildmap_index_set_territory_name (const char *name) {

   BuildMapCurrentTerritory->name = 
      buildmap_dictionary_add (MapNames, name, strlen(name));
}


void buildmap_index_add_authority_name (const char *name) {

   int i;
   int count;
   RoadMapString name_index;

   count = BuildMapCurrentAuthority->name_count;
   name_index = buildmap_dictionary_add (MapNames, name, strlen(name));

   for (i = 0; i < count; ++i) {
      if (BuildMapCurrentAuthority->names[i] == name_index) return;
   }

   if (count >= BUILDMAP_MAX_NAME) {

      RoadMapString symbol =
         BuildMapCurrentAuthority->symbol;

      buildmap_fatal (0, "too many names for authority %s",
                      buildmap_dictionary_get (MapNames, symbol));
   }

   BuildMapCurrentAuthority->names[count] = name_index;
   BuildMapCurrentAuthority->name_count = count + 1;
}


void buildmap_index_set_map_edges (const RoadMapArea *edges) {

   if (BuildMapCurrentTerritory == NULL) {
      buildmap_fatal (0, "no current map");
   }
   buildmap_index_include_area (&(BuildMapCurrentTerritory->edges), edges);

   buildmap_index_include_area (&(BuildMapCurrentAuthority->edges),
                                &(BuildMapCurrentTerritory->edges));
}


void buildmap_index_add_city (const char *city) {

   int block;
   int offset;
   BuildMapCity *this_city;
   BuildMapTerritory *this_territory = BuildMapCurrentTerritory;


   block = CityCount / BUILDMAP_BLOCK;
   offset = CityCount % BUILDMAP_BLOCK;

   if (MapCity[block] == NULL) {

      /* We need to add a new block to the table. */

      MapCity[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapCity));
      if (MapCity[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
   }

   this_city = MapCity[block] + offset;

   this_city->next = this_territory->cities;
   this_city->name = buildmap_dictionary_add (MapCities, city, strlen(city));

   this_territory->cities = this_city;
   this_territory->city_count += 1;

   CityCount += 1;
}


void buildmap_index_add_postal_code (unsigned int code) {

   if ((BuildMapCurrentTerritory->postal_low == 0) ||
       (BuildMapCurrentTerritory->postal_low > code)) {
      BuildMapCurrentTerritory->postal_low = code;
   }

   if (BuildMapCurrentTerritory->postal_high < code) {
      BuildMapCurrentTerritory->postal_high = code;
   }
}


static void buildmap_index_common_parent (char *common, const char *file) {

   char *p1;
   const char *p2;

   for (p1 = common, p2 = file; *p1 > 0; ++p1, ++p2) {
      if (*p1 != *p2) break;
   }

   if (*p1 == 0) return; /* Nothing to do. */

   *p1 = 0;
   if (p1 != common) {
      p1 = roadmap_path_parent (NULL, common);
      strcpy (common, p1);
      roadmap_path_free (p1);
   }
}


void buildmap_index_sort (void) {

   int index;
   int length;
   const char *p;
   BuildMapFile      *this_map;
   BuildMapTerritory *this_territory;
   BuildMapAuthority *this_authority;


   buildmap_info ("sorting index...");

   /* Retrieve the path for each territory by finding the
    * common denominator among the child maps.
    */
   for (index = 0; index < TerritoryCount; ++index) {

      this_territory =
         Territory[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      /* Find if there is any common path for all the maps. */

      this_territory->pathname =
         roadmap_path_parent (NULL, this_territory->maps->filename);

      if (strcmp (RoadMapPathCurrentDirectory, this_territory->pathname) == 0) {

         this_territory->pathname[0] = 0;

      } else {

         for (this_map = this_territory->maps->next;
              this_map != NULL;
              this_map = this_map->next) {

            buildmap_index_common_parent
               (this_territory->pathname, this_map->filename);
         } 
      }

      /* Register the file names, skipping the common path we found. */

      length = strlen (this_territory->pathname);

      if (length > 0) {

         p = roadmap_path_skip_separator
                (this_territory->maps->filename + length);
         if (p == NULL) {
            buildmap_fatal
               (0, "invalid common path %s in %s",
                this_territory->pathname,
                this_territory->maps->filename);
         }
         /* Ajust the length to avoid the path separator sequence. */
         length = (int) (p - this_territory->maps->filename);
      }

      for (this_map = this_territory->maps;
           this_map != NULL;
           this_map = this_map->next) {

         const char *relative = this_map->filename + length;

         this_map->filename_index =
            buildmap_dictionary_add (MapFiles, relative, strlen(relative));
      }
   }


   /* Retrieve the path for each authority by finding the
    * common denominator among the child territories.
    */
   NameCount = 0;

   for (index = 0; index < AuthorityCount; ++index) {

      this_authority =
         MapAuthority[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      if (this_authority->territories == NULL) continue;

      if (this_authority->territories->pathname[0] == 0) {

         this_authority->pathname = "";

      } else {

         this_authority->pathname =
            strdup (this_authority->territories->pathname);

         for (this_territory = this_authority->territories->next;
              this_territory != NULL;
              this_territory = this_territory->next) {

            buildmap_index_common_parent
               (this_authority->pathname, this_territory->pathname);
         }
      }

      /* Remove the common part from the territory paths. */

      length = strlen (this_authority->pathname);

      for (this_territory = this_authority->territories;
           this_territory != NULL;
           this_territory = this_territory->next) {

         if (this_territory->pathname[length] != 0) {

            const char *relative = this_territory->pathname;

            if (length > 0) {
               relative = roadmap_path_skip_separator (relative + length);
               if (relative == NULL) {
                   buildmap_fatal
                      (0, "invalid common path %s in %s",
                       this_authority->pathname,
                       this_territory->pathname);
               }
            }

            this_territory->pathname_index =
               buildmap_dictionary_add (MapFiles, relative, strlen(relative));

         } else {
            this_territory->pathname_index = 0;
         }
      }

      NameCount += this_authority->name_count; /* Total number of names. */
   }
}


void buildmap_index_save (void) {

   int i;

   int index;
   int territory_cursor;
   int map_cursor;
   int name_cursor;
   int city_cursor;

   RoadMapAuthority *one_authority;
   RoadMapTerritory *one_territory;

   RoadMapAuthority *db_authority;
   RoadMapTerritory *db_territory;
   RoadMapMap       *db_map;
   RoadMapString    *db_name;
   RoadMapString    *db_city;

   buildmap_db *root;
   buildmap_db *authority_table;
   buildmap_db *territory_table;
   buildmap_db *map_table;
   buildmap_db *name_table;
   buildmap_db *city_table;

   BuildMapFile      *this_map;
   BuildMapCity      *this_city;
   BuildMapTerritory *this_territory;


   buildmap_info ("saving index...");

   root = buildmap_db_add_section (NULL, "index");

   authority_table = buildmap_db_add_section (root, "authority");
   buildmap_db_add_data
      (authority_table, AuthorityCount, sizeof(RoadMapAuthority));

   territory_table = buildmap_db_add_section (root, "territory");
   buildmap_db_add_data
      (territory_table, TerritoryCount, sizeof(RoadMapTerritory));

   map_table = buildmap_db_add_section (root, "map");
   buildmap_db_add_data (map_table, MapCount, sizeof(RoadMapMap));

   name_table = buildmap_db_add_section (root, "name");
   buildmap_db_add_data (name_table, NameCount, sizeof(RoadMapString));

   city_table = buildmap_db_add_section (root, "city");
   buildmap_db_add_data (city_table, CityCount, sizeof(RoadMapString));

   db_authority = (RoadMapAuthority *) buildmap_db_get_data (authority_table);
   db_territory = (RoadMapTerritory *) buildmap_db_get_data (territory_table);
   db_map       = (RoadMapMap *) buildmap_db_get_data (map_table);
   db_name      = (RoadMapString *) buildmap_db_get_data (name_table);
   db_city      = (RoadMapString *) buildmap_db_get_data (city_table);


   name_cursor = 0;
   city_cursor = 0;
   map_cursor = 0;
   territory_cursor = 0;

   for (index = 0; index < AuthorityCount; ++index) {

      BuildMapAuthority *this_authority =
         MapAuthority[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

      one_authority = db_authority + index;

      one_authority->symbol = this_authority->symbol;
      one_authority->pathname = this_authority->pathname_index;
      one_authority->edges = this_authority->edges;

      one_authority->name_first = name_cursor;

      for (i = 0; i < this_authority->name_count; ++i) {
         if (name_cursor >= NameCount) {
            buildmap_fatal (0, "invalid map count");
         }
         db_name[name_cursor++] = this_authority->names[i];
      }
      one_authority->name_count = this_authority->name_count;


      one_authority->territory_first = territory_cursor;

      for (this_territory = this_authority->territories;
           this_territory != NULL;
           this_territory = this_territory->next) {

         if (territory_cursor >= TerritoryCount) {
            buildmap_fatal (0, "invalid territory count");
         }
         one_territory = db_territory + territory_cursor;

         one_territory->pathname = this_territory->pathname_index;
         one_territory->name = 0;

         one_territory->edges = this_territory->edges;

         one_territory->map_first = map_cursor;

         for (this_map = this_territory->maps;
              this_map != NULL;
              this_map = this_map->next) {

            if (map_cursor >= MapCount) {
               buildmap_fatal (0, "invalid map count");
            }
            db_map[map_cursor].class = this_map->class;
            db_map[map_cursor].filename = this_map->filename_index;

            map_cursor += 1;
         }
         one_territory->map_count = map_cursor - one_territory->map_first;

         for (this_city = this_territory->cities;
              this_city != NULL;
              this_city = this_city->next) {

            if (city_cursor >= CityCount) {
               buildmap_fatal (0, "invalid city count");
            }
            db_city[city_cursor++] = this_city->name;
         }
         one_territory->city_count = city_cursor - one_territory->city_first;

         one_territory->postal_low = this_territory->postal_low;
         one_territory->postal_high = this_territory->postal_high;

         territory_cursor += 1;
      }
      one_authority->territory_count =
         territory_cursor - one_authority->territory_first;
   }
}


void buildmap_index_summary (void) {

   fprintf (stderr,
            "-- index statistics: %d authorities, %d territories, %d maps\n",
            AuthorityCount, TerritoryCount, MapCount);
}

