/* roadmap_street.c - Handle streets operations and attributes.
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
 *   see roadmap_street.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap_db_street.h"
#include "roadmap_db_range.h"

#include "roadmap.h"
#include "roadmap_dbread.h"

#include "roadmap_locator.h"

#include "roadmap_math.h"
#include "roadmap_square.h"
#include "roadmap_street.h"
#include "roadmap_shape.h"
#include "roadmap_line.h"
#include "roadmap_dictionary.h"


static char *RoadMapStreetType = "RoadMapStreetContext";
static char *RoadMapZipType    = "RoadMapZipContext";
static char *RoadMapRangeType  = "RoadMapRangeContext";

typedef struct {

   char *type;

   RoadMapStreet        *RoadMapStreets;
   char                 *RoadMapStreetType;
   int                   RoadMapStreetsCount;

} RoadMapStreetContext;

typedef struct {

   char *type;

   int *RoadMapZipCode;
   int  RoadMapZipCodeCount;

} RoadMapZipContext;

typedef struct {

   char *type;

   RoadMapRangeByStreet *RoadMapByStreet;
   int                   RoadMapByStreetCount;

   RoadMapRangeByCity *RoadMapByCity;
   int                 RoadMapByCityCount;

   RoadMapRangePlace *RoadMapPlace;
   int                RoadMapPlaceCount;

   RoadMapRangeByZip *RoadMapByZip;
   int                RoadMapByZipCount;

   RoadMapRangeBySquare *bysquare;
   int                   square_count;

   RoadMapRange *RoadMapAddr;
   int           RoadMapAddrCount;

   RoadMapRangeNoAddress *noaddr;
   int                    noaddr_count;

   RoadMapDictionary RoadMapCityNames;
   RoadMapDictionary RoadMapStreetPrefix;
   RoadMapDictionary RoadMapStreetNames;
   RoadMapDictionary RoadMapStreetType;
   RoadMapDictionary RoadMapStreetSuffix;

} RoadMapRangeContext;


static RoadMapStreetContext *RoadMapStreetActive = NULL;
static RoadMapZipContext    *RoadMapZipActive    = NULL;
static RoadMapRangeContext  *RoadMapRangeActive  = NULL;


static void *roadmap_street_map (roadmap_db *root) {

   RoadMapStreetContext *context;

   roadmap_db *table;


   context = malloc (sizeof(RoadMapStreetContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_FATAL, "no more memory");
   }
   context->type = RoadMapStreetType;

   table = roadmap_db_get_subsection (root, "name");
   context->RoadMapStreets = (RoadMapStreet *) roadmap_db_get_data (table);
   context->RoadMapStreetsCount = roadmap_db_get_count(table);

   if (context->RoadMapStreetsCount !=
       roadmap_db_get_size (table) / sizeof(RoadMapStreet)) {
      roadmap_log (ROADMAP_FATAL,
                   "invalid street structure (%d != %d / %d)",
                   context->RoadMapStreetsCount,
                   roadmap_db_get_size (table),
                   sizeof(RoadMapStreet));
   }

   table = roadmap_db_get_subsection (root, "type");
   context->RoadMapStreetType = (char *) roadmap_db_get_data (table);

   if (roadmap_db_get_count(table) != context->RoadMapStreetsCount) {
      roadmap_log (ROADMAP_FATAL, "inconsistent count of street");
   }

   return context;
}

static void roadmap_street_activate (void *context) {

   RoadMapStreetContext *this = (RoadMapStreetContext *) context;

   if (this->type != RoadMapStreetType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (bad context type)");
   }
   RoadMapStreetActive = this;
}

static void roadmap_street_unmap (void *context) {

   RoadMapStreetContext *this = (RoadMapStreetContext *) context;

   if (this->type != RoadMapStreetType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapStreetActive == this) {
      RoadMapStreetActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapStreetHandler = {
   "street",
   roadmap_street_map,
   roadmap_street_activate,
   roadmap_street_unmap
};


static void *roadmap_street_zip_map (roadmap_db *root) {

   RoadMapZipContext *context;


   context = malloc (sizeof(RoadMapZipContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_FATAL, "no more memory");
   }
   context->type = RoadMapZipType;

   context->RoadMapZipCode = (int *) roadmap_db_get_data (root);
   context->RoadMapZipCodeCount = roadmap_db_get_count(root);

   if (context->RoadMapZipCodeCount !=
       roadmap_db_get_size (root) / sizeof(int)) {
      roadmap_log (ROADMAP_FATAL, "invalid zip structure");
   }

   return context;
}

static void roadmap_street_zip_activate (void *context) {

   RoadMapZipContext *this = (RoadMapZipContext *) context;

   if (this->type != RoadMapZipType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (bad context type)");
   }
   RoadMapZipActive = this;
}

static void roadmap_street_zip_unmap (void *context) {

   RoadMapZipContext *this = (RoadMapZipContext *) context;

   if (this->type != RoadMapZipType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapZipActive == this) {
      RoadMapZipActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapZipHandler = {
   "zip",
   roadmap_street_zip_map,
   roadmap_street_zip_activate,
   roadmap_street_zip_unmap
};


static void *roadmap_street_range_map (roadmap_db *root) {

   RoadMapRangeContext *context;
   roadmap_db *table;


   context = malloc (sizeof(RoadMapRangeContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_FATAL, "no more memory");
   }
   context->type = RoadMapRangeType;
   context->RoadMapStreetPrefix = NULL;
   context->RoadMapStreetNames  = NULL;
   context->RoadMapStreetType   = NULL;
   context->RoadMapStreetSuffix = NULL;
   context->RoadMapCityNames    = NULL;

   table = roadmap_db_get_subsection (root, "bystreet");
   context->RoadMapByStreet =
       (RoadMapRangeByStreet *) roadmap_db_get_data (table);
   context->RoadMapByStreetCount = roadmap_db_get_count (table);

   if (context->RoadMapByStreetCount !=
       roadmap_db_get_size (table) / sizeof(RoadMapRangeByStreet)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/bystreet structure");
   }

   table = roadmap_db_get_subsection (root, "bycity");
   context->RoadMapByCity = (RoadMapRangeByCity *) roadmap_db_get_data (table);
   context->RoadMapByCityCount = roadmap_db_get_count (table);

   if (context->RoadMapByCityCount !=
       roadmap_db_get_size (table) / sizeof(RoadMapRangeByCity)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/bycity structure");
   }

   table = roadmap_db_get_subsection (root, "place");
   context->RoadMapPlace = (RoadMapRangePlace *) roadmap_db_get_data (table);
   context->RoadMapPlaceCount = roadmap_db_get_count (table);

   if (context->RoadMapPlaceCount !=
       roadmap_db_get_size (table) / sizeof(RoadMapRangePlace)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/place structure");
   }

   table = roadmap_db_get_subsection (root, "byzip");
   context->RoadMapByZip = (RoadMapRangeByZip *) roadmap_db_get_data (table);
   context->RoadMapByZipCount = roadmap_db_get_count (table);

   if (context->RoadMapByZipCount !=
       roadmap_db_get_size (table) / sizeof(RoadMapRangeByZip)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/byzip structure");
   }

   table = roadmap_db_get_subsection (root, "bysquare");
   context->bysquare = (RoadMapRangeBySquare *) roadmap_db_get_data (table);
   context->square_count = roadmap_db_get_count (table);

   if (context->square_count !=
       roadmap_db_get_size (table) / sizeof(RoadMapRangeBySquare)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/bysquare structure");
   }

   table = roadmap_db_get_subsection (root, "addr");
   context->RoadMapAddr = (RoadMapRange *) roadmap_db_get_data (table);
   context->RoadMapAddrCount = roadmap_db_get_count (table);

   if (context->RoadMapAddrCount !=
       roadmap_db_get_size (table) / sizeof(RoadMapRange)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/addr structure");
   }

   table = roadmap_db_get_subsection (root, "noaddr");
   context->noaddr = (RoadMapRangeNoAddress *) roadmap_db_get_data (table);
   context->noaddr_count = roadmap_db_get_count (table);

   if (context->noaddr_count !=
       roadmap_db_get_size (table) / sizeof(RoadMapRangeNoAddress)) {
      roadmap_log (ROADMAP_FATAL, "invalid range/noaddr structure");
   }

   return context;
}

static void roadmap_street_range_activate (void *context) {

   RoadMapRangeContext *this = (RoadMapRangeContext *) context;

   if (this->type != RoadMapRangeType) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (bad context type)");
   }
   RoadMapRangeActive = this;

   if (this->RoadMapStreetPrefix == NULL) {
      this->RoadMapStreetPrefix = roadmap_dictionary_open ("prefix");
   }
   if (this->RoadMapStreetNames == NULL) {
      this->RoadMapStreetNames = roadmap_dictionary_open ("street");
   }
   if (this->RoadMapStreetType == NULL) {
      this->RoadMapStreetType = roadmap_dictionary_open ("type");
   }
   if (this->RoadMapStreetSuffix == NULL) {
      this->RoadMapStreetSuffix = roadmap_dictionary_open ("suffix");
   }
   if (this->RoadMapCityNames == NULL) {
      this->RoadMapCityNames = roadmap_dictionary_open ("city");
   }
   if (this->RoadMapStreetNames  == NULL ||
       this->RoadMapStreetPrefix == NULL ||
       this->RoadMapStreetType   == NULL ||
       this->RoadMapStreetSuffix == NULL ||
       this->RoadMapCityNames    == NULL) {
      roadmap_log (ROADMAP_FATAL, "cannot open dictionary");
   }
}

static void roadmap_street_range_unmap (void *context) {

   RoadMapRangeContext *this = (RoadMapRangeContext *) context;

   if (this->type != RoadMapRangeType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapRangeActive == this) {
      RoadMapRangeActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapRangeHandler = {
   "range",
   roadmap_street_range_map,
   roadmap_street_range_activate,
   roadmap_street_range_unmap
};


typedef struct roadmap_street_identifier {

   RoadMapString prefix;
   RoadMapString name;
   RoadMapString suffix;
   RoadMapString type;

} RoadMapStreetIdentifier;


static void roadmap_street_locate (const char *name,
                                   RoadMapStreetIdentifier *street) {

   char *space;
   char  buffer[128];

   street->prefix = 0;
   street->suffix = 0;
   street->type   = 0;

   /* Search for a prefix. If found, remove the prefix from the name
    * by shifting the name's pointer.
    */
   space = strchr (name, ' ');

   if (space != NULL) {

      int  length;

      length = space - name;

      if (length < sizeof(buffer)) {

         strncpy (buffer, name, length);
         buffer[length] = 0;

         street->prefix =
            roadmap_dictionary_locate
               (RoadMapRangeActive->RoadMapStreetPrefix, buffer);

         if (street->prefix > 0) {
            name = name + length;
            while (*name == ' ') ++name;
         }
      }

      /* Search for a street type. If found, extract the street name
       * and stor it in the temporary buffer, which will be substituted
       * for the name.
       */
      space = strrchr (name, ' ');

      if (space != NULL) {

         char *trailer;

         trailer = space + 1;

         street->type =
            roadmap_dictionary_locate
               (RoadMapRangeActive->RoadMapStreetType, trailer);

         if (street->type > 0) {

            length = space - name;

            if (length < sizeof(buffer)) {

               strncpy (buffer, name, length);
               buffer[length] = 0;

               while (buffer[length-1] == ' ') buffer[--length] = 0;

               space = strrchr (buffer, ' ');

               if (space != NULL) {

                  trailer = space + 1;

                  street->suffix =
                     roadmap_dictionary_locate
                        (RoadMapRangeActive->RoadMapStreetSuffix, trailer);

                  if (street->suffix > 0) {

                     while (*space == ' ') {
                        *space = 0;
                        --space;
                     }
                  }
               }

               name = buffer;
            }
         }
      }
   }

   street->name =
      roadmap_dictionary_locate (RoadMapRangeActive->RoadMapStreetNames, name);
}


