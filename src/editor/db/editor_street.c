/* editor_street.c - street databse layer
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *   See editor_street.h
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"

#include "../editor_log.h"
#include "../editor_main.h"

#include "editor_db.h"
#include "editor_shape.h"
#include "editor_line.h"
#include "editor_square.h"
#include "editor_dictionary.h"
#include "editor_trkseg.h"
#include "editor_point.h"

#include "editor_street.h"

static editor_db_section *ActiveRangeDB;
static editor_db_section *ActiveStreetDB;

static EditorDictionary ActiveStreetDictionary;
static EditorDictionary ActiveCityDictionary;
static EditorDictionary ActiveTypeDictionary;
static EditorDictionary ActiveZipDictionary;
static EditorDictionary ActiveT2SDictionary;

static void editor_street_activate (void *context) {
   ActiveStreetDB = (editor_db_section *) context;
   ActiveStreetDictionary = editor_dictionary_open ("streets");
   ActiveTypeDictionary = editor_dictionary_open ("types");
   ActiveCityDictionary = editor_dictionary_open ("cities");
   ActiveZipDictionary = editor_dictionary_open ("zips");
   ActiveT2SDictionary = editor_dictionary_open ("t2s");
}

roadmap_db_handler EditorStreetHandler = {
   "streets",
   editor_map,
   editor_street_activate,
   editor_unmap
};

static void editor_range_activate (void *context) {
   ActiveRangeDB = (editor_db_section *) context;
}

roadmap_db_handler EditorRangeHandler = {
   "ranges",
   editor_map,
   editor_range_activate,
   editor_unmap
};


int editor_street_create (const char *_name,
                          const char *_type,
                          const char *_prefix,
                          const char *_suffix) {

   int street_id;
   editor_db_street street;

   street.fename =
      editor_dictionary_locate (ActiveStreetDictionary, _name);
   street.fetype =
      editor_dictionary_locate (ActiveTypeDictionary, _type);
   street.fedirp = -1;
   street.fedirs = -1;

   //TODO optimize - this is a linear search

   if (street.fename != -1) {
      int i;

      for (i=0; i < ActiveStreetDB->num_items; i++) {
         editor_db_street *street_ptr =
            (editor_db_street *) editor_db_get_item
                        (ActiveStreetDB, i, 0, NULL);

         assert (street_ptr != NULL);

         if ((street_ptr != NULL) &&
               !memcmp (street_ptr, &street, sizeof (editor_db_street))) {
            return i;
         }
      }
   } else {

      street.fename = 
         editor_dictionary_add
            (ActiveStreetDictionary, _name, strlen(_name));

      street.fetype = 
         editor_dictionary_add
            (ActiveTypeDictionary, _type, strlen(_type));
   }

   street_id = editor_db_add_item (ActiveStreetDB, &street);

   if (street_id == -1) {

      editor_db_grow ();
      street_id = editor_db_add_item (ActiveStreetDB, &street);
   }

   return street_id;
}


int editor_street_create_city (const char *name) {

   return
      editor_dictionary_add
         (ActiveCityDictionary, name, strlen(name));
}


int editor_street_create_zip (const char *name) {

   return
      editor_dictionary_add
         (ActiveZipDictionary, name, strlen(name));
}


const char *editor_street_get_name (int street_id) {

   editor_db_street *street;

   street = (editor_db_street *) editor_db_get_item
                           (ActiveStreetDB, street_id, 0, NULL);

   if ((street == NULL) || (street->fename == -1)) {
      return NULL;
   }

   return editor_dictionary_get (ActiveStreetDictionary, street->fename);
}


static int editor_street_get_distance_with_shape
              (const RoadMapPosition *position,
               int line,
               int fips,
               RoadMapNeighbour *neighbour) {

   RoadMapPosition from;
   RoadMapPosition to;
   int first_shape;
   int last_shape;
   int cfcc;
   int trkseg;
   int i;
   int smallest_distance;
   RoadMapNeighbour current;

   editor_line_get (line, &from, &to, &trkseg, &cfcc, NULL);

   roadmap_plugin_set_line (&current.line, EditorPluginID, line, cfcc, fips);
   smallest_distance = 0x7fffffff;
   current.from = from;

   editor_trkseg_get (trkseg, &i, &first_shape, &last_shape, NULL);
   editor_point_position (i, &current.to);

   if (first_shape == -1) {
      /* skip the for loop */
      last_shape = -2;
   }
      
   for (i = first_shape; i <= last_shape; i++) {

      editor_shape_position (i, &current.to);

      if (roadmap_math_line_is_visible (&current.from, &current.to)) {

         current.distance =
            roadmap_math_get_distance_from_segment
               (position, &current.from, &current.to, &current.intersection);

         if (current.distance < smallest_distance) {
            smallest_distance = current.distance;
            *neighbour = current;
         }
      }

      current.from = current.to;
   }

   current.to = to;

   if (roadmap_math_line_is_visible (&current.from, &current.to)) {
      current.distance =
         roadmap_math_get_distance_from_segment
            (position, &current.to, &current.from, &current.intersection);

      if (current.distance < smallest_distance) {
         smallest_distance = current.distance;
         *neighbour = current;
      }
   }

   return smallest_distance < 0x7fffffff;
}


