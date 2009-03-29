/*
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

/**
 * @file
 * @brief The main function of the map builder tool.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_hash.h"
#include "roadmap_path.h"

#include "buildmap.h"
#include "buildmap_opt.h"
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
#define BUILDMAP_FORMAT_STATE     5
#define BUILDMAP_FORMAT_PROVINCE  6
#define BUILDMAP_FORMAT_EMPTY     7

static int   BuildMapFormatFamily = 0;

static int   BuildMapVerbose = 0;
static char *BuildMapFormat  = "2002";
static char *BuildMapClass   = "default/All";

static char *BuildMapResult;

int BuildMapNoLongLines;

struct opt_defs options[] = {
   {"format", "f", opt_string, "2002",
#ifdef ROADMAP_USE_SHAPEFILES
      "Input file format (TIGER or Shapefile data source)"
#else
      "Tiger file format (ShapeFile not enabled)"
#endif
   },
   {"class", "c", opt_string, "default/All",
        "The class file to create the map for"},
   {"maps", "m", opt_string, "",
        "Location for the generated map files"},
   {"nolonglines", "n", opt_flag, "0",
        "Suppress 'long line' lists (inter-square lines)"},
   {"verbose", "v", opt_flag, "0",
        "Show more progress information"},
   OPT_DEFS_END
};


static int  buildmap_county_select_format (void) {

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

   } else if (strncmp (BuildMapFormat, "STATES", strlen("STATES")) == 0) {
      int which = 0;

      if (strcmp (BuildMapFormat, "STATES=AK") == 0) {
        which = JUST_AK;
      } else if (strcmp (BuildMapFormat, "STATES=HI") == 0) {
        which = JUST_HI;
      } else if (strcmp (BuildMapFormat, "STATES=continental") == 0) {
        which = JUST_CONTINENTAL;
      } else if (strcmp (BuildMapFormat, "STATES=all") == 0 ||
            strcmp (BuildMapFormat, "STATES") == 0) {
        which = 0;
      }
      // US state boundaries
      BuildMapFormatFamily = BUILDMAP_FORMAT_STATE;

      buildmap_shapefile_set_states(which);

   } else if (strcmp (BuildMapFormat, "PROVINCES") == 0) {

      // US state boundaries
      BuildMapFormatFamily = BUILDMAP_FORMAT_PROVINCE;
#endif

   } else if (strcmp (BuildMapFormat, "EMPTY") == 0) {

      BuildMapFormatFamily = BUILDMAP_FORMAT_EMPTY;

   } else {
      fprintf (stderr, "%s: unsupported input format -- must be one of:\n",
        BuildMapFormat);
      fprintf (stderr, "   2002, 2000, EMPTY,\n");
#ifdef ROADMAP_USE_SHAPEFILES
      fprintf (stderr, "   SHAPE, RNF, DCW, PROVINCES,\n");
      fprintf (stderr, "   or STATES={AK,HI,continental,all}\n");
#endif
      return 0;
   }
   return 1;
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

      case BUILDMAP_FORMAT_STATE:
         buildmap_shapefile_state_process (source, county, verbose);
         break;

      case BUILDMAP_FORMAT_PROVINCE:
         buildmap_shapefile_province_process (source, county, verbose);
         break;
#endif

      case BUILDMAP_FORMAT_EMPTY:
         buildmap_empty_process (source);
         break;

      default:
         buildmap_fatal (0, "unsupported format %d", BuildMapFormatFamily);
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

void usage(char *progpath, const char *msg) {

   char *prog = strrchr(progpath, '/');

   if (prog)
       prog++;
   else
       prog = progpath;

   if (msg)
       fprintf(stderr, "%s: %s\n", prog, msg);
   fprintf(stderr,
       "usage: %s [options] <FIPS code> <source>\n", prog);
   opt_desc(options, 1);
   exit(1);
}

int main (int argc, char **argv) {

   int error;

   BuildMapResult = strdup(roadmap_path_preferred("maps")); /* default. */

   /* parse the options */
   error = opt_parse(options, &argc, argv, 0);
   if (error) usage(argv[0], opt_strerror(error));

   /* then, fetch the option values */
   error = opt_val("verbose", &BuildMapVerbose) ||
           opt_val("format", &BuildMapFormat) ||
           opt_val("class", &BuildMapClass) ||
           opt_val("maps", &BuildMapResult) ||
           opt_val("nolonglines", &BuildMapNoLongLines);
   if (error)
      usage(argv[0], opt_strerror(error));

   if (!buildmap_county_select_format())
      exit(1);

   if (argc != 3) usage(argv[0], "missing required arguments");

   buildmap_layer_load (BuildMapClass);

   buildmap_county_process (argv[2], argv[1], BuildMapVerbose);

   return 0;
}

