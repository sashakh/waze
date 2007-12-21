/* buildplace_main.c - The main function of the placename builder tool.
 *
 * LICENSE:
 *
 *   Copyright 2004 Stephen Woodbridge
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <search.h>

#include "roadmap_types.h"
#include "roadmap_hash.h"
#include "roadmap_path.h"

#include "buildmap.h"
#include "buildmap_opt.h"

#include "buildmap_square.h"
#include "buildmap_point.h"
#include "buildmap_place.h"

#if ROADMAP_USE_SHAPEFILES

#include <shapefil.h>

/* shapefile column names */

#define F_NAME  "NAME"
#define F_STATE "STATE"
#define F_CC    "CC"
#define F_TYPE  "TYPE"

#endif /* ROADMAP_USE_SHAPEFILES */


#define BUILDPLACE_FORMAT_SHAPE     1
#define BUILDPLACE_FORMAT_TXT       2

#define BUILDPLACE_MAX_DSG       1024
static char *BuildPlaceDSGStrings[BUILDPLACE_MAX_DSG] = {NULL};
static int   BuildPlaceDSGcfcc[BUILDPLACE_MAX_DSG];
static int   BuildPlaceDSGCount = 0;

static int   BuildPlaceFormatFamily = 0;

static int   BuildPlaceVerbose = 0;
static char *BuildPlaceFormat  = "SHAPE";

static char *BuildPlaceResult;
static char *BuildPlaceDSGFile  = "./designations.txt";

struct opt_defs options[] = {
   {"format", "f", opt_string, "TXT",
      "Input files format (Text or ShapeFile)"},
   {"dsg", "d", opt_string, "./designations.txt",
        "The designations.txt file"},
   {"maps", "m", opt_string, "",
        "Location for the generated map files"},
   {"verbose", "v", opt_flag, "0",
        "Show progress information"},
   OPT_DEFS_END
};


#if ROADMAP_USE_SHAPEFILES
static RoadMapString
str2dict (BuildMapDictionary d, const char *string) {

    if (!strlen(string)) {
        return buildmap_dictionary_add (d, "", 0);
    }

    return buildmap_dictionary_add (d, (char *) string, strlen(string));
}
#endif /* ROADMAP_USE_SHAPEFILES */


static int  buildplace_select_format (void) {

   if (strcmp (BuildPlaceFormat, "TXT") == 0) {

      BuildPlaceFormatFamily = BUILDPLACE_FORMAT_TXT;

   } else if (strcmp (BuildPlaceFormat, "SHAPE") == 0) {

      BuildPlaceFormatFamily = BUILDPLACE_FORMAT_SHAPE;

   } else {
      fprintf (stderr, "%s: unsupported input format, must be TXT or SHAPE\n",
         BuildPlaceFormat);
      return 0;
   }
   return 1;
}


static void buildplace_save (const char *name) {

   char *cursor;
   char db_name[128];

   snprintf (db_name, 127, "usc%s", name);

   /* Remove the suffix if any was provided. */

   cursor = strrchr (db_name, '.');
   if (cursor != NULL) {
      *cursor = 0;
   }

   if (buildmap_db_open (BuildPlaceResult, db_name) < 0) {
      buildmap_fatal (0, "cannot create database %s", db_name);
   }

   buildmap_db_save ();

   buildmap_db_close ();
}

static void buildplace_dsg_reset(void) {
    int i;

    hdestroy();
    for (i=0; i<BUILDPLACE_MAX_DSG; i++) {
        if (BuildPlaceDSGStrings[i]) free(BuildPlaceDSGStrings[i]);
        BuildPlaceDSGStrings[i] = NULL;
    }
    BuildPlaceDSGCount = 0;
}


static void buildplace_dsg_initialize (void) {
    int i;

    hcreate(BUILDPLACE_MAX_DSG);
    for (i=0; i<BUILDPLACE_MAX_DSG; i++) BuildPlaceDSGcfcc[i] = 0;
    BuildPlaceDSGCount = 0;
}


#if ROADMAP_USE_SHAPEFILES
static int dsg2cfcc (const char *dsg) {
    ENTRY e, *ep;

    e.key = (char *) dsg;
    ep = hsearch(e, FIND);
    return ep ? BuildPlaceDSGcfcc[(int)ep->data] : 0;
}
#endif


static void buildplace_dsg_add (const char *dsg, int cfcc) {
    ENTRY e, *ep;

    e.key = (char *) dsg;
    ep = hsearch(e, FIND);
    if (!ep) {
        if (BuildPlaceDSGCount + 1 > BUILDPLACE_MAX_DSG)
            buildmap_fatal(0, "maximum designations has been exceeded");
        e.key = BuildPlaceDSGStrings[BuildPlaceDSGCount] = strdup(dsg);
        e.data = (char *) BuildPlaceDSGCount;
        ep = hsearch(e, ENTER);
        if (!ep)
            buildmap_fatal(0, "failed to add designation to hash");
        BuildPlaceDSGcfcc[BuildPlaceDSGCount++] = cfcc;
    }
}


