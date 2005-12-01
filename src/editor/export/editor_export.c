/* editor_export.c - export db to GPX
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *   See editor_export.h
 */

#include <assert.h>
#include "stdlib.h"
#include "time.h"
#include "string.h"

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_layer.h"
#include "roadmap_point.h"
#include "roadmap_fileselection.h"

#include "../editor_log.h"

#include "../db/editor_db.h"
#include "../db/editor_point.h"
#include "../db/editor_shape.h"
#include "../db/editor_line.h"
#include "../db/editor_trkseg.h"
#include "../db/editor_street.h"
#include "../db/editor_route.h"

#include "../track/editor_track_main.h"
#include "editor_export.h"

#define NO_ELEVATION 1000000

static void add_timestamp(FILE *file, time_t time) {
   
   struct tm *tms = gmtime (&time);
   
   fprintf (file, "<time>%04d-%02d-%02dT%02d:%02d:%02dZ</time>\n",
         tms->tm_year+1900, tms->tm_mon+1, tms->tm_mday,
         tms->tm_hour, tms->tm_min, tms->tm_sec);
}


static void add_trkpt(FILE *file, int lon, int lat, int ele, time_t time) {
   
   fprintf (file, "<trkpt lat=\"%d.%06d\" lon=\"%d.%06d\">\n",
            lat / 1000000, lat % 1000000, lon / 1000000, lon % 1000000);

   if (ele != NO_ELEVATION) {
      fprintf (file, "<ele>%d</ele>\n", ele);
   }
   
   add_timestamp (file, time);

   fprintf (file, "</trkpt>\n");
}


static void open_trk(FILE *file) {
   
   fprintf (file, "<trk>\n");
}


static void close_trk(FILE *file) {
   
   fprintf (file, "</trk>\n");
}


static void open_trkseg(FILE *file) {
   
   fprintf (file, "<trkseg>\n");
}


static void close_trkseg(FILE *file) {
   
   fprintf (file, "</trkseg>\n");
}


