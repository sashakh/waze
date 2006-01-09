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

   fprintf (file, "table index/authority %d"
                       " symbol"
                       " path"
                       " edges.east"
                       " edges.north"
                       " edges.west"
                       " edges.south"
                       " name.first"
                       " name.count"
                       " territory.first"
                       " territory.count\n",
                  RoadMapIndexActive->authority_count);

   fprintf (file, "table index/territory %d"
                       " wtid"
                       " name"
                       " path"
                       " edges.east"
                       " edges.north"
                       " edges.west"
                       " edges.south"
                       " map.first"
                       " map.count"
                       " city.first"
                       " city.count"
                       " postal.low"
                       " postal.high\n",
                  RoadMapIndexActive->territory_count);

   fprintf (file, "table index/map %d"
                       " class"
                       " file\n",
                  RoadMapIndexActive->map_count);

   fprintf (file, "table index/name %d index\n",
                  RoadMapIndexActive->name_count);

   fprintf (file, "table index/city %d index\n",
                  RoadMapIndexActive->city_count);
}


static void rdmxchange_index_export_data (FILE *file) {

   int i;
   RoadMapAuthority *authority;
   RoadMapTerritory *territory;
   RoadMapMap       *map;
   RoadMapString    *name;
   RoadMapString    *city;


   fprintf (file, "table index/authority\n");
   authority = RoadMapIndexActive->authority;

   for (i = 0; i < RoadMapIndexActive->authority_count; ++i) {
      fprintf (file, "%d,%d,", authority[i].symbol, authority[i].pathname);
      fprintf (file, "%d,%d,%d,%d,", authority[i].edges.east,
                                     authority[i].edges.north,
                                     authority[i].edges.west,
                                     authority[i].edges.south);
      fprintf (file, "%d,%d,", authority[i].name_first,
                               authority[i].name_count);
      fprintf (file, "%d,%d\n", authority[i].territory_first,
                                authority[i].territory_count);
   }
   fprintf (file, "\n");


   fprintf (file, "table index/territory\n");
   territory = RoadMapIndexActive->territory;

   for (i = 0; i < RoadMapIndexActive->territory_count; ++i) {
      fprintf (file, "%d,", territory[i].wtid);
      fprintf (file, "%d,%d,", territory[i].name, territory[i].pathname);
      fprintf (file, "%d,%d,%d,%d,", territory[i].edges.east,
                                     territory[i].edges.north,
                                     territory[i].edges.west,
                                     territory[i].edges.south);
      fprintf (file, "%d,%d,", territory[i].map_first,
                               territory[i].map_count);
      fprintf (file, "%d,%d,", territory[i].city_first,
                               territory[i].city_count);
      fprintf (file, "%d,%d\n", territory[i].postal_low,
                                territory[i].postal_high);
   }
   fprintf (file, "\n");


   fprintf (file, "table index/map\n");
   map = RoadMapIndexActive->map;

   for (i = 0; i < RoadMapIndexActive->map_count; ++i) {
      fprintf (file, "%d,%d\n", map[i].class, map[i].filename);
   }
   fprintf (file, "\n");


   fprintf (file, "table index/name\n");
   name = RoadMapIndexActive->name;

   for (i = 0; i < RoadMapIndexActive->name_count; ++i) {
      fprintf (file, "%d\n", name[i]);
   }
   fprintf (file, "\n");


   fprintf (file, "table index/city\n");
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
