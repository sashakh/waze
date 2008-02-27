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
#include "roadmap_path.h"


static RoadMapConfigDescriptor RoadMapConfigGeometryMain =
                        ROADMAP_CONFIG_ITEM("Geometry", "Main");

static RoadMapConfigDescriptor RoadMapConfigGeneralUnit =
                        ROADMAP_CONFIG_ITEM("General", "Unit");

static RoadMapConfigDescriptor RoadMapConfigAddressPosition =
                        ROADMAP_CONFIG_ITEM("Address", "Position");

static RoadMapConfigDescriptor RoadMapConfigGeneralToolbar =
                        ROADMAP_CONFIG_ITEM("General", "Toolbar");

static RoadMapConfigDescriptor RoadMapConfigGeneralIcons =
                        ROADMAP_CONFIG_ITEM("General", "Icons");

static RoadMapConfigDescriptor RoadMapConfigMapCache =
                        ROADMAP_CONFIG_ITEM("Map", "Cache");

static RoadMapConfigDescriptor RoadMapConfigTripName =
                        ROADMAP_CONFIG_ITEM("Trip", "Name");


static int roadmap_option_verbose = ROADMAP_MESSAGE_WARNING;
static int roadmap_option_no_area = 0;
static int roadmap_option_square  = 0;
static int roadmap_option_global_sq  = 0;
static int roadmap_option_cache_size = 0;
static int roadmap_option_synchronous = 0;

static char **roadmap_option_debug = NULL;
static char *roadmap_option_gps = NULL;
static char *roadmap_option_config = NULL;
static char *roadmap_option_icons = NULL;

static RoadMapUsage RoadMapOptionUsage = NULL;


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
      case ROADMAP_SHOW_GLOBAL_SQUARE:
         return roadmap_option_square || roadmap_option_global_sq;;
   }

   return 1;
}


char *roadmap_gps_source (void) {

   return roadmap_option_gps;
}

char *roadmap_extra_config (void) {

   return roadmap_option_config;
}

char *roadmap_icon_path (void) {

   return roadmap_option_icons;
}


int roadmap_verbosity (void) {

   return roadmap_option_verbose;
}


char **roadmap_debug (void) {

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
        /* assumes no dialog named "default" */
        option = roadmap_option_get_geometry ("Default");
    }

    if (option == NULL || option[0] == 0) {
        return 300;
    }
    return atoi (option);
}


int roadmap_option_height (const char *name) {

    const char *option = roadmap_option_get_geometry (name);
    char *separator;

    if (option == NULL || option[0] == 0) {
        /* assumes no dialog named "default" */
        option = roadmap_option_get_geometry ("Default");
    }

    separator = strchr (option, 'x');
    if (separator == NULL) {
        return 200;
    }
    return atoi(separator+1);
}


int roadmap_option_is_synchronous (void) {

   return roadmap_option_synchronous;
}


static void roadmap_option_set_configpath (const char *value) {

    if (!value || !value[0]) {
       roadmap_log (ROADMAP_FATAL, "invalid config path '%s'", value);
    }

    if (roadmap_option_config != NULL) {
        free (roadmap_option_config);
    }
    roadmap_option_config = strdup (value);

    roadmap_path_set("config", value);
}

static void roadmap_option_set_iconpath (const char *value) {

    if (!value || !value[0]) {
       roadmap_log (ROADMAP_FATAL, "invalid icon path '%s'", value);
    }

    if (roadmap_option_icons != NULL) {
        free (roadmap_option_icons);
    }
    roadmap_option_icons = strdup (value);

    roadmap_path_set("icons", value);
}

static void roadmap_option_set_mappath (const char *value) {

    if (!value || !value[0]) {
       roadmap_log (ROADMAP_FATAL, "invalid map path '%s'", value);
    }
    roadmap_path_set("maps", value);
}

static void roadmap_option_set_location (const char *value) {

    roadmap_config_set (&RoadMapConfigAddressPosition, value);
}

static void roadmap_option_set_tripname (const char *value) {

    roadmap_config_set (&RoadMapConfigTripName, value);
}


static void roadmap_option_set_metric (const char *value) {

    roadmap_config_set (&RoadMapConfigGeneralUnit, "metric");
}


static void roadmap_option_set_imperial (const char *value) {

    roadmap_config_set (&RoadMapConfigGeneralUnit, "imperial");
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


static void roadmap_option_set_no_icon (const char *value) {

    roadmap_config_set (&RoadMapConfigGeneralIcons, "no");
}


static void roadmap_option_set_square (const char *value) {

    roadmap_option_square = 1;
}

static void roadmap_option_map_boxes (const char *value) {

    roadmap_option_global_sq = 1;
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

       int i;
       int j;
       int count;
       char *debug = strdup (value);

       for (count = 1, i = 0; debug[i] > 0; ++i) if (debug[i] == ',') ++count;

       roadmap_option_debug = (char **) calloc (count+1, sizeof(char *));

       roadmap_option_debug[0] = debug;

       for (j = 0, i = 0; debug[i] > 0 && j < count; ++i) {
          if (debug[i] == ',') {
             roadmap_option_debug[++j] = debug + i + 1;
             debug[i] = 0;
          }
       }
       roadmap_option_debug[j+1] = NULL;
    }
}


static void roadmap_option_set_verbose (const char *value) {

    if (roadmap_option_verbose > ROADMAP_MESSAGE_INFO) {
        roadmap_option_verbose = ROADMAP_MESSAGE_INFO;
    }
}

