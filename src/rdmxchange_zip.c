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
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_dbread.h"

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

   fprintf (file, "table zip %d\n", RoadMapZipActive->ZipCodeCount);
}


static void rdmxchange_zip_export_data (FILE *file) {

   int i;
   int *zipcode;

   fprintf (file, "table zip\n"
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