int editor_street_get_distance (const RoadMapPosition *position,
                                const PluginLine *line,
                                RoadMapNeighbour *result) {

   int fips = editor_db_locator (position);

   if (editor_db_activate (fips) == -1) return 0;

   return editor_street_get_distance_with_shape
      (position,
       roadmap_plugin_get_line_id (line),
       roadmap_plugin_get_fips (line),
       result);

}


int editor_street_get_closest (const RoadMapPosition *position,
                               int *categories,
                               int categories_count,
                               RoadMapNeighbour *neighbours,
                               int count,
                               int max) {
   
   int i;
   int j;
   int square[5];
   int square_count;
   int square_cfccs;
   int min_category = 256;
   int lines_count;
   RoadMapNeighbour this;
   int found = 0;
   int fips = editor_db_locator (position);

   if (editor_db_activate(fips) == -1) {
      return count;
   }

   for (i = 0; i < categories_count; ++i) {
        if (min_category > categories[i]) min_category = categories[i];
   }

   square_count =
      editor_square_find_by_position (position, square, 5, 0);
   
   for (j=0; j<square_count; j++) {
      square_cfccs = editor_square_get_cfccs (square[j]);

      if (! (square_cfccs && (-1 << min_category))) {
         return count;
      }

      lines_count = editor_square_get_num_lines (square[j]);
   
      for (i=0; i<lines_count; i++) {

         RoadMapPosition from;
         RoadMapPosition to;
         int cfcc;
         int line;
         int flag;

         line = editor_square_get_line (square[j], i);
         editor_line_get (line, &from, &to, NULL, &cfcc, &flag);

         if (flag & ED_LINE_DELETED) continue;
         if (cfcc < min_category) continue;

            found = editor_street_get_distance_with_shape
                                 (position, line, fips, &this);

            if (found) {
               count = roadmap_street_replace (neighbours, count, max, &this);
            }
       }
   }

   return count;
}

int editor_street_get_connected_lines (const RoadMapPosition *crossing,
                                       PluginLine *plugin_lines,
                                       int size) {

   int square;
   int square_cfccs;
   int lines_count;
   int count = 0;
   int i;

   /* FIXME - this is wrong */
   int fips = roadmap_locator_active ();
   
   if (editor_db_activate (fips) == -1) return 0;

   editor_square_find_by_position (crossing, &square, 1, 0);
   square_cfccs = editor_square_get_cfccs (square);

   //FIXME:
#if 0
   if (! (square_cfccs && (-1 << ROADMAP_ROAD_FIRST))) {
      return count;
   }
#endif
   lines_count = editor_square_get_num_lines (square);
   
   for (i=0; i<lines_count; i++) {

      RoadMapPosition line_from;
      RoadMapPosition line_to;
      int cfcc;
      int line;
      int flag;

      line = editor_square_get_line (square, i);
      editor_line_get
         (line, &line_from, &line_to, NULL, &cfcc, &flag);

      if (flag & ED_LINE_DELETED) continue;
      //if (cfcc < ROADMAP_ROAD_FIRST) continue;

      if ((line_from.latitude != crossing->latitude) ||
          (line_from.longitude != crossing->longitude)) {

          if ((line_to.latitude != crossing->latitude) ||
              (line_to.longitude != crossing->longitude)) {

              continue;
          }
      }

      roadmap_plugin_set_line
         (&plugin_lines[count++], EditorPluginID, line, cfcc, fips);

      if (count >= size) return count;
   }

   return count;
}


