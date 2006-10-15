/* update_range.c - Street range entry
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
 *   See update_range.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_dialog.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_gps.h"
#include "roadmap_locator.h"
#include "roadmap_county.h"
#include "roadmap_math.h"
#include "roadmap_adjust.h"
#include "roadmap_plugin.h"
#include "roadmap_preferences.h"
#include "roadmap_navigate.h"
#include "roadmap_messagebox.h"

#include "../db/editor_db.h"
#include "../db/editor_line.h"
#include "../db/editor_street.h"
#include "../db/editor_marker.h"
#include "../editor_main.h"
#include "../editor_log.h"

#include "update_range.h"

#ifdef _WIN32
#define NEW_LINE "\r\n"
#else
#define NEW_LINE "\n"
#endif

#define STREET_PREFIX "Street"
#define CITY_PREFIX   "City"
#define UPDATE_LEFT   "Update left"
#define UPDATE_RIGHT  "Update right"

static RoadMapGpsPosition CurrentGpsPoint;
static RoadMapPosition    CurrentFixedPosition;

static int extract_field(const char *note, const char *field_name,
                         char *field, int size) {

   const char *lang_field_name = roadmap_lang_get (field_name);

   /* Save space for string termination */
   size--;

   while (*note) {
      if (!strncmp (field_name, note, strlen(field_name)) ||
          !strncmp (lang_field_name, note, strlen(lang_field_name))) {

         const char *end;

         while (*note && (*note != ':')) note++;
         if (!*note) break;
         note++;

         while (*note && (*note == ' ')) note++;

         end = note;

         while (*end && (*end != NEW_LINE[0])) end++;

         if (note == end) {
            field[0] = '\0';
            return 0;
         }

         if ((end - note) < size) size = end - note;
         strncpy(field, note, size);
         field[size] = '\0';

         return 0;
      }

      while (*note && (*note != NEW_LINE[strlen(NEW_LINE)-1])) note++;
      if (*note) note++;
   }

   return -1;
}


static int update_range_export(int marker,
                               const char **description,
                               const char  *keys[MAX_ATTR],
                               char        *values[MAX_ATTR],
                               int         *count) {
   
   char field[100];
   const char *note = editor_marker_note (marker);
   *count = 0;
   *description = NULL;
   
   if (extract_field (note, STREET_PREFIX, field, sizeof(field)) == -1) {
      roadmap_log (ROADMAP_ERROR, "Update range - Can't find street name.");
      return -1;
   } else {
      keys[*count] = "street";
      values[*count] = strdup(field);
      (*count)++;
   }

   if (extract_field (note, CITY_PREFIX, field, sizeof(field)) == -1) {
      roadmap_log (ROADMAP_ERROR, "Update range - Can't find city name.");
      return -1;
   } else {
      keys[*count] = "city";
      values[*count] = strdup(field);
      (*count)++;
   }

   if (extract_field (note, UPDATE_LEFT, field, sizeof(field)) != -1) {
      if (atoi(field) <= 0) {
         roadmap_log (ROADMAP_ERROR, "Update range - Left range is invalid.");
         return -1;
      } else {
         keys[*count] = "left";
         values[*count] = strdup(field);
         (*count)++;
      }
   }

   if (extract_field (note, UPDATE_RIGHT, field, sizeof(field)) != -1) {
      if (atoi(field) <= 0) {
         roadmap_log (ROADMAP_ERROR, "Update range - Right range is invalid.");
         return -1;
      } else {
         keys[*count] = "right";
         values[*count] = strdup(field);
         (*count)++;
      }
   }

   if (*count < 2) {
      roadmap_log (ROADMAP_ERROR,
                     "Update range - No range updates were found.");
      return -1;
   }

   return 0;
}


