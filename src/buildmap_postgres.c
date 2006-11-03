/* buildmap_postgres.c - a module to read postgis DB.
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
 *   see buildmap_postgres.h
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

#if ROADMAP_USE_POSTGRES

#include <libpq-fe.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_path.h"

#include "buildmap.h"
#include "buildmap_zip.h"
#include "buildmap_shapefile.h"
#include "buildmap_city.h"
#include "buildmap_square.h"
#include "buildmap_point.h"
#include "buildmap_line.h"
#include "buildmap_line_route.h"
#include "buildmap_line_speed.h"
#include "buildmap_dglib.h"
#include "buildmap_street.h"
#include "buildmap_range.h"
#include "buildmap_area.h"
#include "buildmap_shape.h"
#include "buildmap_turn_restrictions.h"
#include "buildmap_polygon.h"

/* DB schemes */

#define BordersTlidStart 1000000
#define WaterTlidStart 1100000
/* ROADS */

static const char *roads_sql = "SELECT segments.id AS id, AsText(simplify(segments.the_geom,  0.00002)) AS the_geom, segments.road_type AS layer, segments.from_node AS from_node_id, segments.to_node AS to_node_id, streets.name AS street_name, streets.text2speech as text2speech, cities.name as city_name, fraddl, toaddl, fraddr, toaddr, from_travel_ref, to_travel_ref FROM segments LEFT JOIN streets ON segments.street_id = streets.id LEFT JOIN cities ON streets.city_id = cities.id WHERE segments.the_geom @ SetSRID ('BOX3D(34 29.2, 36.2 33.6)'::box3d, 4326);";
static const char *roads_route_sql = "SELECT segments.id AS id, segments.from_car_allowed AS from_car_allowed, segments.to_car_allowed AS to_car_allowed, segments.from_max_speed AS from_max_speed, segments.to_max_speed AS to_max_speed, segments.from_cross_time AS from_cross_time, segments.to_cross_time AS to_cross_time, segments.road_type AS layer FROM segments WHERE segments.the_geom @ SetSRID ('BOX3D(34 29.2, 36.2 33.6)'::box3d, 4326);";
static const char *country_borders_sql = "SELECT id AS id, AsText(simplify(the_geom,  0.00002)) AS the_geom FROM borders;";
static const char *water_sql = "SELECT id AS id, AsText(simplify(the_geom,  0.00002)) AS the_geom FROM water;";
static const char *turn_restrictions_sql = "SELECT node_id, seg1_id, seg2_id FROM turn_restrictions;";
static const char *cities_sql = "SELECT name FROM cities;";

static BuildMapDictionary DictionaryPrefix;
static BuildMapDictionary DictionaryStreet;
static BuildMapDictionary DictionaryText2Speech;
static BuildMapDictionary DictionaryType;
static BuildMapDictionary DictionarySuffix;
static BuildMapDictionary DictionaryCity;
static BuildMapDictionary DictionaryLandmark;

static int BuildMapSea = ROADMAP_WATER_SEA;
static int BuildMapBorders = ROADMAP_WATER_SHORELINE;

PGconn *hPGConn;

static RoadMapString
str2dict (BuildMapDictionary d, const char *string) {

   if (!strlen(string)) {
      return buildmap_dictionary_add (d, "", 0);
   }

   return buildmap_dictionary_add (d, (char *) string, strlen(string));
}


static int pg2layer (int layer) {

   switch (layer) {
      case 1: return ROADMAP_ROAD_STREET;
      case 2: return ROADMAP_ROAD_MAIN;
      case 3: return ROADMAP_ROAD_FREEWAY;
      case 4: return ROADMAP_ROAD_RAMP;
      case 5: return ROADMAP_ROAD_TRAIL;
   }

   return ROADMAP_ROAD_STREET;
}


static void postgres_summary (int verbose, int count) {

   buildmap_summary (verbose, "%d records", count);
}


