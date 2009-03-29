/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2009, Danny Backx
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

/**
 * @file
 * @brief a module to read the original Tiger files.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_path.h"
#include "roadmap_line.h"

#include "buildmap.h"
#include "buildmap_zip.h"
#include "buildmap_tiger.h"
#include "buildmap_city.h"
#include "buildmap_square.h"
#include "buildmap_point.h"
#include "buildmap_line.h"
#include "buildmap_street.h"
#include "buildmap_range.h"
#include "buildmap_area.h"
#include "buildmap_shape.h"
#include "buildmap_polygon.h"
#include "buildmap_metadata.h"

#include "buildmap_layer.h"


static BuildMapDictionary DictionaryPrefix;
static BuildMapDictionary DictionaryStreet;
static BuildMapDictionary DictionaryType;
static BuildMapDictionary DictionarySuffix;
static BuildMapDictionary DictionaryCity;
static BuildMapDictionary DictionaryLandmark;

static int   BuildMapFormat = 2000;

/* Road layers. */

static int BuildMapLayerFreeway = 0;
static int BuildMapLayerRamp = 0;
static int BuildMapLayerMain = 0;
static int BuildMapLayerStreet = 0;
static int BuildMapLayerTrail = 0;
static int BuildMapLayerRail = 0;

/* Area layers. */

static int BuildMapLayerPark = 0;
static int BuildMapLayerHospital = 0;
static int BuildMapLayerAirport = 0;
static int BuildMapLayerStation = 0;
static int BuildMapLayerMall = 0;

/* Water layers. */

static int BuildMapLayerShoreline = 0;
static int BuildMapLayerRiver = 0;
static int BuildMapLayerCanal = 0;
static int BuildMapLayerLake = 0;
static int BuildMapLayerSea = 0;

static struct {
   const char *name;
   int line_count;
   int landmark_count;
} BuildMapTigerLayerStatistics[256];

static void buildmap_tiger_set_one_layer (int verbose,
                                          int *layer, const char *name) {

   *layer = buildmap_layer_get (name);

   if (*layer > 0) {
      if (verbose) fprintf (stderr, "   %3d %s\n", *layer, name);
      if (*layer < 256) {
         BuildMapTigerLayerStatistics[*layer].name = name;
         BuildMapTigerLayerStatistics[*layer].line_count = 0;
         BuildMapTigerLayerStatistics[*layer].landmark_count = 0;
      }
   }
}

static void buildmap_tiger_set_layers (int verbose) {

   if (verbose) fprintf (stderr, "Enabled TIGER layers:\n");

   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerFreeway,   "freeways");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerRamp,      "ramps");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerMain,      "highways");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerStreet,    "streets");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerTrail,     "trails");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerRail,      "railroads");

   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerPark,      "parks");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerHospital,  "hospitals");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerAirport,   "airports");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerStation,   "stations");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerMall,      "malls");

   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerShoreline, "shore");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerRiver,     "rivers");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerCanal,     "canals");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerLake,      "lakes");
   buildmap_tiger_set_one_layer (verbose, &BuildMapLayerSea,       "sea");
}


static void tigerAdjust (char *line, int *start, int *end) {

   /* The tiger format starts at position 1, C starts at position 0. */
   *end -= 1;
   *start -= 1;

   /* Get rid of header and trailer spaces. */
   while (line[*end] == ' ')   (*end)--;
   while (line[*start] == ' ') (*start)++;
}

static int tiger2int (char *line, int start, int end) {

   int i;
   int sign;
   int result = 0;
   char digit;

   tigerAdjust (line, &start, &end);

   if (end < start) return 0;

   if (line[start] == '-') {
      sign = -1;
      start++;
   }else if (line[start] == '+') {
      sign = 1;
      start++;
   } else {
      sign = 1;
   }

   for (i = start; i <= end; i++) {
      digit = line[i];
      if ((digit < '0') || (digit > '9')) {
         buildmap_error (i, "bad numerical character %c", digit);
         if (i > start) continue;
      }
      result = (result * 10) + (digit - '0');
   }

   if ((end - start >= 11) && (line[start] >= 2)) {
      buildmap_error (i, "%sinteger overflow in %s",
                      (line[start] >= 3) ? "" : "potential ",
                      line+start);
   }

   return sign * result;
}