static int roadmap_street_block_by_county_subdivision
              (RoadMapStreetIdentifier *street,
               int city,
               RoadMapBlocks *blocks, int size) {

   int i;
   int j;
   int count = 0;
   int range_count = RoadMapRangeActive->RoadMapByStreetCount;

   for (i = 0; i < range_count; i++) {

      RoadMapStreet *this_street = RoadMapStreetActive->RoadMapStreets + i;
      RoadMapRangeByStreet *by_street = RoadMapRangeActive->RoadMapByStreet + i;

      if (this_street->fename == street->name &&
          (street->prefix == 0 || this_street->fedirp == street->prefix) &&
          (street->suffix == 0 || this_street->fedirs == street->suffix) &&
          (street->type   == 0 || this_street->fetype == street->type  ) &&
          by_street->count_range >= 0) {

         int range_index;
         int range_end;

         range_index = by_street->first_range;
         range_end  =
             by_street->first_range + by_street->count_range;

         for (j = by_street->first_city; range_index < range_end; j++) {

            RoadMapRangeByCity *by_city = RoadMapRangeActive->RoadMapByCity + j;

            if ((city < 0) || (by_city->city == city)) {

               blocks[count].street = i;
               blocks[count].first  = range_index;
               blocks[count].count  = by_city->count;

               if (++count >= size) {
                  return count;
               }
               break;
            }

            range_index += by_city->count;
         }
      }
   }
   return count;
}