static int decode_line_string(char *linestring, float **lon, float **lat)
{
   char *itr = linestring;
   char *tmp;
   char *end = linestring + strlen(linestring);
   char token[100];
   int num = 0;
   int curr_size = 100;
   *lon = malloc(sizeof(float) * curr_size);
   *lat = malloc(sizeof(float) * curr_size);

   if (strncmp(linestring, "LINESTRING(", 11) == 0) {
      itr += 11;
   } else if (strncmp(linestring, "MULTILINESTRING(", 16) == 0) {
      itr += 17;
   } else if (strncmp(linestring, "POLYGON(", 8) == 0) {
      itr += 9;
   } else {
            fprintf(stderr, "this is not a linestring: %s\n",
            linestring);
      exit(-1);
   }

   while ((itr+1)  < end) {
      tmp = strchr(itr, ' ');
      if ((!tmp) || ((unsigned)(tmp-itr) > sizeof(token))) {
               fprintf(stderr,
            "Can't decode this part of linestring: %s\n",
               itr);
         exit(-1);
      }

      strncpy(token, itr, tmp-itr);
      token[tmp-itr] = '\0';

      (*lon)[num] = atof(token);
      itr = tmp+1;

      tmp = strchr(itr, ',');
      if (!tmp) {
         tmp = strchr(itr, ')');
      }
      if ((!tmp) || ((unsigned)(tmp-itr) > sizeof(token))) {
               fprintf(stderr,
            "Can't decode this part of linestring: %s\n",
               itr);
         exit(-1);
      }

      strncpy(token, itr, tmp-itr);
      token[tmp-itr] = '\0';

      (*lat)[num] = atof(token);

      itr = tmp+1;

      num++;
      
      if (num == curr_size) {
         curr_size *= 2;
         *lon = realloc(*lon, sizeof(float) * curr_size);
         *lat = realloc(*lat, sizeof(float) * curr_size);
      }
   }

   return num;
}


