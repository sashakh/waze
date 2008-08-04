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
 * @brief include file for the static data of buildmap_osm
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

#define AREA 1

typedef struct layer_info {
    char *name;
    int  *layerp;
    int  flags;
} layer_info_t;

typedef struct layer_info_sublist {
    char *name;
    layer_info_t *list;
    int  *layerp;
} layer_info_sublist_t;

/* Road layers. */

extern int BuildMapLayerFreeway;
extern int BuildMapLayerRamp;
extern int BuildMapLayerMain;
extern int BuildMapLayerStreet;
extern int BuildMapLayerTrail;
extern int BuildMapLayerRail;

/* Area layers. */

extern int BuildMapLayerPark;
extern int BuildMapLayerHospital;
extern int BuildMapLayerAirport;
extern int BuildMapLayerStation;
extern int BuildMapLayerMall;

/* Water layers. */

extern int BuildMapLayerShoreline;
extern int BuildMapLayerRiver;
extern int BuildMapLayerCanal;
extern int BuildMapLayerLake;
extern int BuildMapLayerSea;

extern int BuildMapLayerBoundary;

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

extern BuildMapDictionary DictionaryPrefix;
extern BuildMapDictionary DictionaryStreet;
extern BuildMapDictionary DictionaryType;
extern BuildMapDictionary DictionarySuffix;
extern BuildMapDictionary DictionaryCity;

/**
 * @brief initialize layers
 */
extern void buildmap_osm_common_find_layers (void);
extern char *stringtype[];
extern char *datetype[];
extern layer_info_t highway_to_layer[];
extern layer_info_t cycleway_to_layer[];
extern layer_info_t waterway_to_layer[];
extern layer_info_t abutters_to_layer[];
extern layer_info_t railway_to_layer[];
extern layer_info_t natural_to_layer[];
extern layer_info_t boundary_to_layer[];
extern layer_info_t amenity_to_layer[];
extern layer_info_t place_to_layer[];
extern layer_info_t leisure_to_layer[];
extern layer_info_t historic_to_layer[];
extern layer_info_sublist_t list_info[];