int editor_street_get_range (int range_id,
                             int side,
                             EditorString *city,
                             EditorString *zip,
                             int *from,
                             int *to) {

   editor_db_range *range;

   range = (editor_db_range *) editor_db_get_item
                                 (ActiveRangeDB, range_id, 0, NULL);

   if (range == NULL) return -1;

   if (side == ED_STREET_LEFT_SIDE) {
      if (city != NULL) *city = range->left_city;
      if (zip != NULL) *zip = range->left_zip;
      if (from != NULL) *from = range->left_from_number;
      if (to != NULL) *to = range->left_to_number;
      
   } else if (side == ED_STREET_RIGHT_SIDE) {
      if (city != NULL) *city = range->right_city;
      if (zip != NULL) *zip = range->right_zip;
      if (from != NULL) *from = range->right_from_number;
      if (to != NULL) *to = range->right_to_number;

   } else {
      assert (0);
   }

   return 0;
}


int editor_street_add_range (EditorString l_city,
                             EditorString l_zip,
                             int l_from,
                             int l_to,
                             EditorString r_city,
                             EditorString r_zip,
                             int r_from,
                             int r_to) {

   editor_db_range range;
   int range_id;

   range.left_city = l_city;
   range.left_zip = l_zip;
   range.left_from_number = l_from;
   range.left_to_number = l_to;
      
   range.right_city = r_city;
   range.right_zip = r_zip;
   range.right_from_number = r_from;
   range.right_to_number = r_to;

   range_id = editor_db_add_item (ActiveRangeDB, &range);

   if (range_id == -1) {

      editor_db_grow ();
      range_id = editor_db_add_item (ActiveRangeDB, &range);
   }

   return range_id;
}


int editor_street_set_range (int range_id,
                             int side,
                             EditorString *city,
                             EditorString *zip,
                             int *from,
                             int *to) {

   editor_db_range *range;

   range = (editor_db_range *) editor_db_get_item
                                 (ActiveRangeDB, range_id, 0, NULL);

   if (range == NULL) return -1;

   if (side == ED_STREET_LEFT_SIDE) {
      if (city != NULL) range->left_city = *city;
      if (zip != NULL)  range->left_zip = *zip;
      if (from != NULL) range->left_from_number = *from;
      if (to != NULL)   range->left_to_number = *to;
      
   } else if (side == ED_STREET_RIGHT_SIDE) {
      if (city != NULL) range->right_city = *city;
      if (zip != NULL)  range->right_zip = *zip;
      if (from != NULL) range->right_from_number = *from;
      if (to != NULL)   range->right_to_number = *to;

   } else {
      assert (0);
   }

   return 0;
}


void editor_street_get_properties
               (int line, EditorStreetProperties *properties) {
   
   int range;

   properties->street = -1;
   properties->city = -1;
   properties->range_id = -1;
   properties->first_range.fradd = properties->first_range.toadd = -1;
   properties->second_range.fradd = properties->second_range.toadd = -1;
   
   if (editor_line_get_street
         (line, &properties->street, &range) == -1) {
      return;
   }

   if ((range != -1) && (editor_street_get_range
                              (range,
                               ED_STREET_LEFT_SIDE,
                              &properties->city,
                               NULL,
                              &properties->first_range.fradd,
                              &properties->first_range.toadd) != -1)) {

      properties->range_id = range;
   }

   if (range != -1) {
      EditorString city;

      if (editor_street_get_range
                              (range,
                               ED_STREET_RIGHT_SIDE,
                              &city,
                               NULL,
                              &properties->second_range.fradd,
                              &properties->second_range.toadd) == -1) {
         return;
      }

      if (properties->city == -1) {
         properties->city = city;
      }

   }
}


