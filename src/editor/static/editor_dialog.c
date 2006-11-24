/* editor_dialog.c - editor navigation
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
 *   See editor_dialog.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "roadmap.h"
#ifdef SSD
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_entry.h"
#include "ssd/ssd_button.h"
#else
#include "roadmap_dialog.h"
#endif
#include "roadmap_lang.h"
#include "roadmap_layer.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"
#include "roadmap_locator.h"
#include "roadmap_address.h"
#include "roadmap_preferences.h"

#include "../db/editor_db.h"
#include "../db/editor_line.h"
#include "../db/editor_shape.h"
#include "../db/editor_point.h"
#include "../db/editor_street.h"
#include "../db/editor_route.h"
#include "../db/editor_override.h"
#include "../editor_main.h"
#include "../editor_log.h"

#include "editor_dialog.h"


typedef struct dialog_selected_lines {
   SelectedLine *lines;
   int           count;
} DialogSelectedLines;

static const char *def_values[2] = {"", ""};

static void editor_dialog_city_result (const char *result, void *data) {

#ifdef SSD
   ssd_dialog_activate ("Segment Properties", data);
#else
   roadmap_dialog_activate ("Segment Properties", data);
#endif   

   if ((result == NULL) || !strlen (result)) return;

#ifdef SSD
   ssd_dialog_set_value ("City", result);
#else
   roadmap_dialog_set_data ("General", "City", result);
#endif   
}


#ifdef SSD
static int editor_dialog_city_cb (SsdWidget widget, const char *new_value) {
   if (!strcmp(new_value, def_values[1])) {

      ssd_dialog_set_value ("City", def_values[0]);

      roadmap_address_search_dialog
         (NULL, editor_dialog_city_result, widget->context);

      return 0;
   }

   return 1;
}

#else

static void editor_dialog_city_cb (const char *name, void *context) {
   if (!strcmp(roadmap_dialog_get_data ("General", "City"), def_values[1])) {

      roadmap_dialog_set_data ("General", "City", def_values[0]);
      roadmap_dialog_hide ("Segment Properties");

      roadmap_address_search_dialog
         (NULL, editor_dialog_city_result, context);
   }
}

#endif


#ifndef SSD
static void editor_segments_cancel (const char *name, void *context) {

   free (context);
   roadmap_dialog_hide (name);
}
#endif

#ifdef SSD
static int editor_segments_apply (SsdWidget widget, const char *new_value) {
   DialogSelectedLines *selected_lines =
      (DialogSelectedLines *) ssd_dialog_context ();
#else
static void editor_segments_apply (const char *name, void *context) {
   DialogSelectedLines *selected_lines = (DialogSelectedLines *)context;
#endif


   int i;
   int route_id;
   LineRouteFlag route_from_flags;
   LineRouteFlag route_to_flags;
#ifdef SSD
   int type = (int) ssd_dialog_get_data ("Road type");
   int cfcc = type + ROADMAP_ROAD_FIRST;
   int direction = 0;
   
   char *street_type = (char *) ssd_dialog_get_value ("Street type");
   
   char *street_name = (char *) ssd_dialog_get_value ("Name");
   
   char *t2s = (char *) ssd_dialog_get_value ("Text to Speech");
   
   char *city = (char *) ssd_dialog_get_value ("City");
   char *zip = "";

   char *speed_limit_str = "";
#else   
   int type = (int) roadmap_dialog_get_data ("General", "Road type");
   int cfcc = type + ROADMAP_ROAD_FIRST;
   int direction = (int) roadmap_dialog_get_data ("General", "Direction");
   
   char *street_type =
      (char *) roadmap_dialog_get_data ("General", "Street type");
   
   char *street_name =
      (char *) roadmap_dialog_get_data ("General", "Name");
   
   char *t2s =
      (char *) roadmap_dialog_get_data ("General", "Text to Speech");
   
/*   char *street_range =
      (char *) roadmap_dialog_get_data ("General", "Street range"); */

   char *city = (char *) roadmap_dialog_get_data ("General", "City");
   char *zip = (char *) roadmap_dialog_get_data ("General", "Zip code");

   char *speed_limit_str =
      (char *) roadmap_dialog_get_data ("General", "Speed Limit");
