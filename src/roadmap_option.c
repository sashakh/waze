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

static int roadmap_option_verbose = ROADMAP_MESSAGE_WARNING;
static int roadmap_option_no_area = 0;
static int roadmap_option_square  = 0;

static char *roadmap_option_gps = NULL;


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


int roadmap_option_width (char *name) {
    
    char *option = roadmap_config_get ("Geometry", name);
    
    if (option == NULL) {
        return 300;
    }
    return atoi (option);
}


int roadmap_option_height (char *name) {

    char *option = roadmap_config_get ("Geometry", name);

    option = strchr (option, 'x');
    if (option == NULL) {
        return 200;
    }
    return atoi (option+1);
}


void roadmap_option (int argc, char **argv) {

   int i;

   for (i = 1; i < argc; i++) {

      if (strncmp (argv[i], "--location=", 11) == 0) {

         roadmap_config_set ("Locations", "Location", argv[i] + 11);

      } else if (strcmp (argv[i], "--metric") == 0) {

         roadmap_config_set ("General", "Unit", "metric");

      } else if (strcmp (argv[i], "--imperial") == 0) {

         roadmap_config_set ("General", "Unit", "imperial");

      } else if (strcmp (argv[i], "--no-area") == 0) {

         roadmap_option_no_area = 1;

      } else if (strncmp (argv[i], "--geometry=", 11) == 0) {

         roadmap_config_set ("Geometry", "Main", argv[i]+11);

      } else if (strncmp (argv[i], "--geometry:", 11) == 0) {

         char *p;
         char *value;
         char buffer[256];
          
         strncpy (buffer, argv[i]+11, sizeof(buffer));
          
         value = strchr (buffer, '=');
         if (value == NULL) {
            roadmap_log (ROADMAP_FATAL, "%s: invalid option syntax", argv[i]);
         }
         *(value++) = 0;
         
         for (p = strchr(buffer, '-'); p != NULL; p =strchr(p, '-')) {
             *p = ' ';
         }
         
         roadmap_config_declare ("preferences", "Geometry", buffer, "300x200");
         roadmap_config_set ("Geometry", buffer, value);

      } else if (strcmp (argv[i], "--no-toolbar") == 0) {

         roadmap_config_set ("General", "Toolbar", "no");

      } else if (strcmp (argv[i], "--square") == 0) {

         roadmap_option_square = 1;

      } else if (strncmp (argv[i], "--gps=", 6) == 0) {

         roadmap_option_gps = argv[i] + 6;

      } else if (strcmp (argv[i], "--debug") == 0) {

         if (roadmap_option_verbose > ROADMAP_MESSAGE_DEBUG) {
            roadmap_option_verbose = ROADMAP_MESSAGE_DEBUG;
         }

      } else if (strcmp (argv[i], "--verbose") == 0) {

         if (roadmap_option_verbose > ROADMAP_MESSAGE_INFO) {
            roadmap_option_verbose = ROADMAP_MESSAGE_INFO;
         }

      } else {
         roadmap_log (ROADMAP_FATAL, "illegal option %s", argv[i]);
      }
   }
}