int roadmap_street_blocks_by_city
       (char *street_name, char *city_name, RoadMapBlocks *blocks, int size) {

   int i;
   int count;
   int total = 0;
   int place_count = RoadMapRangeActive->RoadMapPlaceCount;

   int city;
   RoadMapStreetIdentifier street;


   roadmap_street_locate (street_name, &street);
   if (street.name <= 0) {
      return ROADMAP_STREET_NOSTREET;
   }

   if (city_name[0] == '?') {

      city = -1;

   } else {

      city = roadmap_dictionary_locate (RoadMapRangeActive->RoadMapCityNames,
                                           city_name);
      if (city <= 0) {
            return ROADMAP_STREET_NOCITY;
      }
   }

   /* FIXME: at this time the place -> city mapping is totally unreliable,
    * because the two do not always match. For example, the Pasadena city
    * and the Los Angeles place intersect, which cause all of Pasadena to
    * be included in Los Angeles. Bummer..
    * The long term solution is to have two indexes: one for cities and one
    * for places.
    *
    * START OF FILTERED CODE.
   for (i = 0; i < place_count; i++) {

      RoadMapRangePlace *this_place = RoadMapRangeActive->RoadMapPlace + i;

      if ((city < 0) ||
          ((this_place->place == city) && (this_place->city  != city))) {

         count = roadmap_street_block_by_county_subdivision
                              (&street, this_place->city, blocks, size);

         if (count > 0) {

            total  += count;

            blocks += count;
            size   -= count;

            if (size <= 0) {
               return total;
            }
         }
      }
   }
   * END OF FILTERED CODE. */

   count = roadmap_street_block_by_county_subdivision
                                (&street, city, blocks, size);

   if (count > 0) {
      total += count;
   }

   return total;
}