#endif


   int street_id;
   int street_range_id;

   int city_id;
   int zip_id;
   int l_from, l_to;
   int r_from, r_to;
   LineRouteMax speed_limit;

   l_from = l_to = r_from = r_to = -1;

   //decode_range (street_range, &l_from, &l_to, &r_from, &r_to);
   speed_limit = (LineRouteMax) atoi (speed_limit_str);

   for (i=0; i<selected_lines->count; i++) {

      SelectedLine *line = &selected_lines->lines[i];

      if (editor_db_activate (line->line.fips) == -1) {

         editor_db_create (line->line.fips);

         if (editor_db_activate (line->line.fips) == -1) {
            continue;
         }
      }

      if (line->line.plugin_id != EditorPluginID) {

         int new_line = editor_line_copy
                        (line->line.line_id, line->line.cfcc, line->line.fips);
         if (new_line == -1) {
            editor_log
               (ROADMAP_ERROR,
                "Can't set line attributes - copy line failed.");
            continue;
         }

         line->line.line_id = new_line;
         line->line.plugin_id = EditorPluginID;
      }

      editor_line_modify_properties (line->line.line_id, cfcc, 0);

      editor_line_get_street (line->line.line_id, &street_id, &street_range_id);
      street_id = editor_street_create (street_name, street_type, "", "");
      
      editor_street_set_t2s (street_id, t2s);

      city_id = editor_street_create_city (city);
      zip_id = editor_street_create_zip (zip);

      if (street_range_id == -1) {
         street_range_id =
            editor_street_add_range
               (city_id, zip_id, l_from, l_to, city_id, zip_id, r_from, r_to);
      } else {

         editor_street_set_range
            (street_range_id, ED_STREET_LEFT_SIDE,
             &city_id, &zip_id, &l_from, &l_to);
         
         editor_street_set_range
            (street_range_id, ED_STREET_RIGHT_SIDE,
             &city_id, &zip_id, &r_from, &r_to);
         
      }
      editor_line_set_street (line->line.line_id, &street_id, &street_range_id);

      route_id = editor_line_get_route (line->line.line_id);

      if (route_id == -1) {
         route_id = editor_route_segment_add (0, 0, 0, 0);
         editor_line_set_route (line->line.line_id, route_id);
      } else {
         editor_route_segment_get
            (route_id, &route_from_flags, &route_to_flags, NULL, NULL);
      }

      if ((direction == 1) || (direction == 3)) {
         route_from_flags |= ROUTE_CAR_ALLOWED;
      } else {
         route_from_flags &= ~ROUTE_CAR_ALLOWED;
      }

      if ((direction == 2) || (direction == 3)) {
         route_to_flags |= ROUTE_CAR_ALLOWED;
      } else {
         route_to_flags &= ~ROUTE_CAR_ALLOWED;
      }

      if (route_id != -1) {
         editor_route_segment_set
            (route_id,
             route_from_flags, route_to_flags,
             speed_limit, speed_limit);
      }

      editor_line_mark_dirty (line->line.line_id);
   }

   if (selected_lines->count > 1) {

      int lines[100];
      unsigned int count = selected_lines->count;
      unsigned int i;

      if (count > sizeof(lines)/sizeof(int)) {
         count = sizeof(lines)/sizeof(int);
      }

      for (i=0; i < count; i++) {
         lines[i] = selected_lines->lines[i].line.line_id;
      }

      editor_street_distribute_range
         (lines, count, l_from, l_to, r_from, r_to);
   }
   
   editor_screen_reset_selected ();

#ifdef SSD
   ssd_dialog_hide_current ();
   return 1;
#else
   free (context);
   roadmap_dialog_hide (name);
#endif
}


