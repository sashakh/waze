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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <zlib.h>
#include "time.h"

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_layer.h"
#include "roadmap_point.h"
#include "roadmap_line.h"
#include "roadmap_locator.h"
#include "roadmap_line_route.h"
#include "roadmap_fileselection.h"
#include "roadmap_messagebox.h"
#include "roadmap_download.h"
#include "roadmap_config.h"

#include "../editor_log.h"
#include "../editor_main.h"

#include "../db/editor_db.h"
#include "../db/editor_point.h"
#include "../db/editor_shape.h"
#include "../db/editor_line.h"
#include "../db/editor_trkseg.h"
#include "../db/editor_street.h"
#include "../db/editor_override.h"
#include "../db/editor_route.h"
#include "../db/editor_marker.h"

#include "../track/editor_track_main.h"
#include "editor_upload.h"
#include "editor_sync.h"
#include "editor_export.h"

#define NO_ELEVATION 1000000

enum StreamType {NULL_STREAM, FILE_STREAM, ZLIB_STREAM};

typedef struct {
   enum StreamType type;
   void *stream;
} ExportStream;

static RoadMapConfigDescriptor RoadMapConfigAutoUpload =
                  ROADMAP_CONFIG_ITEM("FreeMap", "Upload file after export");

static void editor_export_upload(const char *filename) {

   if (roadmap_config_match(&RoadMapConfigAutoUpload, "yes")) {
      editor_upload_file (filename);
   }
}


static void export_write(ExportStream *stream, char *format, ...) {

   char buf[1024];
   va_list ap;

   va_start(ap, format);
   vsnprintf (buf, sizeof(buf), format, ap);
   va_end(ap);

   switch (stream->type) {
   case FILE_STREAM:
      fwrite (buf, sizeof(char), strlen(buf), (FILE *)stream->stream);
      break;
   case ZLIB_STREAM:
      gzwrite ((gzFile)stream->stream, buf, strlen(buf));
      break;
   default:
      break;
   }

   return;
}


static void add_timestamp(ExportStream *stream, time_t time) {
   
   struct tm *tms = gmtime (&time);
   
   export_write (stream, "<time>%04d-%02d-%02dT%02d:%02d:%02dZ</time>\n",
         tms->tm_year+1900, tms->tm_mon+1, tms->tm_mday,
         tms->tm_hour, tms->tm_min, tms->tm_sec);
}


static void add_trkpt(ExportStream *stream, int lon, int lat, int ele,
                      time_t time) {
   
   export_write (stream, "<trkpt lat=\"%d.%06d\" lon=\"%d.%06d\">\n",
            lat / 1000000, lat % 1000000, lon / 1000000, lon % 1000000);

   if (ele != NO_ELEVATION) {
      export_write (stream, "<ele>%d</ele>\n", ele);
   }
   
   if (time != (time_t) -1) {
      add_timestamp (stream, time);
   }

   export_write (stream, "</trkpt>\n");
}


static void open_trk(ExportStream *stream) {
   
   export_write (stream, "<trk>\n");
}


static void close_trk(ExportStream *stream) {
   
   export_write (stream, "</trk>\n");
}


static void open_trkseg(ExportStream *stream) {
   
   export_write (stream, "<trkseg>\n");
}


static void close_trkseg(ExportStream *stream) {
   
   export_write (stream, "</trkseg>\n");
}


