/* buildmap_tiger.c - a module to read the original Tiger files.
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
 *   void buildmap_tiger_initialize (int verbose);
 *   void buildmap_tiger_read_rt1 (char *path, char *name, int verbose);
 *   void buildmap_tiger_read_rt2 (char *path, char *name, int verbose);
 *   void buildmap_tiger_read_rtc (char *path, char *name, int verbose);
 *   void buildmap_tiger_read_rt7 (char *path, char *name, int verbose);
 *   void buildmap_tiger_read_rt8 (char *path, char *name, int verbose);
 *   void buildmap_tiger_read_rti (char *path, char *name, int verbose);
 *   void buildmap_tiger_sort (void);
 *   void buildmap_tiger_summary (void);
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

#include "roadmap_types.h"
#include "roadmap_math.h"

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


static BuildMapDictionary DictionaryPrefix;
static BuildMapDictionary DictionaryStreet;
static BuildMapDictionary DictionaryType;
static BuildMapDictionary DictionarySuffix;
static BuildMapDictionary DictionaryCity;
static BuildMapDictionary DictionaryLandmark;

static int BuildMapCanals = ROADMAP_WATER_RIVER;
static int BuildMapRivers = ROADMAP_WATER_RIVER;

static char *BuildMapPrefix = NULL; /* Must be initialized. */
static int   BuildMapFormat = 2000;


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