#ifdef SSD
static void activate_dialog (const char *name,
                             DialogSelectedLines *selected_lines) {

   if (!ssd_dialog_activate (name, selected_lines)) {

      int *values;
      int count = ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1;
      int i;
      char **categories;
      static const char *lang_categories[ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1];
      SsdWidget dialog;
      SsdWidget group;

      if (!def_values[1][0]) {
         def_values[1] = roadmap_lang_get ("Search");
      }

      dialog = ssd_dialog_new (name, roadmap_lang_get (name),
                               SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE);

      group = ssd_container_new ("Length group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Length"), -1,
                        SSD_TEXT_LABEL));
      ssd_widget_add (group, ssd_text_new ("Length", "", -1, 0));
      ssd_widget_add (dialog, group);

      group = ssd_container_new ("Speed group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Speed"), -1,
                        SSD_TEXT_LABEL));
      ssd_widget_add (group, ssd_text_new ("Speed", "", -1, 0));
      ssd_widget_add (dialog, group);

      group = ssd_container_new ("Time group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_set_color (group, NULL, NULL);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Time"), -1,
                        SSD_TEXT_LABEL));
      ssd_widget_add (group, ssd_text_new ("Time", "", -1, SSD_END_ROW));
      ssd_widget_add (dialog, group);

      roadmap_layer_get_categories_names (&categories, &i);
      values = malloc ((count+1) * sizeof (int));

      for (i=0; i<count; i++) {
         values[i] = i;
         lang_categories[i] = roadmap_lang_get (categories[i]);
      }

      group = ssd_container_new ("Road type group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);

      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Road type"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
         ssd_choice_new ("Road type", count,
                                 (const char **)lang_categories,
                                 (const void**)values,
                                 SSD_END_ROW|SSD_ALIGN_VCENTER, NULL));
      ssd_widget_add (dialog, group);

      group = ssd_container_new ("Name group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Name"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
         ssd_entry_new ("Name", "", SSD_ALIGN_VCENTER));
      ssd_widget_add (dialog, group);

      group = ssd_container_new ("Text to Speech group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);
      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("Text to Speech"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
         ssd_entry_new ("Text to Speech", "", SSD_ALIGN_VCENTER));
      ssd_widget_add (dialog, group);

      group = ssd_container_new ("City group", NULL,
                  SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WIDGET_SPACE|SSD_END_ROW);

      ssd_widget_add (group,
         ssd_text_new ("Label", roadmap_lang_get("City"), -1,
                        SSD_TEXT_LABEL|SSD_ALIGN_VCENTER));

      ssd_widget_add (group,
         ssd_choice_new ("City",
                         sizeof(def_values) / sizeof(char **),
                         def_values,
                         (const void **)def_values,
                          SSD_END_ROW|SSD_ALIGN_VCENTER,
                          editor_dialog_city_cb));

      ssd_widget_add (dialog, group);

      ssd_widget_add (dialog,
         ssd_button_label ("confirm", roadmap_lang_get ("Ok"),
                        SSD_ALIGN_CENTER|SSD_START_NEW_ROW,
                        editor_segments_apply));

      ssd_dialog_activate (name, selected_lines);
   }
}

#else

static void activate_dialog (const char *name,
                             DialogSelectedLines *selected_lines) {

   if (roadmap_dialog_activate ("Segment Properties", selected_lines)) {

      int *values;
      int count = ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1;
      int i;
      char **categories;
      static const char *lang_categories[ROADMAP_ROAD_LAST - ROADMAP_ROAD_FIRST + 1];

      if (!def_values[1][0]) {
         def_values[1] = roadmap_lang_get ("Search");
      }

      /*
      char *direction_txts[] =
         { "Unknown", "With road", "Against road", "Both directions"};
      int direction_values[] = {0, 1, 2, 3};
      */
      
      roadmap_dialog_new_label ("Info", "Length");
      roadmap_dialog_new_label ("Info", "Time");

#if 0      
      roadmap_dialog_new_entry ("Right", "Street range", NULL);
      roadmap_dialog_new_entry ("Right", "City", NULL);
      roadmap_dialog_new_entry ("Right", "Zip code", NULL);

      roadmap_dialog_new_entry ("Left", "Street range", NULL);
      roadmap_dialog_new_entry ("Left", "City", NULL);
      roadmap_dialog_new_entry ("Left", "Zip code", NULL);
#endif
      roadmap_layer_get_categories_names (&categories, &i);
      values = malloc ((count+1) * sizeof (int));

      for (i=0; i<count; i++) {
         values[i] = i;
         lang_categories[i] = roadmap_lang_get (categories[i]);
      }

      roadmap_dialog_new_choice ("General", "Road type", count,
                                 (const char **)lang_categories,
                                 (void**)values, NULL);
      free (values);

/*      roadmap_dialog_new_entry ("General", "Street type", NULL); */
      roadmap_dialog_new_entry ("General", "Name", NULL);
      roadmap_dialog_new_entry ("General", "Text to Speech", NULL);
/*      roadmap_dialog_new_entry ("General", "Street range", NULL); */
      roadmap_dialog_new_choice ("General", "City",
         sizeof(def_values) / sizeof(char **),
         def_values, (void **)def_values, editor_dialog_city_cb);

/*      roadmap_dialog_new_choice ("General", "Direction", 4, direction_txts,
                                 (void**)direction_values, NULL);
      roadmap_dialog_new_entry ("General", "Zip code", NULL);
      roadmap_dialog_new_entry ("General", "Speed Limit", NULL); */

      roadmap_dialog_add_button ("Cancel", editor_segments_cancel);
      roadmap_dialog_add_button ("Ok", editor_segments_apply);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard ());
   }
}
#endif