int roadmap_street_blocks_by_zip
       (char *street_name, int zip, RoadMapBlocks *blocks, int size) {

   int i;
   int j;
   int zip_index = 0;
   int zip_count = RoadMapZipActive->RoadMapZipCodeCount;
   int range_count = RoadMapRangeActive->RoadMapByStreetCount;
   int count = 0;

   RoadMapStreetIdentifier street;


   roadmap_street_locate (street_name, &street);

   if (street.name <= 0) {
      return ROADMAP_STREET_NOSTREET;
   }

   for (i = 1; i < zip_count; i++) {
      if (RoadMapZipActive->RoadMapZipCode[i] == zip) {
         zip_index = i;
         break;
      }
   }
   if (zip_index <= 0) {
      return ROADMAP_STREET_NOSTREET;
   }

   for (i = 0; i < range_count; i++) {

      RoadMapStreet *this_street = RoadMapStreetActive->RoadMapStreets + i;
      RoadMapRangeByStreet *by_street = RoadMapRangeActive->RoadMapByStreet + i;

      if (this_street->fename == street.name && by_street->count_range >= 0) {

         int range_index;
         int range_end;

         range_index = by_street->first_range;
         range_end  = by_street->first_range + by_street->count_range;

         for (j = by_street->first_zip; range_index < range_end; j++) {

            RoadMapRangeByZip *by_zip = RoadMapRangeActive->RoadMapByZip + j;

            if (by_zip->zip == zip_index) {

               blocks[count].street = i;
               blocks[count].first  = range_index;
               blocks[count].count  = by_zip->count;

               if (++count >= size) {
                  return count;
               }
               break;
            }

            range_index += by_zip->count;
         }
      }
   }
   return count;
}


