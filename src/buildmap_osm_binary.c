/* buildmap_osm_binary.c - a module to read OSM Mobile Binary format
 *
 * LICENSE:
 *
 *   Copyright 2007 Stephen Woodbridge
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
#include <time.h>


#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_osm.h"

#include "buildmap.h"
#include "buildmap_zip.h"
#include "buildmap_city.h"
#include "buildmap_square.h"
#include "buildmap_point.h"
#include "buildmap_line.h"
#include "buildmap_street.h"
#include "buildmap_range.h"
#include "buildmap_area.h"
#include "buildmap_shape.h"
#include "buildmap_polygon.h"

#include "buildmap_layer.h"
#include "buildmap_osm_binary.h"

extern char *BuildMapResult;


int KMaxLon = 180000000;
int KMaxLat = 90000000;


typedef struct layer_info {
    char *name;
    int  *layerp;
    int  flags;
} layer_info_t;

#define AREA 1

typedef struct layer_info_sublist {
    char *name;
    layer_info_t *list;
    int  *layerp;
} layer_info_sublist_t;

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

static int BuildMapLayerBoundary = 0;

#define FREEWAY     &BuildMapLayerFreeway     
#define RAMP        &BuildMapLayerRamp        
#define MAIN        &BuildMapLayerMain        
#define STREET      &BuildMapLayerStreet      
#define TRAIL       &BuildMapLayerTrail       
#define RAIL        &BuildMapLayerRail       

#define PARK        &BuildMapLayerPark
#define HOSPITAL    &BuildMapLayerHospital
#define AIRPORT     &BuildMapLayerAirport
#define STATION     &BuildMapLayerStation
#define MALL        &BuildMapLayerMall

#define SHORELINE   &BuildMapLayerShoreline   
#define RIVER       &BuildMapLayerRiver       
#define CANAL       &BuildMapLayerCanal       
#define LAKE        &BuildMapLayerLake        
#define SEA         &BuildMapLayerSea         
#define BOUNDARY    &BuildMapLayerBoundary

static BuildMapDictionary DictionaryPrefix;
static BuildMapDictionary DictionaryStreet;
static BuildMapDictionary DictionaryType;
static BuildMapDictionary DictionarySuffix;
static BuildMapDictionary DictionaryCity;
// static BuildMapDictionary DictionaryFSA;

void buildmap_osm_binary_find_layers (void) {

   BuildMapLayerFreeway   = buildmap_layer_get ("freeways");
   BuildMapLayerRamp      = buildmap_layer_get ("ramps");
   BuildMapLayerMain      = buildmap_layer_get ("highways");
   BuildMapLayerStreet    = buildmap_layer_get ("streets");
   BuildMapLayerTrail     = buildmap_layer_get ("trails");
   BuildMapLayerRail      = buildmap_layer_get ("railroads");

   BuildMapLayerPark      = buildmap_layer_get ("parks");
   BuildMapLayerHospital  = buildmap_layer_get ("hospitals");
   BuildMapLayerAirport   = buildmap_layer_get ("airports");
   BuildMapLayerStation   = buildmap_layer_get ("stations");
   BuildMapLayerMall      = buildmap_layer_get ("malls");

   BuildMapLayerShoreline = buildmap_layer_get ("shore");
   BuildMapLayerRiver     = buildmap_layer_get ("rivers");
   BuildMapLayerCanal     = buildmap_layer_get ("canals");
   BuildMapLayerLake      = buildmap_layer_get ("lakes");
   BuildMapLayerSea       = buildmap_layer_get ("sea");

   BuildMapLayerBoundary = buildmap_layer_get ("boundaries");
}

static RoadMapString str2dict (BuildMapDictionary d, const char *string) {

   if (!strlen(string)) {
      return buildmap_dictionary_add (d, "", 0);
   }

   return buildmap_dictionary_add (d, (char *) string, strlen(string));
}

/* the major options coming back from the server can indicate
 * four types of values.  those with a value that is itself an
 * enumerated type, and those with arbitrary numeric, time, or
 * string values.
 */
#define key_enumerated_start    1
#define key_numeric_start       128
#define key_date_start          176
#define key_string_start        192

