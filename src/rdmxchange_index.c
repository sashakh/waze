/* rdmxchange_index.c - Export the directory index used to select a map.
 *
 * LICENSE:
 *
 *   Copyright 2006 Pascal F. Martin
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, as of version 2 of the License.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_hash.h"
#include "roadmap_dbread.h"
#include "roadmap_db_index.h"

#include "rdmxchange.h"


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

} RoadMapIndexContext;

static RoadMapIndexContext *RoadMapIndexActive = NULL;

static void rdmxchange_index_register_export (void);


static void *rdmxchange_index_map (roadmap_db *root) {

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

   rdmxchange_index_register_export();

   return context;
}

static void rdmxchange_index_activate (void *context) {

   RoadMapIndexActive = (RoadMapIndexContext *) context;
}

static void rdmxchange_index_unmap (void *context) {

   RoadMapIndexContext *index_context = (RoadMapIndexContext *) context;

   if (index_context->type != RoadMapIndexType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (invalid context type)");
   }

   if (index_context == RoadMapIndexActive) {
      RoadMapIndexActive = NULL;
   }
   free (index_context);
}

roadmap_db_handler RoadMapIndexExport = {
   "index",
   rdmxchange_index_map,
   rdmxchange_index_activate,
   rdmxchange_index_unmap
};


static void rdmxchange_index_export_head (FILE *file) {

   fprintf (file, "index/authority,%d\n",
                  RoadMapIndexActive->authority_count);

   fprintf (file, "index/territory,%d\n",
                  RoadMapIndexActive->territory_count);

   fprintf (file, "index/map,%d\n",
                  RoadMapIndexActive->map_count);

   fprintf (file, "index/name,%d\n",
                  RoadMapIndexActive->name_count);

   fprintf (file, "index/city,%d\n",
                  RoadMapIndexActive->city_count);
}


static void rdmxchange_index_export_data (FILE *file) {

   int i;
   RoadMapAuthority *authority;
   RoadMapTerritory *territory;
   RoadMapMap       *map;
   RoadMapString    *name;
   RoadMapString    *city;


   fprintf (file, "index/authority\n"
                  "symbol,"
                  "path,"
                  "edges.east,"
                  "edges.north,"
                  "edges.west,"
                  "edges.south,"
                  "name.first,"
                  "name.count,"
                  "territory.first,"
                  "territory.count\n");
   authority = RoadMapIndexActive->authority;

   for (i = 0; i < RoadMapIndexActive->authority_count; ++i) {
      fprintf (file, "%.0d,%.0d,", authority[i].symbol, authority[i].pathname);
      fprintf (file, "%.0d,%.0d,%.0d,%.0d,", authority[i].edges.east,
                                             authority[i].edges.north,
                                             authority[i].edges.west,
                                             authority[i].edges.south);
      fprintf (file, "%.0d,%.0d,", authority[i].name_first,
                                   authority[i].name_count);
      fprintf (file, "%.0d,%.0d\n", authority[i].territory_first,
                                    authority[i].territory_count);
   }
   fprintf (file, "\n");


   fprintf (file, "index/territory\n"
                  "wtid,"
                  "name,"
                  "path,"
                  "edges.east,"
                  "edges.north,"
                  "edges.west,"
                  "edges.south,"
                  "map.first,"
                  "map.count,"
                  "city.first,"
                  "city.count,"
                  "postal.low,"
                  "postal.high\n");
   territory = RoadMapIndexActive->territory;

   for (i = 0; i < RoadMapIndexActive->territory_count; ++i) {
      fprintf (file, "%.0d,", territory[i].wtid);
      fprintf (file, "%.0d,%.0d,", territory[i].name,
                                   territory[i].pathname);
      fprintf (file, "%.0d,%.0d,%.0d,%.0d,", territory[i].edges.east,
                                             territory[i].edges.north,
                                             territory[i].edges.west,
                                             territory[i].edges.south);
      fprintf (file, "%.0d,%.0d,", territory[i].map_first,
                                   territory[i].map_count);
      fprintf (file, "%.0d,%.0d,", territory[i].city_first,
                                   territory[i].city_count);
      fprintf (file, "%.0d,%.0d\n", territory[i].postal_low,
                                    territory[i].postal_high);
   }
   fprintf (file, "\n");


   fprintf (file, "index/map\n"
                  "class,file\n");
   map = RoadMapIndexActive->map;

   for (i = 0; i < RoadMapIndexActive->map_count; ++i) {
      fprintf (file, "%.0d,%.0d\n", map[i].class, map[i].filename);
   }
   fprintf (file, "\n");


   fprintf (file, "index/name\n"
                  "index\n");
   name = RoadMapIndexActive->name;

   for (i = 0; i < RoadMapIndexActive->name_count; ++i) {
      fprintf (file, "%d\n", name[i]);
   }
   fprintf (file, "\n");


   fprintf (file, "index/city\n"
                  "index\n");
   city = RoadMapIndexActive->city;

   for (i = 0; i < RoadMapIndexActive->city_count; ++i) {
      fprintf (file, "%d\n", city[i]);
   }
   fprintf (file, "\n");
}



static RdmXchangeExport RdmXchangeIndexExport = {
   "index",
   rdmxchange_index_export_head,
   rdmxchange_index_export_data
};

static void rdmxchange_index_register_export (void) {

   rdmxchange_main_register_export (&RdmXchangeIndexExport);
}


/* The import side. ----------------------------------------------------- */