static unsigned int tiger2address (char *line, int start, int end) {

   tigerAdjust (line, &start, &end);

   if (end < start) return 0;

   if ((end - start >= 11) && (line[start] >= 2)) {
      buildmap_error (start,
                      "%sinteger overflow in %s",
                      (line[start] >= 3) ? "" : "potential ",
                      line+start);
   }

   return roadmap_math_street_address (line + start, end - start + 1);
}

static RoadMapString
tiger2string (BuildMapDictionary d, char *line, int start, int end) {

   tigerAdjust (line, &start, &end);

   if (end < start) {
      return buildmap_dictionary_add (d, "", 0);
   }

   return buildmap_dictionary_add (d, line+start, end - start + 1);
}


/* This function returns the roadmap type for a given polygon or
 * polygone's line.
 */
static char tiger2area (char *line, int start, int end) {

   switch (line[start-1]) { /* Adjust: tiger start from 1, C starts from 0. */

      case 'D': /* Area. */

         switch (line[start]) {

            case '8': return BuildMapLayerPark;

            case '3':
                      if (line[start+1] == '1') {
                         return BuildMapLayerHospital;
                      }
                      break;

            case '5':
                      switch (line[start+1]) {
                         case '1': return BuildMapLayerAirport;
                         case '2': return BuildMapLayerStation;
                         case '3': return BuildMapLayerStation;
                      }
                      break;

            case '6':
                      if (line[start+1] == '1') {
                         return BuildMapLayerMall;
                      }
                      break;
         }
         break;

      case 'H': /* Rivers, lakes and sea. */

         switch (line[start]) {

            case '3': return BuildMapLayerLake;
            case '4': return BuildMapLayerLake;  /* Reservoir. */
            case '6': return BuildMapLayerLake;  /* Quarry. */
         }
         break;
   }

   return 0; /* Skip this record. */
}


/* This function returns the roadmap type for a Tiger line.
 * If the value is 0, the line will be ignored.
 * Note that we must not ignore lines that belong to a type of
 * area that will be processed: this is why we fallback to tiger2area.
 */
static char tiger2type (char *line, int start, int end) {

   switch (line[start-1]) { /* Adjust: tiger start from 1, C starts from 0. */

      case 'A': /* Roads. */

         switch (line[start]) {

            case '1': return BuildMapLayerFreeway;
            case '2': return BuildMapLayerMain;
            case '3': return BuildMapLayerMain;
            case '4': return BuildMapLayerStreet;
            case '6':
               if (line[start+1] == '3') {
                  return BuildMapLayerRamp;
               }
               break;
            default:  return BuildMapLayerTrail;
         }
         break;

      case 'B': /* Railroads. */

         if ((line[start] == '4') ||  /* rail on ferry */
             (line[start] == '1' &&
              (line[start+1] == '4' ||  /* various abandoned */
               line[start+1] == '5' ||
               line[start+1] == '6'))) {
               break;
         }

         return BuildMapLayerRail;

      case 'H': /* Rivers, lakes and sea. */

         switch (line[start]) {

            case '0': return BuildMapLayerShoreline;
            case '1': return BuildMapLayerRiver;
            case '2': return BuildMapLayerCanal;
            case '5': return BuildMapLayerSea;
            default:  return tiger2area (line, start, end);
         }
         break;

      default:

         return tiger2area (line, start, end);
   }

   return 0;
}


static char *buildmap_tiger_read
                (const char *source,
                 const char *extension,
                 int verbose,
                 int *size) {

   int   file;
   char *data;
   struct stat state_result;

   char *full_name = malloc(strlen(source) + strlen(extension) + 4);

   roadmap_check_allocated(full_name);

   strcpy (full_name, source);
   strcat (full_name, extension);

   buildmap_set_source (full_name);

   file = open (full_name, O_RDONLY);
   if (file < 0) {
      buildmap_error (0, "cannot open file %s", full_name);
      return NULL;
   }

   if (fstat (file, &state_result) != 0) {
      buildmap_error (0, "cannot stat file");
      return NULL;
   }

   if (verbose) {
      buildmap_info ("size = %d Mbytes", state_result.st_size / (1024 * 1024));
   }

   data = mmap (NULL, state_result.st_size, PROT_READ, MAP_PRIVATE, file, 0);
   if (data == MAP_FAILED) {
      buildmap_error (0, "cannot mmap");
      return NULL;
   }

   close (file);
   free  (full_name);

   *size = state_result.st_size;
   return data;
}