/* NOTE: This function modifies the street_range parameter */
#if 0
static void decode_range (char *street_range,
                          int *from1,
                          int *to1,
                          int *from2,
                          int *to2) {

   int i = 0;
   int max = strlen(street_range);
   char *ranges[4];
   int num_ranges=0;

   while (i <= max) {

      while (!isdigit (street_range[i]) && (i <= max)) i++;
      if (i>max) break;

      ranges[num_ranges++] = street_range+i;
      if (num_ranges == (sizeof(ranges) / sizeof(ranges[0]) - 1)) break;

      while (isdigit (street_range[i]) && (i <= max)) i++;

      street_range[i] = '\0';
      i++;
   }
 
   if (num_ranges == 4) {
      *from1 = atoi (ranges[0]);
      *to1   = atoi (ranges[1]);

      if (from2 && to2) {
         *from2 = atoi (ranges[2]);
         *to2   = atoi (ranges[3]);
      }

   } else if (num_ranges > 1) {
      *from1 = atoi (ranges[0]);
      *to1   = atoi (ranges[1]);

      if (((*from1 & 1) && !(*to1 & 1)) ||
          (!(*from1 & 1) && (*to1 & 1))) {
         (*to1)--;
      }

      if (from2 && to2) {
         *from2 = (*from1) + 1;
         *to2   = (*to1) + 1;
      }
   }
}
#endif


static const char *editor_segments_find_city
         (int line, int plugin_id, int fips) {

   int i;
   int count;
   int layers_count;
   int layers[128];
   RoadMapPosition point;
   RoadMapNeighbour neighbours[50];

   roadmap_locator_activate (fips);
   editor_db_activate (fips);

   if (plugin_id == EditorPluginID) {
      editor_line_get (line, &point, NULL, NULL, NULL, NULL);
   } else {
      roadmap_line_from (line, &point);
   }

   layers_count = roadmap_layer_all_roads (layers, 128);

   count = roadmap_street_get_closest
           (&point,
            layers,
            layers_count,
            neighbours,
            sizeof(neighbours) / sizeof(RoadMapNeighbour));

   count = roadmap_plugin_get_closest
               (&point, layers, layers_count, neighbours, count,
                sizeof(neighbours) / sizeof(RoadMapNeighbour));

   for (i = 0; i < count; ++i) {

      const char *city;
      
      if (roadmap_plugin_get_id (&neighbours[i].line) == EditorPluginID) {
         EditorStreetProperties properties;

         editor_street_get_properties
            (roadmap_plugin_get_line_id (&neighbours[i].line),
             &properties);

         city =
            editor_street_get_street_city
               (&properties, ED_STREET_LEFT_SIDE);
         
      } else if (roadmap_plugin_get_id (&neighbours[i].line) ==
            ROADMAP_PLUGIN_ID) {
         RoadMapStreetProperties properties;

         roadmap_street_get_properties
            (roadmap_plugin_get_line_id (&neighbours[i].line),
             &properties);

         city =
            roadmap_street_get_street_city
               (&properties, ROADMAP_STREET_LEFT_SIDE);

      } else {
         city = "";
      }

      if (strlen(city)) {
         return city;
      }
   }

   return "";
}


