/* buildmap_main.c - The main function of the map builder tool.
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <popt.h>

#include "roadmap_types.h"
#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_tiger.h"

#include "buildmap_square.h"
#include "buildmap_street.h"
#include "buildmap_range.h"
#include "buildmap_line.h"
#include "buildmap_point.h"
#include "buildmap_shape.h"
#include "buildmap_city.h"
#include "buildmap_zip.h"
#include "buildmap_area.h"
#include "buildmap_polygon.h"


static int   BuildMapCanals  = 0;
static int   BuildMapRivers  = 0;
static int   BuildMapVerbose = 0;
static int   BuildMapFormat  = 2002;
static char *BuildMapTiger  = ".";
static char *BuildMapPrefix = "TGR";
static char *BuildMapState  = NULL;
static char *BuildMapPath   = "/usr/local/share/roadmap";

static struct poptOption BuildMapTigerOptions [] = {

   {"path", 'd',
      POPT_ARG_STRING, &BuildMapTiger, 0,
      "Location of the tiger files (source files)", "PATH"},

   {"prefix", 'p',
      POPT_ARG_STRING, &BuildMapPrefix, 0, "Tiger files prefix", "XXX"},

   {"state", 's',
      POPT_ARG_STRING, &BuildMapState, 0,
      "Process this state only (2 digits code)", "XX"},

   {"format", 'f',
      POPT_ARG_INT, &BuildMapFormat, 0, "Tiger files format", "2000|2002"},

   {NULL, 0, 0, NULL, 0, NULL, NULL}
};

static struct poptOption BuildMapDataOptions [] = {

   {"canals", 'c',
      POPT_ARG_NONE, &BuildMapCanals, 0, "Show canals on maps", NULL},

   {"rivers", 'r',
      POPT_ARG_NONE, &BuildMapRivers, 0, "Show rivers on maps", NULL},

   {NULL, 0, 0, NULL, 0, NULL, NULL}
};

static struct poptOption BuildMapGeneralOptions [] = {

   {"verbose", 'v',
      POPT_ARG_NONE, &BuildMapVerbose, 0, "Show progress information", NULL},

   {"maps", 'm',
      POPT_ARG_STRING, &BuildMapPath, 0,
      "Location of the RoadMap maps (generated files)", "PATH"},

   {NULL, 0, 0, NULL, 0, NULL, NULL}
};

static struct poptOption BuildMapOptionTable [] = {

   POPT_AUTOHELP

   {NULL, 0,
        POPT_ARG_INCLUDE_TABLE, BuildMapTigerOptions, 0, "Tiger file options", NULL},

   {NULL, 0,
        POPT_ARG_INCLUDE_TABLE, BuildMapDataOptions, 0, "Map filter options", NULL},

   {NULL, 0,
        POPT_ARG_INCLUDE_TABLE, BuildMapGeneralOptions, 0, "BuildMap's general options", NULL},

   {NULL, 0, 0, NULL, 0, NULL, NULL}
};


static void buildmap_county_initialize (void) {

   buildmap_zip_initialize();
   buildmap_city_initialize();
   buildmap_point_initialize();
   buildmap_range_initialize();
   buildmap_line_initialize();
   buildmap_polygon_initialize();
   buildmap_shape_initialize();
   buildmap_street_initialize();
   buildmap_area_initialize();
}


static void buildmap_county_save (char *name) {

   char *cursor;
   char db_name[128];

   snprintf (db_name, 127, "usc%s", name);

   /* Remove the suffix if any was provided. */

   cursor = strrchr (db_name, '.');
   if (cursor != NULL) {
      *cursor = 0;
   }

   buildmap_db_open (BuildMapPath, db_name);

   buildmap_square_save ();
   buildmap_line_save ();
   buildmap_point_save ();
   buildmap_shape_save ();
   buildmap_dictionary_save ();
   buildmap_city_save ();
   buildmap_street_save ();
   buildmap_range_save ();
   buildmap_polygon_save ();
   buildmap_zip_save ();

   buildmap_db_close ();
}

static void buildmap_county_reset (void) {

   buildmap_square_reset ();
   buildmap_line_reset ();
   buildmap_point_reset ();
   buildmap_shape_reset ();
   buildmap_dictionary_reset ();
   buildmap_city_reset ();
   buildmap_street_reset ();
   buildmap_range_reset ();
   buildmap_polygon_reset ();
   buildmap_zip_reset ();
   roadmap_hash_reset ();
}

static void buildmap_county_process
               (char *path, char *name, int verbose, int canals, int rivers) {

   buildmap_county_initialize ();

   buildmap_tiger_initialize (verbose, canals, rivers);

   buildmap_tiger_read_rtc (path, name, verbose);

   buildmap_tiger_read_rt7 (path, name, verbose);

   buildmap_tiger_read_rt8 (path, name, verbose);

   buildmap_tiger_read_rti (path, name, verbose);

   buildmap_tiger_read_rt1 (path, name, verbose);

   buildmap_tiger_read_rt2 (path, name, verbose);

   buildmap_tiger_sort();

   if (verbose) {
      roadmap_hash_summary ();
      buildmap_dictionary_summary ();
      buildmap_tiger_summary ();
   }

   buildmap_county_save (name);
   buildmap_county_reset ();
}


int main (int argc, const char **argv) {

   const char **leftovers;

   char *extension;
   DIR  *directory;
   struct dirent *entry;

   poptContext decoder =
      poptGetContext ("buildmap", argc, argv, BuildMapOptionTable, 0);

   while (poptGetNextOpt(decoder) > 0) ;

   if (BuildMapFormat != 2000 && BuildMapFormat != 2002) {

      fprintf (stderr, "Bad TIGER format %d\n", BuildMapFormat);
      poptPrintUsage (decoder, stderr, 0);
      exit (1);
   }

   buildmap_tiger_set_format (BuildMapFormat);
   buildmap_tiger_set_prefix (BuildMapPrefix);

   directory = opendir (BuildMapTiger);

   if (directory == NULL) {
      fprintf (stderr, "cannot open directory %s\n", BuildMapTiger);
   }

   leftovers = poptGetArgs(decoder);

   if (leftovers != NULL && leftovers[0] != NULL)
   {
      while (*leftovers != NULL) {

         buildmap_county_process
            (BuildMapTiger, (char *) (*leftovers),
             BuildMapVerbose, BuildMapCanals, BuildMapRivers);

         if (BuildMapVerbose) {
            printf ("Done with county %s\n", *leftovers);
         }
         leftovers += 1;
      }

   } else {

      int prefix_length = strlen(BuildMapPrefix);

      for (entry = readdir (directory);
           entry != NULL;
           entry = readdir (directory)) {

         extension = entry->d_name + strlen(entry->d_name) - 4;

         if (extension == NULL) continue;

         if (strncmp (entry->d_name, BuildMapPrefix, prefix_length) == 0 &&
             strcmp (extension, ".RT1") == 0) {

            char *from;
            char *to;
            char  county[8];

            if ((BuildMapState != NULL) &&
                (strncmp (entry->d_name + prefix_length,
                          BuildMapState,
                          strlen(BuildMapState)) != 0)) continue;

            for (from = entry->d_name + 3, to = county;
                 from < extension && to < county + 7;
                 from++, to++) {
               *to = *from;
            }
            *to = 0;

            buildmap_county_process
               (BuildMapTiger, county,
                BuildMapVerbose, BuildMapCanals, BuildMapRivers);

            if (BuildMapVerbose) {
               printf ("Done with county %s\n", county);
            }
         }
      }
   }

   closedir (directory);
   poptFreeContext (decoder);

   return 0;
}

