/* roadmap_nmea.c - Decode a NMEA sentence.
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
 *   See roadmap_nmea.h
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _WIN32
#include <errno.h>
#endif

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_preferences.h"
#include "roadmap_nmea.h"


#define TIGER_COORDINATE_UNIT 1000000


static RoadMapNmeaFields RoadMapNmeaReceived;

static char RoadMapNmeaDate[16];

static RoadMapDynamicStringCollection RoadMapNmeaCollection;


static void safecpy (char *d, const char *s, int length) {

   strncpy (d, s, length);
   d[length-1] = 0;
}


static int hex2bin (char c) {

   if ((c >= '0') && (c <= '9')) {
      return c - '0';
   } else if ((c >= 'A') && (c <= 'F')) {
      return c - ('A' - 10);
   } else if ((c >= 'a') && (c <= 'f')) {
      return c - ('a' - 10);
   }

   /* Invalid character. */
   roadmap_log (ROADMAP_ERROR, "invalid hexadecimal character '%c'", c);
   return 0;
}


static int dec2bin (char c) {

   if ((c >= '0') && (c <= '9')) {
      return c - '0';
   }

   /* Invalid character. */
   roadmap_log (ROADMAP_ERROR, "invalid decimal character '%c'", c);
   return 0;
}


static time_t roadmap_nmea_decode_time (const char *hhmmss,
                                        const char *ddmmyy) {

   static struct tm tm;


   if (sscanf (hhmmss, "%02d%02d%02d",
               &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec)) != 3) {
      return -1;
   }

   if (ddmmyy != NULL) {

      if (sscanf (ddmmyy, "%02d%02d%02d",
                  &(tm.tm_mday), &(tm.tm_mon), &(tm.tm_year)) != 3) {
         return -1;
      }

      if (tm.tm_year < 50) {
         tm.tm_year += 100; /* Y2K. */
      }
      tm.tm_mon -= 1;

   } else if (tm.tm_year == 0) {

      return -1; /* The date is not yet known. */
   }

   /* FIXME: th time zone might change if we are moving !. */

   return mktime(&tm);
}


static int roadmap_nmea_decode_numeric (char *value, int unit) {

   int result;

   if (strchr (value, '.') != NULL) {
      result = (int) (atof(value) * unit);
   } else {
      result = atoi (value) * unit;
   }
   return result;
}


static int roadmap_nmea_decode_coordinate
              (char *value, char *side, char positive, char negative) {

   /* decode longitude & latitude from the nmea format (ddmm.mmmmm)
    * to the format used by the census bureau (dd.dddddd):
    */

   int result;
   char *dot = strchr (value, '.');


   if (dot == NULL) {
      if (value[0] == 0) return 0;
      dot = value + strlen(value);
   }

   dot -= 2;

   result = 0;
   while (value < dot) {
      result = dec2bin(*value) + (10 * result);
      value += 1;
   }
   result *= TIGER_COORDINATE_UNIT;

   result += roadmap_nmea_decode_numeric (dot, TIGER_COORDINATE_UNIT) / 60;

   if (side[1] == 0) {

      if (side[0] == negative) {
         return 0 - result;
      }
      if (side[0] == positive) {
         return result;
      }
   }

   return 0;
}


static char *roadmap_nmea_decode_unit (const char *original) {

    if (strcasecmp (original, "M") == 0) {
        return "cm";
    }

    roadmap_log (ROADMAP_ERROR, "unknown distance unit '%s'", original);
    return "??";
}

typedef int (*RoadMapNmeaDecoder) (int argc, char *argv[]);