void editor_segments_fill_dialog (SelectedLine *lines, int lines_count) {

   const char *street_name = "";
   const char *street_type = "";
   const char *t2s = "";
   const char *l_city = "";
   const char *r_city = "";
   const char *l_zip = "";
   const char *r_zip = "";
   int l_from = -1;
   int l_to = -1;
   int r_from = -1;
   int r_to = -1;
   int direction = 0;
   LineRouteMax speed_limit = 0;
   int cfcc = ROADMAP_ROAD_LAST;
   int same_street = 1;
   int same_direction = 1;
   int same_speed_limit = 1;
   int same_l_city = 1;
   int same_r_city = 1;
   int same_l_zip = 1;
   int same_r_zip = 1;

   //char range_str[100];
   int i;

   for (i=0; i<lines_count; i++) {
      
      const char *this_name = "";
      const char *this_type = "";
      const char *this_t2s = "";
      const char *this_l_city = "";
      const char *this_r_city = "";
      const char *this_l_zip = "";
      const char *this_r_zip = "";
      int this_l_from = -1;
      int this_l_to = -1;
      int this_r_from = -1;
      int this_r_to = -1;
      int this_direction = 0;
      LineRouteMax this_speed_limit = 0;
      int route_id;

      if (lines[i].line.plugin_id == EditorPluginID) {
         if (editor_db_activate (lines[i].line.fips) != -1) {

            EditorStreetProperties properties;
            editor_street_get_properties (lines[i].line.line_id, &properties);

            this_name = editor_street_get_street_fename (&properties);
            this_type = editor_street_get_street_fetype (&properties);
            this_t2s = editor_street_get_street_t2s (&properties);
            this_l_city =
               editor_street_get_street_city
                  (&properties, ED_STREET_LEFT_SIDE);
            this_r_city =
               editor_street_get_street_city
                  (&properties, ED_STREET_RIGHT_SIDE);
            this_l_zip =
               editor_street_get_street_zip
                  (&properties, ED_STREET_LEFT_SIDE);
            this_r_zip =
               editor_street_get_street_zip
                  (&properties, ED_STREET_RIGHT_SIDE);
            editor_street_get_street_range
               (&properties, ED_STREET_LEFT_SIDE, &this_l_from, &this_l_to);
            editor_street_get_street_range
               (&properties, ED_STREET_RIGHT_SIDE, &this_r_from, &this_r_to);

            route_id = editor_line_get_route (lines[i].line.line_id);

            if (route_id != -1) {
               this_direction =
                  editor_route_get_direction (route_id, ROUTE_CAR_ALLOWED);
               editor_route_segment_get
                  (route_id, NULL, NULL,
                   &this_speed_limit, NULL);
            }
         }
      } else {

         RoadMapStreetProperties properties;

         if (roadmap_locator_activate (lines[i].line.fips) < 0) {
            continue;
         }

         roadmap_street_get_properties (lines[i].line.line_id, &properties);
    
         this_name = roadmap_street_get_street_fename (&properties);
         this_type = roadmap_street_get_street_fetype (&properties);
         this_t2s = roadmap_street_get_street_t2s (&properties);

         this_l_city =
            roadmap_street_get_street_city
               (&properties, ROADMAP_STREET_LEFT_SIDE);
         this_r_city =
            roadmap_street_get_street_city
               (&properties, ROADMAP_STREET_RIGHT_SIDE);
         this_l_zip =
            roadmap_street_get_street_zip
               (&properties, ROADMAP_STREET_LEFT_SIDE);
         this_r_zip =
            roadmap_street_get_street_zip
               (&properties, ROADMAP_STREET_RIGHT_SIDE);
         roadmap_street_get_street_range
            (&properties, ROADMAP_STREET_LEFT_SIDE, &this_l_from, &this_l_to);
         roadmap_street_get_street_range
            (&properties, ROADMAP_STREET_RIGHT_SIDE, &this_r_from, &this_r_to);

         route_id = editor_override_line_get_route (lines[0].line.line_id);

         if (route_id != -1) {
            this_direction =
               editor_route_get_direction (route_id, ROUTE_CAR_ALLOWED);
            editor_route_segment_get
               (route_id, NULL, NULL, &this_speed_limit, NULL);
         } else {

            this_direction =
               roadmap_line_route_get_direction
                  (lines[i].line.line_id, ROUTE_CAR_ALLOWED);
            roadmap_line_route_get_speed_limit
                  (lines[i].line.line_id,
                   &this_speed_limit, &this_speed_limit);
         }
      }

      if (same_street) {
         if (!strlen(street_type)) {
            street_type = this_type;
         }

         if (!strlen(t2s)) {
            t2s = this_t2s;
         }

         if (l_from == -1) {
            l_from = this_l_from;
         }
         l_to = this_l_to;

         if (r_from == -1) {
            r_from = this_r_from;
         }
         r_to = this_r_to;

         if (!strlen(street_name)) {
            street_name = this_name;
         } else {
            if (strlen(this_name) && strcmp(this_name, street_name)) {
               street_name = "";
               street_type = "";
               t2s = "";
               same_street = 0;
               l_from = l_to = r_from = r_to = -1;
            }
         }
       }

      if (same_direction) {

         if (!direction) {
            direction = this_direction;
         } else {
            if (this_direction && (direction != this_direction)) {
               direction = 0;
               same_direction = 0;
            }
         }
      }

      if (same_speed_limit) {

         if (!speed_limit) {
            speed_limit = this_speed_limit;
         } else {
            if (this_speed_limit && (speed_limit != this_speed_limit)) {
               speed_limit = 0;
               same_speed_limit = 0;
            }
         }
      }

      if (same_l_city) {
         if (!strlen(l_city)) {
            l_city = this_l_city;
         } else {
            if (strlen(this_l_city) && strcmp(this_l_city, l_city)) {
               same_l_city = 0;
               l_city = "";
            }
         }
      } 
      
      if (same_r_city) {
         if (!strlen(r_city)) {
            r_city = this_r_city;
         } else {
            if (strlen(this_r_city) && strcmp(this_r_city, r_city)) {
               same_r_city = 0;
               r_city = "";
            }
         }
      } 
      
      if (same_l_zip) {
         if (!strlen(l_zip)) {
            l_zip = this_l_zip;
         } else {
            if (strlen(this_l_zip) && strcmp(this_l_zip, l_zip)) {
               same_l_zip = 0;
               l_zip = "";
            }
         }
      } 
      
      if (same_r_zip) {
         if (!strlen(r_zip)) {
            r_zip = this_r_zip;
         } else {
            if (strlen(this_r_zip) && strcmp(this_r_zip, r_zip)) {
               same_r_zip = 0;
               r_zip = "";
            }
         }
      } 
      
      if (lines[i].line.cfcc < cfcc) {
         cfcc = lines[i].line.cfcc;
      }
   }

   if (same_l_city && same_r_city && !strlen(l_city) && !strlen(r_city)) {
      
      l_city = r_city = editor_segments_find_city
             (lines[0].line.line_id, lines[0].line.plugin_id, lines[0].line.fips);
   }

#ifdef SSD
   ssd_dialog_set_data ("Road type", (void *) (cfcc - ROADMAP_ROAD_FIRST));
   ssd_dialog_set_value ("Name", street_name);
   ssd_dialog_set_value ("Text to Speech", t2s);

   if (strlen(l_city)) {
      ssd_dialog_set_value ("City", l_city);
   } else {
      ssd_dialog_set_value ("City", r_city);
   }

#else
   roadmap_dialog_set_data
      ("General", "Road type", (void *) (cfcc - ROADMAP_ROAD_FIRST));
   roadmap_dialog_set_data ("General", "Street type", street_type);
   roadmap_dialog_set_data ("General", "Name", street_name);
   roadmap_dialog_set_data ("General", "Text to Speech", t2s);

   if (strlen(l_city)) {
      roadmap_dialog_set_data ("General", "City", l_city);
   } else {
      roadmap_dialog_set_data ("General", "City", r_city);
   }

#endif


#if 0
   if ((l_from != -1) &&
         (l_to != -1) &&
         (r_from != -1) &&
         (r_to != -1)) {

      snprintf (range_str, sizeof(range_str),
            "%d-%d, %d-%d", l_from, l_to, r_from, r_to);

   } else {
      range_str[0] = '\0';
   }
   roadmap_dialog_set_data ("General", "Street range", range_str);
   
   roadmap_dialog_set_data ("General", "Direction", (void *)direction);
   
   if (strlen(l_zip)) {
      roadmap_dialog_set_data ("General", "Zip code", l_zip);
   } else {
      roadmap_dialog_set_data ("General", "Zip code", r_zip);
   }
   snprintf(range_str, sizeof(range_str), "%d", speed_limit);
   roadmap_dialog_set_data ("General", "Speed Limit", range_str);

   /* Left side */
   if ((l_from != -1) && (l_to != -1)) {
      snprintf (range_str, sizeof(range_str), "%d-%d", l_from , l_to);
   } else {
      range_str[0] = '\0';
   }
   roadmap_dialog_set_data ("Left", "Street range", range_str);
   roadmap_dialog_set_data ("Left", "City", l_city);
   roadmap_dialog_set_data ("Left", "Zip code", l_zip);

   /* Right side */
   if ((r_from != -1) && (r_to != -1)) {
      snprintf (range_str, sizeof(range_str), "%d-%d", r_from , r_to);
   } else {
      range_str[0] = '\0';
   }
   roadmap_dialog_set_data ("Right", "Street range", range_str);
   roadmap_dialog_set_data ("Right", "City", r_city);
   roadmap_dialog_set_data ("Right", "Zip code", r_zip);
#endif
}