static int update_range_verify(int marker,
                               unsigned char *flags,
                               const char **note) {

   char field[100]; 
   int found_update = 0;
   
   if (extract_field (*note, STREET_PREFIX, field, sizeof(field)) == -1) {
      roadmap_messagebox ("Error", "Can't find street name.");
      return -1;
   }

   if (extract_field (*note, CITY_PREFIX, field, sizeof(field)) == -1) {
      roadmap_messagebox ("Error", "Can't find city name.");
      return -1;
   }

   if (extract_field (*note, UPDATE_LEFT, field, sizeof(field)) != -1) {
      if (atoi(field) <= 0) {
         roadmap_messagebox ("Error", "Left range is invalid.");
         return -1;
      }

      found_update++;
   }

   if (extract_field (*note, UPDATE_RIGHT, field, sizeof(field)) != -1) {
      if (atoi(field) <= 0) {
         roadmap_messagebox ("Error", "Right range is invalid.");
         return -1;
      }

      found_update++;
   }

   if (!found_update) {
      roadmap_messagebox ("Error", "No range updates were found.");
      return -1;
   }

   return 0;
}


static int UpdateRangeMarkerType;
static EditorMarkerType UpdateRangeMarker = {
   "Street range",
   update_range_export,
   update_range_verify
};


static int get_estimated_range(const PluginLine *line,
                               const RoadMapPosition *pos,
                               int direction,
                               int fraddl, int toaddl,
                               int fraddr, int toaddr,
                               int *left, int *right) {


   int total_length;
   int point_length;
   double rel;

   point_length =
      roadmap_plugin_calc_length (pos, line, &total_length);

   rel = 1.0 * point_length / total_length;

   *left  = fraddl + (int) ((toaddl - fraddl + 1) / 2 * rel) * 2;
   *right = fraddr + (int) ((toaddr - fraddr + 1) / 2 * rel) * 2;

   if (direction == ROUTE_DIRECTION_AGAINST_LINE) {
      int tmp = *left;

      *left = *right;
      *right = tmp;
   }
      
   return 0;
}
 

static int fill_dialog(PluginLine *line, RoadMapPosition *pos,
                       int direction) {

   const char *street_name;
   const char *city_name;
   int fraddl;
   int toaddl;
   int fraddr;
   int toaddr;
   int left;
   int right;
   char str[100];

   if (line->plugin_id == EditorPluginID) {
      EditorStreetProperties properties;

      if (editor_db_activate (line->fips) == -1) return -1;

      editor_street_get_properties (line->line_id, &properties);

      street_name = editor_street_get_street_name (&properties);

      city_name = editor_street_get_street_city
                        (&properties, ED_STREET_LEFT_SIDE);

      editor_street_get_street_range
         (&properties, ED_STREET_LEFT_SIDE, &fraddl, &toaddl);
      editor_street_get_street_range
         (&properties, ED_STREET_RIGHT_SIDE, &fraddr, &toaddr);

   } else {
      RoadMapStreetProperties properties;

      if (roadmap_locator_activate (line->fips) < 0) return -1;

      roadmap_street_get_properties (line->line_id, &properties);

      street_name = roadmap_street_get_street_name (&properties);

      city_name = roadmap_street_get_street_city
                        (&properties, ROADMAP_STREET_LEFT_SIDE);

      roadmap_street_get_street_range
         (&properties, ROADMAP_STREET_LEFT_SIDE, &fraddl, &toaddl);
      roadmap_street_get_street_range
         (&properties, ROADMAP_STREET_RIGHT_SIDE, &fraddr, &toaddr);
   }

   roadmap_dialog_set_data ("Update", STREET_PREFIX, street_name);

   if (!city_name) city_name = "";
   roadmap_dialog_set_data ("Update", CITY_PREFIX, city_name);

   get_estimated_range
         (line, pos, direction, fraddl, toaddl, fraddr, toaddr, &left, &right);

   snprintf(str, sizeof(str), "%s:%d  %s:%d",
         roadmap_lang_get ("Left"), left,
         roadmap_lang_get ("Right"), right);

   roadmap_dialog_set_data ("Update", "Estimated", str);

   roadmap_dialog_set_data ("Update", UPDATE_LEFT, "");
   roadmap_dialog_set_data ("Update", UPDATE_RIGHT, "");
   return 0;
}