static RoadMapAuthority *IndexAuthority = NULL;
static RoadMapTerritory *IndexTerritory = NULL;
static RoadMapMap       *IndexMap = NULL;
static RoadMapString    *IndexName = NULL;
static RoadMapString    *IndexCity = NULL;
static int  IndexAuthorityCount = 0;
static int  IndexTerritoryCount = 0;
static int  IndexMapCount = 0;
static int  IndexNameCount = 0;
static int  IndexCityCount = 0;
static int  IndexCursor = 0;


static void rdmxchange_index_save (void) {

   int i;

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


   /* Create the tables. */

   root = buildmap_db_add_section (NULL, "index");
   if (root == NULL) buildmap_fatal (1, "cannot create map section");

   authority_table = buildmap_db_add_section (root, "authority");
   buildmap_db_add_data
      (authority_table, IndexAuthorityCount, sizeof(RoadMapAuthority));

   territory_table = buildmap_db_add_section (root, "territory");
   buildmap_db_add_data
      (territory_table, IndexTerritoryCount, sizeof(RoadMapTerritory));

   map_table = buildmap_db_add_section (root, "map");
   buildmap_db_add_data (map_table, IndexMapCount, sizeof(RoadMapMap));

   name_table = buildmap_db_add_section (root, "name");
   buildmap_db_add_data (name_table, IndexNameCount, sizeof(RoadMapString));

   city_table = buildmap_db_add_section (root, "city");
   buildmap_db_add_data (city_table, IndexCityCount, sizeof(RoadMapString));

   db_authority = (RoadMapAuthority *) buildmap_db_get_data (authority_table);
   db_territory = (RoadMapTerritory *) buildmap_db_get_data (territory_table);
   db_map       = (RoadMapMap *) buildmap_db_get_data (map_table);
   db_name      = (RoadMapString *) buildmap_db_get_data (name_table);
   db_city      = (RoadMapString *) buildmap_db_get_data (city_table);


   /* Fill the data in. */

   for (i = 0; i < IndexAuthorityCount; ++i) {
      db_authority[i] = IndexAuthority[i];
   }

   for (i = 0; i < IndexTerritoryCount; ++i) {
      db_territory[i] = IndexTerritory[i];
   }

   for (i = 0; i < IndexMapCount; ++i) {
      db_map[i] = IndexMap[i];
   }

   for (i = 0; i < IndexNameCount; ++i) {
      db_name[i] = IndexName[i];
   }

   for (i = 0; i < IndexCityCount; ++i) {
      db_city[i] = IndexCity[i];
   }

   /* Do not save this data ever again. */
   free (IndexAuthority);
   IndexAuthority = NULL;
   IndexAuthorityCount = 0;

   free (IndexTerritory);
   IndexTerritory = NULL;
   IndexTerritoryCount = 0;

   free (IndexMap);
   IndexMap = NULL;
   IndexMapCount = 0;

   free (IndexName);
   IndexName = NULL;
   IndexNameCount = 0;

   free (IndexCity);
   IndexCity = NULL;
   IndexCityCount = 0;
}


