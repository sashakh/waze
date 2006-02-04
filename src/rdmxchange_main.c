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
 *   the Free Software Foundation, as of version 2 of the License.
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

static  char *RdmXchangePath = ".";
static int    RdmXchangeVerbose = 0;
static int    RdmXchangeDebug = 0;


#define RDMXCHANGE_MAX_MODULES       256

static const RdmXchangeExport *RdmXchangeExportList[RDMXCHANGE_MAX_MODULES];
static int RdmXchangeExportRegistered = 0;


static const RdmXchangeImport *RdmXchangeImportList[RDMXCHANGE_MAX_MODULES];
static int RdmXchangeImportLength[RDMXCHANGE_MAX_MODULES];
static int RdmXchangeImportRegistered = 0;


static void rdmxchange_main_trim (char *text) {

   char *eol;

   eol = strchr (text, '\n');
   if (eol != NULL) *eol = 0;
}


static int rdmxchange_main_split (char *text, char *field[], int max) {

   int   i;
   char *p;


   rdmxchange_main_trim (text);

   field[0] = text;
   p = strchr (text, ',');

   for (i = 1; p != NULL && *p != 0; ++i) {

      *p = 0;
      if (i >= max) {
         buildmap_fatal ((int)(p - text), "too many fields");
      }
      field[i] = ++p;

      p = strchr (p, ',');
   }

   return i;
}


static const RdmXchangeImport *rdmxchange_main_search (const char *table) {

   int i;

   for (i = RdmXchangeImportRegistered - 1; i >= 0; --i) {

      int length = RdmXchangeImportLength[i];

      if (strncmp (RdmXchangeImportList[i]->type, table, length) == 0) {

         if ((table[length] == '/') || (table[length] == 0)) {
            return RdmXchangeImportList[i];
         }
      }
   }
   buildmap_fatal (1, "unknown table %s", table);
   return NULL;
}


static void rdmxchange_main_analyse (FILE *input) {

   int   line_count;
   int   count;
   int   table_index;
   char *field[256];
   char  table_name[1024];
   char  buffer[1024];
   const RdmXchangeImport *importer;

   line_count = 0;

   /* Analyze all the table declarations. */

   while (! feof(input)) {

      if (fgets (buffer, sizeof(buffer), input) == NULL) return;

      buildmap_set_line (++line_count);

      /* The first empty line signals the end of the table declarations. */
      if (buffer[0] == '\n' || buffer[0] == 0) break;

      if (RdmXchangeDebug) {
         printf ("%s", buffer);
         fflush(stdout);
      }
      if (rdmxchange_main_split (buffer, field, 256) != 2) {
         buildmap_fatal (1, "invalid table descriptor");
      }

      importer = rdmxchange_main_search (field[0]);
      if (importer == NULL) continue;

      count = atoi(field[1]);
      if (count <= 0) {
         buildmap_fatal (2, "%s: invalid record count %d", field[0], count);
      }

      importer->table (field[0], count);
   }

   /* Analyze the data records, table per table. */

   while (! feof(input)) {

      /* read the table name. */

      if (fgets (table_name, sizeof(table_name), input) == NULL) return;

      buildmap_set_line (++line_count);
      if (table_name[0] == '\n' || table_name[0] == 0) continue;

      if (RdmXchangeDebug) {
         printf ("%s", table_name);
         fflush(stdout);
      }
      rdmxchange_main_trim (table_name);
      importer = rdmxchange_main_search (table_name);


      /* Read the field line. */

      if (fgets (buffer, sizeof(buffer), input) == NULL) return;

      buildmap_set_line (++line_count);
      if (buffer[0] == '\n' || buffer[0] == 0) continue;

      if (RdmXchangeDebug) {
         printf ("%s", buffer);
         fflush(stdout);
      }
      count = rdmxchange_main_split (buffer, field, 256);
      table_index = importer->schema (table_name, field, count);


      /* Read the data records until we find an empty line. */

      while (! feof(input)) {

         if (fgets (buffer, sizeof(buffer), input) == NULL) return;

         buildmap_set_line (++line_count);
         if (buffer[0] == '\n' || buffer[0] == 0) break;

         if (importer == NULL) continue;

         if (RdmXchangeDebug) {
            printf ("%s", buffer);
            fflush(stdout);
         }
         count = rdmxchange_main_split (buffer, field, 256);
         importer->data (table_index, field, count);
      }
   }
}


static void rdmxchange_main_declare (const roadmap_db_handler *dbhandler,
                                     const RdmXchangeImport   *importer) {

   /* Declare the export side (map reader). */

   RdmMapModel = roadmap_db_register (RdmMapModel, dbhandler->name, dbhandler);


   /* Declare the import side (import code). */

   if (RdmXchangeImportRegistered >= RDMXCHANGE_MAX_MODULES) {
      fprintf (stderr, "too many tables\n");
      exit(1);
   }
   RdmXchangeImportLength[RdmXchangeImportRegistered] = strlen(importer->type);
   RdmXchangeImportList[RdmXchangeImportRegistered++] = importer;
}