static void tiger_summary (int verbose, int count) {

   buildmap_summary (verbose, "%d records", count);
}


/* Table 1: lines. */

static void buildmap_tiger_read_rt1 (const char *source, int verbose) {

   int    size;
   int    estimated;
   int    line_count;
   int    record_count;
   int    merged_range_count;
   char  *data;
   char  *cursor;
   char  *end_of_data;

   int line;
   int street;
   RoadMapZip zip = 0;

   int cfcc;
   int  tlid;
   RoadMapString fedirp;
   RoadMapString fename;
   RoadMapString fetype;
   RoadMapString fedirs;
   RoadMapString city;
   RoadMapString place;
   unsigned int fraddl;
   unsigned int toaddl;
   unsigned int fraddr;
   unsigned int toaddr;
   int zipl;
   int zipr;
   int frlong;
   int frlat;
   int tolong;
   int tolat;
   int from_point;
   int to_point;


   DictionaryPrefix = buildmap_dictionary_open ("prefix");
   DictionaryStreet = buildmap_dictionary_open ("street");
   DictionaryType   = buildmap_dictionary_open ("type");
   DictionarySuffix = buildmap_dictionary_open ("suffix");

   data = buildmap_tiger_read (source, ".RT1", verbose, &size);
   if (data == NULL) return;

   estimated = size / 229;

   line_count = 0;
   record_count = 0;
   merged_range_count = 0;
   end_of_data = data + size;

   for (cursor = data; cursor < end_of_data; cursor += 229) {

      line_count += 1;
      buildmap_set_line (line_count);

      if (cursor[0] != '1') {
         fprintf (stderr, "%*.*s\n", 228, 228, cursor);
         buildmap_fatal (0, "bad type '%c'", cursor[0]);
      }

      cfcc = tiger2type (cursor, 56, 58);
      tlid   = tiger2int (cursor, 6, 15);

      if (cfcc == 0) {
         /* Even if filtered out, a line should be still recorded if
          * it is part of a polygon we recorded: we need the line to
          * draw the polygon.
          */
         cfcc = buildmap_polygon_use_line (tlid);
      }

      if (cfcc > 0) {

         fedirp = tiger2string (DictionaryPrefix, cursor, 18, 19);
         fename = tiger2string (DictionaryStreet, cursor, 20, 49);
         fetype = tiger2string (DictionaryType,   cursor, 50, 53);
         fedirs = tiger2string (DictionarySuffix, cursor, 54, 55);

         fraddl = tiger2address (cursor, 59, 69);
         toaddl = tiger2address (cursor, 70, 80);

         fraddr = tiger2address (cursor, 81, 91);
         toaddr = tiger2address (cursor, 92, 102);

         zipl   = tiger2int (cursor, 107, 111);
         zipr   = tiger2int (cursor, 112, 116);

         frlong = tiger2int (cursor, 191, 200);
         frlat  = tiger2int (cursor, 201, 209);

         tolong = tiger2int (cursor, 210, 219);
         tolat  = tiger2int (cursor, 220, 228);


         from_point = buildmap_point_add (frlong, frlat);
         to_point   = buildmap_point_add (tolong, tolat);

         BuildMapTigerLayerStatistics[cfcc].line_count += 1;

         line = buildmap_line_add (tlid, cfcc, from_point, to_point,
			 ROADMAP_LINE_DIRECTION_BOTH);

         if (cursor[55] == 'A') { /* Roads. */

            street = buildmap_street_add
                        (cfcc, fedirp, fename, fetype, fedirs, line);

            if ((zipl > 0) || (zipr > 0)) {

               /* Check if the two sides of the street can be merged into
                * a single range (same city/place, same zip, and same range
                * beside the even/odd difference).
                */
               int merged_range = 0;
               int diff_fr = fraddr - fraddl;
               int diff_to = toaddr - toaddl;

               if ((zipl == zipr) &&
                   (diff_fr > -10) && (diff_fr < 10) &&
                   (diff_to > -10) && (diff_to < 10)) {

                  RoadMapString cityl;
                  RoadMapString cityr;

                  cityl = buildmap_city_get_name (tiger2int (cursor, 141, 145));
                  cityr = buildmap_city_get_name (tiger2int (cursor, 146, 150));

                  if (cityl == cityr) {

                     RoadMapString placel;
                     RoadMapString placer;

                     placel =
                        buildmap_city_get_name (tiger2int (cursor, 161, 165));
                     placer =
                        buildmap_city_get_name (tiger2int (cursor, 166, 170));

                     if (placel == placer) {

                        unsigned int fradd;
                        unsigned int toadd;

                        buildmap_range_merge (fraddl, toaddl,
                                              fraddr, toaddr,
                                              &fradd, &toadd);

                        zip = buildmap_zip_add (zipl, frlong, frlat);

                        if (placel > 0) {
                           buildmap_range_add_place (placel, cityl);
                           cityl = placel;
                        }
                        buildmap_range_add
                           (line, street, fradd, toadd, zip, cityl);

                        merged_range = 1;
                        merged_range_count += 1;
                     }
                  }
               }

               if (zipl > 0) {

                  city = buildmap_city_get_name (tiger2int (cursor, 141, 145));

                  place = buildmap_city_get_name (tiger2int (cursor, 161, 165));

                  if (! merged_range) {

                     if (place > 0) {
                        buildmap_range_add_place (place, city);
                        city = place;
                     }

                     zip = buildmap_zip_add (zipl, frlong, frlat);

                     buildmap_range_add
                        (line, street, fraddl, toaddl, zip, city);
                  }
               }

               if (zipr > 0) {

                  city = buildmap_city_get_name (tiger2int (cursor, 146, 150));

                  place = buildmap_city_get_name (tiger2int (cursor, 166, 170));

                  if (! merged_range) {

                     if (place > 0) {
                        buildmap_range_add_place (place, city);
                        city = place;
                     }

                     if (zipr != zipl) {
                        zip = buildmap_zip_add (zipr, frlong, frlat);
                     }
                     buildmap_range_add
                        (line, street, fraddr, toaddr, zip, city);
                  }
               }

            } else {
               RoadMapString cityl;
               RoadMapString cityr;
               int added = 0;
 
               /* NB -- no place support here.  the use of places is
                * disabled in roadmap currently anyway.
                */

               cityl = buildmap_city_get_name (tiger2int (cursor, 141, 145));
               if (cityl > 0) {
                   buildmap_range_add (line, street, 0, 0, 0, cityl);
                   added = 1;
               }
 
               cityr = buildmap_city_get_name (tiger2int (cursor, 146, 150));
               if (cityr > 0 && cityr != cityl) {
                  buildmap_range_add (line, street, 0, 0, 0, cityr);
                  added = 1;
               }
 
               if (!added) {
                  /* no zip, no city -- fallback list */
                  buildmap_range_add_no_address (line, street);
               }
            }

         } else if (cursor[55] == 'D') { /* Areas. */

            buildmap_area_add (cfcc, fedirp, fename, fetype, fedirs, tlid);
         }

         record_count += 1;
      }

      if (verbose) {
         if ((line_count & 0xff) == 0) {
            buildmap_progress (line_count, estimated);
         }
      }

      if (cursor[228] == '\r') {
         cursor += 1;   /* Case of an MS-DOS format file. */
      }
      if (cursor[228] != '\n') {
         buildmap_error (0, "bad end of line");
      }
   }

   munmap (data, size);

   tiger_summary (verbose, record_count);
   if (merged_range_count > 0) {
      buildmap_summary (verbose, "%d ranges merged", merged_range_count);
   }
}