static buildmap_db_module RdmXchangeIndexModule = {
   "index",
   NULL,
   rdmxchange_index_save,
   NULL,
   NULL
};


static void rdmxchange_index_import_table (const char *name, int count) {

   buildmap_db_register (&RdmXchangeIndexModule);

   if (strcmp (name, "index/authority") == 0) {

      if (IndexAuthority != NULL) free (IndexAuthority);

      IndexAuthority = calloc (count, sizeof(RoadMapAuthority));
      IndexAuthorityCount = count;

   } else if (strcmp (name, "index/territory") == 0) {

      if (IndexTerritory != NULL) free (IndexTerritory);

      IndexTerritory = calloc (count, sizeof(RoadMapTerritory));
      IndexTerritoryCount = count;

   } else if (strcmp (name, "index/map") == 0) {

      if (IndexMap != NULL) free (IndexMap);

      IndexMap = calloc (count, sizeof(RoadMapMap));
      IndexMapCount = count;

   } else if (strcmp (name, "index/name") == 0) {

      if (IndexName != NULL) free (IndexName);

      IndexName = calloc (count, sizeof(RoadMapString));
      IndexNameCount = count;

   } else if (strcmp (name, "index/city") == 0) {

      if (IndexCity != NULL) free (IndexCity);

      IndexCity = calloc (count, sizeof(RoadMapString));
      IndexCityCount = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }
}


