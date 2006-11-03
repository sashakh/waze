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
#include <popt.h>
#include <time.h>

#include "roadmap_types.h"
#include "roadmap_hash.h"
#include "roadmap_path.h"

#include "buildmap.h"
#include "buildmap_tiger.h"
#include "buildmap_shapefile.h"
#include "buildmap_empty.h"
#include "buildmap_postgres.h"

#include "buildmap_square.h"
#include "buildmap_street.h"
#include "buildmap_range.h"
#include "buildmap_line.h"
#include "buildmap_line_route.h"
#include "buildmap_line_speed.h"
#include "buildmap_dglib.h"
#include "buildmap_point.h"
#include "buildmap_shape.h"
#include "buildmap_turn_restrictions.h"
#include "buildmap_city.h"
#include "buildmap_zip.h"
#include "buildmap_area.h"
#include "buildmap_polygon.h"
#include "buildmap_metadata.h"


#define BUILDMAP_FORMAT_TIGER     1
#define BUILDMAP_FORMAT_SHAPE     2
#define BUILDMAP_FORMAT_DCW       3
#define BUILDMAP_FORMAT_EMPTY     4
#define BUILDMAP_FORMAT_PG        5

static int   BuildMapFormatFamily = 0;

static int   BuildMapCanals  = 0;
static int   BuildMapRivers  = 0;
static int   BuildMapVerbose = 0;
static char *BuildMapFormat  = "2002";

static char *BuildMapResult;

static time_t creation_time;

static struct poptOption BuildMapTigerOptions [] = {

   {"format", 'f',
      POPT_ARG_STRING, &BuildMapFormat, 0,
      "Input files format (Tiger or ShapeFile)", "2000|2002|SHAPE|DCW|EMPTY|PG"},

   POPT_TABLEEND
};

static struct poptOption BuildMapDataOptions [] = {

   {"canals", 'c',
      POPT_ARG_NONE, &BuildMapCanals, 0, "Show canals on maps", NULL},

   {"rivers", 'r',
      POPT_ARG_NONE, &BuildMapRivers, 0, "Show rivers on maps", NULL},

   POPT_TABLEEND
};

static struct poptOption BuildMapGeneralOptions [] = {

   {"verbose", 'v',
      POPT_ARG_NONE, &BuildMapVerbose, 0, "Show progress information", NULL},

   {"maps", 'm',
      POPT_ARG_STRING, &BuildMapResult, 0,
      "Location of the RoadMap maps (generated files)", "PATH"},

   POPT_TABLEEND
};

static struct poptOption BuildMapOptionTable [] = {

   POPT_AUTOHELP

   {NULL, 0,
        POPT_ARG_INCLUDE_TABLE, BuildMapTigerOptions, 0, "Tiger file options", NULL},

   {NULL, 0,
        POPT_ARG_INCLUDE_TABLE, BuildMapDataOptions, 0, "Map filter options", NULL},

   {NULL, 0,
        POPT_ARG_INCLUDE_TABLE, BuildMapGeneralOptions, 0, "BuildMap's general options", NULL},

   POPT_TABLEEND
};


static void  buildmap_county_select_format (poptContext decoder) {

   if (strcmp (BuildMapFormat, "2002") == 0) {

      BuildMapFormatFamily = BUILDMAP_FORMAT_TIGER;

      buildmap_tiger_set_format (2002);

   } else if (strcmp (BuildMapFormat, "2000") == 0) {

      BuildMapFormatFamily = BUILDMAP_FORMAT_TIGER;

      buildmap_tiger_set_format (2000);

   } else if (strcmp (BuildMapFormat, "SHAPE") == 0) {

      BuildMapFormatFamily = BUILDMAP_FORMAT_SHAPE;

   } else if (strcmp (BuildMapFormat, "DCW") == 0) {

      BuildMapFormatFamily = BUILDMAP_FORMAT_DCW;
         
   } else if (strcmp (BuildMapFormat, "EMPTY") == 0) {

      BuildMapFormatFamily = BUILDMAP_FORMAT_EMPTY;
         
   } else if (strcmp (BuildMapFormat, "PG") == 0) {

      BuildMapFormatFamily = BUILDMAP_FORMAT_PG;
         
   } else {
      fprintf (stderr, "%s: unsupported input format\n", BuildMapFormat);
      poptPrintUsage (decoder, stderr, 0);
      exit (1);
   }
}

static void buildmap_county_initialize (void) {

   buildmap_zip_initialize();
   buildmap_city_initialize();
   buildmap_point_initialize();
   buildmap_range_initialize();
   buildmap_line_initialize();
   buildmap_line_route_initialize();
   buildmap_line_speed_initialize();
   buildmap_dglib_initialize(creation_time);
   buildmap_polygon_initialize();
   buildmap_shape_initialize();
   buildmap_turn_restrictions_initialize();
   buildmap_street_initialize();
   buildmap_area_initialize();
   buildmap_metadata_initialize();
}