/* Table 2: shapes. */

static void buildmap_tiger_read_rt2 (const char *source, int verbose) {

   static int LocationOfPoint[] = {19, 38, 57, 76, 95, 114, 133, 152, 171, 190};

   int    size;
   int    estimated_lines;
   int    line_count;
   int    record_count;
   char  *data;
   char  *cursor;
   char  *end_of_data;

   int    i;

   int    location;
   int    tlid;
   int    sequence;
   int    longitude;
   int    latitude;
   int    line_index;


   data = buildmap_tiger_read (source, ".RT2", verbose, &size);
   if (data == NULL) return;

   estimated_lines = size / 209;

   end_of_data = data + size;

   /* since lines are sorted by square, we need to have all the
    * square limits before we sort the lines
    */
   line_count = 0;
   for (cursor = data; cursor < end_of_data; cursor += 209) {

      line_count += 1;
      buildmap_set_line (line_count);

      if (cursor[0] != '2') {
         buildmap_error (0, "bad type %c", cursor[0]);
         continue;
      }

      tlid = tiger2int (cursor, 6, 15);

      for (i = 0; i < 10; i++) {

         location  = LocationOfPoint[i];
         longitude = tiger2int (cursor, location, location + 9);

         if (longitude != 0) {

            latitude = tiger2int (cursor, location+10, location+18);

            buildmap_square_adjust_limits(longitude, latitude);
         }
      }

      if (cursor[208] == '\r') {
         cursor += 1;   /* Case of an MS-DOS format file. */
      }
      if (cursor[208] != '\n') {
         buildmap_error (0, "bad end of line");
      }
   }

   /* We need the lines to be sorted, because we will order the shape
    * according to the orders of the lines.
    */
   buildmap_line_sort();

   line_count = 0;
   record_count = 0;
   for (cursor = data; cursor < end_of_data; cursor += 209) {

      line_count += 1;
      buildmap_set_line (line_count);

      if (cursor[0] != '2') {
         buildmap_error (0, "bad type %c", cursor[0]);
         continue;
      }

      tlid = tiger2int (cursor, 6, 15);

      line_index = buildmap_line_find_sorted (tlid);

      if (line_index >= 0) {

         sequence = tiger2int (cursor, 16, 18) - 1;

         for (i = 0; i < 10; i++) {

            location  = LocationOfPoint[i];
            longitude = tiger2int (cursor, location, location + 9);

            if (longitude != 0) {

               latitude = tiger2int (cursor, location+10, location+18);

               buildmap_shape_add
                  (line_index, 0, tlid, (10 * sequence) + i, longitude, latitude);

               record_count += 1;
            }
         }
      }

      if (verbose) {
         if ((line_count & 0xff) == 0) {
            buildmap_progress (line_count, estimated_lines);
         }
      }

      if (cursor[208] == '\r') {
         cursor += 1;   /* Case of an MS-DOS format file. */
      }
      if (cursor[208] != '\n') {
         buildmap_error (0, "bad end of line");
      }
   }

   munmap (data, size);

   tiger_summary (verbose, record_count);
}


