/* rdmxchange_zip.c - Export Postal code data.
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
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rdmxchange.h"


static char *RoadMapZipType    = "RoadMapZipContext";

typedef struct {

   char *type;

   int *ZipCode;
   int  ZipCodeCount;

} RoadMapZipContext;

static RoadMapZipContext    *RoadMapZipActive    = NULL;


static void rdmxchange_zip_register_export (void);


static void *rdmxchange_zip_map (roadmap_db *root) {

   RoadMapZipContext *context;


   context = malloc (sizeof(RoadMapZipContext));
   roadmap_check_allocated(context);

   context->type = RoadMapZipType;

   context->ZipCode = (int *) roadmap_db_get_data (root);
   context->ZipCodeCount = roadmap_db_get_count(root);

   if (roadmap_db_get_size (root) != context->ZipCodeCount * sizeof(int)) {
      roadmap_log (ROADMAP_FATAL, "invalid zip structure");
   }

   rdmxchange_zip_register_export ();

   return context;
}

static void rdmxchange_zip_activate (void *context) {

   RoadMapZipContext *this = (RoadMapZipContext *) context;

   if ((this != NULL) && (this->type != RoadMapZipType)) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (bad context type)");
   }
   RoadMapZipActive = this;
}

static void rdmxchange_zip_unmap (void *context) {

   RoadMapZipContext *this = (RoadMapZipContext *) context;

   if (this->type != RoadMapZipType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapZipActive == this) {
      RoadMapZipActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapZipExport = {
   "zip",
   rdmxchange_zip_map,
   rdmxchange_zip_activate,
   rdmxchange_zip_unmap
};


static void rdmxchange_zip_export_head (FILE *file) {

   fprintf (file, "zip,%d\n", RoadMapZipActive->ZipCodeCount);
}


static void rdmxchange_zip_export_data (FILE *file) {

   int i;
   int *zipcode;

   fprintf (file, "zip\n"
                  "code\n");
   zipcode = RoadMapZipActive->ZipCode;

   for (i = 0; i < RoadMapZipActive->ZipCodeCount; ++i, ++zipcode) {

      fprintf (file, "%d\n", *zipcode);
   }
   fprintf (file, "\n");
}


static RdmXchangeExport RdmXchangeZipExport = {
   "zip",
   rdmxchange_zip_export_head,
   rdmxchange_zip_export_data
};

static void rdmxchange_zip_register_export (void) {

   rdmxchange_main_register_export (&RdmXchangeZipExport);
}


/* The import side. ----------------------------------------------------- */

static int *ZipCode = NULL;
static int  ZipCodeCount = 0;
static int  ZipCodeCursor = 0;


static void rdmxchange_zip_save (void) {

   int i;
   int *db_zip;
   buildmap_db *root  = buildmap_db_add_section (NULL, "zip");

   /* Create the tables. */

   if (root == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (root, ZipCodeCount, sizeof(int));

   db_zip = (int *) buildmap_db_get_data (root);

   /* Fill the data in. */

   for (i = 0; i < ZipCodeCount; ++i) {
      db_zip[i] = ZipCode[i];
   }

   /* Do not save this data ever again. */
   free (ZipCode);
   ZipCode = NULL;
   ZipCodeCount = 0;
}


static buildmap_db_module RdmXchangeZipModule = {
   "zip",
   NULL,
   rdmxchange_zip_save,
   NULL,
   NULL
};


static void rdmxchange_zip_import_table (const char *name, int count) {

   buildmap_db_register (&RdmXchangeZipModule);

   if (strcmp (name, "zip") == 0) {

      if (ZipCode != NULL) free (ZipCode);

      ZipCode = calloc (count, sizeof(int));
      ZipCodeCount = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }
}


static int rdmxchange_zip_import_schema (const char *table,
                                         char *fields[], int count) {

   if (strcmp (table, "zip") == 0) {

      if (ZipCode == NULL || count != 1 || strcmp(fields[0], "code") != 0) {
         buildmap_fatal (1, "invalid schema for table zip");
      }
      ZipCodeCursor = 0;

      return 1;
   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}

static void rdmxchange_zip_import_data (int table,
                                        char *fields[], int count) {

   switch (table) {

      case 1:

         if (count != 1) {
            buildmap_fatal (count, "invalid zip record");
         }
         ZipCode[ZipCodeCursor++] = rdmxchange_import_int (fields[0]);
         break;

      default:

          buildmap_fatal (1, "invalid table");
   }
}


RdmXchangeImport RdmXchangeZipImport = {
   "zip",
   rdmxchange_zip_import_table,
   rdmxchange_zip_import_schema,
   rdmxchange_zip_import_data
};