static char *stringtype[] = {
        "name",
        "name_left",
        "name_right",
        "name_other",
        "int_name",
        "nat_name",
        "reg_name",
        "loc_name",
        "old_name",
        "ref",
        "int_ref",
        "nat_ref",
        "reg_ref",
        "loc_ref",
        "old_ref",
        "ncn_ref",
        "rcn_ref",
        "lcn_ref",
        "icao",
        "iata",
        "place_name",
        "place_numbers",
        "postal_code",
        "is_in",
        "note",
        "description",
        "image",
        "source",
        "source_ref",
        "created_by",
};

static char *datetype[] = {
        "date_on",
        "date_off",
        "start_date",
        "end_date",
};

#if NEEDED
static char *numeric_type[] = {
        "lanes",
        "layer",
        "ele",
        "width",
        "est_width",
        "maxwidth",
        "maxlength",
        "maxspeed",
        "minspeed",
        "day_on",
        "day_off",
        "hour_on",
        "hour_off",
        "maxweight",
        "maxheight",
};
#endif

static layer_info_t highway_to_layer[] = {
        { 0,                    NULL,           0 },
        { "motorway",           FREEWAY,        0 },            /* 1 */
        { "motorway_link",      FREEWAY,        0 },            /* 2 */
        { "trunk",              FREEWAY,        0 },            /* 3 */
        { "trunk_link",         FREEWAY,        0 },            /* 4 */
        { "primary",            MAIN,           0 },            /* 5 */
        { "primary_link",       MAIN,           0 },            /* 6 */
        { "secondary",          STREET,         0 },            /* 7 */
        { "tertiary",           STREET,         0 },            /* 8 */
        { "unclassified",       STREET,         0 },            /* 9 */
        { "minor",              STREET,         0 },            /* 10 */
        { "residential",        STREET,         0 },            /* 11 */
        { "service",            STREET,         0 },            /* 12 */
        { "track",              TRAIL,          0 },            /* 13 */
        { "cycleway",           TRAIL,          0 },            /* 14 */
        { "bridleway",          TRAIL,          0 },            /* 15 */
        { "footway",            TRAIL,          0 },            /* 16 */
        { "steps",              TRAIL,          0 },            /* 17 */
        { "pedestrian",         TRAIL,          0 },            /* =>17 */
};

static layer_info_t cycleway_to_layer[] = {
        { 0,                    NULL,           0 },
        { "lane",               TRAIL,          0 },            /* 1 */
        { "track",              TRAIL,          0 },            /* 2 */
        { "opposite_lane",      TRAIL,          0 },            /* 3 */
        { "opposite_track",     TRAIL,          0 },            /* 4 */
        { "opposite",           NULL,           0 },            /* 5 */
};

static layer_info_t waterway_to_layer[] = {
        { 0,                    NULL,           0 },
        { "river",              RIVER,          0 },            /* 1 */
        { "canal",              RIVER,          0 },            /* 2 */
};

static layer_info_t abutters_to_layer[] = {
        { 0,                    NULL,           0 },
        { "residential",        NULL,           0 },            /* 1 */
        { "retail",             NULL,           0 },            /* 2 */
        { "industrial",         NULL,           0 },            /* 3 */
        { "commercial",         NULL,           0 },            /* 4 */
        { "mixed",              NULL,           0 },            /* 5 */
};

static layer_info_t railway_to_layer[] = {
        { 0,                    NULL,           0 },
        { "rail",               RAIL,           0 },            /* 1 */
        { "tram",               RAIL,           0 },            /* 2 */
        { "light_rail",         RAIL,           0 },            /* 3 */
        { "subway",             RAIL,           0 },            /* 4 */
        { "station",            NULL,           0 },            /* 5 */
};

static layer_info_t natural_to_layer[] = {
        { 0,                    NULL,           0 },
        { "coastline",          SHORELINE,      0 },            /* 1 */
        { "water",              LAKE,           AREA },         /* 2 */
        { "wood",               NULL,           AREA },         /* 3 */
        { "peak",               NULL,           0 },            /* 4 */
};

static layer_info_t boundary_to_layer[] = {
        { 0,                    NULL,           0 },
        { "administrative",     NULL,           AREA },         /* 1 */
        { "civil",              NULL,           AREA },         /* 2 */
        { "political",          NULL,           AREA },         /* 3 */
        { "national_park",      NULL,           AREA },         /* 4 */
};