static int create_export_stream(ExportStream *stream, const char *name) {

   FILE *file;
   gzFile gz_file;
  
   if ((strlen(name) < 3) || strcmp(name + strlen(name) - 3, ".gz")) {
      file = roadmap_file_fopen (NULL, name, "w");

      if (file == NULL) {
         editor_log (ROADMAP_ERROR, "Can't create file: %s", name);
         roadmap_messagebox ("Export Error", "Can't create file.");
         return -1;
      }

      stream->type = FILE_STREAM;
      stream->stream = file;
   } else {

      gz_file = gzopen(name, "wb");
      if (gz_file == NULL) {
         editor_log (ROADMAP_ERROR, "Can't create file: %s", name);
         roadmap_messagebox ("Export Error", "Can't create file.");
         return -1;
      }

      stream->type = ZLIB_STREAM;
      stream->stream = gz_file;
   }

   export_write (stream, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
   export_write (stream, "<gpx\nversion=\"1.1\"\ncreator=\"RoadMap Editor (%s) - http://roadmap.digitalomaha.net/editor.html\"\n", editor_main_get_version ());
   export_write (stream, "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
   export_write (stream, "xmlns:freemap=\"http://www.freemap.co.il/xmlns/freemap\"\n");
   export_write (stream, "xmlns=\"http://www.topografix.com/GPX/1/1\"\n");
   export_write (stream, "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\n");
   export_write (stream, "<metadata>\n");
   add_timestamp (stream, time(NULL));
   export_write (stream, "</metadata>\n");

   return 0;
}


static void close_export_stream(ExportStream *stream) {

   export_write (stream, "</gpx>\n");

   switch (stream->type) {
   case FILE_STREAM:
      fclose ((FILE *)stream->stream);
      break;
   case ZLIB_STREAM:
      gzclose ((gzFile)stream->stream);
      break;
   default:
      break;
   }
}


static void add_trkpts(ExportStream *stream, int trkseg) {

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

         if (start_time != (time_t) -1) {
            editor_shape_time (i, &start_time);
         }

         add_trkpt
            (stream, trkseg_pos.longitude, trkseg_pos.latitude,
             NO_ELEVATION, start_time);
      }
   }
}


static const char *cfcc2type(int cfcc) {

   char **categories;
   int count;

   roadmap_layer_get_categories_names (&categories, &count);

   return categories[cfcc - ROADMAP_ROAD_FIRST];
}


static void add_attribute(ExportStream *stream,
                          const char *ns,
                          const char *name,
                          const char *value) {

   char str[500];
   char *itr = str;

   while (*value) {

      if ((itr - str) > 450) break;

      switch (*value) {
      case '"':
         strcpy (itr, "&quot;");
         itr += 6;
         break;
      case '\'':
         strcpy (itr, "&apos;");
         itr += 6;
         break;
      case '&':
         strcpy (itr, "&amp;");
         itr += 5;
         break;
      case '<':
         strcpy (itr, "&lt;");
         itr += 4;
         break;
      case '>':
         strcpy (itr, "&qt;");
         itr += 4;
         break;

      default:
         *itr = *value;
         itr++;
         break;
      }

      value++;
   }
   
   *itr = 0;

   if (ns) {
      export_write (stream, "<%s:%s>%s</%s:%s>\n", ns, name, str, ns, name);
   } else {
      export_write (stream, "<%s>%s</%s>\n", name, str, name);
   }
}


static void open_waypoint(ExportStream *stream, int lon, int lat, int ele,
                          int steering, time_t time, const char *type,
                          const char *note) {
   
   if (!note) note = "";

   export_write (stream, "<wpt lat=\"%d.%06d\" lon=\"%d.%06d\">\n",
            lat / 1000000, lat % 1000000, lon / 1000000, lon % 1000000);

   if (ele != NO_ELEVATION) {
      export_write (stream, "<ele>%d</ele>\n", ele);
   }
   
   add_timestamp (stream, time);

   while (steering < 0) steering += 360;
   while (steering > 360) steering -= 360;
   export_write (stream, "<magvar>%d</magvar>\n", steering);

   add_attribute (stream, NULL, "desc", note);
   export_write (stream, "<type>%s</type>\n", type);
}


static void close_waypoint(ExportStream *stream) {

   export_write (stream, "</wpt>\n");
}


