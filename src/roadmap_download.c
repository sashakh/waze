/* roadmap_download.c - Download RoadMap maps.
 *
 * LICENSE:
 *
 *   Copyright 2003 Pascal Martin.
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
 *   See roadmap_download.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_hash.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_config.h"
#include "roadmap_gui.h"
#include "roadmap_county.h"
#include "roadmap_messagebox.h"
#include "roadmap_dialog.h"
#include "roadmap_preferences.h"

#include "roadmap_download.h"


static RoadMapConfigDescriptor RoadMapConfigSource =
                                  ROADMAP_CONFIG_ITEM("Download", "Source");

static RoadMapConfigDescriptor RoadMapConfigDestination =
                                  ROADMAP_CONFIG_ITEM("Download", "Destination");

static RoadMapHash *RoadMapDownloadCancel = NULL;
static int *RoadMapDownloadCancelList = NULL;
static int  RoadMapDownloadCancelCount = 0;

static int *RoadMapDownloadQueue = NULL;
static int  RoadMapDownloadQueueConsumer = 0;
static int  RoadMapDownloadQueueProducer = 0;
static int  RoadMapDownloadQueueSize = 0;

static int  RoadMapDownloadRefresh = 0;


struct roadmap_download_protocol {

   struct roadmap_download_protocol *next;

   const char *prefix;
   RoadMapDownloadProtocol handler;
};

static struct roadmap_download_protocol *RoadMapDownloadProtocolMap = NULL;


static void roadmap_download_no_handler (void) {}

static RoadMapDownloadEvent RoadMapDownloadWhenDone =
                               roadmap_download_no_handler;



static int roadmap_download_request (int size) {
   // TBD: for the time being, answer everything is fine.
   return 1;
}

static void roadmap_download_progress (int loaded) {
   // TBD
}

static RoadMapDownloadCallbacks RoadMapDownloadCallbackFunctions = {
   roadmap_download_request, roadmap_download_progress
};


static int roadmap_download_increment (int cursor) {

   if (++cursor >= RoadMapDownloadQueueSize) {
      cursor = 0;
   }
   return cursor;
}


static void roadmap_download_end (void) {

   static void roadmap_download_next_county (void);


   RoadMapDownloadQueueConsumer =
      roadmap_download_increment (RoadMapDownloadQueueConsumer);

   if (RoadMapDownloadQueueConsumer != RoadMapDownloadQueueProducer) {

      /* The queue is not yet empty: start the next download. */
      roadmap_download_next_county ();

   } else if (RoadMapDownloadRefresh) {

      /* The queue is empty: tell the final consumer, but only
       * if there was at least one successful download.
       */
      RoadMapDownloadRefresh = 0;
      RoadMapDownloadWhenDone ();
   }
}


static void roadmap_download_ok (const char *name, void *context) {

   int  fips = RoadMapDownloadQueue[RoadMapDownloadQueueConsumer];
   struct roadmap_download_protocol *protocol;

   char source[256];
   char destination[256];

   const char *format;
   const char *directory;


   format = roadmap_dialog_get_data (".file", "From");
   snprintf (source, sizeof(source), format, fips);

   format = roadmap_dialog_get_data (".file", "To");
   snprintf (destination, sizeof(destination), format, fips);

   roadmap_dialog_hide (name);

   directory = roadmap_path_parent (NULL, destination);
   roadmap_path_create (directory);
   roadmap_path_free (directory);


   for (protocol = RoadMapDownloadProtocolMap;
        protocol != NULL;
        protocol = protocol->next) {

      if (strncmp (source, protocol->prefix, strlen(protocol->prefix)) == 0) {
         if (protocol->handler (&RoadMapDownloadCallbackFunctions,
                                source, destination)) {
            RoadMapDownloadRefresh = 1;
            break;
         }
      }
   }

   if (protocol == NULL) {

      roadmap_messagebox ("Error", "invalid download protocol");
      roadmap_log (ROADMAP_WARNING, "invalid download source %s", source);
   }

   roadmap_download_end ();
}


static void roadmap_download_cancel (const char *name, void *context) {

   int fips = RoadMapDownloadQueue[RoadMapDownloadQueueConsumer];

   roadmap_dialog_hide (name);

   roadmap_hash_add (RoadMapDownloadCancel,
                     fips,
                     RoadMapDownloadCancelCount);

   RoadMapDownloadCancelList[RoadMapDownloadCancelCount] = fips;
   RoadMapDownloadCancelCount += 1;

   roadmap_download_end ();
}


