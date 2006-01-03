/* roadmap_metadata.c - Manage the map metadata information.
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
 *
 * SYNOPSYS:
 *
 *   const char *roadmap_metadata_get_attribute (const char *category,
 *                                               const char *name);
 *
 *   const char *roadmap_metadata_get_attribute_next (const char *category,
 *                                                    const char *name,
 *                                                    int index);
 *
 *   void roadmap_metadata_scan_attributes (RoadMapMetadataIterator iterator);
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dictionary.h"
#include "roadmap_metadata.h"

#include "roadmap_db_metadata.h"


static const char *RoadMapMetadataType = "RoadMapMetadataContext";

typedef struct {

   const char *type;

   RoadMapAttribute *Attributes;
   int               AttributesCount;

   RoadMapString    *Values;
   int               ValuesCount;

   RoadMapDictionary strings;

} RoadMapMetadataContext;

static RoadMapMetadataContext *RoadMapMetadataActive = NULL;


static void *roadmap_metadata_map (roadmap_db *root) {

   RoadMapMetadataContext *context;

   roadmap_db *attributes_table;
   roadmap_db *values_table;


   context = (RoadMapMetadataContext *) malloc (sizeof(RoadMapMetadataContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }
   context->type = RoadMapMetadataType;
   context->strings = NULL;

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
      goto roadmap_metadata_map_abort;
   }

   if (roadmap_db_get_size (values_table) !=
       context->ValuesCount * sizeof(RoadMapString)) {
      roadmap_log (ROADMAP_ERROR, "invalid metadata/values structure");
      goto roadmap_metadata_map_abort;
   }

   return context;

roadmap_metadata_map_abort:

   free(context);
   return NULL;
}

static void roadmap_metadata_activate (void *context) {

   RoadMapMetadataContext *this = (RoadMapMetadataContext *) context;

   if (this != NULL) {

      if (this->type != RoadMapMetadataType) {
         roadmap_log (ROADMAP_FATAL, "invalid metadata context activated");
      }

      if (this->strings == NULL) {
         this->strings = roadmap_dictionary_open ("attributes");

         if (this->strings == NULL) {
            roadmap_log (ROADMAP_FATAL, "cannot open dictionary");
         }
      }
   }

   RoadMapMetadataActive = this;
}

static void roadmap_metadata_unmap (void *context) {

   RoadMapMetadataContext *this = (RoadMapMetadataContext *) context;

   if (this->type != RoadMapMetadataType) {
      roadmap_log (ROADMAP_FATAL, "unmapping invalid line context");
   }
   if (RoadMapMetadataActive == this) {
      RoadMapMetadataActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapMetadataHandler = {
   "metadata",
   roadmap_metadata_map,
   roadmap_metadata_activate,
   roadmap_metadata_unmap
};


const char *roadmap_metadata_get_attribute (const char *category,
                                            const char *name) {

   return roadmap_metadata_get_attribute_next (category, name, 0);
}


const char *roadmap_metadata_get_attribute_next (const char *category,
                                                 const char *name,
                                                 int index) {

   int i;
   RoadMapString coded_category;
   RoadMapString coded_name;


   if (RoadMapMetadataActive == NULL) return "";

   coded_category =
      roadmap_dictionary_locate (RoadMapMetadataActive->strings, category);

   coded_name =
      roadmap_dictionary_locate (RoadMapMetadataActive->strings, name);

   for (i = RoadMapMetadataActive->AttributesCount - 1; i >= 0; --i) {

      if (RoadMapMetadataActive->Attributes[i].name == coded_name &&
          RoadMapMetadataActive->Attributes[i].category == coded_category) {

         short cursor =
            RoadMapMetadataActive->Attributes[i].value_first + index;

         if (index >=  RoadMapMetadataActive->Attributes[i].value_count) {
            return NULL;
         }
         return roadmap_dictionary_get
                   (RoadMapMetadataActive->strings,
                    RoadMapMetadataActive->Values[cursor]);
      }
   }

   return "";
}


void roadmap_metadata_scan_attributes (RoadMapMetadataIterator iterator) {

   int i;
   int j;
   int value_end;
   const char *category;
   const char *name;
   RoadMapAttribute *this_attribute;

   if (RoadMapMetadataActive == NULL) return;

   for (i = RoadMapMetadataActive->AttributesCount - 1; i >= 0; --i) {

      this_attribute = RoadMapMetadataActive->Attributes + i;

      category =
         roadmap_dictionary_get (RoadMapMetadataActive->strings,
                                 this_attribute->category);

      name =
         roadmap_dictionary_get (RoadMapMetadataActive->strings,
                                 this_attribute->name);

      value_end = this_attribute->value_first + this_attribute->value_count;

      for (j = this_attribute->value_first; j < value_end; ++j) {
         iterator (category,
                   name,
                   roadmap_dictionary_get (RoadMapMetadataActive->strings,
                                           RoadMapMetadataActive->Values[j]));
      }
   }
}