/* Table C: cities. */

static void buildmap_tiger_read_rtc (const char *source, int verbose) {

   int    rtc_line_size;

   int    size;
   int    estimated_lines;
   int    line_count;
   int    record_count;
   char  *data;
   char  *cursor;
   char  *end_of_data;

   int    fips;
   int    year;
   short  city;


   DictionaryCity = buildmap_dictionary_open ("city");

   data = buildmap_tiger_read (source, ".RTC", verbose, &size);
   if (data == NULL) return;

   if (BuildMapFormat == 2002) {
      rtc_line_size = 123;
   } else {
      rtc_line_size = 113;
   }
   estimated_lines = size / rtc_line_size;

   line_count = 0;
   record_count = 0;
   end_of_data = data + size;

   for (cursor = data; cursor < end_of_data; cursor += rtc_line_size) {

      line_count += 1;
      buildmap_set_line (line_count);

      if (cursor[0] != 'C') {
         fprintf (stderr, "%*.*s\n", 112, 112, cursor);
         buildmap_fatal (0, "bad type %c", cursor[0]);
         continue;
      }

      /* Forget the records that describe the 1990 census code.
       */
      if (cursor[10] != '1') {

         fips = tiger2int (cursor, 15, 19);

         if (cursor[10] == 'C') {
                 year = 2000;
         } else if (cursor[10] == 'E') {
                 year = 2002;
         } else {
                 year = tiger2int (cursor, 11, 14);
         }

         /* Ignore when no FIPS or the "balance of county" FIPS. */

         if (fips > 0 && fips != 99999) {

            if (BuildMapFormat == 2002) {
               city = tiger2string (DictionaryCity, cursor, 63, 122);
            } else {
               city = tiger2string (DictionaryCity, cursor, 53, 112);
            }

            if (cursor[21] != ' ' && cursor[21] != 'X' && cursor[21] != 'Z') {
               buildmap_city_add (fips, year, city);
            }

            record_count += 1;
         }
      }

      if (verbose) {
         if ((line_count & 0xff) == 0) {
            buildmap_progress (line_count, estimated_lines);
         }
      }

      if (cursor[rtc_line_size - 1] == '\r') {
         cursor += 1;   /* Case of an MS-DOS format file. */
      }
      if (cursor[rtc_line_size - 1] != '\n') {
         buildmap_error (0, "bad end of line");
      }
   }

   munmap (data, size);

   tiger_summary (verbose, record_count);
}


