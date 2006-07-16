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
#include <string.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_locator.h"
#include "roadmap_metadata.h"
#include "roadmap_messagebox.h"

#include "editor_upload.h"
#include "editor_export.h"
#include "editor_download.h"
#include "editor_sync.h"


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

   snprintf (name, sizeof(name), "rm_%02d_%02d_%02d%02d.gpx.gz",
                     tm->tm_mday, tm->tm_mon, tm->tm_hour, tm->tm_min);
 
   full_name = roadmap_path_join (path, name);

   res = editor_export_data (full_name, 0);
   free (full_name);

   return res;
}


static int sync_do_upload (void) {

   char **files;
   char **cursor;
   char directory[255];
   snprintf (directory, sizeof(directory), "%s/queue", roadmap_path_user());

   files = roadmap_path_list (directory, ".gpx.gz");

   for (cursor = files; *cursor != NULL; ++cursor) {
      
      char *full_name = roadmap_path_join (directory, *cursor);
      int res = editor_upload_auto (full_name);

      if (res == 0) {
         roadmap_file_remove (NULL, full_name);
      }

      free (full_name);
   }

   roadmap_path_list_free (files);

   return 0;
}


int export_sync (void) {

   int res;
   int fips;
   struct tm now_tm;
   struct tm map_time_tm;
   time_t now_t;
   time_t map_time_t;

   res = sync_do_export ();
   res = sync_do_upload ();

   fips = roadmap_locator_active ();

   if (fips < 0) {
      fips = 77001;
   }

   if (roadmap_locator_activate (fips) == -1) {
      roadmap_messagebox ("Error.", "Can't load map data.");
      return -1;
   }

   now_t = time (NULL);
   map_time_t = atoi(roadmap_metadata_get_attribute ("Version", "UnixTime"));

   if ((map_time_t + 3600*24) > now_t) {
      /* Map is less than 24 hours old.
       * A new version may still be available.
       */

      now_tm = *gmtime(&now_t);
      map_time_tm = *gmtime(&map_time_t);

      if (now_tm.tm_mday == map_time_tm.tm_mday) {

         return 0;
      } else {
         /* new day - only download if new maps were already generated. */
         if (now_tm.tm_hour < 2) return 0;
      }
   }

   res = editor_download_update_map (EDITOR_DOWNLOAD_AUTO);

   return 0;
}