static void buildplace_dsg_summary (void) {

    fprintf (stderr,
        "-- DSG hash statistics: %d DSG codes\n",
        BuildPlaceDSGCount);
}


static void buildplace_shapefile_process (const char *source, int verbose) {

#if ROADMAP_USE_SHAPEFILES

    static BuildMapDictionary DictionaryName;

    char name[160];
    int irec;
    int record_count;
    int place;
    int pname;
    int cfcc;
    int point;
    int lat, lon;

    int iNAME, iSTATE, iCC, iTYPE;

    DBFHandle hDBF;
    SHPHandle hSHP;
    SHPObject *shp;

    DictionaryName = buildmap_dictionary_open ("placename");
    
    buildmap_set_source((char *) source);
    
    hDBF = DBFOpen(source, "rb");
    hSHP = SHPOpen(source, "rb");

    iNAME  = DBFGetFieldIndex(hDBF, F_NAME);
    iSTATE = DBFGetFieldIndex(hDBF, F_STATE);
    iCC    = DBFGetFieldIndex(hDBF, F_CC  );
    iTYPE  = DBFGetFieldIndex(hDBF, F_TYPE);

    record_count = DBFGetRecordCount(hDBF);

    for (irec=0; irec<record_count; irec++) {
    
        strcpy(name, DBFReadStringAttribute(hDBF, irec, iCC));
        strcat(name, "/");
        strcat(name, DBFReadStringAttribute(hDBF, irec, iSTATE));
        strcat(name, "/");
        strcat(name, DBFReadStringAttribute(hDBF, irec, iNAME));

        pname = str2dict (DictionaryName, name);

        cfcc  = dsg2cfcc(DBFReadStringAttribute(hDBF, irec, iTYPE));
        if (cfcc == 0)
            continue;

        /* add the place */

        shp = SHPReadObject(hSHP, irec);

        lon = shp->padfX[0] * 1000000.0;
        lat = shp->padfY[0] * 1000000.0;

        SHPDestroyObject(shp);

        point = buildmap_point_add (lon, lat);

        place = buildmap_place_add (pname, cfcc, point);
        
        if (verbose) {
            if ((irec & 0xff) == 0) {
                buildmap_progress (irec, record_count);
            }
        }     
              
    }

    DBFClose(hDBF);
    SHPClose(hSHP);

#else

   fprintf (stderr,
            "cannot process %s: built with no shapefile support.\n",
            source);
   exit(1);

#endif /* ROADMAP_USE_SHAPEFILES */
}


static void buildplace_txt_process (const char *source, int verbose) {

}


static void buildplace_read_dsg (const char *dsgfile) {

    FILE *file;
    char buff[2048];
    char *p;
    int  c;
    int  cfcc;
   
    buildplace_dsg_initialize();
   
    file = fopen (dsgfile, "rb");
    if (file == NULL) {
        buildmap_fatal (0, "cannot open file %s", dsgfile);
    }

    while (!feof(file)) {

        if (fgets(buff, 2048, file)) {
            c = strspn(buff, " \t\r\n");
            if (buff[c] == '#' || strlen(buff+c) == 0) continue;
            
            cfcc = strtol(buff+c, &p, 10);
            if (cfcc < 0 || cfcc > BUILDMAP_MAX_PLACE_CFCC)
                buildmap_fatal (0, "place cfcc is out of range");
            
            while (isspace(*p)) p++; /* skip leading blanks */
            c = strcspn(p, " \t\r\n");
            p[c] = '\0';
            
            buildplace_dsg_add(p, cfcc);
        }
    }

    fclose (file);
}


static void buildplace_process (const char *source, const char *fips,
                                int verbose) {

   switch (BuildPlaceFormatFamily) {

      case BUILDPLACE_FORMAT_SHAPE:
         buildplace_shapefile_process (source, verbose);
         break;

      case BUILDPLACE_FORMAT_TXT:
         buildplace_txt_process (source, verbose);
         break;
   }

   buildmap_db_sort();

   if (verbose) {

      roadmap_hash_summary ();
      buildmap_db_summary ();
      buildplace_dsg_summary ();
   }

   buildplace_save (fips);

   buildmap_db_reset ();
   buildplace_dsg_reset ();
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
   BuildPlaceResult = strdup(roadmap_path_preferred("maps")); /* default. */

   /* parse the options */
   error = opt_parse(options, &argc, argv, 0);
   if (error) usage(argv[0], opt_strerror(error));

   /* then, fetch the option values */
   error = opt_val("verbose", &BuildPlaceVerbose) ||
           opt_val("format", &BuildPlaceFormat) ||
           opt_val("dsg", &BuildPlaceDSGFile) ||
           opt_val("maps", &BuildPlaceResult);
   if (error)
      usage(argv[0], opt_strerror(error));

   if (!buildplace_select_format())
      exit(1);

   if (argc != 3) usage(argv[0], "missing required arguments");

   buildplace_read_dsg (BuildPlaceDSGFile);

   buildplace_process (argv[2], argv[1], BuildPlaceVerbose);

   return 0;
}