/* Table 7: landmarks. */

static void buildmap_tiger_read_rt7 (const char *source, int verbose) {

   int    size;
   int    estimated_lines;
   int    line_count;
   int    record_count;
   char  *data;
   char  *cursor;
   char  *end_of_data;

   int    landid;
   int    cfcc;
   short  landname;


   DictionaryLandmark = buildmap_dictionary_open ("landmark");

   data = buildmap_tiger_read (source, ".RT7", verbose, &size);
   if (data == NULL) return;

   estimated_lines = size / 75;

   line_count = 0;
   record_count = 0;
   end_of_data = data + size;

   for (cursor = data; cursor < end_of_data; cursor += 75) {

      line_count += 1;
      buildmap_set_line (line_count);

      if (cursor[0] != '7') {
         fprintf (stderr, "%*.*s\n", 74, 74, cursor);
         buildmap_fatal (0, "bad type %c", cursor[0]);
         continue;
      }

      cfcc = tiger2area (cursor, 22, 24);

      if (cfcc > 0) {

         BuildMapTigerLayerStatistics[cfcc].landmark_count += 1;

         landid   = tiger2int (cursor, 11, 20);
         landname = tiger2string (DictionaryLandmark, cursor, 25, 54);

         buildmap_polygon_add_landmark (landid, cfcc, landname);
      }

      record_count += 1;

      if (verbose) {
         if ((line_count & 0xff) == 0) {
            buildmap_progress (line_count, estimated_lines);
         }
      }

      if (cursor[74] == '\r') {
         cursor += 1;   /* Case of an MS-DOS format file. */
      }
      if (cursor[74] != '\n') {
         buildmap_error (0, "bad end of line");
      }
   }

   munmap (data, size);

   tiger_summary (verbose, record_count);
}


/* Table 8: polygons. */

static void buildmap_tiger_read_rt8 (const char *source, int verbose) {

   int    size;
   int    estimated_lines;
   int    line_count;
   int    record_count;
   char  *data;
   char  *cursor;
   char  *end_of_data;

   int    landid;
   int    polyid;
   short  cenid;

   BuildMapDictionary dictionary_cenid = buildmap_dictionary_open (".cenid");


   data = buildmap_tiger_read (source, ".RT8", verbose, &size);
   if (data == NULL) return;

   estimated_lines = size / 37;

   line_count = 0;
   record_count = 0;
   end_of_data = data + size;


   for (cursor = data; cursor < end_of_data; cursor += 37) {

      line_count += 1;
      buildmap_set_line (line_count);

      if (cursor[0] != '8') {
         fprintf (stderr, "%*.*s\n", 36, 36, cursor);
         buildmap_fatal (0, "bad type %c", cursor[0]);
         continue;
      }

      landid = tiger2int (cursor, 26, 35);

      cenid  = tiger2string (dictionary_cenid, cursor, 11, 15);
      polyid = tiger2int (cursor, 16, 25);

      buildmap_polygon_add (landid, cenid, polyid);

      record_count += 1;

      if (verbose) {
         if ((line_count & 0xff) == 0) {
            buildmap_progress (line_count, estimated_lines);
         }
      }

      if (cursor[36] == '\r') {
         cursor += 1;   /* Case of an MS-DOS format file. */
      }
      if (cursor[36] != '\n') {
         buildmap_error (0, "bad end of line");
      }
   }

   munmap (data, size);

   tiger_summary (verbose, record_count);
}


/* Table I: polygon lines. */