static int export_markers(ExportStream *stream, const char *name) {

   int count = editor_marker_count();
   int exported = 0;
   int i;

   for (i=0; i<count; i++) {
      int flags = editor_marker_flags (i);
      RoadMapPosition pos;
      time_t marker_time;
      int steering;
      const char *description;
      const char *keys[MAX_ATTR];
      char       *values[MAX_ATTR];
      int attr_count;

      if (!(flags & ED_MARKER_DIRTY) || !(flags & ED_MARKER_UPLOAD)) continue;

      if (editor_marker_export
            (i, &description, keys, values, &attr_count) == -1) {
         continue;
      }

      if (stream->type == NULL_STREAM) {
         
         if (create_export_stream (stream, name) != 0) {
            return 0;
         }
      }

      editor_marker_position (i, &pos, &steering);
      marker_time = editor_marker_time (i);
      
      open_waypoint (stream, pos.longitude, pos.latitude, NO_ELEVATION,
                     steering, marker_time,
                     editor_marker_type (i),
                     description);

      if (attr_count > 0) {
         int j;

         export_write (stream, "<extensions>\n");

         for (j=0; j<attr_count; j++) {
   
            add_attribute (stream, "freemap", keys[j], values[j]);

            free(values[j]);
         }

         export_write (stream, "</extensions>\n");
      }

      close_waypoint (stream);

      editor_marker_update (i, flags & ~ED_MARKER_DIRTY,
                            editor_marker_note (i));
      exported++;
   }

   return exported;
}
      

static void add_line_data(ExportStream *stream,
                          int line_id,
                          int plugin_id,
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
   /* LineRouteMax speed_limit; */

   export_write (stream, "<extensions>\n");
   
   if (flags & ED_LINE_DELETED) {
      
      export_write (stream, "<freemap:action trk_type=\"fake\">Delete</freemap:action>\n");
   } else {

      if (trkseg_flags & ED_TRKSEG_FAKE) {
         trk_type = "fake";
      } else if (trkseg_flags & ED_TRKSEG_IGNORE) {
         trk_type = "ignore";
      } else {
         trk_type = "valid";
      }

      export_write (stream, "<freemap:action trk_type=\"%s\">Update</freemap:action>\n", trk_type);
   }

   if (plugin_id == EditorPluginID) {

      editor_line_get_points (line_id, &from_point_id, &to_point_id);

      editor_point_roadmap_id (from_point_id, &roadmap_from_id);
      editor_point_roadmap_id (to_point_id, &roadmap_to_id);

      if (roadmap_from_id >= 0)
         roadmap_from_id = roadmap_point_db_id (roadmap_from_id);

      if (roadmap_to_id >= 0)
         roadmap_to_id = roadmap_point_db_id (roadmap_to_id);

      if (roadmap_from_id == -1) roadmap_from_id = -from_point_id-2;
      if (roadmap_to_id == -1) roadmap_to_id = -to_point_id-2;
   } else {

      roadmap_line_points (line_id, &roadmap_from_id, &roadmap_to_id);
      roadmap_from_id = roadmap_point_db_id (roadmap_from_id);
      roadmap_to_id = roadmap_point_db_id (roadmap_to_id);
   }

   export_write (stream, "<freemap:line from_id=\"%d\" to_id=\"%d\">\n",
         roadmap_from_id, roadmap_to_id);

   add_trkpt
      (stream, line_from->longitude, line_from->latitude,
       NO_ELEVATION, start_time);

   add_trkpt
      (stream, line_to->longitude, line_to->latitude,
       NO_ELEVATION, end_time);

   export_write (stream, "</freemap:line>\n");

   if ((plugin_id == EditorPluginID) && (flags & ED_LINE_DIRTY)) {

      export_write (stream, "<freemap:attributes>\n");

      editor_street_get_properties (line_id, &properties);

      add_attribute (stream, NULL, "road_type", cfcc2type (cfcc));
   
      add_attribute (stream, NULL, "street_name",
         editor_street_get_street_fename (&properties));
   
      add_attribute (stream, NULL, "text2speech",
         editor_street_get_street_t2s (&properties));

      add_attribute (stream, NULL, "city_name",
         editor_street_get_street_city
            (&properties, ED_STREET_LEFT_SIDE));

      route_id = editor_line_get_route (line_id);

#if 0
      if (route_id != -1) {

         editor_route_segment_get
            (route_id, NULL, NULL, &speed_limit, NULL);

         if (speed_limit) {
            char str[100];
            snprintf (str, sizeof(str), "%d", speed_limit);
            add_attribute (stream, "speed_limit", str);
         }
      }
#endif
      export_write (stream, "</freemap:attributes>\n");

      editor_line_modify_properties
         (line_id, cfcc, flags & ~ED_LINE_DIRTY);
   }
 
   export_write (stream, "</extensions>\n");
}