int roadmap_street_get_ranges
       (RoadMapBlocks *blocks, int count, RoadMapStreetRange *ranges) {

   int i;
   int end;

   int fradd;
   int toadd;

   int total = 0;


   for (i = blocks->first, end = blocks->first + blocks->count; i < end; i++) {

      RoadMapRange *this_addr = RoadMapRangeActive->RoadMapAddr + i;

      if (HAS_CONTINUATION(this_addr)) {

         fradd = ((int)(this_addr->fradd) & 0xffff)
                    + (((int)(this_addr[1].fradd) & 0xffff) << 16);
         toadd = ((int)(this_addr->toadd) & 0xffff)
                    + (((int)(this_addr[1].toadd) & 0xffff) << 16);

      } else {

         fradd = this_addr->fradd;
         toadd = this_addr->toadd;
      }

      if (total >= count) return total;

      if (fradd > toadd) {
         ranges->min = toadd;
         ranges->max = fradd;
      } else {
         ranges->min = fradd;
         ranges->max = toadd;
      }
      ranges += 1;
      total  += 1;
   }
   return total;
}


int roadmap_street_get_position (RoadMapBlocks *blocks,
                                 int number,
                                 RoadMapPosition *position) {

   int i;
   int end;
   int number_max;
   int number_min;

   int fradd;
   int toadd;

   RoadMapPosition from;
   RoadMapPosition to;


   for (i = blocks->first, end = blocks->first + blocks->count; i < end; i++) {

      RoadMapRange *this_addr = RoadMapRangeActive->RoadMapAddr + i;

      if ((number & 1) != (this_addr->fradd & 1)) continue;

      if (HAS_CONTINUATION(this_addr)) {

         fradd = ((int)(this_addr->fradd) & 0xffff)
                    + (((int)(this_addr[1].fradd) & 0xffff) << 16);
         toadd = ((int)(this_addr->toadd) & 0xffff)
                    + (((int)(this_addr[1].toadd) & 0xffff) << 16);

      } else {

         fradd = this_addr->fradd;
         toadd = this_addr->toadd;
      }

      if (fradd > toadd) {
         number_max = fradd;
         number_min = toadd;
      } else {
         number_max = toadd;
         number_min = fradd;
      }

      if (number >= number_min && number <= number_max) {

         int line = this_addr->line & (~ CONTINUATION_FLAG);

         roadmap_line_from (line, &from);
         roadmap_line_to   (line, &to);

         position->longitude =
            from.longitude -
               ((from.longitude - to.longitude)
                  * abs(fradd - number)) / (number_max - number_min);

         position->latitude =
            from.latitude -
               ((from.latitude - to.latitude)
                  * abs(fradd - number)) / (number_max - number_min);

         return line;
      }
   }
   return -1;
}


static int roadmap_street_get_distance_with_shape
              (RoadMapPosition *position,
               int  line, int first_shape, int last_shape) {

   int i;
   RoadMapPosition position1;
   RoadMapPosition position2;

   int distance;
   int minimum = 0x7fffffff;

   /* Note: the position of a shape point is relative to the position
    * of the previous point, starting with the from point.
    * The logic here takes care of this property.
    */
   roadmap_line_from (line, &position2);

   for (i = first_shape; i <= last_shape; i++) {

      position1 = position2;

      roadmap_shape_get_position (i, &position2);

      if (roadmap_math_line_is_visible (&position1, &position2)) {

         distance =
            roadmap_math_get_distance_from_segment
                              (position, &position1, &position2);

         if (distance < minimum) {
            minimum = distance;
         }
      }
   }

   roadmap_line_to (line, &position1);

   if (roadmap_math_line_is_visible (&position1, &position2)) {

      distance =
         roadmap_math_get_distance_from_segment
                           (position, &position1, &position2);

      if (distance < minimum) {
         minimum = distance;
      }
   }

   return minimum;
}


