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
 *   see roadmap_street.h.
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

   fprintf (file, "table street/name %d\n", RoadMapStreetActive->StreetsCount);
   fprintf (file, "table street/type %d\n", RoadMapStreetActive->StreetsCount);
}


static void rdmxchange_street_export_data (FILE *file) {

   int i;
   char *streetstype;
   RoadMapStreet *street;


   fprintf (file, "table street/name\n"
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


   fprintf (file, "table street/type\n"
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

