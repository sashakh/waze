/* rdmxchange_main.c - a tool to import/export RoadMap map file.
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
 *
 *   The purpose of the rdmxchange tool is to convert map and index files
 *   both way between the binary representation (.rdm, used by roadmap) and
 *   the ascii representation (.rdx, used for download and architecture
 *   independent).
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_dbread.h"

#include "rdmxchange.h"


static roadmap_db_model *RdmMapModel = NULL;
static roadmap_db_model *RdmIndexModel = NULL;

static  char *RdmXchangePath = ".";
static int    RdmXchangeVerbose = 0;


#define RDMXCHANGE_MAX_MODULES       256

static RdmXchangeExport *RdmXchangeExportList[RDMXCHANGE_MAX_MODULES];
static int RdmXchangeExportRegistered = 0;


void rdmxchange_main_register_export (RdmXchangeExport *export) {

   int i;

   for (i = 0; i < RdmXchangeExportRegistered; ++i) {
      if (RdmXchangeExportList[i] == export) {
         return; /* Already registered. */
      }
   }

   if (RdmXchangeExportRegistered >= RDMXCHANGE_MAX_MODULES) {
      fprintf (stderr, "too many tables\n");
      exit(1);
   }
   RdmXchangeExportList[RdmXchangeExportRegistered++] = export;
}


/* Main program. -----------------------------------------------------------
 */

int main (int argc, const char **argv) {

   int i;


   RdmMapModel =
      roadmap_db_register (RdmMapModel, "zip", &RoadMapZipExport);
   RdmMapModel =
      roadmap_db_register (RdmMapModel, "street", &RoadMapStreetExport);
   RdmMapModel =
      roadmap_db_register (RdmMapModel, "range", &RoadMapRangeExport);
   RdmMapModel =
      roadmap_db_register (RdmMapModel, "polygon", &RoadMapPolygonExport);
   RdmMapModel =
      roadmap_db_register (RdmMapModel, "shape", &RoadMapShapeExport);
   RdmMapModel =
      roadmap_db_register (RdmMapModel, "line", &RoadMapLineExport);
   RdmMapModel =
      roadmap_db_register (RdmMapModel, "point", &RoadMapPointExport);
   RdmMapModel =
      roadmap_db_register (RdmMapModel, "square", &RoadMapSquareExport);
   RdmMapModel =
      roadmap_db_register (RdmMapModel, "metadata", &RoadMapMetadataExport);
   RdmMapModel =
      roadmap_db_register (RdmMapModel, "string", &RoadMapDictionaryExport);

   RdmIndexModel =
      roadmap_db_register (RdmIndexModel, "index", &RoadMapIndexExport);
   RdmIndexModel =
      roadmap_db_register (RdmIndexModel, "metadata", &RoadMapMetadataExport);
   RdmIndexModel =
      roadmap_db_register (RdmIndexModel, "string", &RoadMapDictionaryExport);


   for (i = 1; i < argc; ++i) {

      if (argv[i][0] == '-') {

         /* This is an option. */

         if (strcasecmp (argv[i], "-h") == 0 ||
               strcasecmp (argv[i], "--help") == 0) {
            printf ("Usage: rdmxchange [-v|--verbose] file ...\n");
            exit(0);
         }

         if (strcasecmp (argv[i], "-v") == 0 ||
               strcasecmp (argv[i], "--verbose") == 0) {

            RdmXchangeVerbose = 1;
         }
         else if (strncasecmp (argv[i], "--output=", 9) == 0) {

            RdmXchangePath = strdup(argv[i]+9);
         }
         else {
            fprintf (stderr, "invalid option %s\n", argv[i]);
            exit(1);
         }

      } else {

         /* This is a file to import or export. */

         FILE *output;

         char *fullname;
         char *basename = strdup(roadmap_path_skip_directories (argv[i]));
         char *extension = strrchr (basename, '.');

         if (extension == NULL) {
            fprintf (stderr, "invalid input file %s\n", argv[i]);
            exit(1);
         }

         if (strcmp (extension, ".rdm") == 0) {

            /* This is a file to export. */

            int export;
            roadmap_db_model *model;


            if (strcmp (basename, "index.rdm") == 0) {

               model = RdmIndexModel;

            } else if (strcmp (basename, "usdir.rdm") == 0) {

               fprintf (stderr, "obsolete US directory file found. Skipped.\n");
               free (basename);
               continue;

            } else {

               model = RdmMapModel;
            }


            RdmXchangeExportRegistered = 0;

            if (! roadmap_db_open ("", argv[i], model)) {
               fprintf (stderr, "cannot open map database %s", argv[i]);
               exit(1);
            }
            roadmap_db_activate ("", argv[i]);

            strcpy (extension, ".rdx");

            fullname = roadmap_path_join (RdmXchangePath, basename);
            output = fopen (fullname, "w");
            if (output == NULL) {
               fprintf (stderr, "cannot open output file %s", fullname);
               exit(1);
            }

            for (export = 0; export < RdmXchangeExportRegistered; ++export) {
               RdmXchangeExportList[export]->head (output);
            }

            fprintf (output, "\n");

            for (export = 0; export < RdmXchangeExportRegistered; ++export) {
               RdmXchangeExportList[export]->data (output);
            }

            fclose (output);
            roadmap_db_close ("", argv[i]);
            roadmap_path_free (fullname);

         } else if (strcmp (argv[i], ".rdx") == 0) {

            /* This is a file to import. */

         } else {

            fprintf (stderr, "invalid input file %s\n", argv[i]);
            exit(1);
         }

         free (basename);
      }
   }

   roadmap_db_end ();

   return 0;
}