static void update_range_apply(const char *name, void *context) {

   const char *updated_left =
      roadmap_dialog_get_data ("Update", UPDATE_LEFT);

   const char *updated_right =
      roadmap_dialog_get_data ("Update", UPDATE_RIGHT);

   if (!*updated_left && !*updated_right) {
      roadmap_dialog_hide (name);
      return;
      
   } else {
      char note[100];
      int fips;

      if (roadmap_county_by_position (&CurrentFixedPosition, &fips, 1) < 1) {
         roadmap_messagebox ("Error", "Can't locate county");
         return;
      }

      if (editor_db_activate (fips) == -1) {

         editor_db_create (fips);

         if (editor_db_activate (fips) == -1) {

            roadmap_messagebox ("Error", "Can't update range");
            return;
         }
      }

      snprintf(note, sizeof(note), "%s: %s%s",
               roadmap_lang_get (STREET_PREFIX), 
               (char *)roadmap_dialog_get_data ("Update", STREET_PREFIX),
               NEW_LINE);

      snprintf(note + strlen(note), sizeof(note) - strlen(note),
               "%s: %s%s", roadmap_lang_get (CITY_PREFIX), 
               (char *)roadmap_dialog_get_data ("Update", CITY_PREFIX),
               NEW_LINE);

      if (*updated_left) {

         snprintf(note + strlen(note), sizeof(note) - strlen(note),
                  "%s: %s%s", roadmap_lang_get (UPDATE_LEFT), updated_left,
                  NEW_LINE);
      }

      if (*updated_right) {

         snprintf(note + strlen(note), sizeof(note) - strlen(note),
                  "%s: %s%s", roadmap_lang_get (UPDATE_RIGHT), updated_right,
                  NEW_LINE);
      }

      if (editor_marker_add (CurrentFixedPosition.longitude,
                             CurrentFixedPosition.latitude,
                             CurrentGpsPoint.steering,
                             time(NULL),
                             UpdateRangeMarkerType,
                             ED_MARKER_UPLOAD, note) == -1) {

         roadmap_messagebox ("Error", "Can't save marker.");
      }
   }

   roadmap_dialog_hide (name);
}


static void update_range_cancel(const char *name, void *context) {

   roadmap_dialog_hide (name);
}


void update_range_dialog(void) {

   RoadMapPosition from;
   RoadMapPosition to;
   PluginLine line;
   int direction;

   if (roadmap_navigate_get_current
         (&CurrentGpsPoint, &line, &direction) == -1) {

      roadmap_messagebox ("Error", "Can't find current street.");
      return;
   }

   roadmap_plugin_line_from (&line, &from);
   roadmap_plugin_line_to   (&line, &to);

   if (roadmap_math_get_distance_from_segment
            ((RoadMapPosition *)&CurrentGpsPoint, &from, &to,
             &CurrentFixedPosition, NULL) > 100) {

      roadmap_messagebox ("Error", "Can't find a road near point.");
      return;
   }


   if (roadmap_dialog_activate ("Update street range", NULL)) {

      roadmap_dialog_new_label ("Update", STREET_PREFIX);
      roadmap_dialog_new_label ("Update", CITY_PREFIX);
      roadmap_dialog_new_label ("Update", "Estimated");

      roadmap_dialog_new_entry ("Update", UPDATE_LEFT, NULL);
      roadmap_dialog_new_entry ("Update", UPDATE_RIGHT, NULL);

      roadmap_dialog_add_button ("Cancel", update_range_cancel);
      roadmap_dialog_add_button ("OK", update_range_apply);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard ());
   }

   fill_dialog (&line, &CurrentFixedPosition, direction);
}


void update_range_initialize(void) {
   UpdateRangeMarkerType = editor_marker_reg_type (&UpdateRangeMarker);
}
