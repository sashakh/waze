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
#include <popt.h>

#include "roadmap_dbread.h"
#include "roadmap_dictionary.h"
#include "roadmap_types.h"
#include "roadmap_hash.h"
#include "roadmap_square.h"
#include "roadmap_street.h"
#include "roadmap_path.h"

#include "buildmap.h"

#include "buildus_fips.h"
#include "buildus_county.h"


static roadmap_db_model *RoadMapCountyModel;


static int   BuildMapVerbose = 0;
static int   BuildMapSilent = 0;
static char *BuildMapTiger  = ".";
static char *BuildMapPath;

static struct poptOption BuildUsOptions[] = {

   POPT_AUTOHELP

   {"path", 'd',
      POPT_ARG_STRING, &BuildMapTiger, 0,
      "Location of the tiger files (source files)", "PATH"},

   {"maps", 'm',
      POPT_ARG_STRING, &BuildMapPath, 0,
      "Location of the RoadMap maps (source & generated files)", "PATH"},

   {"verbose", 'v',
      POPT_ARG_NONE, &BuildMapVerbose, 0, "Show progress information", NULL},

   {"silent", 's',
      POPT_ARG_NONE, &BuildMapSilent, 0, "Show nothing", NULL},

   {NULL, 0, 0, NULL, 0, NULL, NULL}
};


static void buildus_save (void) {

   buildmap_set_source ("usdir");

   if (buildmap_db_open (BuildMapPath, "usdir") < 0) {
      buildmap_fatal (0, "cannot create database 'usdir'");
   }

   buildmap_dictionary_save ();
   buildus_county_save ();

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

   roadmap_path_set ("maps", BuildMapPath);

   directory = opendir (BuildMapPath);

   if (directory == NULL) {
      buildmap_fatal (0, "cannot scan directory %s", BuildMapPath);
   }

   for (entry = readdir(directory);
        entry != NULL;
        entry = readdir(directory)) {

      if (strncmp (entry->d_name, "usc", 3) != 0) continue;

      extension = strrchr (entry->d_name, '.');

      if (extension == NULL) continue;
      if (strcmp (extension, ".rdm") != 0) continue;

      fips = atoi (entry->d_name + 3);

      buildmap_set_source (entry->d_name);
      if (! BuildMapSilent) buildmap_info ("scanning the county file...");

      if (! roadmap_db_open (BuildMapPath, entry->d_name, RoadMapCountyModel)) {
         buildmap_fatal (0, "cannot open map database %s in %s",
                            entry->d_name, BuildMapPath);
      }
      roadmap_db_activate (BuildMapPath, entry->d_name);

      buildus_scan_cities (fips);

      roadmap_square_edges (ROADMAP_SQUARE_GLOBAL, &edges);
      buildus_county_set_position (fips, &edges);

      roadmap_db_close (BuildMapPath, entry->d_name);
   }

   closedir (directory);
}


int main (int argc, const char **argv) {

   poptContext decoder;

   BuildMapPath = strdup(roadmap_path_preferred("maps"));

   decoder = poptGetContext ("buildus", argc, argv, BuildUsOptions, 0);


   while (poptGetNextOpt(decoder) > 0) ;


   buildus_county_initialize ();

   buildus_fips_read (BuildMapTiger, BuildMapVerbose);

   buildus_scan_maps ();

   buildus_county_sort();

   if (BuildMapVerbose) {

      roadmap_hash_summary ();
      buildmap_dictionary_summary ();
      buildus_fips_summary ();
   }

   buildus_save ();

   return 0;
}