static int export_dirty_lines(ExportStream *stream, const char *name) {

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

      if (stream->type == NULL_STREAM) {
         
         if (create_export_stream (stream, name) != 0) {
            return 0;
         }
      }

      if (!(flags & ED_LINE_DELETED)) {

         if (!current_trk_open) {

            open_trk (stream);
            current_trk_open = 1;
         }

         open_trkseg (stream);

         editor_line_get_trksegs (i, &trkseg, &last_trkseg);

         add_trkpts (stream, trkseg);

         editor_trkseg_get_time (trkseg, &start_time, &end_time);
         editor_trkseg_get (trkseg, NULL, NULL, NULL, &trkseg_flags);

         if (trkseg_flags & ED_TRKSEG_OPPOSITE_DIR) {
         
            add_line_data (stream, i, EditorPluginID, cfcc, flags, trkseg_flags,
                           &to, &from, start_time, end_time);
         } else {
         
            add_line_data (stream, i, EditorPluginID, cfcc, flags, trkseg_flags,
                           &from, &to, start_time, end_time);
         }
      
         close_trkseg (stream);

         found++;
      }

      editor_line_modify_properties
         (i, cfcc, flags & ~ED_LINE_DIRTY);
   }

   if (current_trk_open) {
      close_trk (stream);
   }

   return found;
}
 

int editor_export_data(const char *name, RoadMapDownloadCallbacks *callbacks) {
   
   ExportStream stream;
   int trkseg;
   int line_id;
   int plugin_id;
   int current_trk_open = 0;
   int fips;
   int exported;
   int estimated_exports;

   stream.type = NULL_STREAM;

   fips = roadmap_locator_active ();

   if (callbacks) (*callbacks->size) (100);

   if (fips < 0) {
      if (callbacks) {
         fips = 77001;
      } else {
         editor_log (ROADMAP_ERROR, "Can't locate current fips");
         roadmap_messagebox ("Export Error", "Can't locate fips.");
         return -1;
      }
   }

   if (editor_db_activate (fips) == -1) {
      if (!callbacks) {
         roadmap_messagebox ("Export Error", "No editor data to export.");
      }
      return 0;
   }

   editor_track_end ();

   trkseg = editor_trkseg_get_next_export ();

   if (trkseg == -1) {

      int exported = 0;

      if (callbacks) (*callbacks->progress) (50);

      exported  = export_markers (&stream, name);
      exported += export_dirty_lines (&stream, name);

      if (!exported) {

         if (callbacks) {
            (*callbacks->progress) (100);
         } else {
            editor_log (ROADMAP_INFO, "No data is available for export.");
            roadmap_messagebox ("Export Error", "No new data to export.");
         }

         if (stream.type != NULL_STREAM) {
            close_export_stream (&stream);
            roadmap_file_remove (NULL, name);
         }
         return 0;
      }

      editor_db_sync (fips);

      if (callbacks) (*callbacks->progress) (100);
      close_export_stream (&stream);
      
      if (!callbacks) {
         editor_export_upload (name);
      }
      return 1;
   }

   if (create_export_stream (&stream, name) != 0) {
      return -1;
   }

   export_markers (&stream, name);

   estimated_exports = editor_trkseg_get_count ();
   exported = 0;
   while (trkseg != -1) {
      
      RoadMapPosition from;
      RoadMapPosition to;
      int cfcc;
      int flags;
      int trkseg_flags = 0;
      time_t start_time;
      time_t end_time;

      editor_trkseg_get_line (trkseg, &line_id, &plugin_id);
      
      if (line_id == -1) {
         roadmap_log (ROADMAP_ERROR, "trkseg has an unknwon line_id.");

         goto close_trk;
      }

      if (plugin_id == EditorPluginID) {

         editor_line_get (line_id, &from, &to, NULL, &cfcc, &flags);

         if (flags & (ED_LINE_DELETED|ED_LINE_CONNECTION)) {

            editor_line_modify_properties
               (line_id, cfcc, flags & ~ED_LINE_DIRTY);

            goto close_trk;
         }
      } else {

         flags = editor_override_line_get_flags (line_id);

         if (flags & ED_LINE_DELETED) {

            goto close_trk;
         }

         roadmap_line_from (line_id, &from);
         roadmap_line_to (line_id, &to);
         cfcc = 4;
      }

      editor_trkseg_get
         (trkseg, NULL, NULL, NULL, &trkseg_flags);

      if (trkseg_flags & ED_TRKSEG_NEW_TRACK) {

         if (current_trk_open) {
            close_trk (&stream);
            current_trk_open = 0;
         }
      }

      if (!current_trk_open) {

         open_trk (&stream);
         current_trk_open = 1;
      }

      open_trkseg (&stream);

      add_trkpts (&stream, trkseg);

      editor_trkseg_get_time (trkseg, &start_time, &end_time);

      if (trkseg_flags & ED_TRKSEG_OPPOSITE_DIR) {
         
         add_line_data (&stream, line_id, plugin_id, cfcc, flags, trkseg_flags,
                        &to, &from, start_time, end_time);
      } else {
         
         add_line_data (&stream, line_id, plugin_id, cfcc, flags, trkseg_flags,
                        &from, &to, start_time, end_time);
      }
      
      close_trkseg (&stream);

      if (!(trkseg_flags & ED_TRKSEG_END_TRACK)) {

         goto next_trkseg;
      }

close_trk:
      if (current_trk_open) {
         close_trk (&stream);
         current_trk_open = 0;
      }

next_trkseg:
      if (exported < estimated_exports) exported++;
      if (callbacks) (*callbacks->progress) (85 * exported / estimated_exports);
      trkseg = editor_trkseg_next_in_global (trkseg);
   }

   if (current_trk_open) {
      close_trk (&stream);
   }

   export_dirty_lines (&stream, name);

   if (callbacks) (*callbacks->progress) (100);
   close_export_stream (&stream);

   editor_trkseg_reset_next_export ();

   editor_db_sync (fips);

   if (!callbacks) {
      editor_export_upload (name);
   }

   return 1;
}