static int tiger2address (char *line, int start, int end) {

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

   start -= 1; /* Adjust: tiger start from 1, C starts from 0. */

   switch (line[start]) {

      case 'D': /* Area. */

         if (line[start+1] == '8') {
            return ROADMAP_AREA_PARC;
         } else if (strncmp (line+start, "D31", 3) == 0) {
            return ROADMAP_AREA_HOSPITAL;
         } else if (strncmp (line+start, "D51", 3) == 0) {
            return ROADMAP_AREA_AIRPORT;
         } else if (strncmp (line+start, "D52", 3) == 0) {
            return ROADMAP_AREA_STATION;
         } else if (strncmp (line+start, "D53", 3) == 0) {
            return ROADMAP_AREA_STATION;
         } else if (strncmp (line+start, "D61", 3) == 0) {
            return ROADMAP_AREA_MALL;
         }
         break;

      case 'H': /* Rivers, lakes and sea. */

         switch (line[start+1]) {

            case '3': return ROADMAP_WATER_LAKE;
            case '4': return ROADMAP_WATER_LAKE;  /* Reservoir. */
            case '6': return ROADMAP_WATER_LAKE;  /* Quarry. */
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

   start -= 1; /* Adjust: tiger start from 1, C starts from 0. */

   switch (line[start]) {

      case 'A': /* Roads. */

         switch (line[start+1]) {

            case '1': return ROADMAP_ROAD_FREEWAY;
            case '2': return ROADMAP_ROAD_MAIN;
            case '3': return ROADMAP_ROAD_MAIN;
            case '4': return ROADMAP_ROAD_STREET;
            case '6':
               if (line[start+2] == '3') {
                  return ROADMAP_ROAD_RAMP;
               }
               return 0;
            default:  return ROADMAP_ROAD_TRAIL;
         }
         break;

      case 'H': /* Rivers, lakes and sea. */

         switch (line[start+1]) {

            case '0': return ROADMAP_WATER_SHORELINE;
            case '1': return BuildMapRivers;
            case '2': return BuildMapCanals;
            case '5': return ROADMAP_WATER_SEA;
         }
         break;
   }

   return tiger2area (line, start+1, end);
}


static char *buildmap_tiger_read
                (char *path, char *name, char *extension,
                 int verbose,
                 int *size) {

   int   file;
   char *data;
   struct stat state_result;

   char full_name[1025];

   if (BuildMapPrefix == NULL) {
      buildmap_fatal (0, "no Tiger file prefix defined");
   }
   snprintf (full_name, 1024,
             "%s/%s%s%s", path, BuildMapPrefix, name, extension);

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
   if (data == NULL) {
      buildmap_error (0, "cannot mmap");
      return NULL;
   }

   close (file);

   *size = state_result.st_size;
   return data;
}


static void tiger_summary (int verbose, int count) {

   buildmap_summary (verbose, "%d records", count);
}


void buildmap_tiger_set_format (int year) {
   BuildMapFormat = year;
}


void buildmap_tiger_set_prefix (char *prefix) {
   BuildMapPrefix = prefix;
}


void buildmap_tiger_initialize (int verbose, int canals, int rivers) {

   if (! canals) {
      BuildMapCanals = 0;
   }

   if (! rivers) {
      BuildMapRivers = 0;
   }
}


void buildmap_tiger_read_rt1 (char *path, char *name, int verbose) {

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

   char cfcc;
   int  tlid;
   RoadMapString fedirp;
   RoadMapString fename;
   RoadMapString fetype;
   RoadMapString fedirs;
   RoadMapString city;
   RoadMapString place;
   int fraddl;
   int toaddl;
   int fraddr;
   int toaddr;
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

   data = buildmap_tiger_read (path, name, ".RT1", verbose, &size);
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
         if (buildmap_polygon_use_line (tlid)) {
            cfcc = ROADMAP_AREA_PARC; /* to force loading this line. */
         }
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

         line = buildmap_line_add (tlid, cfcc, from_point, to_point);

         if (cfcc >= ROADMAP_ROAD_FIRST &&
             cfcc <= ROADMAP_ROAD_LAST) {

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

                        int fradd = fraddr;
                        int toadd = toaddr;

                        if (fradd < toadd) {
                           if (fradd > fraddl) fradd = fraddl;
                           if (toadd < toaddl) toadd = toaddl;
                        } else {
                           if (fradd < fraddl) fradd = fraddl;
                           if (toadd > toaddl) toadd = toaddl;
                        }

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
               buildmap_range_add_no_address (line, street);
            }

         } else if (cfcc >= ROADMAP_AREA_FIRST &&
                    cfcc <= ROADMAP_AREA_LAST) {

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


void buildmap_tiger_read_rt2 (char *path, char *name, int verbose) {

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


   data = buildmap_tiger_read (path, name, ".RT2", verbose, &size);
   if (data == NULL) return;

   estimated_lines = size / 209;

   line_count = 0;
   record_count = 0;
   end_of_data = data + size;

   /* We need the lines to be sorted, because we will order the shape
    * according to the orders of the lines.
    */
   buildmap_line_sort();

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
                  (line_index, (10 * sequence) + i, longitude, latitude);

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


void buildmap_tiger_read_rtc (char *path, char *name, int verbose) {

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

   data = buildmap_tiger_read (path, name, ".RTC", verbose, &size);
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
         year = tiger2int (cursor, 11, 14);

         if (fips > 0) {

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


void buildmap_tiger_read_rt7 (char *path, char *name, int verbose) {

   int    size;
   int    estimated_lines;
   int    line_count;
   int    record_count;
   char  *data;
   char  *cursor;
   char  *end_of_data;

   int    landid;
   char   cfcc;
   short  landname;


   DictionaryLandmark = buildmap_dictionary_open ("landmark");

   data = buildmap_tiger_read (path, name, ".RT7", verbose, &size);
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


void buildmap_tiger_read_rt8 (char *path, char *name, int verbose) {

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


   data = buildmap_tiger_read (path, name, ".RT8", verbose, &size);
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


void buildmap_tiger_read_rti (char *path, char *name, int verbose) {

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


   data = buildmap_tiger_read (path, name, ".RTI", verbose, &size);
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


void buildmap_tiger_sort (void) {

   buildmap_line_sort ();
   buildmap_street_sort ();
   buildmap_range_sort ();
   buildmap_shape_sort ();
   buildmap_polygon_sort ();
}


void buildmap_tiger_summary (void) {

   buildmap_zip_summary ();
   buildmap_square_summary ();
   buildmap_street_summary ();
   buildmap_line_summary ();
   buildmap_range_summary ();
   buildmap_shape_summary ();
   buildmap_polygon_summary ();
}

