/* editor_sync.c - synchronize data and maps
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   See editor_sync.h
 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_locator.h"
#include "roadmap_metadata.h"
#include "roadmap_dialog.h"
#include "roadmap_main.h"
#include "roadmap_messagebox.h"

#include "editor_upload.h"
#include "editor_export.h"
#include "editor_download.h"
#include "../editor_main.h"
#include "editor_sync.h"

#define MAX_MSGS 10
#define MIN_FREE_SPACE 5000 //Free space in KB

static int SyncProgressItems;
static int SyncProgressCurrentItem;
static int SyncProgressTarget;
static int SyncProgressLoaded;

static int roadmap_download_request (int size) {

   SyncProgressTarget = size;
   SyncProgressLoaded = 0;
   SyncProgressCurrentItem++;
   return 1;
}


static void roadmap_download_error (const char *format, ...) {

   va_list ap;
   char message[2048];

   va_start(ap, format);
   vsnprintf (message, sizeof(message), format, ap);
   va_end(ap);

   roadmap_log (ROADMAP_ERROR, "Sync error: %s", message);
   roadmap_messagebox ("Download Error", message);
}


static void roadmap_download_progress (int loaded) {

   if (roadmap_dialog_activate ("Sync process", NULL, 1)) {

      const char *icon = roadmap_path_join (roadmap_path_user(), "skins/default/sync.bmp");
      roadmap_dialog_new_image  ("Sync", icon);
      roadmap_dialog_new_label  ("Sync", "Progress status");
      roadmap_dialog_new_progress  ("Sync", "Progress");
      roadmap_path_free (icon);

      roadmap_dialog_complete (0);
   }

   if ((SyncProgressLoaded > loaded) || !loaded || !SyncProgressItems) {
      roadmap_dialog_set_progress ("Sync", "Progress", 0);
      SyncProgressLoaded = loaded;
   } else {

      if (SyncProgressLoaded == loaded) {
         return;

      } else if ((loaded < SyncProgressTarget) &&
            (100 * (loaded - SyncProgressLoaded) / SyncProgressTarget) < 5) {
         return;
      }

      SyncProgressLoaded = loaded;

      roadmap_dialog_set_progress ("Sync", "Progress",
         100 / SyncProgressItems * (SyncProgressCurrentItem - 1) +
         (100 / SyncProgressItems) * SyncProgressLoaded / SyncProgressTarget);
   }

   roadmap_main_flush ();
}


static RoadMapDownloadCallbacks SyncDownloadCallbackFunctions = {
   roadmap_download_request,
   roadmap_download_progress,
   roadmap_download_error
};


static int sync_do_export (void) {

   char path[255];
   char name[255];
   struct tm *tm;
   char *full_name;
   time_t now;
   int res;

   snprintf (path, sizeof(path), "%s/queue", roadmap_path_user());
   roadmap_path_create (path);

   time(&now);
   tm = gmtime (&now);

   snprintf (name, sizeof(name), "rm_%02d_%02d_%02d_%02d%02d.gpx.gz",
                     tm->tm_mday, tm->tm_mon+1, tm->tm_year-100,
                     tm->tm_hour, tm->tm_min);
 
   full_name = roadmap_path_join (path, name);

   SyncProgressItems = 1;
   SyncProgressCurrentItem = 0;
   res = editor_export_data (full_name, &SyncDownloadCallbackFunctions);
   free (full_name);

   return res;
}


static int sync_do_upload (char *messages[MAX_MSGS], int *num_msgs) {

   char **files;
   char **cursor;
   char directory[255];
   int count;
   snprintf (directory, sizeof(directory), "%s/queue", roadmap_path_user());

   files = roadmap_path_list (directory, ".gpx.gz");

   count = 0;
   for (cursor = files; *cursor != NULL; ++cursor) {
      count++;
   }

   SyncProgressItems = count;
   SyncProgressCurrentItem = 0;
   *num_msgs = 0;

   for (cursor = files; *cursor != NULL; ++cursor) {
      
      char *full_name = roadmap_path_join (directory, *cursor);
      int res = editor_upload_auto (full_name, &SyncDownloadCallbackFunctions,
                                    messages + *num_msgs);

      if (res == 0) {
         roadmap_file_remove (NULL, full_name);
         if (messages[*num_msgs] != NULL) (*num_msgs)++;
      }

      free (full_name);

      if (*num_msgs == MAX_MSGS) break;
   }

   roadmap_path_list_free (files);

   return 0;
}


int export_sync (void) {

   int i;
   int res;
   int fips;
   struct tm now_tm;
   struct tm map_time_tm;
   time_t now_t;
   time_t map_time_t;
   char *messages[MAX_MSGS];
   int num_msgs;

   if (!editor_is_enabled ()) {
      return 0;
   }

   res = roadmap_file_free_space (roadmap_path_user());

   if ((res >= 0) && (res < MIN_FREE_SPACE)) {
      roadmap_messagebox ("Error",
                  "Please free at least 5MB of space before synchronizing.");
      return -1;
   }

   roadmap_download_progress (0);
   roadmap_dialog_set_data ("Sync", "Progress status",
                            roadmap_lang_get ("Preparing export data..."));
   roadmap_main_flush ();
   res = sync_do_export ();
   roadmap_download_progress (0);
   roadmap_dialog_set_data ("Sync", "Progress status",
                            roadmap_lang_get ("Uploading data..."));
   roadmap_main_flush ();
   res = sync_do_upload (messages, &num_msgs);

   fips = roadmap_locator_active ();

   if (fips < 0) {
      fips = 77001;
   }

   if (roadmap_locator_activate (fips) == ROADMAP_US_OK) {
      now_t = time (NULL);
      map_time_t = atoi(roadmap_metadata_get_attribute ("Version", "UnixTime"));

      if ((map_time_t + 3600*24) > now_t) {
         /* Map is less than 24 hours old.
          * A new version may still be available.
          */

         now_tm = *gmtime(&now_t);
         map_time_tm = *gmtime(&map_time_t);

         if (now_tm.tm_mday == map_time_tm.tm_mday) {

            goto end_sync;
         } else {
            /* new day - only download if new maps were already generated. */
            if (now_tm.tm_hour < 2) goto end_sync;
         }
      }
   }

   SyncProgressItems = 2;
   SyncProgressCurrentItem = 0;
   roadmap_download_progress (0);
   roadmap_dialog_set_data ("Sync", "Progress status",
                            roadmap_lang_get ("Downloading new maps..."));
   roadmap_main_flush ();
   res = editor_download_update_map (&SyncDownloadCallbackFunctions);

   if (res == -1) {
      roadmap_messagebox ("Download Error", roadmap_lang_get("Error downloading map update"));
   }
end_sync:

   for (i=0; i<num_msgs; i++) {
      roadmap_messagebox ("Info", messages[i]);
      free (messages[i]);
   }
   roadmap_dialog_hide ("Sync process");
   return 0;
}

