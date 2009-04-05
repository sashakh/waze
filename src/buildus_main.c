/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2008, Danny Backx.
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
 * @brief The main function of the US directory builder tool.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

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
#include "roadmap_iso.h"

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

/**
 * @brief
 */
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

/**
 * @brief
 * @param fips
 */
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
#ifdef VERBOSE
	   fprintf(stderr, "buildus_scan_cities [cities %p] (fips %d), i %d,",
			   us_cities, fips, i);
	   fprintf(stderr, " name [%s]\n", name);
#endif
      city = buildmap_dictionary_add (us_cities, name, strlen(name));
      buildus_county_add_city (fips, city);
   }
}

/**
 * @brief read the specified directory, filter out the map files, scan them
 */
static void buildus_scan_maps (void) {
   

   char *extension;
   const char *mappath;
   int found = 0, n;

   char country_iso[4] = {0, 0, 0, 0};
   char	country_division[4] = {0, 0, 0, 0};

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

	 /* Make sure all files read end in ".rdm" */
         extension = strrchr (entry->d_name, '.');
         if (extension == NULL) continue;
         if (strcmp (extension, ".rdm") != 0) continue;

	 /* Set the name of the file we're reading rather early,
	  * so error messages mention it.
	  */
         buildmap_set_source (entry->d_name);

	 n = buildmap_osm_filename_iso(entry->d_name, country_iso,
			 country_division, ".rdm");
	 if (n) {
	     static BuildMapDictionary BuildMapStateDictionary;
	     static BuildMapDictionary county_dictionary;
	     RoadMapString state_symbol, county_name;

	     fips = roadmap_iso_alpha_to_num(country_iso) * 1000 + 1000000;
	     if (n == 2) {
                 fips += roadmap_iso_division_to_num(country_iso,
				 country_division);
		 buildmap_info("Country %s division %s fips %d",
				 country_iso, country_division, fips);
	     } else {
                 buildmap_info("Country %s fips %d", country_iso, fips);
	     }

	     /* Create a fake county */
	     BuildMapStateDictionary = buildmap_dictionary_open ("state");

	     state_symbol = buildmap_dictionary_add (BuildMapStateDictionary,
			     country_iso, strlen(country_iso));
	     if (state_symbol == 0) {
                 buildmap_fatal (0, "invalid state description");
	     }
	     buildus_county_add_state (state_symbol, state_symbol);
	     county_dictionary = buildmap_dictionary_open ("county");
	     if (n == 2) {
		     county_name = buildmap_dictionary_add (county_dictionary,
			     country_division, strlen(country_division));
	     } else {
		     county_name = buildmap_dictionary_add (county_dictionary,
				     "fake county", 11);
	     }

	     buildus_county_add (fips, county_name, state_symbol);
	 } else if (buildmap_osm_filename_usc(entry->d_name, &fips)) {
		 ;	/* It's decoded, that's all we needed. */
	 } else {
		 /* File name is invalid, bail out */
		 continue;
	 }

         found = 1;

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

	 /* Don't report stuff about files we're already done with */
	 buildmap_set_source (NULL);
      }

      closedir (directory);
   }

   /* Call this again just in case we've fallen through the loop with a file
    * name we have no intention of reading. */
   buildmap_set_source (NULL);

   if (!found) {
      buildmap_fatal (0, "No maps found\n", mappath);
   }
}

/**
 * @brief
 * @param progpath
 * @param msg
 */
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

/**
 * @brief load all countries from the ISO list
 */
void roadmap_iso_create_all_countries(void)
{
    int i;
    static BuildMapDictionary state_dictionary;
    static BuildMapDictionary county_dictionary;
    RoadMapString state_symbol, state_name, dw;
    char symbol[8];

    state_dictionary = buildmap_dictionary_open ("state");
    county_dictionary = buildmap_dictionary_open ("county");
    dw = buildmap_dictionary_add(state_dictionary, "DW", 2);
    for (i=0; IsoCountryCodeTable[i].name; i++) {
        sprintf(symbol, "%s", IsoCountryCodeTable[i].alpha2);
	state_symbol = buildmap_dictionary_add (state_dictionary,
            IsoCountryCodeTable[i].alpha3,
	    strlen(IsoCountryCodeTable[i].alpha3));
	state_name = buildmap_dictionary_add (state_dictionary,
            IsoCountryCodeTable[i].name,
	    strlen(IsoCountryCodeTable[i].name));
	if (state_symbol == 0 || state_name == 0) {
            buildmap_fatal (0, "invalid state description");
        }
	buildus_county_add_state (state_name, state_symbol);
    }
}

/**
 * @brief
 * @param argc
 * @param argv
 * @return
 */
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

   /* US specific : read states and counties from *.txt files */
   buildus_fips_read (BuildMapTiger, BuildMapVerbose);

   /* ISO stuff */
   roadmap_iso_create_all_countries();

   buildus_scan_maps ();

   buildmap_db_sort();

   if (BuildMapVerbose) {
      roadmap_hash_summary ();
      buildmap_db_summary ();
   }

   buildus_save ();

   return 0;
}