static int roadmap_nmea_rmc (int argc, char *argv[]) {

   if (argc <= 9) return 0;

   RoadMapNmeaReceived.rmc.status = *(argv[2]);


   RoadMapNmeaReceived.rmc.fixtime =
      roadmap_nmea_decode_time (argv[1], argv[9]);

   safecpy (RoadMapNmeaDate, argv[9], sizeof(RoadMapNmeaDate));

   if (RoadMapNmeaReceived.rmc.fixtime < 0) return 0;


   RoadMapNmeaReceived.rmc.latitude =
      roadmap_nmea_decode_coordinate  (argv[3], argv[4], 'N', 'S');

   if (RoadMapNmeaReceived.rmc.latitude == 0) return 0;

   RoadMapNmeaReceived.rmc.longitude =
      roadmap_nmea_decode_coordinate (argv[5], argv[6], 'E', 'W');

   if (RoadMapNmeaReceived.rmc.longitude == 0) return 0;


   RoadMapNmeaReceived.rmc.speed =
      roadmap_nmea_decode_numeric (argv[7], 1);

   RoadMapNmeaReceived.rmc.steering =
      roadmap_nmea_decode_numeric (argv[8], 1);

   return 1;
}


static int roadmap_nmea_gga (int argc, char *argv[]) {

   if (argc <= 10) return 0;

   RoadMapNmeaReceived.gga.fixtime =
      roadmap_nmea_decode_time (argv[1], RoadMapNmeaDate);

   if (RoadMapNmeaReceived.gga.fixtime < 0) return 0;

   RoadMapNmeaReceived.gga.latitude =
      roadmap_nmea_decode_coordinate  (argv[2], argv[3], 'N', 'S');

   RoadMapNmeaReceived.gga.longitude =
      roadmap_nmea_decode_coordinate (argv[4], argv[5], 'E', 'W');

   switch (*argv[6]) {

      case '0':
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_INVALID;
         break;

      case '1':
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_GPS;
         break;

      case '2':
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_DGPS;
         break;

      case '3':
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_PPS;
         break;

      default:
         RoadMapNmeaReceived.gga.quality = ROADMAP_NMEA_QUALITY_OTHER;
         break;
   }

   RoadMapNmeaReceived.gga.count =
      roadmap_nmea_decode_numeric (argv[7], 1);

   RoadMapNmeaReceived.gga.dilution =
      roadmap_nmea_decode_numeric (argv[8], 100);

   RoadMapNmeaReceived.gga.altitude =
      roadmap_nmea_decode_numeric (argv[9], 100);

   strcpy (RoadMapNmeaReceived.gga.altitude_unit,
           roadmap_nmea_decode_unit (argv[10]));

   return 1;
}


static int roadmap_nmea_gsa (int argc, char *argv[]) {

   int i;
   int index;
   int last_satellite;

   if (argc <= 2) return 0;

   RoadMapNmeaReceived.gsa.automatic = *(argv[1]);
   RoadMapNmeaReceived.gsa.dimension = atoi(argv[2]);

   /* The last 3 arguments (argc-3 .. argc-1) are not satellites. */
   last_satellite = argc - 4;

   for (index = 2, i = 0;
        index < last_satellite && i < ROADMAP_NMEA_MAX_SATELLITE; ++i) {

      RoadMapNmeaReceived.gsa.satellite[i] = atoi(argv[++index]);
   }
   while (i < ROADMAP_NMEA_MAX_SATELLITE) {
      RoadMapNmeaReceived.gsa.satellite[i++] = 0;
   }

   RoadMapNmeaReceived.gsa.dilution_position   = (float) atof(argv[++index]);
   RoadMapNmeaReceived.gsa.dilution_horizontal = (float) atof(argv[++index]);
   RoadMapNmeaReceived.gsa.dilution_vertical   = (float) atof(argv[++index]);

   return 1;
}


