/* roadmap_help.c - Manage access to some help.
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
 *   See roadmap_help.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_config.h"

#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_spawn.h"

#include "roadmap_help.h"

#define RDM_MANUAL "usermanual.html"

#ifndef ROADMAP_BROWSER
#define ROADMAP_BROWSER "firefox"
#endif

static int RoadMapHelpSourceLocal;

static RoadMapConfigDescriptor RoadMapConfigBrowser =
                        ROADMAP_CONFIG_ITEM("Help", "Browser");

static RoadMapConfigDescriptor RoadMapConfigHelpWebsite =
                        ROADMAP_CONFIG_ITEM("Help", "Help Website");

static RoadMapConfigDescriptor RoadMapConfigHelpSource =
                        ROADMAP_CONFIG_ITEM("Help", "Source");

static char *RoadMapHelpManualUrl = NULL;

void roadmap_help_toggle_source (void) {
    RoadMapHelpSourceLocal = ! RoadMapHelpSourceLocal;
    if (RoadMapHelpSourceLocal) {
        roadmap_config_set (&RoadMapConfigHelpSource , "local");
    } else {
        roadmap_config_set (&RoadMapConfigHelpSource , "web");
    }
}


/* -- The help display functions. -------------------------------------- */

static void roadmap_help_make_url (const char *path) {

   if (!RoadMapHelpManualUrl) {
      RoadMapHelpManualUrl = malloc (1024);
      roadmap_check_allocated (RoadMapHelpManualUrl);
   }

   /* leave room for section name in url string (added later) */
   if (RoadMapHelpSourceLocal) {
       snprintf(RoadMapHelpManualUrl, 1024 - 30, "file://%s/%s",
          path, RDM_MANUAL);
   } else {
       snprintf(RoadMapHelpManualUrl, 1024 - 30, "http://%s/%s",
          roadmap_config_get(&RoadMapConfigHelpWebsite), RDM_MANUAL);
   }

}

static int roadmap_help_prepare (void) {

   const char *path;

   if (!RoadMapHelpSourceLocal) {
      roadmap_help_make_url("");
      return 1;
   }

   /* First look for the user directory. */
   path = roadmap_path_user();
   if (roadmap_file_exists(path, RDM_MANUAL)) {
      roadmap_help_make_url (path);
      return 1;
   }


   /* Then look throughout the system path list. */

   for (path = roadmap_path_first("config");
         path != NULL;
         path = roadmap_path_next("config", path))
   {
      if (roadmap_file_exists(path, RDM_MANUAL)) {
         roadmap_help_make_url (path);
         return 1;
      }
   }

   roadmap_log(ROADMAP_ERROR, RDM_MANUAL " not found");
   return 0;
}

static void roadmap_help_show (const char *index) {

    if (!roadmap_help_prepare()) {
          return;
    }

    if (index == NULL || index[0] == 0) index = "#TableOfContents";

    roadmap_log(ROADMAP_DEBUG, "activating help %s", index);

    if (index) {
       strcat (RoadMapHelpManualUrl, index);
    }

    roadmap_spawn
        (roadmap_config_get(&RoadMapConfigBrowser), RoadMapHelpManualUrl);
}

static void roadmap_help_contents(void)     {roadmap_help_show (0);}
static void roadmap_help_quickstart(void)   {roadmap_help_show ("#Quickstart");}
static void roadmap_help_installation(void) {roadmap_help_show ("#Installation");}
static void roadmap_help_using(void)        {roadmap_help_show ("#UsingRoadmap");}
static void roadmap_help_trips(void)        {roadmap_help_show ("#ManagingTrips");}
static void roadmap_help_configuration(void){roadmap_help_show ("#Configuration");}
static void roadmap_help_osm(void)          {roadmap_help_show ("#OpenStreetMap");}


/* -- The help display dictionnary. ------------------------------------ */

typedef struct {
   const char *label;
   RoadMapCallback callback;
} RoadMapHelpList;

static RoadMapHelpList RoadMapHelpTopics[] = {
   {"Table of Contents",            roadmap_help_contents},
   {"Quickstart",                   roadmap_help_quickstart},
   {"Installation",                 roadmap_help_installation},
   {"Using RoadMap",                roadmap_help_using},
   {"Managing Trips",               roadmap_help_trips},
   {"Configuration",                roadmap_help_configuration},
   {"OpenStreetMap",                roadmap_help_osm},
   {NULL, NULL}
};
static RoadMapHelpList *RoadMapHelpTopicsCursor = NULL;

/* -- The help initialization functions. ------------------------------- */

static int roadmap_help_get_topic (const char **label,
                                   RoadMapCallback *callback) {

   if (RoadMapHelpTopicsCursor->label == NULL) {
      RoadMapHelpTopicsCursor = NULL;
      return 0;
   }

   *label = RoadMapHelpTopicsCursor->label;
   *callback = RoadMapHelpTopicsCursor->callback;

   return 1;
}

int roadmap_help_first_topic (const char **label,
                              RoadMapCallback *callback) {

   RoadMapHelpTopicsCursor = RoadMapHelpTopics;

   return roadmap_help_get_topic(label, callback);
}

int roadmap_help_next_topic (const char **label,
                             RoadMapCallback *callback) {

   if (RoadMapHelpTopicsCursor == NULL) {
      roadmap_log(ROADMAP_ERROR, "next called before first");
      return 0;
   }
   if (RoadMapHelpTopicsCursor->label == NULL) {
      RoadMapHelpTopicsCursor = NULL;
      return 0;
   }
   
   RoadMapHelpTopicsCursor += 1;
   return roadmap_help_get_topic(label, callback);
}


void roadmap_help_initialize (void) {

   // unsafe.  no longer supported
   // roadmap_config_declare
   //    ("preferences", &RoadMapConfigBrowserOptions, "%s");

   roadmap_config_declare
      ("preferences", &RoadMapConfigBrowser, ROADMAP_BROWSER);

   roadmap_config_declare
      ("preferences", &RoadMapConfigHelpWebsite, "roadmap.sourceforge.net");

   roadmap_config_declare_enumeration
        ("preferences", &RoadMapConfigHelpSource,
            "local", "web", NULL);

   RoadMapHelpSourceLocal = roadmap_config_match
                                (&RoadMapConfigHelpSource , "local");

}