static void buildmap_tiger_read_rti (const char *source, int verbose) {

   int    rti_line_size;

   int    size;
   int    estimated_lines;
   int    line_count;
   int    record_count;
   char  *data;
   char  *cursor;
   char  *end_of_data;

   int    tlid;
   short  cenidl;
   int    polyidl;
   short  cenidr;
   int    polyidr;

   BuildMapDictionary dictionary_cenid = buildmap_dictionary_open (".cenid");


   data = buildmap_tiger_read (source, ".RTI", verbose, &size);
   if (data == NULL) return;

   if (BuildMapFormat == 2002) {
      rti_line_size = 128;
   } else {
      rti_line_size = 53;
   }
   estimated_lines = size / rti_line_size;

   line_count = 0;
   record_count = 0;
   end_of_data = data + size;

   for (cursor = data; cursor < end_of_data; cursor += rti_line_size) {

      line_count += 1;
      buildmap_set_line (line_count);

      if (cursor[0] != 'I') {
         fprintf (stderr, "%*.*s\n", rti_line_size-1, rti_line_size-1, cursor);
         buildmap_fatal (0, "bad type %c", cursor[0]);
         continue;
      }

      if (BuildMapFormat == 2002) {

         tlid = tiger2int (cursor, 11, 20);

         cenidl  = tiger2string (dictionary_cenid, cursor, 41, 45);
         polyidl = tiger2int (cursor, 46, 55);

         cenidr  = tiger2string (dictionary_cenid, cursor, 56, 60);
         polyidr = tiger2int (cursor, 61, 70);

      } else {
         tlid = tiger2int (cursor, 6, 15);

         cenidl  = tiger2string (dictionary_cenid, cursor, 22, 26);
         polyidl = tiger2int (cursor, 27, 36);

         cenidr  = tiger2string (dictionary_cenid, cursor, 37, 41);
         polyidr = tiger2int (cursor, 42, 51);
      }

      if ((cenidl != cenidr) || (polyidl != polyidr)) {

         if (cenidl > 0 && polyidl > 0) {

            buildmap_polygon_add_line
               (cenidl, polyidl, tlid, POLYGON_SIDE_LEFT);
         }

         if (cenidr > 0 && polyidr > 0) {

            buildmap_polygon_add_line
               (cenidr, polyidr, tlid, POLYGON_SIDE_RIGHT);
         }
      }

      record_count += 1;

      if (verbose) {
         if ((line_count & 0xff) == 0) {
            buildmap_progress (line_count, estimated_lines);
         }
      }

      if (cursor[rti_line_size-1] == '\r') {
         cursor += 1;   /* Case of an MS-DOS format file. */
      }
      if (cursor[rti_line_size-1] != '\n') {
         buildmap_error (0, "bad end of line");
      }
   }

   munmap (data, size);

   tiger_summary (verbose, record_count);
}


void buildmap_tiger_set_format (int year) {
   BuildMapFormat = year;
}


void buildmap_tiger_process (const char *source,
                             const char *county,
                             int verbose) {

   int i;
   char *base = roadmap_path_remove_extension (source);


   buildmap_tiger_set_layers (verbose);

   buildmap_tiger_read_rtc (base, verbose); /* Cities. */

   buildmap_tiger_read_rt7 (base, verbose); /* landmarks. */

   buildmap_tiger_read_rt8 (base, verbose); /* Polygons. */

   buildmap_tiger_read_rti (base, verbose); /* Polygon lines. */

   buildmap_tiger_read_rt1 (base, verbose); /* Lines. */

   buildmap_tiger_read_rt2 (base, verbose); /* Shapes. */


   buildmap_metadata_add_attribute ("Territory", "Id", county);

   buildmap_metadata_add_attribute ("Territory", "Parent", "us");
   buildmap_metadata_add_value ("Territory", "Parent",
                                "United States of America");

   buildmap_metadata_add_attribute ("Class", "Name",   "All");
   buildmap_metadata_add_attribute ("Data",  "Source", "USCB");

   if (verbose) {
      fprintf (stderr,
               "-- %s: layer statistics:\n",
               roadmap_path_skip_directories(source));

      for (i = 0; i < 256; ++i) {
         if (BuildMapTigerLayerStatistics[i].name != NULL) {
            fprintf (stderr,
                     "-- %s:     %s: %d lines, %d landmarks\n",
                     roadmap_path_skip_directories(source),
                     BuildMapTigerLayerStatistics[i].name,
                     BuildMapTigerLayerStatistics[i].line_count,
                     BuildMapTigerLayerStatistics[i].landmark_count);
         }
      }
   }

   free(base);
}