const char *editor_street_get_full_name
               (const EditorStreetProperties *properties) {

    static char RoadMapStreetName [512];

    const char *address;
    const char *city;


    if (properties->street < 0) {
        return "";
    }

    address = editor_street_get_street_address (properties);
    city    = editor_street_get_street_city
                  (properties, ED_STREET_LEFT_SIDE);
    
    snprintf (RoadMapStreetName, sizeof(RoadMapStreetName),
              "%s%s%s%s%s",
              address,
              (address[0])? " " : "",
              editor_street_get_street_name (properties),
              (city[0])? ", " : "",
              city);
    
    return RoadMapStreetName;
}


const char *editor_street_get_street_address
               (const EditorStreetProperties *properties) {

    static char RoadMapStreetAddress [32];
    int min;
    int max;

    if (properties->first_range.fradd == -1) {
        return "";
    }

    if (properties->first_range.fradd > properties->first_range.toadd) {
       min = properties->first_range.toadd;
       max = properties->first_range.fradd;
    } else {
       min = properties->first_range.fradd;
       max = properties->first_range.toadd;
    }

    if (properties->second_range.fradd != -1) {

       if (properties->second_range.fradd < min) {
          min = properties->second_range.fradd;
       }
       if (properties->second_range.fradd > max) {
          max = properties->second_range.fradd;
       }
       if (properties->second_range.toadd < min) {
          min = properties->second_range.toadd;
       }
       if (properties->second_range.toadd > max) {
          max = properties->second_range.toadd;
       }
    }

    sprintf (RoadMapStreetAddress, "%d - %d", min, max);

    return RoadMapStreetAddress;
}


const char *editor_street_get_street_name
               (const EditorStreetProperties *properties) {
   
   const char *name;
   editor_db_street *street;
   
   if (properties->street < 0) {
      return "";
   }

   street =
      (editor_db_street *) editor_db_get_item
                              (ActiveStreetDB, properties->street, 0, NULL);
   assert (street != NULL);

   if (street->fename < 0) {
      return "";
   }

   name = editor_dictionary_get (ActiveStreetDictionary, street->fename);

   return name;
}


const char *editor_street_get_city_name
               (const EditorStreetProperties *properties) {

   if (properties->city < 0) {
      return "";
   }
   return editor_dictionary_get (ActiveCityDictionary, properties->city);
}


const char *editor_street_get_street_fename
               (const EditorStreetProperties *properties) {
   
   editor_db_street *street;
   
   if (properties->street < 0) {
      return "";
   }

   street =
      (editor_db_street *) editor_db_get_item
                              (ActiveStreetDB, properties->street, 0, NULL);
   assert (street != NULL);

   if (street->fename < 0) {
      return "";
   }

   return editor_dictionary_get (ActiveStreetDictionary, street->fename);
}


const char *editor_street_get_street_fetype
               (const EditorStreetProperties *properties) {
   
   editor_db_street *street;
   
   if (properties->street < 0) {
      return "";
   }

   street =
      (editor_db_street *) editor_db_get_item
                              (ActiveStreetDB, properties->street, 0, NULL);
   assert (street != NULL);

   if (street->fetype < 0) {
      return "";
   }

   return editor_dictionary_get (ActiveTypeDictionary, street->fetype);
}


const char *editor_street_get_street_city
                (const EditorStreetProperties *properties, int side) {

   editor_db_range *range;
   EditorString str;

   if (properties->range_id == -1) return "";

   range = (editor_db_range *) editor_db_get_item
                        (ActiveRangeDB, properties->range_id, 0, NULL);

   assert (range != NULL);
   if (range == NULL) return "";

   if (side == ED_STREET_LEFT_SIDE) {
      str = range->left_city;
   } else if (side == ED_STREET_RIGHT_SIDE) {
      str = range->right_city;
   } else {
      assert (0);
      str = 0;
   }

   if (str == -1) return "";
   
   return editor_dictionary_get (ActiveCityDictionary, str);
}