static layer_info_t amenity_to_layer[] = {
        { 0,                    NULL,           0 },
        { "hospital",           HOSPITAL,       0 },            /* 1 */
        { "pub",                NULL,           0 },            /* 2 */
        { "parking",            NULL,           AREA },         /* 3 */
        { "post_office",        NULL,           0 },            /* 4 */
        { "fuel",               NULL,           0 },            /* 5 */
        { "telephone",          NULL,           0 },            /* 6 */
        { "toilets",            NULL,           0 },            /* 7 */
        { "post_box",           NULL,           0 },            /* 8 */
        { "school",             NULL,           AREA },         /* 9 */
        { "supermarket",        NULL,           0 },            /* 10 */
        { "library",            NULL,           0 },            /* 11 */
        { "theatre",            NULL,           0 },            /* 12 */
        { "cinema",             NULL,           0 },            /* 13 */
        { "police",             NULL,           0 },            /* 14 */
        { "fire_station",       NULL,           0 },            /* 15 */
        { "restaurant",         NULL,           0 },            /* 16 */
        { "fastfood",           NULL,           0 },            /* 17 */
        { "bus_station",        NULL,           0 },            /* 18 */
        { "place_of_worship",   NULL,           0 },            /* 19 */
        { "cafe",               NULL,           0 },            /* 20 */
        { "bicycle_parking",    NULL,           AREA },         /* 21 */
        { "public_building",    NULL,           AREA },         /* 22 */
        { "grave_yard",         NULL,           AREA },         /* 23 */
        { "university",         NULL,           AREA },         /* 24 */
        { "college",            NULL,           AREA },         /* 25 */
        { "townhall",           NULL,           AREA },         /* 26 */
};

static layer_info_t place_to_layer[] = {
        { 0,                    NULL,           0 },
        { "continent",          NULL,           AREA },         /* 1 */
        { "country",            NULL,           AREA },         /* 2 */
        { "state",              NULL,           AREA },         /* 3 */
        { "region",             NULL,           AREA },         /* 4 */
        { "county",             NULL,           AREA },         /* 5 */
        { "city",               NULL,           AREA },         /* 6 */
        { "town",               NULL,           AREA },         /* 7 */
        { "village",            NULL,           AREA },         /* 8 */
        { "hamlet",             NULL,           AREA },         /* 9 */
        { "suburb",             NULL,           AREA },         /* 10 */
};

static layer_info_t leisure_to_layer[] = {
        { 0,                    NULL,           0 },
        { "park",               PARK,           AREA },         /* 1 */
        { "common",             PARK,           AREA },         /* 2 */
        { "garden",             PARK,           AREA },         /* 3 */
        { "nature_reserve",     PARK,           AREA },         /* 4 */
        { "fishing",            NULL,           AREA },         /* 5 */
        { "slipway",            NULL,           0 },            /* 6 */
        { "water_park",         NULL,           AREA },         /* 7 */
        { "pitch",              NULL,           AREA },         /* 8 */
        { "track",              NULL,           AREA },         /* 9 */
        { "marina",             NULL,           AREA },         /* 10 */
        { "stadium",            NULL,           AREA },         /* 11 */
        { "golf_course",        PARK,           AREA },         /* 12 */
        { "sports_centre",      NULL,           AREA },         /* 13 */
};

static layer_info_t historic_to_layer[] = {
        { 0,                    NULL,           0 },
        { "castle",             NULL,           0 },            /* 1 */
        { "monument",           NULL,           0 },            /* 2 */
        { "museum",             NULL,           0 },            /* 3 */
        { "archaeological_site",NULL,           AREA },         /* 4 */
        { "icon",               NULL,           0 },            /* 5 */
        { "ruins",              NULL,           AREA },         /* 6 */
};

#if NEEDED
static char *oneway_type[] = {
        0,
        "no",                   /* 0 */
        "yes",                  /* 1 */
        "-1"                    /* 2 */
};
#endif


