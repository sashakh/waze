/* track_rec.c - record GPS tracks
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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
 * NOTE:
 * This file implements all the "dynamic" editor functionality.
 * The code here is experimental and needs to be reorganized.
 * 
 * SYNOPSYS:
 *
 *   See track_rec.h
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <java/lang.h>
#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_gps.h"
#include "roadmap_navigate.h"
#include "roadmap_messagebox.h"
#include "roadmap_start.h"
#include "roadmap_state.h"

#include "../../src/editor/track/editor_track_filter.h"
#include "../../src/editor/export/editor_upload.h"

#include "../../src/editor/editor_main.h"

#define GPS_POINTS_DISTANCE "10m"
#define MAX_POINTS_IN_SEGMENT 50
#define GPS_TIME_GAP 4 /* 4 seconds */

typedef struct {

   RoadMapGpsPosition gps_point;
   time_t time;

} TrackPoint;

#define TRACK_FILE_NAME "file:///c:/FreeMap/track_data.bin"

static TrackPoint TrackPoints[MAX_POINTS_IN_SEGMENT];
static int points_count = 0;
static RoadMapGpsPosition TrackLastPosition;

static int RecordGpsData = 0;
static FILE *track_file;

static int open_data_file (void) {
   if (track_file) return 0;

   track_file =
      roadmap_file_fopen (NULL, TRACK_FILE_NAME, "a");

   if (track_file == NULL) {
      roadmap_messagebox("Error", "Can't open track_data");
      return -1;
   }

   if (ftell(track_file) == 0) {

      unsigned char version[4] = {0, 10, 0, 1};
      if (fwrite(version, sizeof(version), 1, track_file) != 1) {
         roadmap_messagebox("Error", "Can't write to track_data");
         fclose(track_file);
         track_file = NULL;
         return -1;
      }
   }

   return 0;
}

static int close_data_file (void) {
   if (track_file) {
      fclose(track_file);
      track_file = NULL;
   }

   return 0;
}

static int save_points(int end_track) {

   int error = 0;
   int i;

   roadmap_log(ROADMAP_INFO, "Saving points, end_track:%d", end_track);
   if (track_file == NULL) return -1;

   for (i=0; i<points_count; i++) {
      RoadMapGpsPosition *pos = &TrackPoints[i].gps_point;
      if (fwrite(&pos->longitude, sizeof(pos->longitude), 1, track_file) != 1) {
         error = 1;
      }
      if (fwrite(&pos->latitude, sizeof(pos->latitude), 1, track_file) != 1) {
         error = 1;
      }
      if (fwrite(&TrackPoints[i].time, sizeof(TrackPoints[i].time), 1,
            track_file) != 1) {
         error = 1;
      }

      fflush(track_file);

      if (error) {
         roadmap_log(ROADMAP_ERROR, "Error writing GPS data.");
         roadmap_messagebox("Error", "Error writing GPS data.");
         close_data_file();
         return -1;
      }
   }

   if (end_track) {
      int null[3] = {0, 0, 0};
      if (fwrite(null, sizeof(null), 1, track_file) != 1) {
         roadmap_messagebox("Error", "Error writing GPS data.");
         close_data_file();
         return -1;
      }
   }

   points_count = 0;

   return 0;
}


static int add_point (RoadMapGpsPosition *point, time_t time) {

   if (points_count == MAX_POINTS_IN_SEGMENT) {
      if (save_points(0) == -1) return -1;
   }

   TrackPoints[points_count].gps_point = *point;
   TrackPoints[points_count].time = time;

   return points_count++;
}


static void track_end (void) {

   save_points(1);
}

static void track_rec_point (time_t gps_time,
                             const RoadMapGpsPrecision *dilution,
                             const RoadMapGpsPosition* gps_position) {

   static struct GPSFilter *filter;
   static time_t last_gps_time;
   const RoadMapGpsPosition *filtered_gps_point;
   RoadMapPosition context_save_pos;
   int context_save_zoom;
   int res;
   
   if (filter == NULL) {

      filter = editor_track_filter_new 
         (roadmap_math_distance_convert ("1000m", NULL),
          600, /* 10 minutes */
          60, /* 1 minute */
          roadmap_math_distance_convert ("10m", NULL));
   }

   if (points_count == 0) last_gps_time = 0;

   roadmap_math_get_context (&context_save_pos, &context_save_zoom);
   roadmap_math_set_context ((RoadMapPosition *)gps_position, 20);

   res = editor_track_filter_add (filter, gps_time, dilution, gps_position);

   if (res == ED_TRACK_END) {

      /* The current point is too far from the previous point, or
       * the time from the previous point is too long.
       * This is probably a new GPS track.
       */

      track_end ();
      goto restore;
   }

   if (last_gps_time && (last_gps_time + GPS_TIME_GAP < gps_time)) {
      track_end ();
   }

   last_gps_time = gps_time;

   while ((filtered_gps_point = editor_track_filter_get (filter)) != NULL) {

      TrackLastPosition = *filtered_gps_point;
      
      add_point (&TrackLastPosition, gps_time);
   }

restore:
   roadmap_math_set_context (&context_save_pos, context_save_zoom);
}


static void
track_rec_gps_update (time_t gps_time,
                   const RoadMapGpsPrecision *dilution,
                   const RoadMapGpsPosition *gps_position) {

   if (RecordGpsData) {

      track_rec_point (gps_time, dilution, gps_position);
   }
}

void exception_handler(NOPH_Exception_t exception, void *arg)
{
  char msg[512];

  NOPH_String_toCharPtr(NOPH_Throwable_toString(exception), msg, sizeof(msg));
  roadmap_messagebox("Error", msg);
  NOPH_Throwable_printStackTrace(exception);
  NOPH_delete(exception);
}

static void track_rec_upload (void) {
   if (RecordGpsData) {
      track_end ();
      RecordGpsData = 0;
      close_data_file();
   }

   NOPH_try(exception_handler, NULL) {
      editor_upload_file (TRACK_FILE_NAME, 1);
   } NOPH_catch();
}

static void track_rec_toggle (void) {
   if (RecordGpsData) {
      track_end ();
      RecordGpsData = 0;
      close_data_file();
   } else {
      if (open_data_file() == -1) return;
      track_end ();
      RecordGpsData = 1;
   }
   roadmap_screen_redraw ();
}

static int track_rec_state (void) {

   return RecordGpsData;
}

void editor_main_initialize (void) {

   editor_upload_initialize ();

   roadmap_start_add_action ("togglegpsrecord", "Toggle GPS recording", NULL,
      NULL, "Toggle GPS recording", track_rec_toggle);

   roadmap_start_add_action ("uploadj2merecord", "Upload GPS recording", NULL,
      NULL, "Upload GPS recording", track_rec_upload);

   roadmap_state_add ("record_gps", &track_rec_state);

   roadmap_gps_register_listener(track_rec_gps_update);
}


void editor_main_shutdown (void) {
   if (RecordGpsData) {
      track_end ();
      RecordGpsData = 0;
   }
   close_data_file();
}