static void roadmap_download_next_county (void) {

   int fips = RoadMapDownloadQueue[RoadMapDownloadQueueConsumer];


   if (roadmap_dialog_activate ("Download a Map", NULL)) {

      roadmap_dialog_new_label  (".file", "County");
      roadmap_dialog_new_label  (".file", "State");
      roadmap_dialog_new_entry  (".file", "From");
      roadmap_dialog_new_entry  (".file", "To");

      roadmap_dialog_add_button ("OK", roadmap_download_ok);
      roadmap_dialog_add_button ("Cancel", roadmap_download_cancel);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard());
   }
   roadmap_dialog_set_data (".file", "County", roadmap_county_get_name (fips));
   roadmap_dialog_set_data (".file", "State", roadmap_county_get_state (fips));

   roadmap_dialog_set_data (".file", "From",
                            roadmap_config_get (&RoadMapConfigSource));
   roadmap_dialog_set_data (".file", "To",
                            roadmap_config_get (&RoadMapConfigDestination));
}


int roadmap_download_get_county (int fips) {

   int i;
   int next;

   if (RoadMapDownloadWhenDone == roadmap_download_no_handler) return 0;

   if (RoadMapDownloadQueue == NULL) {

      RoadMapDownloadQueueSize = roadmap_county_count();

      RoadMapDownloadQueue =
         calloc (RoadMapDownloadQueueSize, sizeof(int));
      roadmap_check_allocated(RoadMapDownloadQueue);

      RoadMapDownloadCancelList =
         calloc (RoadMapDownloadQueueSize, sizeof(int));
      roadmap_check_allocated(RoadMapDownloadCancelList);

      RoadMapDownloadCancel =
         roadmap_hash_new ("download", RoadMapDownloadQueueSize);
   }


   /* Check that we did not refuse to download that county once. */

   for (i = roadmap_hash_get_first (RoadMapDownloadCancel, fips);
        i >= 0;
        i = roadmap_hash_get_next (RoadMapDownloadCancel, i)) {

      if (RoadMapDownloadCancelList[i] == fips) {
         return 0;
      }
   }

   /* Add this county to the download request queue. */

   next = roadmap_download_increment(RoadMapDownloadQueueProducer);

   if (next == RoadMapDownloadQueueConsumer) {
      /* The queue is full: stop downloading more. */
      return 0;
   }

   RoadMapDownloadQueue[RoadMapDownloadQueueProducer] = fips;

   if (RoadMapDownloadQueueProducer == RoadMapDownloadQueueConsumer) {
      /* The queue was empty: start downloading now. */
      RoadMapDownloadQueueProducer = next;
      roadmap_download_next_county();
      return 0;
   }

   RoadMapDownloadQueueProducer = next;
   return 0;
}


void roadmap_download_subscribe_protocol  (const char *prefix,
                                           RoadMapDownloadProtocol handler) {

   struct roadmap_download_protocol *protocol;

   protocol = malloc (sizeof(*protocol));
   roadmap_check_allocated(protocol);

   protocol->prefix = strdup(prefix);
   protocol->handler = handler;

   protocol->next = RoadMapDownloadProtocolMap;
   RoadMapDownloadProtocolMap = protocol;
}


void roadmap_download_subscribe_when_done (RoadMapDownloadEvent handler) {

   if (handler == NULL) {
      RoadMapDownloadWhenDone = roadmap_download_no_handler;
   } else {
      RoadMapDownloadWhenDone = handler;
   }
}


int  roadmap_download_enabled (void) {
   return (RoadMapDownloadWhenDone != roadmap_download_no_handler);
}


void roadmap_download_initialize (void) {

   char default_destination[256];

   roadmap_config_declare
      ("preferences",
      &RoadMapConfigSource, "/usr/local/share/roadmap/usc%05d.rdm");

   snprintf (default_destination, sizeof(default_destination),
               "%s/maps/usc%%05d.rdm", roadmap_path_user());

   roadmap_config_declare
      ("preferences",
      &RoadMapConfigDestination, strdup(default_destination));
}

