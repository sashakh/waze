/*
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
 */

/**
 * @file
 * @brief a module to convert OSM (OpenStreetMap) data into RoadMap maps.
 *
 * This file contains the static data.
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
#include "buildmap_osm_common.h"
#include "buildmap_osm_binary.h"

/* Road layers. */

int BuildMapLayerFreeway = 0;
int BuildMapLayerRamp = 0;
int BuildMapLayerMain = 0;
int BuildMapLayerStreet = 0;
int BuildMapLayerTrail = 0;
int BuildMapLayerRail = 0;

/* Area layers. */

int BuildMapLayerPark = 0;
int BuildMapLayerHospital = 0;
int BuildMapLayerAirport = 0;
int BuildMapLayerStation = 0;
int BuildMapLayerMall = 0;

/* Water layers. */

int BuildMapLayerShoreline = 0;
int BuildMapLayerRiver = 0;
int BuildMapLayerCanal = 0;
int BuildMapLayerLake = 0;
int BuildMapLayerSea = 0;

int BuildMapLayerBoundary = 0;

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

BuildMapDictionary DictionaryPrefix;
BuildMapDictionary DictionaryStreet;
BuildMapDictionary DictionaryType;
BuildMapDictionary DictionarySuffix;
BuildMapDictionary DictionaryCity;

/**
 * @brief initialize layers
 */
void buildmap_osm_common_find_layers (void) {

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

char *stringtype[] = {
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

char *datetype[] = {
        "date_on",
        "date_off",
        "start_date",
        "end_date",
};

#if NEEDED
char *numeric_type[] = {
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

layer_info_t highway_to_layer[] = {
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
        { 0,                    NULL,           0 },
};

layer_info_t cycleway_to_layer[] = {
        { 0,                    NULL,           0 },
        { "lane",               TRAIL,          0 },            /* 1 */
        { "track",              TRAIL,          0 },            /* 2 */
        { "opposite_lane",      TRAIL,          0 },            /* 3 */
        { "opposite_track",     TRAIL,          0 },            /* 4 */
        { "opposite",           NULL,           0 },            /* 5 */
        { 0,                    NULL,           0 },
};

layer_info_t waterway_to_layer[] = {
        { 0,                    NULL,           0 },
        { "river",              RIVER,          0 },            /* 1 */
        { "canal",              RIVER,          0 },            /* 2 */
	{ "stream",		RIVER,		0 },		/* 3 */
	{ "drain",		RIVER,		0 },		/* 4 */
	{ "dock",		RIVER,		0 },		/* 5 */
	{ "lock_gate",		RIVER,		0 },		/* 6 */
	{ "turning_point",	RIVER,		0 },		/* 7 */
	{ "aquaduct",		RIVER,		0 },		/* 8 */
	{ "boatyard",		RIVER,		0 },		/* 9 */
	{ "water_point",	RIVER,		0 },		/* 10 */
	{ "weir",		RIVER,		0 },		/* 11 */
	{ "dam",		RIVER,		0 },		/* 12 */
        { 0,                    NULL,           0 },
};

layer_info_t abutters_to_layer[] = {
        { 0,                    NULL,           0 },
        { "residential",        NULL,           0 },            /* 1 */
        { "retail",             NULL,           0 },            /* 2 */
        { "industrial",         NULL,           0 },            /* 3 */
        { "commercial",         NULL,           0 },            /* 4 */
        { "mixed",              NULL,           0 },            /* 5 */
        { 0,                    NULL,           0 },
};

layer_info_t railway_to_layer[] = {
	{ 0,			NULL,           0 },
	{ "rail",		RAIL,           0 },            /* 1 */
	{ "tram",		RAIL,           0 },            /* 2 */
	{ "light_rail",		RAIL,           0 },            /* 3 */
	{ "subway",		RAIL,           0 },            /* 4 */
	{ "station",		NULL,           0 },            /* 5 */
	{ "preserved",		RAIL,		0 },            /* 6 */
	{ "disused",		RAIL,           0 },            /* 7 */
	{ "abandoned",		RAIL,		0 },		/* 8 */
	{ "narrow_gauge",	RAIL,		0 },            /* 9 */
	{ "monorail",		RAIL,           0 },            /* 10 */
	{ "halt",		RAIL,           0 },            /* 11 */
	{ "tram_stop",		RAIL,           0 },            /* 12 */
	{ "viaduct",		RAIL,           0 },            /* 13 */
	{ "crossing",		RAIL,           0 },            /* 14 */
	{ "level_crossing",	RAIL,           0 },            /* 15 */
	{ "subway_entrance",	RAIL,           0 },            /* 16 */
	{ "turntable",		RAIL,           0 },            /* 17 */
	{ 0,			NULL,           0 },
};

layer_info_t natural_to_layer[] = {
        { 0,                    NULL,           0 },
        { "coastline",          SHORELINE,      0 },            /* 1 */
        { "water",              LAKE,           AREA },         /* 2 */
        { "wood",               NULL,           AREA },         /* 3 */
        { "peak",               NULL,           0 },            /* 4 */
        { 0,                    NULL,           0 },
};

layer_info_t boundary_to_layer[] = {
        { 0,                    NULL,           0 },
        { "administrative",     NULL,           AREA },         /* 1 */
        { "civil",              NULL,           AREA },         /* 2 */
        { "political",          NULL,           AREA },         /* 3 */
        { "national_park",      NULL,           AREA },         /* 4 */
	{ "world_country",	NULL,		AREA },		/* 5 */
        { 0,                    NULL,           0 },
};

layer_info_t amenity_to_layer[] = {
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
        { 0,                    NULL,           0 },
};

layer_info_t place_to_layer[] = {
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
        { 0,                    NULL,           0 },
};

layer_info_t leisure_to_layer[] = {
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
        { 0,                    NULL,           0 },
};

layer_info_t historic_to_layer[] = {
        { 0,                    NULL,           0 },
        { "castle",             NULL,           0 },            /* 1 */
        { "monument",           NULL,           0 },            /* 2 */
        { "museum",             NULL,           0 },            /* 3 */
        { "archaeological_site",NULL,           AREA },         /* 4 */
        { "icon",               NULL,           0 },            /* 5 */
        { "ruins",              NULL,           AREA },         /* 6 */
        { 0,                    NULL,           0 },
};

#if NEEDED
char *oneway_type[] = {
        0,
        "no",                   /* 0 */
        "yes",                  /* 1 */
        "-1"                    /* 2 */
};
#endif


layer_info_sublist_t list_info[] = {
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
        { 0,                    NULL,           0 },
};