const char *editor_street_get_street_zip
                (const EditorStreetProperties *properties, int side) {

   editor_db_range *range;
   EditorString str;

   if (properties->range_id == -1) return "";

   range = (editor_db_range *) editor_db_get_item
                        (ActiveRangeDB, properties->range_id, 0, NULL);

   assert (range != NULL);
   if (range == NULL) return "";

   if (side == ED_STREET_LEFT_SIDE) {
      str = range->left_zip;
   } else if (side == ED_STREET_RIGHT_SIDE) {
      str = range->right_zip;
   } else {
      assert (0);
      str = 0;
   }

   if (str == -1) return "";
   
   return editor_dictionary_get (ActiveZipDictionary, str);
}


void editor_street_get_street_range
    (const EditorStreetProperties *properties, int side, int *from, int *to) {

   editor_db_range *range;

   *from = *to = -1;
   if (properties->range_id == -1) return;

   range = (editor_db_range *) editor_db_get_item
                        (ActiveRangeDB, properties->range_id, 0, NULL);

   assert (range != NULL);
   if (range == NULL) return;

   if (side == ED_STREET_LEFT_SIDE) {
      *from = range->left_from_number;
      *to = range->left_to_number;
   } else if (side == ED_STREET_RIGHT_SIDE) {
      *from = range->right_from_number;
      *to = range->right_to_number;
   } else {
      assert (0);
   }
}


const char *editor_street_get_city_string (EditorString city_id) {

   if (city_id < 0) return "";

   return editor_dictionary_get (ActiveCityDictionary, city_id);
}


const char *editor_street_get_zip_string (EditorString zip_id) {

   if (zip_id < 0) return "";

   return editor_dictionary_get (ActiveZipDictionary, zip_id);
}


const char *editor_street_get_street_t2s
               (const EditorStreetProperties *properties) {

   editor_db_street *street;
   
   if (properties->street < 0) {
      return "";
   }

   street =
      (editor_db_street *) editor_db_get_item
                              (ActiveStreetDB, properties->street, 0, NULL);
   assert (street != NULL);

   if (street->t2s < 0) {
      return "";
   }

   return editor_dictionary_get (ActiveT2SDictionary, street->t2s);
}


void editor_street_set_t2s (int street_id , const char *t2s) {

   editor_db_street *street;
   
   if (street_id < 0) {
      return;
   }

   street =
      (editor_db_street *) editor_db_get_item
                              (ActiveStreetDB, street_id, 0, NULL);
   assert (street != NULL);

   street->t2s =
      editor_dictionary_add (ActiveT2SDictionary, t2s, strlen(t2s));
}


int editor_street_copy_street
   (int source_line, int plugin_id, int dest_line) {

   int street_id;

   if (!plugin_id) {

      RoadMapStreetProperties properties;
      const char *fename;
      const char *fetype;
      const char *fedirs;
      const char *fedirp;
      const char *t2s;

      roadmap_street_get_properties (source_line, &properties);

      if (properties.street == -1) return 0;

      fename = roadmap_street_get_street_fename (&properties);
      fetype = roadmap_street_get_street_fetype (&properties);
      fedirs = roadmap_street_get_street_fedirs (&properties);
      fedirp = roadmap_street_get_street_fedirp (&properties);
      t2s    = roadmap_street_get_street_t2s  (&properties);

      street_id =
         editor_street_create
            (fename, fetype, fedirs, fedirp);

      if (street_id == -1) return -1;

      editor_street_set_t2s (street_id, t2s);
         
   } else {

      editor_db_street *street;
      editor_db_street new_street;

      editor_line_get_street (source_line, &street_id, NULL);
      if (street_id == -1) return 0;
      
      street =
         (editor_db_street *) editor_db_get_item
                 (ActiveStreetDB, street_id, 0, NULL);
      assert (street != NULL);

      new_street.fename = street->fename;
      new_street.fetype = street->fetype;
      new_street.fedirs = street->fedirs;
      new_street.fedirp = street->fedirp;
      new_street.t2s    = street->t2s;

      street_id = editor_db_add_item (ActiveStreetDB, &new_street);

      if (street_id == -1) {

         editor_db_grow ();
         street_id = editor_db_add_item (ActiveStreetDB, &new_street);
      }

      if (street_id == -1) return -1;
   }

   editor_line_set_street (dest_line, &street_id, NULL);

   return 0;
}


