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

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_hash.h"
#include "roadmap_path.h"

#include "buildmap.h"
#include "buildmap_tiger.h"
#include "buildmap_shapefile.h"
#include "buildmap_empty.h"

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
#include "buildmap_metadata.h"

#include "buildmap_layer.h"


#define BUILDMAP_FORMAT_TIGER     1
#define BUILDMAP_FORMAT_SHAPE     2
#define BUILDMAP_FORMAT_DCW       3
#define BUILDMAP_FORMAT_RNF       4
#define BUILDMAP_FORMAT_EMPTY     5

static int   BuildMapFormatFamily = 0;

static int   BuildMapVerbose = 0;
static char *BuildMapFormat  = "2002";
static char *BuildMapClass   = "default/All";

static char *BuildMapResult;

static struct poptOption BuildMapTigerOptions [] = {

   {"format", 'f',
      POPT_ARG_STRING, &BuildMapFormat, 0,
#ifdef ROADMAP_USE_SHAPEFILES
      "Input files format (Tiger or ShapeFile)", "2000|2002|SHAPE|RNF|DCW|EMPTY"},
#else
      "Tiger file format (ShapeFile was not enabled)", "2000|2002|EMPTY"},
#endif

   POPT_TABLEEND
};

static struct poptOption BuildMapDataOptions [] = {

   {"class", 'c',
      POPT_ARG_STRING, &BuildMapClass, 0,
      "The class file to create the map for", NULL},

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

#ifdef ROADMAP_USE_SHAPEFILES
   } else if (strcmp (BuildMapFormat, "SHAPE") == 0) {

      // commercial DMTI canadian format
      BuildMapFormatFamily = BUILDMAP_FORMAT_SHAPE;

   } else if (strcmp (BuildMapFormat, "RNF") == 0) {

      // free RNF canadian format
      BuildMapFormatFamily = BUILDMAP_FORMAT_RNF;

   } else if (strcmp (BuildMapFormat, "DCW") == 0) {

      // Digital Charts of the World
      BuildMapFormatFamily = BUILDMAP_FORMAT_DCW;
#endif

   } else if (strcmp (BuildMapFormat, "EMPTY") == 0) {

      BuildMapFormatFamily = BUILDMAP_FORMAT_EMPTY;

   } else {
      fprintf (stderr, "%s: unsupported input format\n", BuildMapFormat);
      poptPrintUsage (decoder, stderr, 0);
      exit (1);
   }
}


static void buildmap_county_save (const char *name) {

   char db_name[128];

   snprintf (db_name, 127, "usc%s.rdm", name);


   if (buildmap_db_open (BuildMapResult, db_name) < 0) {
      buildmap_fatal (0, "cannot create database %s", db_name);
   }

   buildmap_db_save ();

   buildmap_db_close ();
}


static void buildmap_county_process (const char *source,
                                     const char *county,
                                     int verbose) {

   switch (BuildMapFormatFamily) {

      case BUILDMAP_FORMAT_TIGER:
         buildmap_tiger_process (source, county, verbose);
         break;

#ifdef ROADMAP_USE_SHAPEFILES
      case BUILDMAP_FORMAT_SHAPE:
         buildmap_shapefile_dmti_process (source, county, verbose);
         break;

      case BUILDMAP_FORMAT_RNF:
         buildmap_shapefile_rnf_process (source, county, verbose);
         break;

      case BUILDMAP_FORMAT_DCW:
         buildmap_shapefile_dcw_process (source, county, verbose);
         break;
#endif

      case BUILDMAP_FORMAT_EMPTY:
         buildmap_empty_process (source);
         break;

      default:
         roadmap_log (ROADMAP_ERROR, "unsupported format");
         return;
   }

   buildmap_db_sort();

   if (verbose) {
      roadmap_hash_summary ();
      buildmap_db_summary ();
   }

   buildmap_county_save (county);
   buildmap_db_reset ();
   roadmap_hash_reset ();
}


int main (int argc, const char **argv) {

   const char **leftovers;
   poptContext decoder;


   BuildMapResult = strdup(roadmap_path_preferred("maps")); /* default. */

   decoder =
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

   buildmap_layer_load (BuildMapClass);

   buildmap_county_process
            ((char *) (leftovers[1]),
             (char *) (leftovers[0]),
             BuildMapVerbose);

   poptFreeContext (decoder);

   return 0;
}