/* The export list is dynamic, since we don't need to export a table
 * if that table is not present in the map file.
 */
void rdmxchange_main_register_export (const RdmXchangeExport *export) {

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


int rdmxchange_import_int (const char *data) {

   if (data[0] == 0) return 0;
   return atoi(data);
}


/* Main program. -----------------------------------------------------------
 */

int main (int argc, const char **argv) {

   int i;


   rdmxchange_main_declare (&RoadMapZipExport,      &RdmXchangeZipImport);
   rdmxchange_main_declare (&RoadMapStreetExport,   &RdmXchangeStreetImport);
   rdmxchange_main_declare (&RoadMapRangeExport,    &RdmXchangeRangeImport);
   rdmxchange_main_declare (&RoadMapPolygonExport,  &RdmXchangePolygonImport);
   rdmxchange_main_declare (&RoadMapShapeExport,    &RdmXchangeShapeImport);
   rdmxchange_main_declare (&RoadMapLineExport,     &RdmXchangeLineImport);
   rdmxchange_main_declare (&RoadMapPointExport,    &RdmXchangePointImport);
   rdmxchange_main_declare (&RoadMapSquareExport,   &RdmXchangeSquareImport);
   rdmxchange_main_declare (&RoadMapIndexExport,    &RdmXchangeIndexImport);
   rdmxchange_main_declare (&RoadMapMetadataExport, &RdmXchangeMetadataImport);
   rdmxchange_main_declare (&RoadMapDictionaryExport,
                            &RdmXchangeDictionaryImport);


   for (i = 1; i < argc; ++i) {

      if (argv[i][0] == '-') {

         /* This is an option. */

         if (strcasecmp (argv[i], "-h") == 0 ||
               strcasecmp (argv[i], "--help") == 0) {
            printf ("Usage: rdmxchange [-v|--verbose] [--path=path] file ...\n");
            exit(0);
         }

         if (strcasecmp (argv[i], "-v") == 0 ||
               strcasecmp (argv[i], "--verbose") == 0) {

            RdmXchangeVerbose = 1;
         }
         else if (strcasecmp (argv[i], "--debug") == 0) {

            RdmXchangeDebug = 1;
         }
         else if (strncasecmp (argv[i], "--path=", 7) == 0) {

            RdmXchangePath = strdup(argv[i]+7);
         }
         else {
            fprintf (stderr, "invalid option %s\n", argv[i]);
            exit(1);
         }

      } else {

         /* This is a file to import or export. */

         FILE *io;

         char *fullname;
         char *basename = strdup(roadmap_path_skip_directories (argv[i]));
         char *extension = strrchr (basename, '.');

         if (extension == NULL) {
            fprintf (stderr, "invalid input file %s (no extension)\n", argv[i]);
            exit(1);
         }

         if (strcmp (extension, ".rdm") == 0) {

            /* --- This is a file to export. */

            int export;


            if (strcmp (basename, "usdir.rdm") == 0) {

               fprintf (stderr, "obsolete US directory file found. Skipped.\n");
               free (basename);
               continue;
            }


            RdmXchangeExportRegistered = 0;

            if (! roadmap_db_open ("", argv[i], RdmMapModel)) {
               fprintf (stderr, "cannot open map database %s", argv[i]);
               exit(1);
            }
            roadmap_db_activate ("", argv[i]);

            strcpy (extension, ".rdx");

            fullname = roadmap_path_join (RdmXchangePath, basename);
            io = fopen (fullname, "w");
            if (io == NULL) {
               fprintf (stderr, "cannot open output file %s", fullname);
               exit(1);
            }

            for (export = 0; export < RdmXchangeExportRegistered; ++export) {
               RdmXchangeExportList[export]->head (io);
            }

            fprintf (io, "\n");

            for (export = 0; export < RdmXchangeExportRegistered; ++export) {
               RdmXchangeExportList[export]->data (io);
            }

            fclose (io);
            roadmap_db_close ("", argv[i]);
            roadmap_path_free (fullname);

         } else if (strcmp (extension, ".rdx") == 0) {

            /* --- This is a file to import. */

            io = fopen (argv[i], "r");
            if (io == NULL) {
               fprintf (stderr, "cannot open input file %s", argv[i]);
               exit(1);
            }
            buildmap_set_source (argv[i]);
            rdmxchange_main_analyse (io);
            fclose (io);

            strcpy (extension, ".rdm");

            if (buildmap_db_open (RdmXchangePath, basename) < 0) {
               fprintf (stderr, "cannot create map database %s", basename);
               exit(1);
            }
            buildmap_db_save ();
            buildmap_db_close ();

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