int editor_street_copy_range
   (int source_line, int plugin_id, int dest_line) {

   int range_id;

   if (!plugin_id) {

      RoadMapStreetProperties properties;
      EditorString l_city;
      EditorString l_zip;
      EditorString r_city;
      EditorString r_zip;
      int l_from;
      int l_to;
      int r_from;
      int r_to;

      roadmap_street_get_properties (source_line, &properties);

      if (properties.street == -1) return 0;

      l_city =
         editor_street_create_city
            (roadmap_street_get_street_city
               (&properties, ROADMAP_STREET_LEFT_SIDE));
      l_zip =
         editor_street_create_zip
            (roadmap_street_get_street_zip
               (&properties, ROADMAP_STREET_LEFT_SIDE));

      roadmap_street_get_street_range
         (&properties, ROADMAP_STREET_LEFT_SIDE, &l_from, &l_to);

      r_city =
         editor_street_create_city
            (roadmap_street_get_street_city
               (&properties, ROADMAP_STREET_RIGHT_SIDE));
      r_zip =
         editor_street_create_zip
            (roadmap_street_get_street_zip
               (&properties, ROADMAP_STREET_RIGHT_SIDE));

      roadmap_street_get_street_range
         (&properties, ROADMAP_STREET_RIGHT_SIDE, &r_from, &r_to);

      range_id =
         editor_street_add_range
            (l_city, l_zip, l_from, l_to, r_city, r_zip, r_from, r_to);

      if (range_id == -1) return -1;

   } else {

      editor_db_range *range;
      editor_db_range new_range;

      editor_line_get_street (source_line, NULL, &range_id);
      if (range_id == -1) return 0;
      
      range =
         (editor_db_range *) editor_db_get_item
                 (ActiveRangeDB, range_id, 0, NULL);
      assert (range != NULL);

      new_range.left_city        = range->left_city;
      new_range.left_zip         = range->left_zip;
      new_range.left_from_number = range->left_from_number;
      new_range.left_to_number   = range->left_to_number;

      new_range.right_city        = range->right_city;
      new_range.right_zip         = range->right_zip;
      new_range.right_from_number = range->right_from_number;
      new_range.right_to_number   = range->right_to_number;

      range_id = editor_db_add_item (ActiveRangeDB, &new_range);

      if (range_id == -1) {

         editor_db_grow ();
         range_id = editor_db_add_item (ActiveRangeDB, &new_range);
      }

      if (range_id == -1) return -1;
   }

   editor_line_set_street (dest_line, NULL, &range_id);

   return 0;
}


void editor_street_distribute_range
   (int *lines, int num_lines, int l_from, int l_to, int r_from, int r_to) {
   int i;
   int total_length = 0;
   int line_length[100];
   double delta_left;
   double delta_right;

   if ((unsigned)num_lines > (sizeof(line_length) / sizeof(int))) {
      editor_log
         (ROADMAP_ERROR,
            "editor_street_distribute_range: Too many lines - %d", num_lines);
      num_lines = sizeof(line_length) / sizeof(int);
   }

   for (i=0; i<num_lines; i++) {
      
      line_length[i] = editor_line_length (lines[i]);
      total_length += line_length[i];
   }

   delta_left = l_to - l_from;
   delta_right = r_to - r_from;

   for (i=0; i<num_lines; i++) {
      
      int this_l_to;
      int this_r_to;
      int range_id;
      
      if (!line_length[i]) continue;

      this_l_to = (int) (delta_left * line_length[i] / total_length + l_from);
      this_r_to = (int) (delta_right * line_length[i] / total_length + r_from);

      if (this_l_to > l_to) this_l_to = l_to;
      if (this_r_to > r_to) this_r_to = r_to;

      editor_line_get_street (lines[i], NULL, &range_id);

      assert (range_id != -1);

      if (range_id != -1) {

         editor_street_set_range
            (range_id, ED_STREET_LEFT_SIDE, NULL, NULL, &l_from, &this_l_to);
         editor_street_set_range
            (range_id, ED_STREET_RIGHT_SIDE, NULL, NULL, &r_from, &this_r_to);
      }

      l_from = this_l_to;
      r_from = this_r_to;
   }
}

