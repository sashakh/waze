/* buildmap_street.c - Build a street table & index for RoadMap.
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
 *   void buildmap_street_initialize (void);
 *   int  buildmap_street_add
 *           (char cfcc,
 *            RoadMapString fedirp,
 *            RoadMapString fename,
 *            RoadMapString fetype,
 *            RoadMapString fedirs,
 *            int line);
 *   void buildmap_street_sort (void);
 *   int  buildmap_street_get_sorted (int street);
 *   int  buildmap_street_count (void);
 *   void buildmap_street_save (void);
 *   void buildmap_street_print_sorted (FILE *file, int street);
 *   void buildmap_street_summary (void);
 *   void buildmap_street_reset   (void);
 *
 * These functions are used to build a table of streets from
 * the Tiger maps. The objective is double: (1) reduce the size of
 * the Tiger data by sharing all duplicated information and
 * (2) produce the index data to serve as the basis for a fast
 * search mechanism for streets in roadmap.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap_db_street.h"

#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildmap_street.h"
#include "buildmap_line.h"


struct buildmap_street_struct {

   RoadMapStreet record;
   char cfcc;

   int sorted;
   int start;
};

typedef struct buildmap_street_struct BuildMapStreet;

static int StreetCount = 0;
static BuildMapStreet *Street[BUILDMAP_BLOCK] = {NULL};

static RoadMapHash *StreetByName = NULL;

static int StreetAddCount = 0;

static int *SortedStreet = NULL;


void buildmap_street_initialize (void) {

   StreetByName = roadmap_hash_new ("StreetByName", BUILDMAP_BLOCK);

   StreetAddCount = 0;
   StreetCount = 0;
}


int  buildmap_street_add
        (char cfcc,
         RoadMapString fedirp,
         RoadMapString fename,
         RoadMapString fetype,
         RoadMapString fedirs,
         int line) {

   int longitude_line;
   int latitude_line;
   int longitude_street;
   int latitude_street;

   int index;
   int block;
   int offset;
   BuildMapStreet *this_street;

   StreetAddCount += 1;


   buildmap_line_get_position (line, &longitude_line, &latitude_line);

   /* First search if that street is not yet known. */

   for (index = roadmap_hash_get_first (StreetByName, fename);
        index >= 0;
        index = roadmap_hash_get_next (StreetByName, index)) {

       this_street = Street[index / BUILDMAP_BLOCK] + (index % BUILDMAP_BLOCK);

       if ((this_street->record.fename == fename) &&
           (this_street->record.fedirp == fedirp) &&
           (this_street->record.fetype == fetype) &&
           (this_street->record.fedirs == fedirs)) {

          buildmap_line_get_position
             (this_street->start, &longitude_street, &latitude_street);

          if (longitude_line < longitude_street) {

             this_street->start = line;

          } else if (longitude_line == longitude_street) {

             if (latitude_line < latitude_street) {
                this_street->start = line;
             }
          }

          return index;
        }
   }

   /* This street was not known yet: create a new one. */

   block = StreetCount / BUILDMAP_BLOCK;
   offset = StreetCount % BUILDMAP_BLOCK;

   if (Street[block] == NULL) {

      /* We need to add a new block to the table. */

      Street[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapStreet));
      if (Street[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
      roadmap_hash_resize (StreetByName, (block+1) * BUILDMAP_BLOCK);
   }

   this_street = Street[block] + offset;

   this_street->record.fename = fename;
   this_street->record.fedirp = fedirp;
   this_street->record.fetype = fetype;
   this_street->record.fedirs = fedirs;

   this_street->cfcc  = cfcc;
   this_street->start = line;

   roadmap_hash_add (StreetByName, fename, StreetCount);

   return StreetCount++;
}


static int buildmap_street_compare (const void *r1, const void *r2) {

   int i1 = *((int *)r1);
   int i2 = *((int *)r2);

   BuildMapStreet *record1;
   BuildMapStreet *record2;


   record1 = Street[i1/BUILDMAP_BLOCK] + (i1 % BUILDMAP_BLOCK);
   record2 = Street[i2/BUILDMAP_BLOCK] + (i2 % BUILDMAP_BLOCK);

   return buildmap_line_get_sorted (record1->start)
             - buildmap_line_get_sorted (record2->start);
}

void buildmap_street_sort (void) {

   int i;
   int j;
   BuildMapStreet *this_street;


   if (SortedStreet != NULL) return; /* Sort was already performed. */

   buildmap_line_sort ();


   buildmap_info ("sorting streets...");

   SortedStreet = malloc (StreetCount * sizeof(int));
   if (SortedStreet == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   for (i = 0; i < StreetCount; i++) {
      SortedStreet[i] = i;
   }

   qsort (SortedStreet, StreetCount, sizeof(int), buildmap_street_compare);

   for (i = 0; i < StreetCount; i++) {
      j = SortedStreet[i];
      this_street = Street[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);
      this_street->sorted = i;
   }
}


void buildmap_street_print_sorted (FILE *file, int street) {

   BuildMapStreet *this_street;

   street = SortedStreet[street];

   this_street = Street[street/BUILDMAP_BLOCK] + (street % BUILDMAP_BLOCK);

   fprintf (file, "%s %s %s (%s)",
            buildmap_dictionary_get
               (buildmap_dictionary_open("prefix"),
                this_street->record.fedirp),
            buildmap_dictionary_get
               (buildmap_dictionary_open("street"),
                this_street->record.fename),
            buildmap_dictionary_get
               (buildmap_dictionary_open("suffix"),
                this_street->record.fedirs),
            buildmap_dictionary_get
               (buildmap_dictionary_open("type"),
                this_street->record.fetype));
}


int buildmap_street_get_sorted (int street) {

   BuildMapStreet *this_street;

   this_street = Street[street/BUILDMAP_BLOCK] + (street % BUILDMAP_BLOCK);

   return this_street->sorted;
}


int  buildmap_street_count (void) {
   return StreetCount;
}


void  buildmap_street_save (void) {

   int i;
   int j;
   BuildMapStreet *one_street;
   RoadMapStreet  *db_streets;
   char  *db_cfcc;

   buildmap_db *root;
   buildmap_db *table_name;
   buildmap_db *table_cfcc;


   buildmap_info ("saving street...");

   root = buildmap_db_add_section (NULL, "street");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_name = buildmap_db_add_child
                  (root, "name", StreetCount, sizeof(RoadMapStreet));

   table_cfcc = buildmap_db_add_child
                  (root, "type", StreetCount, sizeof(char));

   db_streets  = (RoadMapStreet *) buildmap_db_get_data (table_name);
   db_cfcc     = (char *) buildmap_db_get_data (table_cfcc);

   for (i = 0; i < StreetCount; i++) {

      j = SortedStreet[i];

      one_street = Street[j/BUILDMAP_BLOCK] + (j % BUILDMAP_BLOCK);

      db_streets[i] = one_street->record;
      db_cfcc[i] = one_street->cfcc;
   }
}


void buildmap_street_summary (void) {

   fprintf (stderr,
            "-- street table statistics: %d streets, %d add, %d bytes used\n",
            StreetCount, StreetAddCount, StreetCount * sizeof(RoadMapStreet));
}


void buildmap_street_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; i++) {
      if (Street[i] != NULL) {
         free (Street[i]);
         Street[i] = NULL;
      }
   }

   StreetCount = 0;

   StreetByName = NULL;

   StreetAddCount = 0;

   free (SortedStreet);
   SortedStreet = NULL;
}