static FILE *create_export_file(const char *name) {

   FILE *file = roadmap_file_fopen (NULL, name, "w");
   if (file == NULL) return NULL;
   
   fprintf (file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
   fprintf (file, "<gpx\nversion=\"1.1\"\ncreator=\"RoadMap Editor - http://roadmap.digitalomaha.net/editor.html\"\n");
   fprintf (file, "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
   fprintf (file, "xmlns=\"http://www.topografix.com/GPX/1/1\"\n");
   fprintf (file, "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd\">\n");

   add_timestamp (file, time(NULL));
   return file;
}


static void close_export_file(FILE *file) {

   fprintf (file, "</gpx>\n");
   fclose (file);
}


static void add_trkpts (FILE *file,
                        int trkseg,
                        int line_id,
                        int flags,
                        RoadMapPosition *from,
                        RoadMapPosition *to) {

   RoadMapPosition trkseg_pos;
   int from_point;
   int first_shape;
   int last_shape;
   int trkseg_flags;
   time_t start_time;
   time_t end_time;

   editor_trkseg_get
      (trkseg, &from_point, &first_shape, &last_shape, &trkseg_flags);
   editor_trkseg_get_time (trkseg, &start_time, &end_time);

   editor_point_position (from_point, &trkseg_pos);

   if (first_shape != -1) {

      int i;

      for (i=first_shape; i<=last_shape; i++) {
         editor_shape_position (i, &trkseg_pos);
         editor_shape_time (i, &start_time);

         add_trkpt
            (file, trkseg_pos.longitude, trkseg_pos.latitude,
             NO_ELEVATION, start_time);
      }
   }
}


static const char *cfcc2type (int cfcc) {

   char **categories;
   int count;

   roadmap_layer_get_categories_names (&categories, &count);

   return categories[cfcc - ROADMAP_ROAD_FIRST];
}


static void add_attribute (FILE *file, const char *name, const char *value) {

   fprintf (file, "<%s>%s</%s>\n", name, value, name);
}


static void add_line_data (FILE *file,
                           int line_id,
                           int cfcc,
                           int flags,
                           int trkseg_flags,
                           RoadMapPosition *line_from,
                           RoadMapPosition *line_to,
                           time_t start_time,
                           time_t end_time) {

   EditorStreetProperties properties;
   char *trk_type;
   int from_point_id;
   int to_point_id;
   int roadmap_from_id;
   int roadmap_to_id;
   int route_id;
   short speed_limit;

   fprintf (file, "<extensions>\n");
   
   if (flags & ED_LINE_DELETED) {
      
      fprintf (file, "<action trk_type=\"fake\">Delete</action>\n");
   } else {

      if (trkseg_flags & ED_TRKSEG_FAKE) {
         trk_type = "fake";
      } else if (trkseg_flags & ED_TRKSEG_IGNORE) {
         trk_type = "ignore";
      } else {
         trk_type = "valid";
      }

      fprintf (file, "<action trk_type=\"%s\">Update</action>\n", trk_type);
   }

   editor_line_get_points (line_id, &from_point_id, &to_point_id);

   editor_point_roadmap_id (from_point_id, &roadmap_from_id);
   editor_point_roadmap_id (to_point_id, &roadmap_to_id);

   if (roadmap_from_id >= 0)
      roadmap_from_id = roadmap_point_db_id (roadmap_from_id);

   if (roadmap_to_id >= 0)
      roadmap_to_id = roadmap_point_db_id (roadmap_to_id);

   if (roadmap_from_id == -1) roadmap_from_id = -from_point_id-2;
   if (roadmap_to_id == -1) roadmap_to_id = -to_point_id-2;

   fprintf (file, "<line from_id=\"%d\" to_id=\"%d\">\n",
         roadmap_from_id, roadmap_to_id);

   add_trkpt
      (file, line_from->longitude, line_from->latitude,
       NO_ELEVATION, start_time);

   add_trkpt
      (file, line_to->longitude, line_to->latitude,
       NO_ELEVATION, end_time);

   fprintf (file, "</line>\n");

   if ((line_id != -1) && (flags & ED_LINE_DIRTY)) {

      fprintf (file, "<attributes>\n");

      editor_street_get_properties (line_id, &properties);

      add_attribute (file, "road_type", cfcc2type (cfcc));
   
      add_attribute (file, "street_name",
         editor_street_get_street_fename (&properties));
   
      add_attribute (file, "text2speech",
         editor_street_get_street_t2s (&properties));

      add_attribute (file, "city_name",
         editor_street_get_street_city
            (&properties, ED_STREET_LEFT_SIDE));

      route_id = editor_line_get_route (line_id);

      if (route_id != -1) {

         editor_route_segment_get
            (route_id, NULL, NULL, &speed_limit);

         if (speed_limit) {
            char str[100];
            snprintf (str, sizeof(str), "%d", speed_limit);
            add_attribute (file, "city_name", str);
         }
      }

      fprintf (file, "</attributes>\n");

      editor_line_modify_properties
         (line_id, cfcc, flags & ~ED_LINE_DIRTY);
   }
 
   fprintf (file, "</extensions>\n");
}


static int export_dirty_lines(FILE *file) {

   int num_lines = editor_line_get_count();
   int current_trk_open = 0;
   int found = 0;
   int i;

   for (i=0; i<num_lines; i++) {
      
      RoadMapPosition from;
      RoadMapPosition to;
      int cfcc;
      int flags;
      int trkseg_flags = 0;
      time_t start_time;
      time_t end_time;
      int trkseg;
      int last_trkseg;

      editor_line_get (i, &from, &to, NULL, &cfcc, &flags);

      if (!(flags & ED_LINE_DIRTY)) continue;

      if (!(flags & ED_LINE_DELETED)) {

         if (!current_trk_open) {

            open_trk (file);
            current_trk_open = 1;
         }

         open_trkseg (file);

         editor_line_get_trksegs (i, &trkseg, &last_trkseg);

         add_trkpts (file, trkseg, i, flags, &from, &to);

         editor_trkseg_get_time (trkseg, &start_time, &end_time);
         editor_trkseg_get (trkseg, NULL, NULL, NULL, &trkseg_flags);

         if (trkseg_flags & ED_TRKSEG_OPPOSITE_DIR) {
         
            add_line_data (file, i, cfcc, flags, trkseg_flags,
                           &to, &from, start_time, end_time);
         } else {
         
            add_line_data (file, i, cfcc, flags, trkseg_flags,
                           &from, &to, start_time, end_time);
         }
      
         close_trkseg (file);

         found++;
      }

      editor_line_modify_properties
         (i, cfcc, flags & ~ED_LINE_DIRTY);
   }

   if (current_trk_open) {
      close_trk (file);
   }

   return found;
}
 

static int editor_export_data(const char *name) {
   
   FILE *file = create_export_file (name);
   int trkseg;
   int line_id;
   int current_trk_open = 0;

   if (file == NULL) {
      editor_log (ROADMAP_ERROR, "Can't create file: %s", name);
      return -1;
   }

   editor_track_end ();

   trkseg = editor_trkseg_get_next_export ();

   if (trkseg == -1) {

      if (!export_dirty_lines (file)) {
         editor_log (ROADMAP_INFO, "No trksegs are available for export.");
      }
      close_export_file (file);
      return 0;
   }

   while (trkseg != -1) {
      
      RoadMapPosition from;
      RoadMapPosition to;
      int cfcc;
      int flags;
      int trkseg_flags = 0;
      time_t start_time;
      time_t end_time;

      line_id = editor_trkseg_get_line (trkseg);
      
      if (line_id != -1) {

         editor_line_get (line_id, &from, &to, NULL, &cfcc, &flags);

         if (flags & ED_LINE_DELETED) {

            editor_line_modify_properties
               (line_id, cfcc, flags & ~ED_LINE_DIRTY);

            if (current_trk_open) {
               close_trk (file);
               current_trk_open = 0;
            }

            goto next_trkseg;
         }
      }

      editor_trkseg_get
         (trkseg, NULL, NULL, NULL, &trkseg_flags);

      if (trkseg_flags & ED_TRKSEG_NEW_TRACK) {

         if (current_trk_open) {
            close_trk (file);
            current_trk_open = 0;
         }
      }

      if (!current_trk_open) {

         open_trk (file);
         current_trk_open = 1;
      }

      open_trkseg (file);

      add_trkpts (file, trkseg, line_id, flags, &from, &to);

      editor_trkseg_get_time (trkseg, &start_time, &end_time);

      if (trkseg_flags & ED_TRKSEG_OPPOSITE_DIR) {
         
         add_line_data (file, line_id, cfcc, flags, trkseg_flags,
                        &to, &from, start_time, end_time);
      } else {
         
         add_line_data (file, line_id, cfcc, flags, trkseg_flags,
                        &from, &to, start_time, end_time);
      }
      
      close_trkseg (file);

next_trkseg:

      if (trkseg_flags & ED_TRKSEG_END_TRACK) {

         if (current_trk_open) {
            close_trk (file);
            current_trk_open = 0;
         }
      }

      trkseg = editor_trkseg_next_in_global (trkseg);
   }

   if (current_trk_open) {
      close_trk (file);
   }

   export_dirty_lines (file);
   close_export_file (file);

   editor_trkseg_reset_next_export ();

   return 0;
}


void editor_export_reset_dirty () {
   
   int count;
   int i;

   count = editor_line_get_count ();

   for (i=0; i<count; i++) {

      int flags;
      int cfcc;

      editor_line_get (i, NULL, NULL, NULL, &cfcc, &flags);
      editor_line_modify_properties (i, cfcc, flags | ED_LINE_DIRTY);
   }
}


static void editor_export_file_dialog_ok
                           (const char *filename, const char *mode) {

   editor_export_data (filename);
}


void editor_export_gpx (void) {
                                
   roadmap_fileselection_new ("Export data",
                              NULL, /* no filter. */
                              roadmap_path_user (),
                              "w",
                              editor_export_file_dialog_ok);
}

