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
#include "buildmap_street.h"
#include "buildmap_range.h"
#include "buildmap_area.h"
#include "buildmap_shape.h"
#include "buildmap_polygon.h"

/* DB schemes */

#define BordersTlidStart 1000000
#define WaterTlidStart 1100000
/* ROADS */

static const char *roads_sql = "SELECT segments.id AS id, AsText(segments.the_geom) AS the_geom, segments.type AS layer, streets.name FROM segments LEFT JOIN streets ON segments.street_id == streets.id;";

static const char *country_borders_sql = "SELECT ogc_fid AS id, AsText(wkb_geometry) AS the_geom FROM boundaries;";
static const char *water_sql = "SELECT ogc_fid AS id, AsText(wkb_geometry) AS the_geom FROM water;";

static BuildMapDictionary DictionaryPrefix;
static BuildMapDictionary DictionaryStreet;
static BuildMapDictionary DictionaryType;
static BuildMapDictionary DictionarySuffix;
static BuildMapDictionary DictionaryCity;
static BuildMapDictionary DictionaryLandmark;

static int BuildMapCanals = ROADMAP_WATER_RIVER;
static int BuildMapRivers = ROADMAP_WATER_RIVER;
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


static int type2layer (const char *type) {

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

      from_point = buildmap_point_add (frlong, frlat);
      to_point   = buildmap_point_add (tolong, tolat);

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

      from_point = buildmap_point_add (frlong, frlat);

      for (j=1; j<num_points-1; j++) {
         
    int lon, lat;

         lon = lon_arr[j] * 1000000.0;
         lat = lat_arr[j] * 1000000.0;

          to_point = buildmap_point_add (lon, lat);
    
          buildmap_line_add (tlid++, BuildMapSea, from_point, to_point);
    from_point = to_point;
      }

      to_point   = buildmap_point_add (tolong, tolat);

      buildmap_line_add (tlid++, BuildMapSea, from_point, to_point);

      free (lon_arr);
      free (lat_arr);

   }

   PQclear(db_result);

   return;
}


static void buildmap_postgres_read_water_shape_points (int verbose) {

   int    irec;
   int    record_count;

   int line_index;
   int tlid;
   int j, lat, lon;

   PGresult *db_result;

   db_result = PQexec(hPGConn, water_sql);

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

      tlid   = WaterTlidStart + atoi(PQgetvalue(db_result, irec, 0));

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



#endif // ROADMAP_USE_POSTGRES


void buildmap_postgres_process (const char *source,
                                 int verbose, int canals, int rivers) {

#if ROADMAP_USE_POSTGRES

   DictionaryPrefix = buildmap_dictionary_open ("prefix");
   DictionaryStreet = buildmap_dictionary_open ("street");
   DictionaryType   = buildmap_dictionary_open ("type");
   DictionarySuffix = buildmap_dictionary_open ("suffix");
   DictionaryCity   = buildmap_dictionary_open ("city");

   if (! canals) {
      BuildMapCanals = 0;
   }

   if (! rivers) {
      BuildMapRivers = 0;
   }

   buildmap_postgres_connect (source);
   buildmap_postgres_read_borders_lines (verbose);
   buildmap_postgres_read_water_lines (verbose);
   buildmap_line_sort();
   buildmap_postgres_read_borders_shape_points (verbose);
   //buildmap_postgres_read_water_shape_points (verbose);
   buildmap_postgres_read_water_polygons (verbose);
   PQfinish (hPGConn);

#else

   fprintf (stderr,
            "cannot process %s: built with no postgres support.\n",
            source);
   exit(1);
#endif // ROADMAP_USE_POSTGRES
}