static layer_info_sublist_t list_info[] = {
        {0,                     NULL,                 NULL },
        {"highway",             highway_to_layer,     NULL },
        {"cycleway",            cycleway_to_layer,    NULL },
        {"tracktype",           NULL,                 TRAIL },
        {"waterway",            waterway_to_layer,    RIVER },
        {"railway",             railway_to_layer,     NULL },
        {"aeroway",             NULL,                 NULL },
        {"aerialway",           NULL,                 NULL },
        {"power",               NULL,                 NULL },
        {"man_made",            NULL,                 NULL },
        {"leisure",             leisure_to_layer,     NULL },
        {"amenity",             amenity_to_layer,     NULL },
        {"shop",                NULL,                 NULL },
        {"tourism",             NULL,                 NULL },
        {"historic",            historic_to_layer,    NULL },
        {"landuse",             NULL,                 NULL },
        {"military",            NULL,                 NULL },
        {"natural",             natural_to_layer,     NULL },
        {"route",               NULL,                 NULL },
        {"boundary",            boundary_to_layer,    NULL },
        {"sport",               NULL,                 NULL },
        {"abutters",            abutters_to_layer,    NULL },
        {"fenced",              NULL,                 NULL },
        {"lit",                 NULL,                 NULL },
        {"area",                NULL,                 NULL },
        {"bridge",              NULL,                 MAIN },
        {"cutting",             NULL,                 NULL },
        {"embankment",          NULL,                 NULL },
        {"surface",             NULL,                 NULL },
        {"access",              NULL,                 NULL },
        {"bicycle",             NULL,                 NULL },
        {"foot",                NULL,                 NULL },
        {"goods",               NULL,                 NULL },
        {"hgv",                 NULL,                 NULL },
        {"horse",               NULL,                 NULL },
        {"motorcycle",          NULL,                 NULL },
        {"motorcar",            NULL,                 NULL },
        {"psv",                 NULL,                 NULL },
        {"motorboat",           NULL,                 NULL },
        {"boat",                NULL,                 NULL },
        {"oneway",              NULL,                 NULL },
        {"noexit",              NULL,                 NULL },
        {"toll",                NULL,                 NULL },
        {"place",               place_to_layer,       NULL },
        {"lock",                NULL,                 NULL },
        {"attraction",          NULL,                 NULL },
        {"wheelchair",          NULL,                 NULL },
        {"junction",            NULL,                 NULL },
};

static unsigned char *
read_4_byte_int(unsigned char *p, int *r)
{
    *r = (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];
    return p + 4;
}

static unsigned char *
read_2_byte_int(unsigned char *p, int *r)
{
    short s;
    s = (p[1] << 8) + p[0];
    *r = s;
    return p + 2;
}

static int
buildmap_osm_binary_parse_options
        (unsigned char *cur, unsigned char *end, char *name, int *flagp)
{

    int key, slen;
    int layer = 0;

    if (buildmap_is_verbose()) {
        fprintf(stderr, "option data is\n");
        int i;
        for (i = 0; i < end - cur; i++) {
            fprintf(stderr, " %02x", cur[i]);
        }
        fprintf(stderr, "\n");

    }

    if (name != NULL)
        *name = '\0';

    while (cur < end) {
        key = *cur++;
        if (key >= key_string_start) {

            key -= key_string_start;
            slen = *cur++;

            buildmap_verbose("'%s' is %.*s", stringtype[key], slen, cur);

            if (key == 0 && name != NULL) {     /* regular "name" */
                memcpy(name, cur, slen);
                name[slen] = '\0';
            }
            cur += slen;

        } else if (key >= key_date_start) {
            time_t tim;
            int itime;

            key -= key_date_start;

            cur = read_4_byte_int(cur, &itime);
            tim = (time_t)itime;

            buildmap_verbose("'%s' is %s", datetype[key], asctime(gmtime(&tim)));

        } else if (key >= key_numeric_start) {
            int val;

            key -= key_numeric_start;
            cur = read_4_byte_int(cur, &val);

            buildmap_verbose("'%s' is %ld", stringtype[key], val);

        } else {
            int val;

            /* the rest are enumerated types, and we have lists for
             * the ones we're interested in.
             */
            val = *cur++;

            buildmap_verbose("list type is '%s', subtype ", list_info[key].name);

            if (list_info[key].list) {
                buildmap_verbose("'%s'", list_info[key].list[val].name);

                if (list_info[key].list[val].layerp) {
                    layer = *list_info[key].list[val].layerp;
                    if (flagp) *flagp = list_info[key].list[val].flags;
                    buildmap_verbose("maps with list to layer %d", layer);
                }
            } else {
                buildmap_verbose("%d", val);

                if (list_info[key].layerp) {
                    layer = *list_info[key].layerp;
                    buildmap_verbose("maps to layer %d", layer);
                }
            }

        }
    }
    return layer;
}

