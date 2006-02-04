/* rdmxchange_metadata.c - Export the map metadata information.
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"

#include "rdmxchange.h"

#include "roadmap_db_metadata.h"


static const char *RoadMapMetadataType = "RoadMapMetadataContext";

typedef struct {

   const char *type;

   RoadMapAttribute *Attributes;
   int               AttributesCount;

   RoadMapString    *Values;
   int               ValuesCount;

} RoadMapMetadataContext;

static RoadMapMetadataContext *RoadMapMetadataActive = NULL;

static void rdmxchange_metadata_register_export (void);


static void *rdmxchange_metadata_map (roadmap_db *root) {

   RoadMapMetadataContext *context;

   roadmap_db *attributes_table;
   roadmap_db *values_table;


   context = (RoadMapMetadataContext *) malloc (sizeof(RoadMapMetadataContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }
   context->type = RoadMapMetadataType;

   attributes_table = roadmap_db_get_subsection (root, "attributes");
   values_table = roadmap_db_get_subsection (root, "values");

   context->Attributes =
      (RoadMapAttribute *) roadmap_db_get_data (attributes_table);
   context->AttributesCount = roadmap_db_get_count (attributes_table);

   context->Values = (RoadMapString *) roadmap_db_get_data (values_table);
   context->ValuesCount = roadmap_db_get_count (values_table);

   if (roadmap_db_get_size (attributes_table) !=
       context->AttributesCount * sizeof(RoadMapAttribute)) {
      roadmap_log (ROADMAP_ERROR, "invalid metadata/attributes structure");
      goto rdmxchange_metadata_map_abort;
   }

   if (roadmap_db_get_size (values_table) !=
       context->ValuesCount * sizeof(RoadMapString)) {
      roadmap_log (ROADMAP_ERROR, "invalid metadata/values structure");
      goto rdmxchange_metadata_map_abort;
   }

   rdmxchange_metadata_register_export();

   return context;

rdmxchange_metadata_map_abort:

   free(context);
   return NULL;
}

static void rdmxchange_metadata_activate (void *context) {

   RoadMapMetadataContext *this = (RoadMapMetadataContext *) context;

   if (this != NULL) {

      if (this->type != RoadMapMetadataType) {
         roadmap_log (ROADMAP_FATAL, "invalid metadata context activated");
      }
   }

   RoadMapMetadataActive = this;
}

static void rdmxchange_metadata_unmap (void *context) {

   RoadMapMetadataContext *this = (RoadMapMetadataContext *) context;

   if (this->type != RoadMapMetadataType) {
      roadmap_log (ROADMAP_FATAL, "unmapping invalid line context");
   }
   if (RoadMapMetadataActive == this) {
      RoadMapMetadataActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapMetadataExport = {
   "metadata",
   rdmxchange_metadata_map,
   rdmxchange_metadata_activate,
   rdmxchange_metadata_unmap
};


static void rdmxchange_metadata_export_head (FILE *file) {

   if (RoadMapMetadataActive == NULL) return; /* No such table in that file. */

   fprintf (file, "metadata/attributes,%d\n",
                  RoadMapMetadataActive->AttributesCount);

   fprintf (file, "metadata/values,%d\n",
                  RoadMapMetadataActive->ValuesCount);
}


static void rdmxchange_metadata_export_data (FILE *file) {

   int i;
   RoadMapAttribute *attributes;
   RoadMapString *values;


   if (RoadMapMetadataActive == NULL) return; /* No such table in that file. */

   fprintf (file, "metadata/attributes\n"
                  "category,"
                  "name,"
                  "value.first,"
                  "value.count\n");
   attributes = RoadMapMetadataActive->Attributes;

   for (i = 0; i < RoadMapMetadataActive->AttributesCount; ++i) {
      fprintf (file, "%.0d,%.0d,%.0d,%.0d\n", attributes[i].category,
                                              attributes[i].name,
                                              attributes[i].value_first,
                                              attributes[i].value_count);
   }
   fprintf (file, "\n");


   fprintf (file, "metadata/values\n"
                  "index\n");
   values = RoadMapMetadataActive->Values;

   for (i = 0; i < RoadMapMetadataActive->ValuesCount; ++i) {
      fprintf (file, "%d\n", values[i]);
   }
   fprintf (file, "\n");
}


static RdmXchangeExport RdmXchangeMetadataExport = {
   "metadata",
   rdmxchange_metadata_export_head,
   rdmxchange_metadata_export_data
};

static void rdmxchange_metadata_register_export (void) {

   rdmxchange_main_register_export (&RdmXchangeMetadataExport);
}


