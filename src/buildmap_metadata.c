/* buildmap_metadata.c - Build a table containing the map's metadata.
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
 *   void buildmap_metadata_initialize (void);
 *   void buildmap_metadata_add_attribute (const char *category,
 *                                         const char *name,
 *                                         const char *value);
 *
 *   void buildmap_metadata_sort    (void);
 *   void buildmap_metadata_save    (void);
 *   void buildmap_metadata_summary (void);
 *   void buildmap_metadata_reset   (void);
 *
 * These functions are used to build a table of attributes that describe
 * what this map file contains.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_db_metadata.h"

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_metadata.h"



static int AttributeCount = 0;
static RoadMapAttribute *Attribute[BUILDMAP_BLOCK] = {NULL};

static RoadMapHash *AttributeByName = NULL;

BuildMapDictionary AttributeDictionary = NULL;


void buildmap_metadata_initialize (void) {

   AttributeByName =
      roadmap_hash_new ("AttributeByName", BUILDMAP_BLOCK);

   Attribute[0] = calloc (BUILDMAP_BLOCK, sizeof(RoadMapAttribute));
   if (Attribute[0] == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   AttributeDictionary = buildmap_dictionary_open ("attributes");
}


void buildmap_metadata_add_attribute (const char *category,
                                      const char *name,
                                      const char *value) {

   int i;
   int block;
   int offset;
   RoadMapAttribute *this_attribute;


   /* First check if the attribute is already known. */

   RoadMapString coded_category =
      buildmap_dictionary_add (AttributeDictionary, category, strlen(category));

   RoadMapString coded_name =
      buildmap_dictionary_add (AttributeDictionary, name, strlen(name));

   RoadMapString coded_value =
      buildmap_dictionary_add (AttributeDictionary, value, strlen(value));

   for (i = roadmap_hash_get_first (AttributeByName, coded_name);
        i >= 0;
        i = roadmap_hash_get_next (AttributeByName, i)) {

      this_attribute = Attribute[i / BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);

      if ((this_attribute->name == coded_name) &&
          (this_attribute->category == coded_category)) {
          
         if (this_attribute->value != coded_value) {
            roadmap_log (ROADMAP_FATAL,
                         "attribute %s.%s changed to %s",
                         category, name, value);
         }
         return;
      }
   }


   /* This is a new attribute: create a new entry. */

   block = AttributeCount / BUILDMAP_BLOCK;
   offset = AttributeCount % BUILDMAP_BLOCK;

   if (Attribute[block] == NULL) {

      /* We need to add a new block to the table. */

      Attribute[block] = calloc (BUILDMAP_BLOCK, sizeof(RoadMapAttribute));
      if (Attribute[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }

      roadmap_hash_resize (AttributeByName, (block+1) * BUILDMAP_BLOCK);
   }

   roadmap_hash_add (AttributeByName, coded_name, AttributeCount);

   this_attribute = Attribute[block] + offset;

   this_attribute->category = coded_category;
   this_attribute->name = coded_name;
   this_attribute->value = coded_value;

   AttributeCount += 1;
}


void buildmap_metadata_sort (void) {}


void buildmap_metadata_save (void) {

   int i;

   RoadMapAttribute *one_attribute;
   RoadMapAttribute *db_attributes;
   
   buildmap_db *root;
   buildmap_db *table_attributes;


   buildmap_info ("saving %d attributes...", AttributeCount);

   root = buildmap_db_add_section (NULL, "metadata");

   table_attributes = buildmap_db_add_section (root, "attributes");
   buildmap_db_add_data
      (table_attributes, AttributeCount, sizeof(RoadMapAttribute));

   db_attributes = (RoadMapAttribute *) buildmap_db_get_data (table_attributes);


   for (i = 0; i < AttributeCount; ++i) {

      one_attribute = Attribute[i/BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);

      db_attributes[i] = *one_attribute;
      db_attributes[i].filler = 0;
   }
}


void buildmap_metadata_summary (void) {

   fprintf (stderr,
            "-- metadata table statistics: %d attributes, %d bytes used\n",
            AttributeCount, AttributeCount * sizeof(RoadMapAttribute));
}


void buildmap_metadata_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; i++) {
      if (Attribute[i] != NULL) {
         free (Attribute[i]);
         Attribute[i] = NULL;
      }
   }

   AttributeCount = 0;

   AttributeByName = NULL;
}