static int
buildmap_osm_binary_node(unsigned char *data, int len)
{
    int id, lon, lat;
    unsigned char *dp = data;
    int prop;
    int layer;

    dp = read_4_byte_int(dp, &id);
    dp = read_4_byte_int(dp, &lon);
    dp = read_4_byte_int(dp, &lat);
    prop = *dp++;

    buildmap_verbose("node: id %ld, lon %ld, lat %ld, prop %d", id, lon, lat, prop);

    if (dp < data + len)
        layer = buildmap_osm_binary_parse_options(dp, data + len, NULL, NULL);

    /* currently, RoadMap has no ability to display simple
     * points, or "nodes" in OSM vocabulary.  so we do this
     * parsing, then drop it on the floor.
     */

    return 0;
}


struct shapeinfo {
    int lineid;
    int count;
    int *lons;
    int *lats;
};

static int numshapes;
static struct shapeinfo *shapes;


static int
buildmap_osm_binary_way(unsigned char *data, int len)
{
    int id, count, savecount;
    int *lonp, *latp;
    unsigned char *dp, *op;
    int layer, flags, line, street, j;
    int frlong, frlat, tolong, tolat, from_point, to_point;
    char name[257];
    static int *lonsbuf, *latsbuf;
    static int lineid = 1;

    RoadMapString rms_dirp = str2dict(DictionaryPrefix, "");
    RoadMapString rms_dirs = str2dict(DictionarySuffix, "");
    RoadMapString rms_type = str2dict(DictionaryType, "");
    RoadMapString rms_name;

    dp = data;
    dp = read_4_byte_int(dp, &id);
    dp = read_4_byte_int(dp, &count);

    /* look ahead to find the options */
    op = dp + 4 + 4 + (count - 1) * (2 + 2);
    if (op >= data + len)
        return 0;

    layer = buildmap_osm_binary_parse_options(op, data + len, name, &flags);

    buildmap_verbose("'%s' is in layer %d", name, layer);

    if (!layer)
        return 0;

    savecount = count;

    lonp = lonsbuf = realloc(lonsbuf, count * sizeof(long));
    buildmap_check_allocated(lonsbuf);
    dp = read_4_byte_int(dp, lonp);

    latp = latsbuf = realloc(latsbuf, count * sizeof(long));
    buildmap_check_allocated(latsbuf);
    dp = read_4_byte_int(dp, latp);

    buildmap_verbose("way: id %ld", id);
    buildmap_verbose("    lon %ld, lat %ld", *lonp, *latp);

    count--;
    while (count--) {
        int tmp;

        dp = read_2_byte_int(dp, &tmp);
        *(lonp + 1) = *lonp + tmp;

        dp = read_2_byte_int(dp, &tmp);
        *(latp + 1) = *latp + tmp;

        lonp++;
        latp++;
        buildmap_verbose("    lon %ld, lat %ld", *lonp, *latp);
    }

    frlong = *lonsbuf;
    frlat = *latsbuf;

    tolong = *lonp;
    tolat = *latp;

    if ((flags & AREA) && (frlong == tolong) && (frlat == tolat)) {
        static int landid;
        static int polyid;
        static int cenid;

        buildmap_verbose("looks like a polygon");

        buildmap_polygon_add_landmark
            (++landid, layer, 0 /* RoadMapString name */ );

        buildmap_polygon_add(landid, ++cenid, ++polyid);

        count = savecount - 1;
        frlong = *lonsbuf;
        frlat = *latsbuf;
        from_point = buildmap_point_add(frlong, frlat);
        lonp = lonsbuf;
        latp = latsbuf;
        while (count--) {

            tolong = *(lonp + 1);
            tolat = *(latp + 1);

            to_point = buildmap_point_add(tolong, tolat);
            line = buildmap_line_add(++lineid, layer, from_point, to_point);

            buildmap_verbose("from: %d, %d        to: %d, %d      lineid %d",
                   frlong, frlat, tolong, tolat, lineid);

            buildmap_polygon_add_line
                (cenid, polyid, lineid, POLYGON_SIDE_RIGHT);
            frlong = tolong;
            frlat = tolat;
            from_point = to_point;
            lonp++;
            latp++;
        }
    } else {

        from_point = buildmap_point_add(frlong, frlat);
        to_point = buildmap_point_add(tolong, tolat);
        line = buildmap_line_add(++lineid, layer, from_point, to_point);

        buildmap_verbose("from: %d, %d    to: %d, %d      lineid %d",
               frlong, frlat, tolong, tolat, lineid);

        rms_name = str2dict(DictionaryStreet, name);
        street =
            buildmap_street_add(layer, rms_dirp, rms_name, rms_type,
                                rms_dirs, line);

        buildmap_range_add_no_address(line, street);

        for (j = 0; j < lonp - lonsbuf + 1; j++) {
            buildmap_square_adjust_limits(lonsbuf[j], latsbuf[j]);
        }

        shapes = realloc(shapes,
                    ((numshapes + 1024) & ~1023) * sizeof(*shapes));
        buildmap_check_allocated(shapes);
        shapes[numshapes].lons = lonsbuf;
        shapes[numshapes].lats = latsbuf;
        shapes[numshapes].count = lonp - lonsbuf + 1;
        shapes[numshapes].lineid = lineid;
        lonsbuf = latsbuf = 0;
        numshapes++;
    }

    return 1;
}