static void roadmap_option_set_synchronous (const char *value) {

    roadmap_option_synchronous = 1;
}

static void roadmap_option_usage (const char *value);


typedef void (*roadmap_option_handler) (const char *value);

struct roadmap_option_descriptor {
    
    const char *name;
    const char *format;
    
    roadmap_option_handler handler;

    int pass;
    
    const char *help;
};

static struct roadmap_option_descriptor RoadMapOptionMap[] = {
    
    {"--config=", "CONFIGPATH", roadmap_option_set_configpath, 0,
        "Override the built-in path to the configuration files"},

    {"--maps=", "MAPPATH", roadmap_option_set_mappath, 1,
        "Override the built-in (or configured) path to the map files"},

    {"--icons=", "ICONPATH", roadmap_option_set_iconpath, 1,
        "Override the built-in (or configured) path to the icon images"},

    {"--location=", "LONGITUDE,LATITUDE", roadmap_option_set_location, 1,
        "Set the location point (see menu entry Screen/Show Location..)"},

    {"--metric", "", roadmap_option_set_metric, 1,
        "Use the metric system for all units"},

    {"--imperial", "", roadmap_option_set_imperial, 1,
        "Use the imperial system for all units"},

    {"--no-area", "", roadmap_option_set_no_area, 1,
        "Do not show the polygons (parks, hospitals, airports, etc..)"},

    {"-geometry=", "WIDTHxHEIGHT", roadmap_option_set_geometry1, 1,
        "Same as the --geometry option below"},

    {"--geometry=", "WIDTHxHEIGHT", roadmap_option_set_geometry1, 1,
        "Set the geometry of the RoadMap main window"},

    {"--geometry:", "WINDOW=WIDTHxHEIGHT", roadmap_option_set_geometry2, 1,
        "Set the geometry of a specific RoadMap window"},

    {"--no-toolbar", "", roadmap_option_set_no_toolbar, 1,
        "Hide the RoadMap main window's toolbar"},

    {"--no-icon", "", roadmap_option_set_no_icon, 1,
        "Do not show icons on the toolbar"},

    {"--map-boxes", "", roadmap_option_map_boxes, 1,
        "Show map bounding boxes as grey lines (debug and troubleshooting)"},

    {"--square", "", roadmap_option_set_square, 1,
        "Show the square boundaries as grey lines (for debug purpose)"},

    {"--gps=", "URL", roadmap_option_set_gps, 1,
        "Use a specific GPS source (mainly for replay of a GPS log)"},

    {"--gps-sync", "", roadmap_option_set_synchronous, 1,
        "Update the map synchronously when receiving each GPS position"},

    {"--trip=", "FILE", roadmap_option_set_tripname, 1,
        "Set the name of the current trip (relative to path in General.TripPaths"},

    {"--cache=", "INTEGER", roadmap_option_set_cache, 1,
        "Set the number of entries in the RoadMap's map cache"},

    {"--debug", "", roadmap_option_set_debug, 0,
        "Show all informational and debug traces"},

    {"--debug=", "SOURCE[,SOURCE...]", roadmap_option_set_debug, 0,
        "Show the informational and debug traces for the specified sources"},

    {"--verbose", "", roadmap_option_set_verbose, 1,
        "Show all informational traces"},

    {"-h", "", roadmap_option_usage, 0,
        "Show this help message"},

    {"-help", "", roadmap_option_usage, 0,
        "Show this help message"},

    {"--help", "", roadmap_option_usage, 0,
        "Show this help message"},

    {"--help=", "OPTIONS/KEYMAP/ACTIONS/ALL", roadmap_option_usage, 1,
        "Show a section of the help message"},

    {NULL, NULL, NULL, 0, NULL}
};


static void roadmap_option_usage (const char *value) {

    struct roadmap_option_descriptor *option;
    int all = (value != NULL && strcasecmp(value, "all") == 0);

    if ((value == NULL) || all || (strcasecmp (value, "options") == 0)) {

       printf ("OPTIONS:\n");

       for (option = RoadMapOptionMap; option->name != NULL; ++option) {

          printf ("  %s%s\n", option->name, option->format);
          printf ("        %s.\n", option->help);
       }
       if (!all)
          exit (0);
    }

    if (RoadMapOptionUsage != NULL) {
       RoadMapOptionUsage (all ? NULL : value);
    }
    exit(0);
}


void roadmap_option (int argc, char **argv, int pass, RoadMapUsage usage) {

    int   i;
    int   length;
    int   compare;
    char *value;
    struct roadmap_option_descriptor *option;


    RoadMapOptionUsage = usage;

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
                if (compare != 0) {
                    if (strncmp(option->name, argv[i], length-1) == 0 &&
                            option->name[length-1] == '=') {
                        roadmap_log (ROADMAP_FATAL,
                            "invalid option ('%s' needs value '%s=%s')",
                            argv[i], option->name, option->format);
                    }
                }
            }

            if (compare == 0) {
                if (pass == option->pass) {
                    option->handler (value);
                }
                break;
            }
        }
        
        if (compare != 0) {
            roadmap_log (ROADMAP_FATAL, "illegal option %s", argv[i]);
        }
    }

    RoadMapOptionUsage = NULL;
}

void roadmap_option_initialize (void) {

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralToolbar, "yes", "no", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralIcons, "yes", "no", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigMapCache, "8");
}

