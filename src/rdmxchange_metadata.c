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

   fprintf (file, "table metadata/attributes %d\n",
                  RoadMapMetadataActive->AttributesCount);

   fprintf (file, "table metadata/values %d\n",
                  RoadMapMetadataActive->ValuesCount);
}


static void rdmxchange_metadata_export_data (FILE *file) {

   int i;
   RoadMapAttribute *attributes;
   RoadMapString *values;


   if (RoadMapMetadataActive == NULL) return; /* No such table in that file. */

   fprintf (file, "table metadata/attributes\n"
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


   fprintf (file, "table metadata/values\n"
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