static int
buildmap_osm_binary_ways_pass2(void)
{
    int i, j, count, lineid;
    int *lons, *lats;
    int line_index;
    buildmap_info("loading shape info ...");
    buildmap_line_sort();
    for (i = 0; i < numshapes; i++) {

        lons = shapes[i].lons;
        lats = shapes[i].lats;
        count = shapes[i].count;
        lineid = shapes[i].lineid;
        if (count <= 2)
            continue;
        line_index = buildmap_line_find_sorted(lineid);
        if (line_index >= 0) {

            // Add the shape points here
            for (j = 1; j < count - 1; j++) {
                buildmap_shape_add
                    (line_index, i, lineid, j - 1, lons[j], lats[j]);
            }
        }
        if ((i & 0xff) == 0) {
            buildmap_progress(i, numshapes);
        }

    }
    return 1;
}

static int
buildmap_osm_binary_time(unsigned char *data)
{
    int itime;
    time_t time;

    /* we do nothing with this (for now?) */
    read_4_byte_int(data, &itime);

    time = (time_t)itime;

    buildmap_verbose("Creation time is '%d', '%s'",
               itime, asctime(gmtime(&time)));
    return 0;
}

static int
buildmap_osm_binary_error(unsigned char *data)
{
    int error = *data++;
    int slen = *data++;
    buildmap_error(0, "Error %d: '%.*s'", error, slen, data);
    if (error == 5)
        return -2;
    return -1;
}

int
buildmap_osm_binary_read(FILE * fdata)
{
    unsigned char *block = NULL;
    int length, got;
    unsigned char type;
    unsigned char buf[5];;
    int n;
    int ret = 0, ranways = 0;

    DictionaryPrefix = buildmap_dictionary_open("prefix");
    DictionaryStreet = buildmap_dictionary_open("street");
    DictionaryType = buildmap_dictionary_open("type");
    DictionarySuffix = buildmap_dictionary_open("suffix");
    DictionaryCity = buildmap_dictionary_open("city");

    n = 1;

    while (ret >= 0) {
        buildmap_set_line(n);
        got = (int)fread(buf, 1, 5, fdata);
        if (got != 5) {
            if (feof(fdata))
                break;
            buildmap_fatal(0, "short read (length)");
        }

        /* four bytes of length */
        read_4_byte_int(buf, &length);

        buildmap_verbose("length is %ld", length);

        /* fifth byte is type */
        type = buf[4];
        buildmap_verbose("type is %c", type);

        block = realloc(block, length - 1);
        buildmap_check_allocated(block);
        got = (int)fread(block, 1, length - 1, fdata);
        if (got != length - 1) {
            if (feof(fdata))
                break;
            buildmap_fatal(0, "short read (data)");
        }

        if (0 && buildmap_is_verbose()) {
            fprintf(stderr, "data is");
            int i;
            for (i = 0; i < length - 1; i++) {
                fprintf(stderr, " %02x", block[i]);
            }
            fprintf(stderr, "\n");
        }

        switch (type) {

        case 'n':               // node
            ret += buildmap_osm_binary_node(block, length - 1);
            break;

        case 'w':               // way
            if (buildmap_osm_binary_way(block, length - 1)) {
                ret++;
                ranways = 1;
            }
            break;

        case 'o':               // omit
            // ret += buildmap_osm_binary_omit(block);
            break;

        case 't':               // time
            ret += buildmap_osm_binary_time(block);
            break;

        case 'f':               // failure
            ret = buildmap_osm_binary_error(block);
            break;

        default:
            break;
        }
    }

    if (ranways) buildmap_osm_binary_ways_pass2();

    if (block) free(block);

    return ret;
}


