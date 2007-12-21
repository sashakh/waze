/* buildus_main.c - The main function of the US directory builder tool.
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
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include "roadmap_dbread.h"
#include "roadmap_dictionary.h"
#include "roadmap_types.h"
#include "roadmap_hash.h"
#include "roadmap_square.h"
#include "roadmap_street.h"
#include "roadmap_path.h"
#include "roadmap_config.h"

#include "buildmap.h"
#include "buildmap_opt.h"

#include "buildus_fips.h"
#include "buildus_county.h"

static RoadMapConfigDescriptor RoadMapConfigMapPath =
                        ROADMAP_CONFIG_ITEM("Map", "Path");


static roadmap_db_model *RoadMapCountyModel;


static int   BuildMapVerbose = 0;
static int   BuildMapSilent = 0;
static int   BuildMapUseConfig = 0;
static char *BuildMapTiger  = ".";
static char *BuildMapPath;
static char *BuildMapResult;

struct opt_defs options[] = {
   {"path", "d", opt_string, ".",
        "Path to index files (usstates.txt, app_a02.txt)"},
   {"maps", "m", opt_string, "",
        "Location(s) of the RoadMap maps (separate multiple directories with ',')"},
   {"config", "c", opt_flag, "0",
        "Use Map.Path preferences setting for map path (overrides -m)"},
   {"verbose", "v", opt_flag, "0",
        "Show progress information"},
   {"silent", "s", opt_flag, "0",
        "Show nothing"},
   OPT_DEFS_END
};

static void buildus_save (void) {

   buildmap_set_source ("usdir.rdm");

   if (buildmap_db_open (BuildMapResult, "usdir.rdm") < 0) {
      buildmap_fatal (0, "cannot create database '%s'", BuildMapResult);
   }

   if (! BuildMapSilent) {
      buildmap_info("Writing index file to directory '%s'", BuildMapResult);
   }

   buildmap_db_save ();

   buildmap_db_close ();
}


static void buildus_scan_cities (int fips) {

   int i;
   char *name;
   RoadMapString city;
   RoadMapDictionary cities;
   BuildMapDictionary us_cities;


   cities = roadmap_dictionary_open ("city");

   if (! cities) return; /* May not exist is all map files. */

   us_cities = buildmap_dictionary_open ("city");

   for (i = 1, name = roadmap_dictionary_get (cities, 1);
        name != NULL;
        name = roadmap_dictionary_get (cities, ++i)) {

      city = buildmap_dictionary_add (us_cities, name, strlen(name));
      buildus_county_add_city (fips, city);
   }
}


static void buildus_scan_maps (void) {
   

   char *extension;
   const char *mappath;
   int found = 0;

   int  fips;

   DIR *directory;
   struct dirent *entry;

   RoadMapArea edges;


   RoadMapCountyModel =
      roadmap_db_register
         (RoadMapCountyModel, "string", &RoadMapDictionaryHandler);
   RoadMapCountyModel =
      roadmap_db_register
         (RoadMapCountyModel, "square", &RoadMapSquareHandler);
   RoadMapCountyModel =
      roadmap_db_register
         (RoadMapCountyModel, "zip", &RoadMapZipHandler);

   for (mappath = roadmap_path_first("maps");
         mappath != NULL;
         mappath = roadmap_path_next("maps", mappath)) {

      if (! BuildMapSilent) {
         buildmap_info("Processing maps from %s", mappath);
      }

      directory = opendir (mappath);

      if (directory == NULL) {
         buildmap_info ("cannot scan directory %s", mappath);
         continue;
      }

      for (entry = readdir(directory);
           entry != NULL;
           entry = readdir(directory)) {

         if (strncmp (entry->d_name, "usc", 3) != 0) continue;

         extension = strrchr (entry->d_name, '.');

         if (extension == NULL) continue;
         if (strcmp (extension, ".rdm") != 0) continue;

         found = 1;

         fips = atoi (entry->d_name + 3);

         buildmap_set_source (entry->d_name);
         if (! BuildMapSilent) buildmap_info ("scanning the county file...");

         if (! roadmap_db_open (mappath, entry->d_name, RoadMapCountyModel)) {
            buildmap_fatal (0, "cannot open map database %s in %s",
                               entry->d_name, mappath);
         }
         roadmap_db_activate (mappath, entry->d_name);

         buildus_scan_cities (fips);

         roadmap_square_edges (ROADMAP_SQUARE_GLOBAL, &edges);
         buildus_county_set_position (fips, &edges);

         roadmap_db_close (mappath, entry->d_name);
      }

      closedir (directory);

   }
   if (!found) {
      buildmap_fatal (0, "No maps found\n", mappath);
   }
}

void usage(char *progpath, const char *msg) {

   char *prog = strrchr(progpath, '/');

   if (prog)
       prog++;
   else
       prog = progpath;

   if (msg)
       fprintf(stderr, "%s: %s\n", prog, msg);
   fprintf(stderr,
       "usage: %s [options]\n", prog);
   opt_desc(options, 1);
   exit(1);
}

int main (int argc, char **argv) {

   int error;

   BuildMapResult = strdup(roadmap_path_preferred("maps"));

   /* parse the options */
   error = opt_parse(options, &argc, argv, 0);
   if (error) usage(argv[0], opt_strerror(error));

   /* then, fetch the option values */
   error = opt_val("path", &BuildMapTiger) ||
           opt_val("maps", &BuildMapResult) ||
           opt_val("config", &BuildMapUseConfig) ||
           opt_val("verbose", &BuildMapVerbose) ||
           opt_val("silent", &BuildMapSilent);
   if (error)
      usage(argv[0], opt_strerror(error));

   if (BuildMapUseConfig) {

      if (! BuildMapSilent) {
         buildmap_info("Fetching Map.Path to find maps", BuildMapResult);
      }

      roadmap_config_initialize ();
      roadmap_config_load ();
      roadmap_config_declare ("preferences", &RoadMapConfigMapPath, "");
      if (roadmap_config_get(&RoadMapConfigMapPath)[0] != 0) {
         roadmap_path_set ("maps", roadmap_config_get(&RoadMapConfigMapPath));
      }
      BuildMapResult = strdup(roadmap_path_first("maps"));

   } else if (BuildMapPath != NULL) { /* set by -m option */

      roadmap_path_set ("maps", BuildMapPath);
      BuildMapResult = strdup(roadmap_path_first("maps"));

   } else {
      roadmap_path_set ("maps", BuildMapResult);
   }


   buildus_fips_read (BuildMapTiger, BuildMapVerbose);

   buildus_scan_maps ();

   buildmap_db_sort();

   if (BuildMapVerbose) {

      roadmap_hash_summary ();
      buildmap_db_summary ();
   }

   buildus_save ();

   return 0;
}

