/* buildus_fips.c - Read the list of FIPS codes from the Census Bureau files.
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
 *   void buildus_fips_read (char *path, int verbose);
 *   void buildus_fips_summary (void);
 */

#define _ISOC9X_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#include "roadmap_types.h"
#include "roadmap_hash.h"

#include "buildmap.h"
#include "buildus_fips.h"
#include "buildus_county.h"

static BuildMapDictionary BuildMapStateDictionary;


static FILE *buildus_fips_open (char *path, char *name, int verbose) {

   FILE *file;

   char full_name[1025];

   snprintf (full_name, 1024, "%s/%s", path, name);

   buildmap_set_source (full_name);

   if (verbose) {

      struct stat state_result;

      if (stat (full_name, &state_result) != 0) {
         buildmap_fatal (0, "cannot stat file");
      }

      buildmap_info ("size = %d Kbytes", state_result.st_size / 1024);
   }

   file = fopen (full_name, "r");
   if (file == NULL) {
      buildmap_fatal (0, "cannot open file %s", full_name);
   }

   return file;
}


static void buildus_fips_read_states (char *path, int verbose) {

   int   i;
   FILE *input;
   char  line[1024];
   int   line_count;

   RoadMapString state_name;
   RoadMapString state_symbol;


   input = buildus_fips_open (path, "usstates.txt", verbose);

   BuildMapStateDictionary = buildmap_dictionary_open ("state");

   line_count = 0;

   while (! feof(input)) {

      if (fgets (line, sizeof(line), input) == NULL) {
         break;
      }

      buildmap_set_line (++line_count);

      /* Separate the symbol from the state's name. */
      line[2] = 0;

      /* Remove the end of line character. */
      for (i = 3; line[i] > 0; ++i) {
          if (line[i] < ' ') {
             line[i] = 0;
             break;
          }
      }

      state_symbol =
          buildmap_dictionary_add
              (BuildMapStateDictionary, line, strlen(line));

      state_name =
          buildmap_dictionary_add
              (BuildMapStateDictionary, line+3, strlen(line+3));

      if (state_symbol == 0 || state_name == 0) {
         buildmap_fatal (0, "invalid state description");
      }

      buildus_county_add_state (state_name, state_symbol);
   }

   fclose (input);
}


static void buildus_fips_read_counties (char *path, int verbose) {

   FILE *input;

   static BuildMapDictionary county_dictionary;

   char *p;
   char  line[1024];
   int   line_count;

   int fips;
   int state_code;
   int county_code;

   RoadMapString state_symbol;
   RoadMapString county_name;


   input = buildus_fips_open (path, "app_a02.txt", verbose);

   county_dictionary = buildmap_dictionary_open ("county");

   line_count = 0;

   while (! feof(input)) {

      if (fgets (line, sizeof(line), input) == NULL) {
         break;
      }

      buildmap_set_line (++line_count);


      /* Skip comment and empty lines. */

      for (p = line; isblank(*p); ++p) ;

      if (isalpha(*p)) {
         continue; /* This is a comment line. */
      }

      if (*p < ' ') {
         continue; /* This is an empty line. */
      }

      if (! isdigit(*p)) {
         buildmap_fatal (0, "invalid line: %s", line);
      }


      /* Retrieve the state's symbol. */

      for (p = line; *p >= ' '; ++p) ; /* Go to the end of the line. */
      *p = 0;

      for (--p; isblank(*p); --p) ; /* Retrieve the state symbol. */
      if (p < line + 6) {
         buildmap_fatal (0, "invalid state symbol in line: %s", line);
      }
      if (*p == '-') {

         char *continuation = p + 1;

         /* Read the continuation line. */
         if (fgets (p, sizeof(line) - (int)(p - line), input) == NULL) {
            buildmap_fatal (0, "no continuation line");
         }
         if (! isblank(*p)) {
            buildmap_fatal (0, "invalid continuation line");
         }
         for (++p; isblank(*p); ++p) ; /* Retrieve the continuation text. */

         /* Remove the duplicated space characters. */
         for (; *p >= ' '; ++p, ++continuation) *continuation = *p;
         *continuation = 0;

         p= continuation; /* Go to the end of the line. */
         *p = 0;

         for (--p; isblank(*p); --p) ; /* Retrieve the state symbol. */
         if (p < line + 6) {
             buildmap_fatal (0, "invalid state symbol in line: %s", line);
         }
      }
      p -= 1;
      if ((! isalpha(p[0])) || (! isalpha(p[1]))) {
         buildmap_fatal (0, "invalid state symbol in line: %s", line);
      }
      state_symbol = buildmap_dictionary_add (BuildMapStateDictionary, p, 2);

      for (--p; isblank(*p); --p) { /* Separate county from state symbol. */
         *p = 0;
      }


      /* Retrieve and decode the state's FIPS code. */

      for (p = line; isblank(*p); ++p) ;

      state_code = atoi(p);


      /* Retrieve and decode the county's FIPS code. */

      for (++p; isdigit(*p); ++p) ; /* Skip the state code. */
      for (++p; isblank(*p); ++p) ; /* Skip the following spaces. */

      if (! isdigit(*p)) {
         buildmap_fatal (0, "invalid line: %s", line);
      }
      county_code = atoi(p);

      fips = state_code * 1000 + county_code;


      /* Retrieve the county's name. */

      for (++p; isdigit(*p); ++p) ; /* Skip the county code. */
      for (++p; isblank(*p); ++p) ; /* Skip the following spaces. */

      county_name = buildmap_dictionary_add (county_dictionary, p, strlen(p));

      buildus_county_add (fips, county_name, state_symbol);
   }

   fclose(input);
}


void buildus_fips_read (char *path, int verbose) {

   buildus_fips_read_states   (path, verbose);
   buildus_fips_read_counties (path, verbose);
}


void buildus_fips_summary (void) {
}