static int roadmap_street_get_distance_no_shape
              (RoadMapPosition *position, int line) {

   RoadMapPosition position1;
   RoadMapPosition position2;


   roadmap_line_from (line, &position1);
   roadmap_line_to   (line, &position2);

   if (roadmap_math_line_is_visible (&position1, &position2)) {

      return roadmap_math_get_distance_from_segment
                            (position, &position1, &position2);
   }

   return 0x7fffffff;
}


int roadmap_street_get_distance (RoadMapPosition *position, int line) {

   int square;
   int first_shape_line;
   int last_shape_line;
   int first_shape;
   int last_shape;

   square = roadmap_square_search (position);

   if (roadmap_shape_in_square (square, &first_shape_line,
                                        &last_shape_line) > 0) {

      if (roadmap_shape_of_line (line, first_shape_line,
                                       last_shape_line,
                                       &first_shape, &last_shape) > 0) {

         return roadmap_street_get_distance_with_shape
                   (position, line, first_shape, last_shape);
      }
   }
   return roadmap_street_get_distance_no_shape (position, line);
}


static int roadmap_street_get_closest_in_square
              (RoadMapPosition *position,
               int square, int cfcc, int *distance) {

   int line;
   int this_distance;
   int first_line;
   int last_line;
   int first_shape_line;
   int last_shape_line;
   int first_shape;
   int last_shape;

   int closest = -1;


   if (roadmap_line_in_square (square, cfcc, &first_line, &last_line) > 0) {

      if (roadmap_shape_in_square (square, &first_shape_line,
                                           &last_shape_line) > 0) {

         for (line = first_line; line <= last_line; line++) {

            if (roadmap_shape_of_line (line, first_shape_line,
                                             last_shape_line,
                                             &first_shape, &last_shape) > 0) {

               this_distance =
                  roadmap_street_get_distance_with_shape
                     (position, line, first_shape, last_shape);

            } else {
               this_distance =
                   roadmap_street_get_distance_no_shape (position, line);
            }
            if (this_distance < *distance) {
               closest = line;
               *distance = this_distance;
            }
         }

      } else {

         for (line = first_line; line <= last_line; line++) {

            this_distance =
               roadmap_street_get_distance_no_shape (position, line);

            if (this_distance < *distance) {
               closest = line;
               *distance = this_distance;
            }
         }
      }
   }

   return closest;
}


int roadmap_street_get_closest
       (RoadMapPosition *position, int count, int *categories, int *distance) {

   static int *fips = NULL;

   int i;
   int county;
   int county_count;
   int square;
   int line;

   int closest_fips = -1;
   int closest_line = -1;

   *distance = 0x7fffffff;

   county_count = roadmap_locator_by_position (position, &fips);

   /* - For each candidate county: */

   for (county = county_count - 1; county >= 0; --county) {

      /* -- Access the county's database. */

      if (roadmap_locator_activate (fips[county]) != ROADMAP_US_OK) continue;

      /* -- Look for the square the current location fits in. */

      square = roadmap_square_search (position);

      if (square >= 0) {

         /* The current location fits in one of the county's squares.
          * We might be in that county, search for the closest street.
          */

         for (i = 0; i < count; ++i) {

            line = roadmap_street_get_closest_in_square
                      (position, square, categories[i], distance);

            if (line >= 0) {
               closest_fips = fips[county];
               closest_line = line;
            }
         }
      }
   }

   if (closest_fips > 0) {
      roadmap_locator_activate (closest_fips);
   }
   return closest_line;
}


static int roadmap_street_check_street (int street, int line) {

   int i;

   RoadMapRange *range = RoadMapRangeActive->RoadMapAddr;

   RoadMapRangeByStreet *by_street =
      RoadMapRangeActive->RoadMapByStreet + street;

   int range_end = by_street->first_range + by_street->count_range;


   for (i = by_street->first_range; i < range_end; i++) {

      if (range[i].line == line) return i;
   }

   return -1;
}


