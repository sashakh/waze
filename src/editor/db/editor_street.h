/* editor_street.h - 
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
 */

#ifndef INCLUDE__EDITOR_STREET__H
#define INCLUDE__EDITOR_STREET__H

#include "roadmap.h"
#include "roadmap_street.h"
#include "editor_dictionary.h"
#include "editor_square.h"

#define ED_STREET_LEFT_SIDE 1
#define ED_STREET_RIGHT_SIDE 2

typedef struct editor_db_street_s {
   EditorString fedirp;
   EditorString fename;
   EditorString fetype;
   EditorString fedirs;
   EditorString t2s;
} editor_db_street;

typedef struct editor_db_range_s {
    EditorString left_city;
    EditorString left_zip;
    int left_from_number;
    int left_to_number;
    EditorString right_city;
    EditorString right_zip;
    int right_from_number;
    int right_to_number;
} editor_db_range;

typedef struct {
    RoadMapStreetRange first_range;
    RoadMapStreetRange second_range;
    int street;
    int range_id;
    EditorString city;
} EditorStreetProperties;

const char *editor_street_get_name (int street_id);

int editor_street_create (const char *_name,
                          const char *_type,
                          const char *_prefix,
                          const char *_suffix);

int editor_street_create_city (const char *name);
int editor_street_create_zip (const char *name);
int editor_street_create_t2s (const char *name);

int editor_street_get_distance
                 (const RoadMapPosition *position,
                  const PluginLine *line,
                  RoadMapNeighbour *result);

int editor_street_get_closest (const RoadMapPosition *position,
                               int *categories,
                               int categories_count,
                               RoadMapNeighbour *neighbours,
                               int count,
                               int max);
   
void editor_street_get_properties
                  (int line, EditorStreetProperties *properties);

const char *editor_street_get_full_name
                  (const EditorStreetProperties *properties);

const char *editor_street_get_street_address
                  (const EditorStreetProperties *properties);

const char *editor_street_get_street_name
                  (const EditorStreetProperties *properties);

const char *editor_street_get_street_fename
                  (const EditorStreetProperties *properties);

const char *editor_street_get_street_fetype
                  (const EditorStreetProperties *properties);

const char *editor_street_get_street_t2s
                   (const EditorStreetProperties *properties);

const char *editor_street_get_street_city
                   (const EditorStreetProperties *properties, int side);

const char *editor_street_get_street_zip
                (const EditorStreetProperties *properties, int side);

void editor_street_get_street_range
    (const EditorStreetProperties *properties, int side, int *from, int *to);

const char *editor_street_get_street_t2s
               (const EditorStreetProperties *properties);

void editor_street_set_t2s (int street_id , const char *t2s);

const char *editor_street_get_city_string (EditorString city_id);

const char *editor_street_get_zip_string (EditorString zip_id);

int editor_street_get_connected_lines (const RoadMapPosition *crossing,
                                       PluginLine *plugin_lines,
                                       int size);

int editor_street_add_range (EditorString l_city,
                             EditorString l_zip,
                             int l_from,
                             int l_to,
                             EditorString r_city,
                             EditorString r_zip,
                             int r_from,
                             int r_to);

int editor_street_set_range (int range_id,
                             int side,
                             EditorString *city,
                             EditorString *zip,
                             int *from,
                             int *to);

int editor_street_get_range (int range_id,
                             int side,
                             EditorString *city,
                             EditorString *zip,
                             int *from,
                             int *to);

int editor_street_copy_range
   (int source_line, int plugin_id, int dest_line);

int editor_street_copy_street
   (int source_line, int plugin_id, int dest_line);

void editor_street_distribute_range
   (int *lines, int num_lines, int l_from, int l_to, int r_from, int r_to);

extern roadmap_db_handler EditorStreetHandler;
extern roadmap_db_handler EditorRangeHandler;

#endif // INCLUDE__EDITOR_STREET__H