static int roadmap_nmea_gll (int argc, char *argv[]) {

   char mode;
   int  valid_fix;


   if (argc <= 6) return 0;

   /* We have to be extra cautious, as some people report that GPGLL
    * returns a mode field, but some GPS do not have this field at all.
    */
   valid_fix = (strcmp (argv[6], "A") == 0);
   if (argc > 7) {
      valid_fix = (valid_fix && (strcmp(argv[7], "N") != 0));
      mode = *(argv[7]);
   } else {
      mode = 'A'; /* Sensible default. */
   }

   if (valid_fix) {

      RoadMapNmeaReceived.gll.latitude =
         roadmap_nmea_decode_coordinate  (argv[1], argv[2], 'N', 'S');

      RoadMapNmeaReceived.gll.longitude =
         roadmap_nmea_decode_coordinate (argv[3], argv[4], 'E', 'W');

      /* The UTC does not seem to be provided by all GPS vendors,
       * ignore it.
       */

      RoadMapNmeaReceived.gll.status = 'A';
      RoadMapNmeaReceived.gll.mode   = mode;

   } else {
       RoadMapNmeaReceived.gll.status = 'V'; /* bad. */
       RoadMapNmeaReceived.gll.mode   = 'N';
   }

   return 1;
}


static int roadmap_nmea_gsv (int argc, char *argv[]) {

   int i;
   int end;
   int index;


   if (argc <= 3) return 0;

   RoadMapNmeaReceived.gsv.total = (char) atoi(argv[1]);
   RoadMapNmeaReceived.gsv.index = (char) atoi(argv[2]);
   RoadMapNmeaReceived.gsv.count = (char) atoi(argv[3]);

   if (RoadMapNmeaReceived.gsv.count < 0) {
      roadmap_log (ROADMAP_ERROR, "%d is an invalid number of satellites",
                   RoadMapNmeaReceived.gsv.count);
      return 0;
   }

   if (RoadMapNmeaReceived.gsv.count > ROADMAP_NMEA_MAX_SATELLITE) {

      roadmap_log (ROADMAP_ERROR, "%d is too many satellite, %d max supported",
                   RoadMapNmeaReceived.gsv.count,
                   ROADMAP_NMEA_MAX_SATELLITE);
      RoadMapNmeaReceived.gsv.count = ROADMAP_NMEA_MAX_SATELLITE;
   }

   end = RoadMapNmeaReceived.gsv.count
            - ((RoadMapNmeaReceived.gsv.index - 1) * 4);

   if (end > 4) end = 4;

   if (argc <= (end * 4) + 3) {
      return 0;
   }

   for (index = 3, i = 0; i < end; ++i) {

      RoadMapNmeaReceived.gsv.satellite[i] = atoi(argv[++index]);
      RoadMapNmeaReceived.gsv.elevation[i] = atoi(argv[++index]);
      RoadMapNmeaReceived.gsv.azimuth[i]   = atoi(argv[++index]);
      RoadMapNmeaReceived.gsv.strength[i]  = atoi(argv[++index]);
   }

   for (i = end; i < 4; ++i) {
      RoadMapNmeaReceived.gsv.satellite[i] = 0;
      RoadMapNmeaReceived.gsv.elevation[i] = 0;
      RoadMapNmeaReceived.gsv.azimuth[i]   = 0;
      RoadMapNmeaReceived.gsv.strength[i]  = 0;
   }

   return 1;
}


static int roadmap_nmea_pgrmm (int argc, char *argv[]) {

    if (argc <= 1) return 0;

    safecpy (RoadMapNmeaReceived.pgrmm.datum,
             argv[1], sizeof(RoadMapNmeaReceived.pgrmm.datum));

    return 1;
}


static int roadmap_nmea_pgrme (int argc, char *argv[]) {

    if (argc <= 6) return 0;

    RoadMapNmeaReceived.pgrme.horizontal =
        roadmap_nmea_decode_numeric (argv[1], 100);
    strcpy (RoadMapNmeaReceived.pgrme.horizontal_unit,
            roadmap_nmea_decode_unit (argv[2]));

    RoadMapNmeaReceived.pgrme.vertical =
        roadmap_nmea_decode_numeric (argv[3], 100);
    strcpy (RoadMapNmeaReceived.pgrme.vertical_unit,
            roadmap_nmea_decode_unit (argv[4]));

    RoadMapNmeaReceived.pgrme.three_dimensions =
        roadmap_nmea_decode_numeric (argv[5], 100);
    strcpy (RoadMapNmeaReceived.pgrme.three_dimensions_unit,
            roadmap_nmea_decode_unit (argv[6]));

    return 1;
}