static int roadmap_street_get_city (int street, int range) {

   int i;
   int next;

   RoadMapRangeByStreet *by_street =
      RoadMapRangeActive->RoadMapByStreet + street;

   RoadMapRangeByCity *by_city =
      RoadMapRangeActive->RoadMapByCity + by_street->first_city;

   int range_end = by_street->first_range + by_street->count_range;


   for (i = by_street->first_range; i < range_end; i = next) {

      next = i + by_city->count;

      if (range >= i && range < next) {
         return by_city->city; /* We found the city we was looking for. */
      }
      by_city += 1;
   }

   return 0; /* No city: return an empty string. */
}


char *roadmap_street_get_name_from_line (int line) {

   static char RoadMapStreetName [512];

   RoadMapStreet *this_street;
   RoadMapRangeBySquare *bysquare;
   RoadMapRangeNoAddress *noaddr;

   char *p;

   int city = 0;
   int range_index = -1;
   int hole;
   int stop;
   int street;
   int square;
   RoadMapPosition position;

   char address_range_image[256];


   RoadMapStreetName[0] = 0;
   address_range_image[0] = 0;

   roadmap_line_from (line, &position);
   square = roadmap_square_search (&position);

   if (square >= 0) {
      square = roadmap_square_index (square);
   }

   if (square >= 0) {

      bysquare = RoadMapRangeActive->bysquare + square;

      street = 0;

      for (hole = 0; hole < ROADMAP_RANGE_HOLES; hole++) {

         stop = street + bysquare->hole[hole].included;

         while (street < stop) {

            range_index = roadmap_street_check_street (street, line);
            if (range_index >= 0) {
               goto found_street_with_address;
            }
            street += 1;
         }

         street += bysquare->hole[hole].excluded;
      }

      stop = RoadMapRangeActive->RoadMapByStreetCount;

      while (street < stop) {

         range_index = roadmap_street_check_street (street, line);
         if (range_index >= 0) {
            goto found_street_with_address;
         }
         street += 1;
      }

      /* Could not find the street: maybe it has no address. */

      noaddr = RoadMapRangeActive->noaddr;
      stop   = bysquare->noaddr_start + bysquare->noaddr_count;

      for (street = bysquare->noaddr_start; street < stop; street++) {

         if (noaddr[street].line == line) {
            street = noaddr[street].street;
            goto found_street;
         }
      }
   }

   return ""; /* Really found nothing. */


found_street_with_address:

   {
      int address_min = 0;
      int address_max = 0;

      address_min = RoadMapRangeActive->RoadMapAddr[range_index].fradd;
      address_max = RoadMapRangeActive->RoadMapAddr[range_index].toadd;

      if (address_min > address_max) {

         int tmp = address_min;

         address_min = address_max;
         address_max = tmp;
      }
      snprintf (address_range_image,
                sizeof(address_range_image),
                "%d - %d,", address_min, address_max);
   }
   city = roadmap_street_get_city (street, range_index);

found_street:

   this_street = RoadMapStreetActive->RoadMapStreets + street;

   snprintf (RoadMapStreetName, sizeof(RoadMapStreetName),
             "%s%s %s %s %s%s%s",
             address_range_image,
             roadmap_dictionary_get
                (RoadMapRangeActive->RoadMapStreetPrefix, this_street->fedirp),
             roadmap_dictionary_get
                (RoadMapRangeActive->RoadMapStreetNames, this_street->fename),
             roadmap_dictionary_get
                (RoadMapRangeActive->RoadMapStreetType, this_street->fetype),
             roadmap_dictionary_get
                (RoadMapRangeActive->RoadMapStreetSuffix, this_street->fedirs),
             (city > 0) ? ", " : "",
             roadmap_dictionary_get
                (RoadMapRangeActive->RoadMapCityNames, city));

   /* trim the resulting string on both sides (right then left): */

   p = RoadMapStreetName + strlen(RoadMapStreetName) - 1;
   while (*p == ' ') *(p--) = 0;

   for (p = RoadMapStreetName; *p == ' '; p++) ;

   return p;
}