static int rdmxchange_index_import_schema (const char *table,
                                         char *fields[], int count) {

   IndexCursor = 0;

   if (strcmp (table, "index/authority") == 0) {

      if (IndexAuthority == NULL ||
            count != 10 ||
            strcmp(fields[0], "symbol") != 0 ||
            strcmp(fields[1], "path") != 0 ||
            strcmp(fields[2], "edges.east") != 0 ||
            strcmp(fields[3], "edges.north") != 0 ||
            strcmp(fields[4], "edges.west") != 0 ||
            strcmp(fields[5], "edges.south") != 0 ||
            strcmp(fields[6], "name.first") != 0 ||
            strcmp(fields[7], "name.count") != 0 ||
            strcmp(fields[8], "territory.first") != 0 ||
            strcmp(fields[9], "territory.count") != 0) {
         buildmap_fatal (1, "invalid schema for table index/authority");
      }

      return 1;

   } else if (strcmp (table, "index/territory") == 0) {

      if (IndexTerritory == NULL ||
            count != 13 ||
            strcmp(fields[0], "wtid") != 0 ||
            strcmp(fields[1], "name") != 0 ||
            strcmp(fields[2], "path") != 0 ||
            strcmp(fields[3], "edges.east") != 0 ||
            strcmp(fields[4], "edges.north") != 0 ||
            strcmp(fields[5], "edges.west") != 0 ||
            strcmp(fields[6], "edges.south") != 0 ||
            strcmp(fields[7], "map.first") != 0 ||
            strcmp(fields[8], "map.count") != 0 ||
            strcmp(fields[9], "city.first") != 0 ||
            strcmp(fields[10], "city.count") != 0 ||
            strcmp(fields[11], "postal.low") != 0 ||
            strcmp(fields[12], "postal.high") != 0) {
         buildmap_fatal (1, "invalid schema for table index/territory");
      }

      return 2;

   } else if (strcmp (table, "index/map") == 0) {

      if (IndexTerritory == NULL ||
            count != 2 ||
            strcmp(fields[0], "class") != 0 ||
            strcmp(fields[1], "file") != 0) {
         buildmap_fatal (1, "invalid schema for table index/map");
      }

      return 3;

   } else if (strcmp (table, "index/name") == 0) {

      if (IndexName == NULL || count != 1 || strcmp(fields[0], "index") != 0) {
         buildmap_fatal (1, "invalid schema for table index/name");
      }

      return 4;

   } else if (strcmp (table, "index/city") == 0) {

      if (IndexCity == NULL || count != 1 || strcmp(fields[0], "index") != 0) {
         buildmap_fatal (1, "invalid schema for table index/city");
      }

      return 5;
   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}

static void rdmxchange_index_import_data (int table,
                                        char *fields[], int count) {

   switch (table) {

      case 1:

         if (count != 10) {
            buildmap_fatal (count, "invalid index/authority record");
         }
         IndexAuthority[IndexCursor].symbol =
            (RoadMapString) rdmxchange_import_int (fields[0]);
         IndexAuthority[IndexCursor].pathname =
            (RoadMapString) rdmxchange_import_int (fields[1]);
         IndexAuthority[IndexCursor].edges.east =
            rdmxchange_import_int (fields[2]);
         IndexAuthority[IndexCursor].edges.north =
            rdmxchange_import_int (fields[3]);
         IndexAuthority[IndexCursor].edges.west =
            rdmxchange_import_int (fields[4]);
         IndexAuthority[IndexCursor].edges.south =
            rdmxchange_import_int (fields[5]);
         IndexAuthority[IndexCursor].name_first =
            (unsigned short) rdmxchange_import_int (fields[6]);
         IndexAuthority[IndexCursor].name_count =
            (unsigned short) rdmxchange_import_int (fields[7]);
         IndexAuthority[IndexCursor].territory_first =
            (unsigned short) rdmxchange_import_int (fields[8]);
         IndexAuthority[IndexCursor].territory_count =
            (unsigned short) rdmxchange_import_int (fields[9]);
         break;

      case 2:

         if (count != 13) {
            buildmap_fatal (count, "invalid index/territory record");
         }
         IndexTerritory[IndexCursor].wtid = rdmxchange_import_int (fields[0]);
         IndexTerritory[IndexCursor].name =
            (RoadMapString) rdmxchange_import_int (fields[1]);
         IndexTerritory[IndexCursor].pathname =
            (RoadMapString) rdmxchange_import_int (fields[2]);
         IndexTerritory[IndexCursor].edges.east =
            rdmxchange_import_int (fields[3]);
         IndexTerritory[IndexCursor].edges.north =
            rdmxchange_import_int (fields[4]);
         IndexTerritory[IndexCursor].edges.west =
            rdmxchange_import_int (fields[5]);
         IndexTerritory[IndexCursor].edges.south =
            rdmxchange_import_int (fields[6]);
         IndexTerritory[IndexCursor].map_first =
            (unsigned short) rdmxchange_import_int (fields[7]);
         IndexTerritory[IndexCursor].map_count =
            (unsigned short) rdmxchange_import_int (fields[8]);
         IndexTerritory[IndexCursor].city_first =
            (unsigned short) rdmxchange_import_int (fields[9]);
         IndexTerritory[IndexCursor].city_count =
            (unsigned short) rdmxchange_import_int (fields[10]);
         IndexTerritory[IndexCursor].postal_low =
            (unsigned int) rdmxchange_import_int (fields[11]);
         IndexTerritory[IndexCursor].postal_high =
            (unsigned int) rdmxchange_import_int (fields[12]);
         break;

      case 3:

         if (count != 2) {
            buildmap_fatal (count, "invalid index/map record");
         }
         IndexMap[IndexCursor].class =
            (RoadMapString) rdmxchange_import_int (fields[0]);
         IndexMap[IndexCursor].filename =
            (RoadMapString) rdmxchange_import_int (fields[1]);
         break;

      case 4:

         if (count != 1) {
            buildmap_fatal (count, "invalid index/name record");
         }
         IndexName[IndexCursor] =
            (RoadMapString) rdmxchange_import_int (fields[0]);
         break;

      case 5:

         if (count != 1) {
            buildmap_fatal (count, "invalid index/city record");
         }
         IndexCity[IndexCursor] =
            (RoadMapString) rdmxchange_import_int (fields[0]);
         break;

      default:

          buildmap_fatal (1, "invalid table");
   }
}


RdmXchangeImport RdmXchangeIndexImport = {
   "index",
   rdmxchange_index_import_table,
   rdmxchange_index_import_schema,
   rdmxchange_index_import_data
};
