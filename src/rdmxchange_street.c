/* rdmxchange_street.c - Export streets attributes.
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

#include "roadmap_db_street.h"

#include "roadmap.h"
#include "roadmap_dbread.h"

#include "rdmxchange.h"


static char *RoadMapStreetType = "RoadMapStreetContext";

typedef struct {

   char *type;

   RoadMapStreet        *Streets;
   char                 *StreetType;
   int                   StreetsCount;

} RoadMapStreetContext;


static RoadMapStreetContext *RoadMapStreetActive = NULL;


static void rdmxchange_street_register_export (void);


static void *rdnxchange_street_map (roadmap_db *root) {

   RoadMapStreetContext *context;

   roadmap_db *table;


   context = malloc (sizeof(RoadMapStreetContext));
   roadmap_check_allocated(context);

   context->type = RoadMapStreetType;

   table = roadmap_db_get_subsection (root, "name");
   context->Streets = (RoadMapStreet *) roadmap_db_get_data (table);
   context->StreetsCount = roadmap_db_get_count(table);

   if (roadmap_db_get_size (table) !=
       context->StreetsCount * sizeof(RoadMapStreet)) {
      roadmap_log (ROADMAP_FATAL,
                   "invalid street structure (%d != %d / %d)",
                   context->StreetsCount,
                   roadmap_db_get_size (table),
                   sizeof(RoadMapStreet));
   }

   table = roadmap_db_get_subsection (root, "type");
   context->StreetType = (char *) roadmap_db_get_data (table);

   if (roadmap_db_get_count(table) != context->StreetsCount) {
      roadmap_log (ROADMAP_FATAL, "inconsistent count of street");
   }

   rdmxchange_street_register_export();

   return context;
}

static void rdnxchange_street_activate (void *context) {

   RoadMapStreetContext *this = (RoadMapStreetContext *) context;

   if ((this != NULL) && (this->type != RoadMapStreetType)) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (bad context type)");
   }
   RoadMapStreetActive = this;
}

static void rdnxchange_street_unmap (void *context) {

   RoadMapStreetContext *this = (RoadMapStreetContext *) context;

   if (this->type != RoadMapStreetType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapStreetActive == this) {
      RoadMapStreetActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapStreetExport = {
   "street",
   rdnxchange_street_map,
   rdnxchange_street_activate,
   rdnxchange_street_unmap
};


static void rdmxchange_street_export_head (FILE *file) {

   fprintf (file, "street/name,%d\n", RoadMapStreetActive->StreetsCount);
   fprintf (file, "street/type,%d\n", RoadMapStreetActive->StreetsCount);
}


static void rdmxchange_street_export_data (FILE *file) {

   int i;
   char *streetstype;
   RoadMapStreet *street;


   fprintf (file, "street/name\n"
                  "fedirp,"
                  "fename,"
                  "fetype,"
                  "fedirs\n");
   street = RoadMapStreetActive->Streets;

   for (i = 0; i < RoadMapStreetActive->StreetsCount; ++i, ++street) {

      fprintf (file, "%.0d,%.0d,%.0d,%.0d\n", street->fedirp,
                                              street->fename,
                                              street->fetype,
                                              street->fedirs);
   }
   fprintf (file, "\n");


   fprintf (file, "street/type\n"
                  "type\n");
   streetstype = RoadMapStreetActive->StreetType;

    for (i = 0; i < RoadMapStreetActive->StreetsCount; ++i, ++streetstype) {

       fprintf (file, "%d\n", *streetstype);
    }
    fprintf (file, "\n");
}


static RdmXchangeExport RdmXchangeStreetExport = {
   "street",
   rdmxchange_street_export_head,
   rdmxchange_street_export_data
};

static void rdmxchange_street_register_export (void) {

   rdmxchange_main_register_export (&RdmXchangeStreetExport);
}


/* The import side. ----------------------------------------------------- */

static char *StreetLayer = NULL;
static RoadMapStreet *StreetName = NULL;
static int  StreetLayerCount = 0;
static int  StreetNameCount = 0;
static int  StreetCursor = 0;


static void rdmxchange_street_save (void) {

   int i;
   RoadMapStreet *db_name;
   char          *db_layer;
   buildmap_db   *root;
   buildmap_db   *table_name;
   buildmap_db   *table_layer;


   root  = buildmap_db_add_section (NULL, "street");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_name = buildmap_db_add_section (root, "name");
   if (table_name == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_name, StreetNameCount, sizeof(RoadMapStreet));

   table_layer = buildmap_db_add_section (root, "type");
   if (table_layer == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_layer, StreetLayerCount, sizeof(char));

   db_name  = (RoadMapStreet *) buildmap_db_get_data (table_name);
   db_layer = (char *) buildmap_db_get_data (table_layer);

   for (i = StreetNameCount-1; i >= 0; --i) {
      db_name[i] = StreetName[i];
   }

   for (i = StreetLayerCount-1; i >= 0; --i) {
      db_layer[i] = StreetLayer[i];
   }

   /* Do not save this data ever again. */
   free (StreetName);
   StreetName = NULL;
   StreetNameCount = 0;

   free (StreetLayer);
   StreetLayer = NULL;
   StreetLayerCount = 0;
}


static buildmap_db_module RdmXchangeStreetModule = {
   "street",
   NULL,
   rdmxchange_street_save,
   NULL,
   NULL
};


static void rdmxchange_street_import_table (const char *name, int count) {

   if (strcmp (name, "street/name") == 0) {

      if (StreetName != NULL) free (StreetName);

      StreetName = calloc (count, sizeof(RoadMapStreet));
      StreetNameCount = count;

   } else if (strcmp (name, "street/type") == 0) {

      if (StreetLayer != NULL) free (StreetLayer);

      StreetLayer = calloc (count, sizeof(char));
      StreetLayerCount = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }

   buildmap_db_register (&RdmXchangeStreetModule);
}


static int rdmxchange_street_import_schema (const char *table,
                                                  char *fields[], int count) {

   StreetCursor = 0;

   if (strcmp (table, "street/name") == 0) {

      if ((StreetName == NULL) ||
          (count != 4) ||
          (strcmp (fields[0], "fedirp") != 0) ||
          (strcmp (fields[1], "fename") != 0) ||
          (strcmp (fields[2], "fetype") != 0) ||
          (strcmp (fields[3], "fedirs") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s\n", table);
      }
      return 1;

   } else if (strcmp (table, "street/type") == 0) {

      if ((StreetLayer == NULL) ||
            (count != 1) || (strcmp (fields[0], "type") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s\n", table);
      }
      return 2;
   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}


static void rdmxchange_street_import_data (int table,
                                              char *fields[], int count) {

   RoadMapStreet *street;

   switch (table) {
      case 1:

         if (count != 4) {
            buildmap_fatal (count, "invalid street/name record");
         }
         street = &(StreetName[StreetCursor++]);

         street->fedirp = (RoadMapString) rdmxchange_import_int (fields[0]);
         street->fename = (RoadMapString) rdmxchange_import_int (fields[1]);
         street->fetype = (RoadMapString) rdmxchange_import_int (fields[2]);
         street->fedirs = (RoadMapString) rdmxchange_import_int (fields[3]);

         break;

      case 2:

         if (count != 1) {
            buildmap_fatal (count, "invalid street/type record");
         }
         StreetLayer[StreetCursor++] = (char)rdmxchange_import_int (fields[0]);
         break;
   }
}


RdmXchangeImport RdmXchangeStreetImport = {
   "street",
   rdmxchange_street_import_table,
   rdmxchange_street_import_schema,
   rdmxchange_street_import_data
};

