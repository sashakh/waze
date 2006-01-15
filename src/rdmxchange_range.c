/* rdmxchange_range.c - Export street address range tables.
 *
 * LICENSE:
 *
 *   Copyright 2005 Pascal F. Martin
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

#include "roadmap_db_range.h"

#include "roadmap.h"
#include "roadmap_dbread.h"

#include "rdmxchange.h"


static char *RoadMapRangeType  = "RoadMapRangeContext";

typedef struct {

   char *type;

   RoadMapRangeByStreet *ByStreet;
   int                   ByStreetCount;

   RoadMapRangeByCity *ByCity;
   int                 ByCityCount;

   RoadMapRangePlace *Place;
   int                PlaceCount;

   RoadMapRangeByZip *ByZip;
   int                ByZipCount;

   RoadMapRangeBySquare *BySquare;
   int                   BySquareCount;

   RoadMapRange *Address;
   int           AddressCount;

   RoadMapRangeNoAddress *NoAddress;
   int                    NoAddressCount;

} RoadMapRangeContext;


static RoadMapRangeContext  *RoadMapRangeActive  = NULL;


static void rdmxchange_range_register_export (void);


static void *rdmxchange_range_map (roadmap_db *root) {

   RoadMapRangeContext *context;
   roadmap_db *table;


   context = malloc (sizeof(RoadMapRangeContext));
   roadmap_check_allocated(context);

   context->type = RoadMapRangeType;

   table = roadmap_db_get_subsection (root, "bystreet");
   context->ByStreet =
       (RoadMapRangeByStreet *) roadmap_db_get_data (table);
   context->ByStreetCount = roadmap_db_get_count (table);

   if (roadmap_db_get_size (table) !=
       context->ByStreetCount * sizeof(RoadMapRangeByStreet)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/bystreet structure");
   }

   table = roadmap_db_get_subsection (root, "bycity");
   context->ByCity = (RoadMapRangeByCity *) roadmap_db_get_data (table);
   context->ByCityCount = roadmap_db_get_count (table);

   if (roadmap_db_get_size (table) !=
       context->ByCityCount * sizeof(RoadMapRangeByCity)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/bycity structure");
   }

   table = roadmap_db_get_subsection (root, "place");
   context->Place = (RoadMapRangePlace *) roadmap_db_get_data (table);
   context->PlaceCount = roadmap_db_get_count (table);

   if (roadmap_db_get_size (table) !=
       context->PlaceCount * sizeof(RoadMapRangePlace)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/place structure");
   }

   table = roadmap_db_get_subsection (root, "byzip");
   context->ByZip = (RoadMapRangeByZip *) roadmap_db_get_data (table);
   context->ByZipCount = roadmap_db_get_count (table);

   if (roadmap_db_get_size (table) !=
       context->ByZipCount * sizeof(RoadMapRangeByZip)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/byzip structure");
   }

   table = roadmap_db_get_subsection (root, "bysquare");
   context->BySquare = (RoadMapRangeBySquare *) roadmap_db_get_data (table);
   context->BySquareCount = roadmap_db_get_count (table);

   if (roadmap_db_get_size (table) !=
       context->BySquareCount * sizeof(RoadMapRangeBySquare)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/bysquare structure");
   }

   table = roadmap_db_get_subsection (root, "addr");
   context->Address = (RoadMapRange *) roadmap_db_get_data (table);
   context->AddressCount = roadmap_db_get_count (table);

   if (roadmap_db_get_size (table) !=
       context->AddressCount * sizeof(RoadMapRange)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/addr structure");
   }

   table = roadmap_db_get_subsection (root, "noaddr");
   context->NoAddress = (RoadMapRangeNoAddress *) roadmap_db_get_data (table);
   context->NoAddressCount = roadmap_db_get_count (table);

   if (roadmap_db_get_size (table) !=
       context->NoAddressCount * sizeof(RoadMapRangeNoAddress)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/noaddr structure");
   }

   rdmxchange_range_register_export();

   return context;
}

static void rdmxchange_range_activate (void *context) {

   RoadMapRangeContext *this = (RoadMapRangeContext *) context;

   if (this != NULL) {
      
      if (this->type != RoadMapRangeType) {
         roadmap_log (ROADMAP_FATAL, "cannot activate (bad context type)");
      }
   }
   RoadMapRangeActive = this;
}

static void rdmxchange_range_unmap (void *context) {

   RoadMapRangeContext *this = (RoadMapRangeContext *) context;

   if (this->type != RoadMapRangeType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapRangeActive == this) {
      RoadMapRangeActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapRangeExport = {
   "range",
   rdmxchange_range_map,
   rdmxchange_range_activate,
   rdmxchange_range_unmap
};


static void rdmxchange_range_export_head (FILE *file) {

   fprintf (file, "table range/bystreet %d\n",
                  RoadMapRangeActive->ByStreetCount);
   fprintf (file, "table range/bycity %d\n",
                  RoadMapRangeActive->ByCityCount);
   fprintf (file, "table range/place %d\n",
                  RoadMapRangeActive->PlaceCount);
   fprintf (file, "table range/byzip %d\n",
                  RoadMapRangeActive->ByZipCount);
   fprintf (file, "table range/bysquare %d\n",
                  RoadMapRangeActive->BySquareCount);
   fprintf (file, "table range/addr %d\n",
                  RoadMapRangeActive->AddressCount);
   fprintf (file, "table range/noaddr %d\n",
                  RoadMapRangeActive->NoAddressCount);
}


static void rdmxchange_range_export_data (FILE *file) {

   int i;
   int j;
   RoadMapRangeByStreet  *bystreet;
   RoadMapRangeByCity    *bycity;
   RoadMapRangePlace     *place;
   RoadMapRangeByZip     *byzip;
   RoadMapRangeBySquare  *bysquare;
   RoadMapRange          *address;
   RoadMapRangeNoAddress *noaddress;


   fprintf (file, "table range/bystreet\n"
                  "first_range,"
                  "first_city,"
                  "first_zip,"
                  "count\n");
   bystreet = RoadMapRangeActive->ByStreet;

   for (i = 0; i < RoadMapRangeActive->ByStreetCount; ++i, ++bystreet) {

      fprintf (file, "%.0d,%.0d,%.0d,%.0d\n", bystreet->first_range,
                                              bystreet->first_city,
                                              bystreet->first_zip,
                                              bystreet->count_range);
   }
   fprintf (file, "\n");


   fprintf (file, "table range/bycity\n"
                  "city,"
                  "count\n");
   bycity = RoadMapRangeActive->ByCity;

   for (i = 0; i < RoadMapRangeActive->ByCityCount; ++i, ++bycity) {

      fprintf (file, "%.0d,%.0d\n", bycity->city, bycity->count);
   }
   fprintf (file, "\n");


   fprintf (file, "table range/place\n"
                  "place,"
                  "city\n");
   place = RoadMapRangeActive->Place;

   for (i = 0; i < RoadMapRangeActive->PlaceCount; ++i, ++place) {

      fprintf (file, "%.0d,%.0d\n", place->place, place->city);
   }
   fprintf (file, "\n");


   fprintf (file, "table range/byzip\n"
                  "zip,"
                  "count\n");
   byzip = RoadMapRangeActive->ByZip;

   for (i = 0; i < RoadMapRangeActive->ByZipCount; ++i, ++byzip) {

      fprintf (file, "%.0d,%.0d\n", byzip->zip, byzip->count);
   }
   fprintf (file, "\n");


   fprintf (file, "table range/bysquare\n"
                  "excluded[%d],"
                  "included[%d],"
                  "start,"
                  "count\n", ROADMAP_RANGE_HOLES, ROADMAP_RANGE_HOLES);
   bysquare = RoadMapRangeActive->BySquare;

   for (i = 0; i < RoadMapRangeActive->BySquareCount; ++i, ++bysquare) {

      for (j = 0; j < ROADMAP_RANGE_HOLES; ++j) {
         fprintf (file, "%.0d,", bysquare->hole[j].excluded);
      }
      for (j = 0; j < ROADMAP_RANGE_HOLES; ++j) {
         fprintf (file, "%.0d,", bysquare->hole[j].included);
      }
      fprintf (file, "%.0d,%.0d\n", bysquare->noaddr_start,
                                    bysquare->noaddr_count);
   }
   fprintf (file, "\n");


   fprintf (file, "table range/addr\n"
                  "line,"
                  "fradd,"
                  "toadd\n");
   address = RoadMapRangeActive->Address;

   for (i = 0; i < RoadMapRangeActive->AddressCount; ++i, ++address) {

      fprintf (file, "%.0d,%.0d,%.0d\n", address->line,
                                         address->fradd,
                                         address->toadd);
   }
   fprintf (file, "\n");


   fprintf (file, "table range/noaddr\n"
                  "line,"
                  "street\n");
   noaddress = RoadMapRangeActive->NoAddress;

   for (i = 0; i < RoadMapRangeActive->NoAddressCount; ++i, ++noaddress) {

      fprintf (file, "%.0d,%.0d\n", noaddress->line, noaddress->street);
   }
   fprintf (file, "\n");
}


static RdmXchangeExport RdmXchangeRangeExport = {
   "street",
   rdmxchange_range_export_head,
   rdmxchange_range_export_data
};

static void rdmxchange_range_register_export (void) {

   rdmxchange_main_register_export (&RdmXchangeRangeExport);
}

