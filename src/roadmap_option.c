/* roadmap_option.c - Manage the RoadMap command line options.
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
 *   see roadmap.h.
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_config.h"


static RoadMapConfigDescriptor RoadMapConfigGeometryMain =
                        ROADMAP_CONFIG_ITEM("Geometry", "Main");

static RoadMapConfigDescriptor RoadMapConfigGeometryUnit =
                        ROADMAP_CONFIG_ITEM("Geometry", "Unit");

static RoadMapConfigDescriptor RoadMapConfigAddressPosition =
                        ROADMAP_CONFIG_ITEM("Address", "Position");

static RoadMapConfigDescriptor RoadMapConfigGeneralToolbar =
                        ROADMAP_CONFIG_ITEM("General", "Toolbar");

static RoadMapConfigDescriptor RoadMapConfigMapCache =
                        ROADMAP_CONFIG_ITEM("Map", "Cache");


static int roadmap_option_verbose = ROADMAP_MESSAGE_WARNING;
static int roadmap_option_no_area = 0;
static int roadmap_option_square  = 0;
static int roadmap_option_cache_size = 0;

static char *roadmap_option_debug = "";
static char *roadmap_option_gps = NULL;


static const char *roadmap_option_get_geometry (const char *name) {

    RoadMapConfigDescriptor descriptor;
    
    descriptor.category = "Geometry";
    descriptor.name = name;
    descriptor.reference = NULL;

    return roadmap_config_get (&descriptor);
}


int roadmap_is_visible (int category) {

   switch (category) {
      case ROADMAP_SHOW_AREA:
         return (! roadmap_option_no_area);
      case ROADMAP_SHOW_SQUARE:
         return roadmap_option_square;
   }

   return 1;
}


char *roadmap_gps_source (void) {

   return roadmap_option_gps;
}


int roadmap_verbosity (void) {

   return roadmap_option_verbose;
}


char *roadmap_debug (void) {

   return roadmap_option_debug;
}


int roadmap_option_cache (void) {

   if (roadmap_option_cache_size > 0) {
      return roadmap_option_cache_size;
   }
   return roadmap_config_get_integer (&RoadMapConfigMapCache);
}


int roadmap_option_width (const char *name) {

    const char *option = roadmap_option_get_geometry (name);
    
    if (option == NULL || option[0] == 0) {
        return 300;
    }
    return atoi (option);
}


int roadmap_option_height (const char *name) {

    const char *option = roadmap_option_get_geometry (name);
    char *separator;

    separator = strchr (option, 'x');
    if (separator == NULL) {
        return 200;
    }
    return atoi(separator+1);
}


static void roadmap_option_set_location (const char *value) {

    roadmap_config_set (&RoadMapConfigAddressPosition, value);
}


static void roadmap_option_set_metric (const char *value) {

    roadmap_config_set (&RoadMapConfigGeometryUnit, "metric");
}


static void roadmap_option_set_imperial (const char *value) {

    roadmap_config_set (&RoadMapConfigGeometryUnit, "imperial");
}


static void roadmap_option_set_no_area (const char *value) {

    roadmap_option_no_area = 1;
}


static void roadmap_option_set_geometry1 (const char *value) {

    roadmap_config_set (&RoadMapConfigGeometryMain, value);
}


static void roadmap_option_set_geometry2 (const char *value) {

    char *p;
    char *geometry;
    char buffer[256];
    RoadMapConfigDescriptor descriptor;

    strncpy (buffer, value, sizeof(buffer));
          
    geometry = strchr (buffer, '=');
    if (geometry == NULL) {
        roadmap_log (ROADMAP_FATAL,
                     "%s: invalid geometry option syntax", value);
    }
    *(geometry++) = 0;
         
    for (p = strchr(buffer, '-'); p != NULL; p =strchr(p, '-')) {
        *p = ' ';
    }

    descriptor.category = "Geometry";
    descriptor.name = strdup(buffer);
    descriptor.reference = NULL;
    roadmap_config_declare ("preferences", &descriptor, "300x200");
    roadmap_config_set (&descriptor, geometry);
}


static void roadmap_option_set_no_toolbar (const char *value) {

    roadmap_config_set (&RoadMapConfigGeneralToolbar, "no");
}


static void roadmap_option_set_square (const char *value) {

    roadmap_option_square = 1;
}


static void roadmap_option_set_gps (const char *value) {

    if (roadmap_option_gps != NULL) {
        free (roadmap_option_gps);
    }
    roadmap_option_gps = strdup (value);
}


static void roadmap_option_set_cache (const char *value) {

    roadmap_option_cache_size = atoi(value);

    if (roadmap_option_cache_size <= 0) {
       roadmap_log (ROADMAP_FATAL, "invalid cache size %s", value);
    }
    roadmap_config_set (&RoadMapConfigMapCache, value);
}


static void roadmap_option_set_debug (const char *value) {

    if (roadmap_option_verbose > ROADMAP_MESSAGE_DEBUG) {
        roadmap_option_verbose = ROADMAP_MESSAGE_DEBUG;
    }
    if (value != NULL && value[0] != 0) {
       roadmap_option_debug = strdup (value);
    }
}


static void roadmap_option_set_verbose (const char *value) {

    if (roadmap_option_verbose > ROADMAP_MESSAGE_INFO) {
        roadmap_option_verbose = ROADMAP_MESSAGE_INFO;
    }
}


static void roadmap_option_usage (const char *value);


typedef void (*roadmap_option_handler) (const char *value);

struct roadmap_option_descriptor {
    
    const char *name;
    const char *format;
    
    roadmap_option_handler handler;
    
    const char *help;
};

static struct roadmap_option_descriptor RoadMapOptionMap[] = {
    
    {"--location=", "LONGITUDE,LATITUDE", roadmap_option_set_location,
        "Set the location point (see menu entry Screen/Show Location..)"},

    {"--metric", "", roadmap_option_set_metric,
        "Use the metric system for all units"},

    {"--imperial", "", roadmap_option_set_imperial,
        "Use the imperial system for all units"},

    {"--no-area", "", roadmap_option_set_no_area,
        "Do not show the polygons (parks, hospitals, airports, etc..)"},

    {"-geometry=", "WIDTHxHEIGHT", roadmap_option_set_geometry1,
        "Same as the --geometry option below"},

    {"--geometry=", "WIDTHxHEIGHT", roadmap_option_set_geometry1,
        "Set the geometry of the RoadMap main window"},

    {"--geometry:", "WINDOW=WIDTHxHEIGHT", roadmap_option_set_geometry2,
        "Set the geometry of a specific RoadMap window"},

    {"--no-toolbar", "", roadmap_option_set_no_toolbar,
        "Hide the RoadMap main window's toolbar"},

    {"--square", "", roadmap_option_set_square,
        "Show the square boundaries as grey lines (for debug purpose)"},

    {"--gps=", "URL", roadmap_option_set_gps,
        "Use a specific GPS source (mainly for replay of a GPS log)"},

    {"--cache=", "INTEGER", roadmap_option_set_cache,
        "Set the number of entries in the RoadMap's map cache"},

    {"--debug", "", roadmap_option_set_debug,
        "Show all informational and debug traces"},

    {"--debug=", "SOURCE", roadmap_option_set_debug,
        "Show the informational and debug traces for a specific source."},

    {"--verbose", "", roadmap_option_set_verbose,
        "Show all informational traces"},

    {"--help", "", roadmap_option_usage,
        "Show this help message"},

    {NULL, NULL, NULL, NULL}
};


static void roadmap_option_usage (const char *value) {

    struct roadmap_option_descriptor *option;

    for (option = RoadMapOptionMap; option->name != NULL; ++option) {

        printf ("  %s%s\n", option->name, option->format);
        printf ("        %s.\n", option->help);
    }
    exit(0);
}


void roadmap_option (int argc, char **argv) {

    int   i;
    int   length;
    int   compare;
    char *value;
    struct roadmap_option_descriptor *option;


    for (i = 1; i < argc; i++) {

        compare = 1; /* Different. */
        
        for (option = RoadMapOptionMap; option->name != NULL; ++option) {

            if (option->format[0] == 0) {

                value = NULL;
                compare = strcmp (option->name, argv[i]);

            } else {
                length = strlen (option->name);
                value = argv[i] + length;
                compare = strncmp (option->name, argv[i], length);
            }

            if (compare == 0) {
                option->handler (value);
                break;
            }
        }
        
        if (compare != 0) {
            roadmap_log (ROADMAP_FATAL, "illegal option %s", argv[i]);
        }
    }
}


void roadmap_option_initialize (void) {

   roadmap_config_declare ("preferences", &RoadMapConfigMapCache, "8");
}

