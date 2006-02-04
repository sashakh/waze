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

   fprintf (file, "range/bystreet,%d\n",
                  RoadMapRangeActive->ByStreetCount);
   fprintf (file, "range/bycity,%d\n",
                  RoadMapRangeActive->ByCityCount);
   fprintf (file, "range/place,%d\n",
                  RoadMapRangeActive->PlaceCount);
   fprintf (file, "range/byzip,%d\n",
                  RoadMapRangeActive->ByZipCount);
   fprintf (file, "range/bysquare,%d\n",
                  RoadMapRangeActive->BySquareCount);
   fprintf (file, "range/addr,%d\n",
                  RoadMapRangeActive->AddressCount);
   fprintf (file, "range/noaddr,%d\n",
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


   fprintf (file, "range/bystreet\n"
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


   fprintf (file, "range/bycity\n"
                  "city,"
                  "count\n");
   bycity = RoadMapRangeActive->ByCity;

   for (i = 0; i < RoadMapRangeActive->ByCityCount; ++i, ++bycity) {

      fprintf (file, "%.0d,%.0d\n", bycity->city, bycity->count);
   }
   fprintf (file, "\n");


   fprintf (file, "range/place\n"
                  "place,"
                  "city\n");
   place = RoadMapRangeActive->Place;

   for (i = 0; i < RoadMapRangeActive->PlaceCount; ++i, ++place) {

      fprintf (file, "%.0d,%.0d\n", place->place, place->city);
   }
   fprintf (file, "\n");


   fprintf (file, "range/byzip\n"
                  "zip,"
                  "count\n");
   byzip = RoadMapRangeActive->ByZip;

   for (i = 0; i < RoadMapRangeActive->ByZipCount; ++i, ++byzip) {

      fprintf (file, "%.0d,%.0d\n", byzip->zip, byzip->count);
   }
   fprintf (file, "\n");


   fprintf (file, "range/addr\n"
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


   fprintf (file, "range/noaddr\n"
                  "line,"
                  "street\n");
   noaddress = RoadMapRangeActive->NoAddress;

   for (i = 0; i < RoadMapRangeActive->NoAddressCount; ++i, ++noaddress) {

      fprintf (file, "%.0d,%.0d\n", noaddress->line, noaddress->street);
   }
   fprintf (file, "\n");


   fprintf (file, "range/bysquare\n"
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
}


static RdmXchangeExport RdmXchangeRangeExport = {
   "street",
   rdmxchange_range_export_head,
   rdmxchange_range_export_data
};

static void rdmxchange_range_register_export (void) {

   rdmxchange_main_register_export (&RdmXchangeRangeExport);
}

 
/* The import side. ----------------------------------------------------- */
 
static RoadMapRangeByStreet  *RangeByStreet = NULL;
static RoadMapRangeByCity    *RangeByCity = NULL;
static RoadMapRangePlace     *RangePlace = NULL;
static RoadMapRangeByZip     *RangeByZip = NULL;
static RoadMapRange          *RangeAddress = NULL;
static RoadMapRangeNoAddress *RangeNoAddress = NULL;
static RoadMapRangeBySquare  *RangeBySquare = NULL;

static int  RangeByStreetCount = 0;
static int  RangeByCityCount = 0;
static int  RangePlaceCount = 0;
static int  RangeByZipCount = 0;
static int  RangeAddressCount = 0;
static int  RangeNoAddressCount = 0;
static int  RangeBySquareCount = 0;

static int  RangeCursor = 0;
   
 
static void rdmxchange_range_save (void) {
         
   int i;

   RoadMapRangeByZip     *db_zip;
   RoadMapRange          *db_ranges;
   RoadMapRangeNoAddress *db_noaddr;
   RoadMapRangeByStreet  *db_streets;
   RoadMapRangeByCity    *db_city;
   RoadMapRangePlace     *db_place;
   RoadMapRangeBySquare  *db_square;

   buildmap_db *root;
   buildmap_db *table_street;
   buildmap_db *table_addr;
   buildmap_db *table_noaddr;
   buildmap_db *table_zip;
   buildmap_db *table_city;
   buildmap_db *table_place;
   buildmap_db *table_square;


   /* Create all the tables. */

   root  = buildmap_db_add_section (NULL, "range");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_street = buildmap_db_add_section (root, "bystreet");
   if (table_street == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data
      (table_street, RangeByStreetCount, sizeof(RoadMapRangeByStreet));

   table_city = buildmap_db_add_section (root, "bycity");
   if (table_city == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data
      (table_city, RangeByCityCount, sizeof(RoadMapRangeByCity));

   table_place = buildmap_db_add_section (root, "place");
   if (table_place == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data
      (table_place, RangePlaceCount, sizeof(RoadMapRangePlace));

   table_zip = buildmap_db_add_section (root, "byzip");
   if (table_zip == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_zip, RangeByZipCount, sizeof(RoadMapRangeByZip));

   table_addr = buildmap_db_add_section (root, "addr");
   if (table_addr == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_addr, RangeAddressCount, sizeof(RoadMapRange));

   table_noaddr = buildmap_db_add_section (root, "noaddr");
   if (table_noaddr == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data (table_noaddr,
         RangeNoAddressCount, sizeof(RoadMapRangeNoAddress));

   table_square = buildmap_db_add_section (root, "bysquare");
   if (table_square == NULL) buildmap_fatal (0, "Can't add a new section");
   buildmap_db_add_data
      (table_square, RangeBySquareCount, sizeof(RoadMapRangeBySquare));

   db_streets = (RoadMapRangeByStreet *)  buildmap_db_get_data (table_street);
   db_city    = (RoadMapRangeByCity *)    buildmap_db_get_data (table_city);
   db_place   = (RoadMapRangePlace *)     buildmap_db_get_data (table_place);
   db_ranges  = (RoadMapRange *)          buildmap_db_get_data (table_addr);
   db_noaddr  = (RoadMapRangeNoAddress *) buildmap_db_get_data (table_noaddr);
   db_zip     = (RoadMapRangeByZip *)     buildmap_db_get_data (table_zip);
   db_square  = (RoadMapRangeBySquare *)  buildmap_db_get_data (table_square);


   /* Write the data to the map. */

   for (i = 0; i < RangeByStreetCount; ++i, ++db_streets) {
      *db_streets = RangeByStreet[i];
   }

   for (i = 0; i < RangeByCityCount; ++i, ++db_city) {
      *db_city = RangeByCity[i];
   }

   for (i = 0; i < RangePlaceCount; ++i, ++db_place) {
      *db_place = RangePlace[i];
   }

   for (i = 0; i < RangeByZipCount; ++i, ++db_zip) {
      *db_zip = RangeByZip[i];
   }

   for (i = 0; i < RangeBySquareCount; ++i, ++db_square) {
      *db_square = RangeBySquare[i];
   }

   for (i = 0; i < RangeAddressCount; ++i, ++db_ranges) {
      *db_ranges = RangeAddress[i];
   }

   for (i = 0; i < RangeNoAddressCount; ++i, ++db_noaddr) {
      *db_noaddr = RangeNoAddress[i];
   }

   /* Do not save this data ever again. */

   free (RangeByStreet);
   RangeByStreet = NULL;
   RangeByStreetCount = 0;

   free (RangeByCity);
   RangeByCity = NULL;
   RangeByCityCount = 0;

   free (RangePlace);
   RangePlace = NULL;
   RangePlaceCount = 0;

   free (RangeByZip);
   RangeByZip = NULL;
   RangeByZipCount = 0;

   free (RangeBySquare);
   RangeBySquare = NULL;
   RangeBySquareCount = 0;

   free (RangeAddress);
   RangeAddress = NULL;
   RangeAddressCount = 0;

   free (RangeNoAddress);
   RangeNoAddress = NULL;
   RangeNoAddressCount = 0;
}
   

static buildmap_db_module RdmXchangeRangeModule = {
   "range",
   NULL, 
   rdmxchange_range_save,
   NULL,
   NULL
}; 
   
   
static void rdmxchange_range_import_table (const char *name, int count) {

   if (strcmp (name, "range/bystreet") == 0) {

      if (RangeByStreet != NULL) free (RangeByStreet);

      RangeByStreet = calloc (count, sizeof(RoadMapRangeByStreet));
      RangeByStreetCount = count;

   } else if (strcmp (name, "range/bycity") == 0) {

      if (RangeByCity != NULL) free (RangeByCity);

      RangeByCity = calloc (count, sizeof(RoadMapRangeByCity));
      RangeByCityCount = count;

   } else if (strcmp (name, "range/place") == 0) {

      if (RangePlace != NULL) free (RangePlace);

      RangePlace = calloc (count, sizeof(RoadMapRangePlace));
      RangePlaceCount = count;

   } else if (strcmp (name, "range/byzip") == 0) {

      if (RangeByZip != NULL) free (RangeByZip);

      RangeByZip = calloc (count, sizeof(RoadMapRangeByZip));
      RangeByZipCount = count;

   } else if (strcmp (name, "range/addr") == 0) {

      if (RangeAddress != NULL) free (RangeAddress);

      RangeAddress = calloc (count, sizeof(RoadMapRange));
      RangeAddressCount = count;

   } else if (strcmp (name, "range/noaddr") == 0) {

      if (RangeNoAddress != NULL) free (RangeNoAddress);

      RangeNoAddress = calloc (count, sizeof(RoadMapRangeNoAddress));
      RangeNoAddressCount = count;

   } else if (strcmp (name, "range/bysquare") == 0) {

      if (RangeBySquare != NULL) free (RangeBySquare);

      RangeBySquare = calloc (count, sizeof(RoadMapRangeBySquare));
      RangeBySquareCount = count;

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }

   buildmap_db_register (&RdmXchangeRangeModule);
}     
   
      
static int rdmxchange_range_import_schema (const char *table,
                                                 char *fields[], int count) {
      
   RangeCursor = 0;

   if (strcmp (table, "range/bystreet") == 0) {

      if ((RangeByStreet == NULL) ||
            (count != 4) ||
            (strcmp(fields[0], "first_range") != 0) ||
            (strcmp(fields[1], "first_city") != 0) ||
            (strcmp(fields[2], "first_zip") != 0) ||
            (strcmp(fields[3], "count") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 1;

   } else if (strcmp (table, "range/bycity") == 0) {

      if ((RangeByCity == NULL) ||
            (count != 2) ||
            (strcmp(fields[0], "city") != 0) ||
            (strcmp(fields[1], "count") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 2;

   } else if (strcmp (table, "range/place") == 0) {

      if ((RangePlace == NULL) ||
            (count != 2) ||
            (strcmp(fields[0], "place") != 0) ||
            (strcmp(fields[1], "city") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 3;

   } else if (strcmp (table, "range/byzip") == 0) {

      if ((RangeByZip == NULL) ||
            (count != 2) ||
            (strcmp(fields[0], "zip") != 0) ||
            (strcmp(fields[1], "count") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 4;

   } else if (strcmp (table, "range/addr") == 0) {

      if ((RangeAddress == NULL) ||
            (count != 3) ||
            (strcmp(fields[0], "line") != 0) ||
            (strcmp(fields[1], "fradd") != 0) ||
            (strcmp(fields[2], "toadd") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 5;

   } else if (strcmp (table, "range/noaddr") == 0) {

      if ((RangeNoAddress == NULL) ||
            (count != 2) ||
            (strcmp(fields[0], "line") != 0) ||
            (strcmp(fields[1], "street") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 6;

   } else if (strcmp (table, "range/bysquare") == 0) {

      char excluded[20];
      char included[20];

      sprintf (excluded, "excluded[%d]", ROADMAP_RANGE_HOLES);
      sprintf (included, "included[%d]", ROADMAP_RANGE_HOLES);

      if ((RangeBySquare == NULL) ||
            (count != 4) ||
            (strcmp(fields[0], excluded) != 0) ||
            (strcmp(fields[1], included) != 0) ||
            (strcmp(fields[2], "start") != 0) ||
            (strcmp(fields[3], "count") != 0)) {
         buildmap_fatal (1, "invalid schema for table %s", table);
      }
      return 7;
   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}                                         
      
static void rdmxchange_range_import_data (int table,
                                          char *fields[], int count) {

   int i;

   RoadMapRangeByStreet  *bystreet;
   RoadMapRangeByCity    *bycity;
   RoadMapRangePlace     *place;
   RoadMapRangeByZip     *byzip;
   RoadMapRange          *address;
   RoadMapRangeNoAddress *noaddress;
   RoadMapRangeBySquare  *bysquare;


   switch (table) {

      case 1:

         if (count != 4) {
            buildmap_fatal (count, "invalid range/bystreet record"); 
         }        
         bystreet = &(RangeByStreet[RangeCursor]);
         bystreet->first_range = rdmxchange_import_int (fields[0]);
         bystreet->first_city =  rdmxchange_import_int (fields[1]);
         bystreet->first_zip =   rdmxchange_import_int (fields[2]);
         bystreet->count_range = rdmxchange_import_int (fields[3]);
         break;

      case 2:

         if (count != 2) {
            buildmap_fatal (count, "invalid range/bycity record"); 
         }        
         bycity = &(RangeByCity[RangeCursor]);
         bycity->city = rdmxchange_import_int (fields[0]);
         bycity->count =  rdmxchange_import_int (fields[1]);
         break;

      case 3:

         if (count != 2) {
            buildmap_fatal (count, "invalid range/place record"); 
         }        
         place = &(RangePlace[RangeCursor]);
         place->place = rdmxchange_import_int (fields[0]);
         place->city =  rdmxchange_import_int (fields[1]);
         break;

      case 4:

         if (count != 2) {
            buildmap_fatal (count, "invalid range/byzip record"); 
         }        
         byzip = &(RangeByZip[RangeCursor]);
         byzip->zip = rdmxchange_import_int (fields[0]);
         byzip->count =  rdmxchange_import_int (fields[1]);
         break;

      case 5:

         if (count != 3) {
            buildmap_fatal (count, "invalid range/addr record"); 
         }        
         address = &(RangeAddress[RangeCursor]);
         address->line  = rdmxchange_import_int (fields[0]);
         address->fradd = rdmxchange_import_int (fields[1]);
         address->toadd = rdmxchange_import_int (fields[2]);
         break;

      case 6:

         if (count != 2) {
            buildmap_fatal (count, "invalid range/noaddr record"); 
         }        
         noaddress = &(RangeNoAddress[RangeCursor]);
         noaddress->line   = rdmxchange_import_int (fields[0]);
         noaddress->street = rdmxchange_import_int (fields[1]);
         break;

      case 7:

         if (count != ((2 * ROADMAP_RANGE_HOLES) + 2)) {
            buildmap_fatal (count, "invalid range/bysquare record"); 
         }        
         bysquare = &(RangeBySquare[RangeCursor]);

         for (i = 0; i < ROADMAP_RANGE_HOLES; ++i) {

            bysquare->hole[i].excluded =
               rdmxchange_import_int (fields[i]);
            bysquare->hole[i].included =
               rdmxchange_import_int (fields[ROADMAP_RANGE_HOLES+i]);
         }
         bysquare->noaddr_start =
            rdmxchange_import_int (fields[(2 * ROADMAP_RANGE_HOLES)]);
         bysquare->noaddr_count =
            rdmxchange_import_int (fields[(2 * ROADMAP_RANGE_HOLES) + 1]);
         break;

      default:

         buildmap_fatal (1, "bad subtable ID in table range import");
   }

   RangeCursor += 1;
}        

         
RdmXchangeImport RdmXchangeRangeImport = {
   "range",
   rdmxchange_range_import_table, 
   rdmxchange_range_import_schema,
   rdmxchange_range_import_data 
};