static void buildmap_postgres_connect (const char *source) {
   
   buildmap_set_source(source);

   hPGConn = PQconnectdb(source);
   
   if( hPGConn == NULL || PQstatus(hPGConn) == CONNECTION_BAD ) {
      
      fprintf(stderr, "Can't open postgres database: %s\n",
               PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(1);
    }
}


static int db_result_ok (PGresult *result) {
   
   if (!result || ((PQresultStatus(result) != PGRES_TUPLES_OK)) ||
         (PQntuples(result) < 0)) {

      return 0;
   }

   return 1;
}


static void buildmap_postgres_read_speeds (int tlid, int speed_ref,
                                           int opposite) {

   int    irec;
   int    record_count;

   BuildMapSpeed *this_speed;
   PGresult *db_result;

   char speeds_sql[200];
   
   sprintf(speeds_sql,
           "SELECT time, speed FROM today_cross_times WHERE seg_ref_id=%d ORDER BY time;",
           speed_ref);

   db_result = PQexec(hPGConn, speeds_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   if (!record_count) return;

   this_speed = buildmap_line_speed_new ();

   for (irec=0; irec<record_count; irec++) {

      int speed;
      int db_time;
      int time_slot;
      int column = 0;

      buildmap_set_line (irec);

      db_time = atoi(PQgetvalue(db_result, irec, column++));
      speed = atoi(PQgetvalue(db_result, irec, column++));

      speed = (speed / 5) * 5;
      time_slot = (db_time / 100) * 2;
      if ((db_time % 100) >= 30) time_slot++;

      buildmap_line_speed_add_slot (this_speed, time_slot, speed);
   }

   buildmap_line_speed_add (this_speed, tlid, opposite);

   buildmap_line_speed_free (this_speed);

   PQclear(db_result);
}


static void buildmap_postgres_read_roads_lines (int verbose) {

   int    irec;
   int    record_count;

   int line;
   int street;
   int zip;
   int tlid;
   int frlong;
   int frlat;
   int tolong;
   int tolat;
   int from_point;
   int to_point;
   int from_node_id;
   int to_node_id;
   RoadMapString fedirp;
   RoadMapString fename;
   RoadMapString t2s;
   RoadMapString fetype;
   RoadMapString fedirs;
   RoadMapString city;

   DictionaryPrefix = buildmap_dictionary_open ("prefix");
   DictionaryStreet = buildmap_dictionary_open ("street");
   DictionaryText2Speech = buildmap_dictionary_open ("text2speech");
   DictionaryType   = buildmap_dictionary_open ("type");
   DictionarySuffix = buildmap_dictionary_open ("suffix");
   DictionaryCity   = buildmap_dictionary_open ("city");

   PGresult *db_result;

   db_result = PQexec(hPGConn, roads_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   for (irec=0; irec<record_count; irec++) {

      float *lon_arr;
      float *lat_arr;
      int num_points;
      int layer;
      int column = 0;
      int speed_ref;

      buildmap_set_line (irec);

      tlid = atoi(PQgetvalue(db_result, irec, column++));

      num_points =
         decode_line_string
         (PQgetvalue(db_result, irec, column++), &lon_arr, &lat_arr);

      frlong = lon_arr[0] * 1000000.0;
      frlat  = lat_arr[0] * 1000000.0;

      tolong = lon_arr[num_points-1] * 1000000.0;
      tolat  = lat_arr[num_points-1] * 1000000.0;

      layer = pg2layer (atoi(PQgetvalue(db_result, irec, column++)));

      from_node_id = atoi(PQgetvalue(db_result, irec, column++));
      to_node_id = atoi(PQgetvalue(db_result, irec, column++));

      fedirp = str2dict (DictionaryPrefix, "");
      
      fename =
         str2dict (DictionaryStreet, PQgetvalue(db_result, irec, column++));
      t2s =
         str2dict (DictionaryText2Speech, PQgetvalue(db_result, irec, column++));

      fetype = str2dict (DictionaryPrefix, "");
      fedirs = str2dict (DictionaryPrefix, "");

      from_point = buildmap_point_add (frlong, frlat, from_node_id);
      to_point   = buildmap_point_add (tolong, tolat, to_node_id);

      line = buildmap_line_add (tlid, layer, from_point, to_point);

      street = buildmap_street_add
                  (layer, fedirp, fename, fetype, fedirs, t2s, line);
      city =
         str2dict (DictionaryCity, PQgetvalue(db_result, irec, column++));
      zip = buildmap_zip_add (0, 0, 0);

      if (!city) {

         buildmap_range_add_no_address (line, street);
         column += 4;
      } else {

         const char *fraddl = PQgetvalue(db_result, irec, column++);
         const char *toaddl = PQgetvalue(db_result, irec, column++);
         const char *fraddr = PQgetvalue(db_result, irec, column++);
         const char *toaddr = PQgetvalue(db_result, irec, column++);

         if (fraddl[0] && fraddr[0] &&
            (atoi(fraddr) >= 0) && (atoi(toaddr) >= 0) &&
            (atoi(fraddl) >= 0) && (atoi(toaddl) >= 0)) {

            buildmap_range_add
               (line, street, atoi(fraddl), atoi(toaddl), zip, city);
            buildmap_range_add
               (line, street, atoi(fraddr), atoi(toaddr), zip, city);
            
         } else {

            //buildmap_range_add_no_address (line, street);
            buildmap_range_add
                          (line, street, 0, 0, zip, city);
                          buildmap_range_add
                          (line, street, 0, 0, zip, city);
        }
      }

      free (lon_arr);
      free (lat_arr);

      /* from speed ref */
      if (!PQgetisnull(db_result, irec, column++)) {
         speed_ref = atoi(PQgetvalue(db_result, irec, column-1));
         buildmap_postgres_read_speeds (tlid, speed_ref, 0);
      }

      if (!PQgetisnull(db_result, irec, column++)) {
         speed_ref = atoi(PQgetvalue(db_result, irec, column-1));
         buildmap_postgres_read_speeds (tlid, speed_ref, 1);
      }
   }

   PQclear(db_result);

   return;
}


static void buildmap_postgres_read_roads_route (int verbose) {

   int    irec;
   int    record_count;

   int line;
   int tlid;

   unsigned char from_car_allowed;
   unsigned char to_car_allowed;
   unsigned char from_max_speed;
   unsigned char to_max_speed;
   unsigned short from_cross_time;
   unsigned short to_cross_time;
   unsigned char layer;
   unsigned short from_speed_ref;
   unsigned short to_speed_ref;
  
   PGresult *db_result;

   db_result = PQexec(hPGConn, roads_route_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   for (irec=0; irec<record_count; irec++) {

      int column = 0;

      buildmap_set_line (irec);

      tlid = atoi(PQgetvalue(db_result, irec, column++));

      from_car_allowed =
         !strcmp("t", PQgetvalue(db_result, irec, column++)) ?
         ROUTE_CAR_ALLOWED: 0;
      to_car_allowed =
         !strcmp("t", PQgetvalue(db_result, irec, column++)) ?
         ROUTE_CAR_ALLOWED: 0;

      from_max_speed  = atoi(PQgetvalue(db_result, irec, column++));
      to_max_speed    = atoi(PQgetvalue(db_result, irec, column++));
      from_cross_time = atoi(PQgetvalue(db_result, irec, column++));
      to_cross_time   = atoi(PQgetvalue(db_result, irec, column++));
      layer =           (unsigned char) pg2layer (atoi(PQgetvalue(db_result, irec, column++)));

      line = buildmap_line_find_sorted(tlid);

      from_speed_ref = buildmap_line_speed_get_ref (tlid, 0);
      to_speed_ref = buildmap_line_speed_get_ref (tlid, 1);
      buildmap_line_route_add
         (from_car_allowed, to_car_allowed, from_max_speed, to_max_speed,
          from_speed_ref, to_speed_ref,
          line);

      buildmap_dglib_add
         (from_car_allowed, to_car_allowed, from_max_speed, to_max_speed,
          from_cross_time, to_cross_time, layer,
          line);
   }

   PQclear(db_result);

   return;
}


static void buildmap_postgres_read_roads_shape_points (int verbose) {

   int    irec;
   int    record_count;

   int line_index;
   int tlid;
   int j, lat, lon;

   PGresult *db_result;

   db_result = PQexec(hPGConn, roads_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   for (irec=0; irec<record_count; irec++) {

      float *lon_arr;
      float *lat_arr;
      int num_points;

      buildmap_set_line (irec);

      tlid = atoi(PQgetvalue(db_result, irec, 0));

      num_points =
         decode_line_string
         (PQgetvalue(db_result, irec, 1), &lon_arr, &lat_arr);

      line_index = buildmap_line_find_sorted(tlid);

      if (line_index >= 0) {

         for (j=1; j<num_points-1; j++) {

            lon = lon_arr[j] * 1000000.0;
            lat = lat_arr[j] * 1000000.0;
            buildmap_shape_add(line_index, j-1, lon, lat);
         }
      }

      free (lon_arr);
      free (lat_arr);

      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }

   }

   PQclear(db_result);

   postgres_summary (verbose, record_count);
}


static void buildmap_postgres_read_turn_restrictions (int verbose) {

   int    irec;
   int    record_count;

   int node_index;
   int node_id;

   PGresult *db_result;

   db_result = PQexec(hPGConn, turn_restrictions_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   for (irec=0; irec<record_count; irec++) {

      int from_line;
      int to_line;

      buildmap_set_line (irec);

      node_id = atoi(PQgetvalue(db_result, irec, 0));
      from_line = atoi(PQgetvalue(db_result, irec, 1));
      to_line = atoi(PQgetvalue(db_result, irec, 2));

      node_index = buildmap_point_find_sorted(node_id);
      from_line = buildmap_line_find_sorted(from_line);
      to_line = buildmap_line_find_sorted(to_line);

      if (node_index < 0) continue;
      buildmap_turn_restrictions_add(node_index, from_line, to_line);

      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }

   }

   PQclear(db_result);

   postgres_summary (verbose, record_count);
}


static void buildmap_postgres_read_borders_lines (int verbose) {

   int    irec;
   int    record_count;

   int line;
   int tlid;
   int frlong;
   int frlat;
   int tolong;
   int tolat;
   int from_point;
   int to_point;

   PGresult *db_result;

   db_result = PQexec(hPGConn, country_borders_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   for (irec=0; irec<record_count; irec++) {

      float *lon_arr;
      float *lat_arr;
      int num_points;

      buildmap_set_line (irec);

      tlid = BordersTlidStart + atoi(PQgetvalue(db_result, irec, 0));

      num_points =
         decode_line_string
         (PQgetvalue(db_result, irec, 1), &lon_arr, &lat_arr);

      frlong = lon_arr[0] * 1000000.0;
      frlat  = lat_arr[0] * 1000000.0;

      tolong = lon_arr[num_points-1] * 1000000.0;
      tolat  = lat_arr[num_points-1] * 1000000.0;

      from_point = buildmap_point_add (frlong, frlat, -1);
      to_point   = buildmap_point_add (tolong, tolat, -1);

      line = buildmap_line_add (tlid, BuildMapBorders, from_point, to_point);

      free (lon_arr);
      free (lat_arr);

   }

   PQclear(db_result);

   return;
}


static void buildmap_postgres_read_borders_shape_points (int verbose) {

   int    irec;
   int    record_count;

   int line_index;
   int tlid;
   int j, lat, lon;

   PGresult *db_result;

   db_result = PQexec(hPGConn, country_borders_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   for (irec=0; irec<record_count; irec++) {

      float *lon_arr;
      float *lat_arr;
      int num_points;

      buildmap_set_line (irec);

      tlid   = BordersTlidStart + atoi(PQgetvalue(db_result, irec, 0));

      num_points =
         decode_line_string
         (PQgetvalue(db_result, irec, 1), &lon_arr, &lat_arr);

      line_index = buildmap_line_find_sorted(tlid);

      if (line_index >= 0) {

         for (j=1; j<num_points-1; j++) {

            lon = lon_arr[j] * 1000000.0;
            lat = lat_arr[j] * 1000000.0;
            buildmap_shape_add(line_index, j-1, lon, lat);
         }
      }

      free (lon_arr);
      free (lat_arr);

      if (verbose) {
         if ((irec & 0xff) == 0) {
            buildmap_progress (irec, record_count);
         }
      }

   }

   PQclear(db_result);

   postgres_summary (verbose, record_count);
}


static void buildmap_postgres_read_water_lines (int verbose) {

   int    irec;
   int    record_count;

   int tlid;
   int frlong;
   int frlat;
   int tolong;
   int tolat;
   int from_point;
   int to_point;

   PGresult *db_result;

   db_result = PQexec(hPGConn, water_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   tlid = WaterTlidStart;
   for (irec=0; irec<record_count; irec++) {

      float *lon_arr;
      float *lat_arr;
      int num_points;
      int j;

      buildmap_set_line (irec);

      num_points =
         decode_line_string
         (PQgetvalue(db_result, irec, 1), &lon_arr, &lat_arr);

      frlong = lon_arr[0] * 1000000.0;
      frlat  = lat_arr[0] * 1000000.0;

      tolong = lon_arr[num_points-1] * 1000000.0;
      tolat  = lat_arr[num_points-1] * 1000000.0;

      from_point = buildmap_point_add (frlong, frlat, -1);

      for (j=1; j<num_points-1; j++) {
         
    int lon, lat;

         lon = lon_arr[j] * 1000000.0;
         lat = lat_arr[j] * 1000000.0;

          to_point = buildmap_point_add (lon, lat, -1);
    
          buildmap_line_add (tlid++, BuildMapSea, from_point, to_point);
    from_point = to_point;
      }

      to_point = buildmap_point_add (tolong, tolat, -1);

      buildmap_line_add (tlid++, BuildMapSea, from_point, to_point);

      free (lon_arr);
      free (lat_arr);

   }

   PQclear(db_result);

   return;
}


static void buildmap_postgres_read_water_polygons (int verbose) {

   int    irec;
   int    record_count;

   int tlid;
   int landid = 0;
   int polyid = 0;
   int cenid = 0;

   PGresult *db_result;

   DictionaryLandmark = buildmap_dictionary_open ("landmark");

   db_result = PQexec(hPGConn, water_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   tlid = WaterTlidStart;

   for (irec=0; irec<record_count; irec++) {

      float *lon_arr;
      float *lat_arr;
      int num_points;
      int j;

      buildmap_set_line (irec);

      num_points =
         decode_line_string
         (PQgetvalue(db_result, irec, 1), &lon_arr, &lat_arr);

      buildmap_polygon_add_landmark (++landid, BuildMapSea, 
         buildmap_dictionary_add (DictionaryLandmark, "", 0));

      buildmap_polygon_add (landid, ++cenid, ++polyid);

      for (j=1; j<num_points; j++) {
         
          buildmap_polygon_add_line
            (cenid, polyid, tlid++, POLYGON_SIDE_LEFT);
      }

      free (lon_arr);
      free (lat_arr);
   }

   PQclear(db_result);

   return;
}


static void buildmap_postgres_read_cities (int verbose) {

   int    irec;
   int    record_count;

   PGresult *db_result;

   db_result = PQexec(hPGConn, cities_sql);

   if (!db_result_ok (db_result)) {

      fprintf
         (stderr, "Can't query database: %s\n", PQerrorMessage(hPGConn));
      PQfinish(hPGConn);
      exit(-1);
   }

   record_count = PQntuples(db_result);

   for (irec=0; irec<record_count; irec++) {

      str2dict (DictionaryCity, PQgetvalue(db_result, irec, 0));
   }

   PQclear(db_result);

   postgres_summary (verbose, record_count);
}



#endif // ROADMAP_USE_POSTGRES


void buildmap_postgres_process (const char *source,
                                 int verbose, int empty) {

#if ROADMAP_USE_POSTGRES

   DictionaryPrefix = buildmap_dictionary_open ("prefix");
   DictionaryStreet = buildmap_dictionary_open ("street");
   DictionaryType   = buildmap_dictionary_open ("type");
   DictionarySuffix = buildmap_dictionary_open ("suffix");
   DictionaryCity   = buildmap_dictionary_open ("city");

   buildmap_postgres_connect (source);
   buildmap_postgres_read_borders_lines (verbose);
   buildmap_postgres_read_water_lines (verbose);

   if (!empty) {
      buildmap_postgres_read_roads_lines (verbose);
   }

   buildmap_line_sort();
   buildmap_postgres_read_borders_shape_points (verbose);

   if (!empty) {
      buildmap_postgres_read_roads_route (verbose);
      buildmap_postgres_read_roads_shape_points (verbose);
      buildmap_postgres_read_turn_restrictions (verbose);
   }
   //buildmap_postgres_read_water_shape_points (verbose);
   buildmap_postgres_read_water_polygons (verbose);

   if (!empty) {
      buildmap_postgres_read_cities (verbose);
   }
   PQfinish (hPGConn);

#else

   fprintf (stderr,
            "cannot process %s: built with no postgres support.\n",
            source);
   exit(1);
#endif // ROADMAP_USE_POSTGRES
}