static int roadmap_nmea_pxrmadd (int argc, char *argv[]) {

    if (argc <= 3) return 0;

    RoadMapNmeaReceived.pxrmadd.id =
       roadmap_string_new_in_collection (argv[1], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmadd.name =
       roadmap_string_new_in_collection (argv[2], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmadd.sprite =
       roadmap_string_new_in_collection (argv[3], &RoadMapNmeaCollection);

    return 1;
}


static int roadmap_nmea_pxrmmov (int argc, char *argv[]) {

    if (argc <= 7) return 0;

    RoadMapNmeaReceived.pxrmmov.id =
       roadmap_string_new_in_collection (argv[1], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmmov.latitude =
        roadmap_nmea_decode_coordinate  (argv[2], argv[3], 'N', 'S');

    RoadMapNmeaReceived.pxrmmov.longitude =
        roadmap_nmea_decode_coordinate (argv[4], argv[5], 'E', 'W');

    RoadMapNmeaReceived.pxrmmov.speed =
       roadmap_nmea_decode_numeric (argv[6], 1);

    RoadMapNmeaReceived.pxrmmov.steering =
       roadmap_nmea_decode_numeric (argv[7], 1);

    return 1;
}


static int roadmap_nmea_pxrmdel (int argc, char *argv[]) {

    if (argc <= 1) return 0;

    RoadMapNmeaReceived.pxrmdel.id =
       roadmap_string_new_in_collection (argv[1], &RoadMapNmeaCollection);

    return 1;
}


static int roadmap_nmea_pxrmsub (int argc, char *argv[]) {

    int i;
    int j;

    if (argc <= 1) return 0;

    for (i = 1, j = 0; i < argc; ++i, ++j) {
       RoadMapNmeaReceived.pxrmsub.subscribed[j].item =
          roadmap_string_new_in_collection (argv[i], &RoadMapNmeaCollection);
    }

    RoadMapNmeaReceived.pxrmsub.count = j;

    return 1;
}


static int roadmap_nmea_pxrmcfg (int argc, char *argv[]) {

    if (argc < 4) return 0;

    RoadMapNmeaReceived.pxrmcfg.category =
       roadmap_string_new_in_collection (argv[1], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmcfg.name =
       roadmap_string_new_in_collection (argv[2], &RoadMapNmeaCollection);

    RoadMapNmeaReceived.pxrmcfg.value =
       roadmap_string_new_in_collection (argv[3], &RoadMapNmeaCollection);

    return 1;
}


static int roadmap_nmea_pgrmz (int argc, char *argv[]) {

   /* Altitude, 'f' for feet, 2 (altimeter) or 3 (GPS). */
   return 0; /* TBD */
}


/* Empty implementations: save from testing NULL. */

static int roadmap_nmea_null_decoder (int argc, char *argv[]) {

   return 0;
}

static void roadmap_nmea_null_filter   (RoadMapNmeaFields *fields) {}
static void roadmap_nmea_null_listener (void *context,
                                        const RoadMapNmeaFields *fields) {}


#define ROADMAP_NMEA_PHRASE(t,s,d) \
    {t,s,roadmap_nmea_null_decoder,d,roadmap_nmea_null_filter,roadmap_nmea_null_listener}

static struct {

   char               *vendor; /* NULL --> NMEA standard sentence. */
   char               *sentence;
   RoadMapNmeaDecoder  active_decoder;
   RoadMapNmeaDecoder  decoder;
   RoadMapNmeaFilter   filter;
   RoadMapNmeaListener listener;

} RoadMapNmeaPhrase[] = {

   ROADMAP_NMEA_PHRASE(NULL, "RMC", roadmap_nmea_rmc),
   ROADMAP_NMEA_PHRASE(NULL, "GGA", roadmap_nmea_gga),
   ROADMAP_NMEA_PHRASE(NULL, "GSA", roadmap_nmea_gsa),
   ROADMAP_NMEA_PHRASE(NULL, "GSV", roadmap_nmea_gsv),
   ROADMAP_NMEA_PHRASE(NULL, "GLL", roadmap_nmea_gll),

   /* We don't care about these ones (waypoints). */
   ROADMAP_NMEA_PHRASE(NULL, "RTE", roadmap_nmea_null_decoder),
   ROADMAP_NMEA_PHRASE(NULL, "RMB", roadmap_nmea_null_decoder),
   ROADMAP_NMEA_PHRASE(NULL, "BOD", roadmap_nmea_null_decoder),

   /* Garmin extensions: */
   ROADMAP_NMEA_PHRASE("GRM", "E", roadmap_nmea_pgrme),
   ROADMAP_NMEA_PHRASE("GRM", "M", roadmap_nmea_pgrmm),
   ROADMAP_NMEA_PHRASE("GRM", "Z", roadmap_nmea_pgrmz),

   /* RoadMap's own extensions: */
   ROADMAP_NMEA_PHRASE("XRM", "ADD", roadmap_nmea_pxrmadd),
   ROADMAP_NMEA_PHRASE("XRM", "MOV", roadmap_nmea_pxrmmov),
   ROADMAP_NMEA_PHRASE("XRM", "DEL", roadmap_nmea_pxrmdel),
   ROADMAP_NMEA_PHRASE("XRM", "SUB", roadmap_nmea_pxrmsub),
   ROADMAP_NMEA_PHRASE("XRM", "CFG", roadmap_nmea_pxrmcfg),

   { NULL, "", NULL, NULL, NULL, NULL}
};


RoadMapNmeaFilter roadmap_nmea_add_filter (const char *vendor,
                                           const char *sentence,
                                           RoadMapNmeaFilter filter) {

   int i;
   int found;
   RoadMapNmeaFilter previous;


   for (i = 0; RoadMapNmeaPhrase[i].decoder != NULL; ++i) {

       if (strcmp (sentence, RoadMapNmeaPhrase[i].sentence) == 0) {

          if ((vendor == NULL) && (RoadMapNmeaPhrase[i].vendor == NULL)) {
             found = 1;
          }
          else if ((vendor != NULL) &&
                   (RoadMapNmeaPhrase[i].vendor != NULL) &&
                   (strcmp (vendor, RoadMapNmeaPhrase[i].vendor) == 0)) {
             found = 1;
          } else {
             found = 0;
          }

          if (found) {
             previous = RoadMapNmeaPhrase[i].filter;
             RoadMapNmeaPhrase[i].filter = filter;

             return previous;
          }
       }
   }

   if (vendor == NULL) {
      roadmap_log (ROADMAP_FATAL, "unsupported standard NMEA sentence '%s'",
                   sentence);
   } else {
      roadmap_log (ROADMAP_FATAL,
                   "unsupported NMEA sentence '%s' from vendor '%s'",
                   sentence, vendor);
   }
   return (RoadMapNmeaFilter)roadmap_nmea_null_filter;
}


RoadMapNmeaListener roadmap_nmea_subscribe (const char *vendor,
                                            const char *sentence,
                                            RoadMapNmeaListener listener) {

   int i;
   int found;
   RoadMapNmeaListener previous;


   for (i = 0; RoadMapNmeaPhrase[i].decoder != NULL; ++i) {

       if (strcmp (sentence, RoadMapNmeaPhrase[i].sentence) == 0) {

          if ((vendor == NULL) && (RoadMapNmeaPhrase[i].vendor == NULL)) {
             found = 1;
          }
          else if ((vendor != NULL) &&
                   (RoadMapNmeaPhrase[i].vendor != NULL) &&
                   (strcmp (vendor, RoadMapNmeaPhrase[i].vendor) == 0)) {
             found = 1;
          } else {
             found = 0;
          }

          if (found) {
             previous = RoadMapNmeaPhrase[i].listener;
             RoadMapNmeaPhrase[i].listener = listener;

             /* Since someone want to listen to this sentence, it is time
              * to activate the decoder.
              */
             RoadMapNmeaPhrase[i].active_decoder = RoadMapNmeaPhrase[i].decoder;
             return previous;
          }
       }
   }

   if (vendor == NULL) {
      roadmap_log (ROADMAP_FATAL, "unsupported standard NMEA sentence '%s'",
                   sentence);
   } else {
      roadmap_log (ROADMAP_FATAL,
                   "unsupported NMEA sentence '%s' for vendor '%s'",
                   sentence, vendor);
   }
   return (RoadMapNmeaListener)roadmap_nmea_null_listener;
}


static int roadmap_nmea_call (void *context,
                              int index, int count, char *field[]) {

   if ((*RoadMapNmeaPhrase[index].active_decoder) (count, field)) {

      (*RoadMapNmeaPhrase[index].filter)   (&RoadMapNmeaReceived);
      (*RoadMapNmeaPhrase[index].listener) (context, &RoadMapNmeaReceived);

      roadmap_string_release_all (&RoadMapNmeaCollection);

      return 1; /* GPS information was successfully made available. */
   }
   return 0; /* Could not decode it. */
}


int roadmap_nmea_decode (void *context, char *sentence) {

   int i;
   char *p = sentence;

   int   count;
   char *field[80];

   unsigned char checksum = 0;


   /* We skip any leftover from previous transmission problems,
    * check that the '$' is really here, compute the checksum
    * and then decode the "csv" format. */

   while ((*p != '$') && (*p >= ' ')) ++p;

   if (*p != '$') return 0; /* Ignore this ill-formed sentence. */

   sentence = p++;

   while ((*p != '*') && (*p >= ' ')) {
      checksum ^= *p;
      p += 1;
   }

   if (*p == '*') {

      unsigned char mnea_checksum = hex2bin(p[1]) * 16 + hex2bin(p[2]);

      if (mnea_checksum != checksum) {
         roadmap_log (ROADMAP_ERROR,
               "mnea checksum error for '%s' (nmea=%02x, calculated=%02x)",
               sentence,
               mnea_checksum,
               checksum);
      }
   }
   *p = 0;

   for (i = 0, p = sentence; p != NULL && *p != 0; ++i, p = strchr (p, ',')) {
      *p = 0;
      field[i] = ++p;
   }
   count = i;


   /* Now that we have separated each argument of the sentence, retrieve
    * the right decoder & listener functions and call them.
    */
   if (*(field[0]) == 'P') {

      /* This is a proprietary sentence. */

      for (i = 0; RoadMapNmeaPhrase[i].decoder != NULL; ++i) {

         /* Skip standard sentences. */
         if (RoadMapNmeaPhrase[i].vendor == NULL) continue;

         if ((strncmp(RoadMapNmeaPhrase[i].vendor, field[0]+1, 3) == 0) &&
             (strcmp(RoadMapNmeaPhrase[i].sentence, field[0]+4) == 0)) {

            return roadmap_nmea_call (context, i, count, field);
         }
      }
   } else {

      /* This is a standard sentence. */

      for (i = 0; RoadMapNmeaPhrase[i].decoder != NULL; ++i) {

         /* Skip proprietary sentences. */
         if (RoadMapNmeaPhrase[i].vendor != NULL) continue;

         if (strcmp (RoadMapNmeaPhrase[i].sentence, field[0]+2) == 0) {

            return roadmap_nmea_call (context, i, count, field);
         }
      }
   }

   roadmap_log (ROADMAP_DEBUG, "unknown nmea sentence %s\n", field[0]);

   return 0; /* Could not decode it. */
}