/* The import side. ----------------------------------------------------- */

static RoadMapAttribute *MetadataAttribute = NULL;
static RoadMapString    *MetadataValue = NULL;
static int  MetadataAttributeCount = 0;
static int  MetadataValueCount = 0;
static int  MetadataCursor = 0;


static void rdmxchange_metadata_save (void) {

   int i;

   RoadMapAttribute  *db_attributes;
   RoadMapString     *db_values;

   buildmap_db *root;
   buildmap_db *table_attributes;
   buildmap_db *table_values;


   /* Create the tables. */

   root = buildmap_db_add_section (NULL, "metadata");
   table_attributes = buildmap_db_add_section (root, "attributes");
   buildmap_db_add_data
      (table_attributes, MetadataAttributeCount, sizeof(RoadMapAttribute));

   table_values = buildmap_db_add_section (root, "values");
   buildmap_db_add_data
      (table_values, MetadataValueCount, sizeof(RoadMapString));

   db_attributes = (RoadMapAttribute *) buildmap_db_get_data (table_attributes);
   db_values     = (RoadMapString *) buildmap_db_get_data (table_values);

   /* Fill the data in. */

   for (i = 0; i < MetadataAttributeCount; ++i) {
      db_attributes[i] = MetadataAttribute[i];
   }

   for (i = 0; i < MetadataValueCount; ++i) {
      db_values[i] = MetadataValue[i];
   }

   /* Do not save this data ever again. */
   free (MetadataAttribute);
   MetadataAttribute = NULL;
   MetadataAttributeCount = 0;

   free (MetadataValue);
   MetadataValue = NULL;
   MetadataValueCount = 0;
}


static buildmap_db_module RdmXchangeMetadataModule = {
   "metadata",
   NULL,
   rdmxchange_metadata_save,
   NULL,
   NULL
};


static void rdmxchange_metadata_import_table (const char *name, int count) {

   buildmap_db_register (&RdmXchangeMetadataModule);

   if (strcmp (name, "metadata/attributes") == 0) {

      if (MetadataAttribute != NULL) free (MetadataAttribute);

      MetadataAttribute = calloc (count, sizeof(RoadMapAttribute));
      MetadataAttributeCount = count;

   } else if (strcmp (name, "metadata/values") == 0) {

      if (MetadataValue != NULL) free (MetadataValue);

      MetadataValue = calloc (count, sizeof(RoadMapString));
      MetadataValueCount = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }
}


static int rdmxchange_metadata_import_schema (const char *table,
                                         char *fields[], int count) {

   MetadataCursor = 0;

   if (strcmp (table, "metadata/attributes") == 0) {

      if (MetadataAttribute == NULL ||
            count != 4 ||
            strcmp(fields[0], "category") != 0 ||
            strcmp(fields[1], "name") != 0 ||
            strcmp(fields[2], "value.first") != 0 ||
            strcmp(fields[3], "value.count") != 0) {
         buildmap_fatal (1, "invalid schema for table metadata/attributes");
      }

      return 1;

   } else if (strcmp (table, "metadata/values") == 0) {

      if (MetadataAttribute == NULL ||
            count != 1 || strcmp(fields[0], "index") != 0) {
         buildmap_fatal (1, "invalid schema for table metadata/values");
      }

      return 2;
   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}

static void rdmxchange_metadata_import_data (int table,
                                        char *fields[], int count) {

   switch (table) {

      case 1:

         if (count != 4) {
            buildmap_fatal (count, "invalid zip record");
         }
         MetadataAttribute[MetadataCursor].category =
            (RoadMapString) rdmxchange_import_int (fields[0]);
         MetadataAttribute[MetadataCursor].name =
            (RoadMapString) rdmxchange_import_int (fields[1]);
         MetadataAttribute[MetadataCursor].value_first =
            (short) rdmxchange_import_int (fields[2]);
         MetadataAttribute[MetadataCursor].value_count =
            (short) rdmxchange_import_int (fields[3]);
         break;

      case 2:

         if (count != 1) {
            buildmap_fatal (count, "invalid zip record");
         }
         MetadataValue[MetadataCursor] =
            (RoadMapString) rdmxchange_import_int (fields[0]);
         break;

      default:

          buildmap_fatal (1, "invalid table");
   }
   MetadataCursor += 1;
}


RdmXchangeImport RdmXchangeMetadataImport = {
   "metadata",
   rdmxchange_metadata_import_table,
   rdmxchange_metadata_import_schema,
   rdmxchange_metadata_import_data
};