static void buildmap_county_sort (void) {

   buildmap_line_sort ();
   buildmap_line_route_sort ();
   buildmap_line_speed_sort ();
   buildmap_dglib_sort ();
   buildmap_street_sort ();
   buildmap_range_sort ();
   buildmap_shape_sort ();
   buildmap_turn_restrictions_sort ();
   buildmap_polygon_sort ();
   buildmap_metadata_sort ();
}

static void buildmap_county_save (const char *name) {

   char *cursor;
   char db_name[128];

   snprintf (db_name, 127, "usc%s", name);

   /* Remove the suffix if any was provided. */

   cursor = strrchr (db_name, '.');
   if (cursor != NULL) {
      *cursor = 0;
   }

   if (buildmap_db_open (BuildMapResult, db_name) < 0) {
      buildmap_fatal (0, "cannot create database %s", db_name);
   }

   buildmap_square_save ();
   buildmap_line_save ();
   buildmap_line_route_save ();
   buildmap_line_speed_save ();
   buildmap_dglib_save (BuildMapResult, db_name);
   buildmap_point_save ();
   buildmap_shape_save ();
   buildmap_turn_restrictions_save ();
   buildmap_dictionary_save ();
   buildmap_city_save ();
   buildmap_street_save ();
   buildmap_range_save ();
   buildmap_polygon_save ();
   buildmap_zip_save ();
   buildmap_metadata_save ();

   buildmap_db_close ();
}

static void buildmap_county_reset (void) {

   buildmap_square_reset ();
   buildmap_line_reset ();
   buildmap_line_route_reset ();
   buildmap_line_speed_reset ();
   buildmap_dglib_reset (creation_time);
   buildmap_point_reset ();
   buildmap_shape_reset ();
   buildmap_turn_restrictions_reset ();
   buildmap_dictionary_reset ();
   buildmap_city_reset ();
   buildmap_street_reset ();
   buildmap_metadata_reset ();
   buildmap_range_reset ();
   buildmap_polygon_reset ();
   buildmap_zip_reset ();
   roadmap_hash_reset ();
}

static void buildmap_county_process (const char *source,
                                     const char *county,
                                     int verbose, int canals, int rivers) {

   char unix_time_str[255];
   buildmap_county_initialize ();

   switch (BuildMapFormatFamily) {

      case BUILDMAP_FORMAT_TIGER:
         buildmap_tiger_process (source, verbose, canals, rivers);
         break;

      case BUILDMAP_FORMAT_SHAPE:
         buildmap_shapefile_process (source, verbose, canals, rivers);
         break;

      case BUILDMAP_FORMAT_DCW:
         buildmap_shapefile_dcw_process (source, verbose, canals, rivers);
         break;

      case BUILDMAP_FORMAT_EMPTY:
         buildmap_empty_process (source);
         break;

      case BUILDMAP_FORMAT_PG:
         buildmap_postgres_process (source, verbose, canals);
         break;
   }

   buildmap_metadata_add_attribute ("Version", "Date",
         asctime (gmtime (&creation_time)));
   
   snprintf (unix_time_str, sizeof(unix_time_str), "%ld", creation_time);
   buildmap_metadata_add_attribute ("Version", "UnixTime", unix_time_str);

   buildmap_county_sort();

   if (verbose) {

      roadmap_hash_summary ();
      buildmap_dictionary_summary ();

      buildmap_zip_summary ();
      buildmap_square_summary ();
      buildmap_street_summary ();
      buildmap_line_summary ();
      buildmap_line_route_summary ();
      buildmap_line_speed_summary ();
      buildmap_dglib_summary ();
      buildmap_range_summary ();
      buildmap_shape_summary ();
      buildmap_turn_restrictions_summary ();
      buildmap_polygon_summary ();
      buildmap_metadata_summary ();
   }

   buildmap_county_save (county);
   buildmap_county_reset ();
}


int main (int argc, const char **argv) {

   const char **leftovers;

   time (&creation_time);

   BuildMapResult = strdup(roadmap_path_preferred("maps")); /* default. */

   poptContext decoder =
      poptGetContext ("buildmap", argc, argv, BuildMapOptionTable, 0);

   poptSetOtherOptionHelp(decoder, "[OPTIONS]* <fips> <source>");

   while (poptGetNextOpt(decoder) > 0) ;

   buildmap_county_select_format (decoder);

   leftovers = poptGetArgs(decoder);

   if (leftovers == NULL || leftovers[0] == NULL || leftovers[1] == NULL)
   {
      poptPrintUsage (decoder, stderr, 0);
      return 1;
   }

   buildmap_county_process
            ((char *) (leftovers[1]),
             (char *) (leftovers[0]),
             BuildMapVerbose, BuildMapCanals, BuildMapRivers);

   poptFreeContext (decoder);

   return 0;
}