void editor_export_reset_dirty(void) {
   
   int count;
   int i;

   count = editor_trkseg_get_current_trkseg ();

   for (i=0; i<count; i++) {
      int trkseg_flags;

      editor_trkseg_get
         (i, NULL, NULL, NULL, &trkseg_flags);

      if (!(trkseg_flags & ED_TRKSEG_FAKE)) {
         editor_trkseg_set_next_export (i);
         break;
      }
   }

   if (i == count) {
      roadmap_messagebox ("Error", "Can't find first trkseg to export.");
      return;
   }

   count = editor_line_get_count ();

   for (i=0; i<count; i++) {

      int flags;
      int cfcc;

      editor_line_get (i, NULL, NULL, NULL, &cfcc, &flags);
      /* editor_line_modify_properties (i, cfcc, flags | ED_LINE_DIRTY); */
   }

}


static void editor_export_file_dialog_ok(const char *filename,
                                         const char *mode) {

   editor_export_data (filename, NULL);
}


void editor_export_gpx(void) {
                                
   roadmap_fileselection_new ("Export data",
                              "gpx.gz",
                              roadmap_path_user (),
                              "w",
                              editor_export_file_dialog_ok);
}

int editor_export_empty(int fips) {
   
   int trkseg;
   int num_lines;
   int i;

   if (fips < 0) {
      return 1;
   }

   if (editor_db_activate (fips) == -1) {
      return 1;
   }
   
   editor_track_end ();

   trkseg = editor_trkseg_get_next_export ();

   if (trkseg != -1) return 0;

   num_lines = editor_line_get_count();

   for (i=0; i<num_lines; i++) {
      
      int flags;
      editor_line_get (i, NULL, NULL, NULL, NULL, &flags);

      if ((flags & ED_LINE_DIRTY)) return 0;
   }

   return 1;
}


void editor_export_initialize(void) {

   roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigAutoUpload, "no", "yes", NULL);
}