void editor_segments_properties (SelectedLine *lines, int lines_count) {

   char str[100];
   int total_length = 0;
   int total_time = 0;
   int i;
   DialogSelectedLines *selected_lines = malloc (sizeof(*selected_lines));

   selected_lines->lines = lines;
   selected_lines->count = lines_count;

   activate_dialog("Segment Properties", selected_lines);
   editor_segments_fill_dialog (lines, lines_count);

   /* Collect info data */
   for (i=0; i<selected_lines->count; i++) {
      int line_length;
      int time;

      SelectedLine *line = &selected_lines->lines[i];

      if (line->line.plugin_id == EditorPluginID) {

         line_length = editor_line_length (line->line.line_id);
         time = editor_line_get_cross_time (line->line.line_id, 1);
         
      } else  {
         LineRouteTime from_cross_time;
         LineRouteTime to_cross_time;

         line_length = roadmap_line_length (line->line.line_id);
         if (roadmap_line_route_get_cross_times
               (line->line.line_id, &from_cross_time, &to_cross_time) == -1) {
            time = -1; //TODO get real speed
         } else {
            time = from_cross_time;
         }
      }

      if (time > 0) {
         total_time += time;
      } else {

         if ((total_time == 0) || (total_length == 0)) {
            total_time += (int)(1.0 *line_length / 10);
         } else {
            total_time +=
               (int)(1.0 * line_length / (total_length/total_time)) + 1;
         }
      }

      total_length += line_length;
   }

#ifdef SSD
   if (selected_lines->lines[0].line.plugin_id == ROADMAP_PLUGIN_ID) {
      int line = selected_lines->lines[0].line.line_id;
      int avg_speed = roadmap_line_route_get_avg_speed (line, 0);
      int cur_speed = roadmap_line_route_get_speed (line, 0);

      snprintf (str, sizeof(str), "%d (%d)",
                avg_speed, cur_speed);
      ssd_dialog_set_value ("Speed", str);
   } else {
      ssd_dialog_set_value ("Speed", "");
   }
#endif   

   snprintf (str, sizeof(str), "%d %s",
             total_length, roadmap_lang_get("meters"));
#ifdef SSD
   ssd_dialog_set_value ("Length", str);
#else   
   roadmap_dialog_set_data ("Info", "Length", str);
#endif   

   snprintf (str, sizeof(str), "%d %s",
             total_time, roadmap_lang_get("seconds"));

#ifdef SSD
   ssd_dialog_set_value ("Time", str);
#else   
   roadmap_dialog_set_data ("Info", "Time", str);
#endif   
}

