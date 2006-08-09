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

#include "roadmap.h"
#include "roadmap_dialog.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_gps.h"
#include "roadmap_locator.h"
#include "roadmap_plugin.h"
#include "roadmap_preferences.h"
#include "roadmap_navigate.h"
#include "roadmap_messagebox.h"

#include "../db/editor_db.h"
#include "../db/editor_line.h"
#include "../db/editor_street.h"
#include "../editor_main.h"
#include "../editor_log.h"

#include "update_range.h"


static int get_estimated_range (PluginLine *line, RoadMapGpsPosition *pos,
                                int direction,
                                int fraddl, int toaddl,
                                int fraddr, int toaddr,
                                int *left, int *right) {


   int total_length;
   int point_length;
   double rel;

   point_length =
      roadmap_plugin_calc_length ((RoadMapPosition *)pos, line, &total_length);

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
 

static int fill_dialog (PluginLine *line, RoadMapGpsPosition *pos,
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

      street_name = editor_street_get_street_fename (&properties);

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

      street_name = roadmap_street_get_street_fename (&properties);

      city_name = roadmap_street_get_street_city
                        (&properties, ROADMAP_STREET_LEFT_SIDE);

      roadmap_street_get_street_range
         (&properties, ROADMAP_STREET_LEFT_SIDE, &fraddl, &toaddl);
      roadmap_street_get_street_range
         (&properties, ROADMAP_STREET_RIGHT_SIDE, &fraddr, &toaddr);
   }

   roadmap_dialog_set_data ("Update", "Street", street_name);
   roadmap_dialog_set_data ("Update", "City", city_name);

   get_estimated_range
         (line, pos, direction, fraddl, toaddl, fraddr, toaddr, &left, &right);

   snprintf(str, sizeof(str), "%d", left);
   roadmap_dialog_set_data ("Update", "Left", str);
   snprintf(str, sizeof(str), "%d", right);
   roadmap_dialog_set_data ("Update", "Right", str);
   return 0;
}


static void update_range_cancel (const char *name, void *context) {

   roadmap_dialog_hide (name);
}


static void update_range_apply (const char *name, void *context) {

   roadmap_dialog_hide (name);
}


void update_range_dialog (void) {

   RoadMapGpsPosition pos;
   PluginLine line;
   int direction;

   if (roadmap_navigate_get_current (&pos, &line, &direction) == -1) {

      roadmap_messagebox ("Error", "Can't find current street.");
      return;
   }

   if (roadmap_dialog_activate ("Update street range", NULL)) {

      roadmap_dialog_new_label ("Update", "Street");
      roadmap_dialog_new_label ("Update", "City");
      roadmap_dialog_new_label ("Update", "Estimated range");
      roadmap_dialog_new_label ("Update", "Left");
      roadmap_dialog_new_label ("Update", "Right");

      roadmap_dialog_new_entry ("Update", "Update left", NULL);
      roadmap_dialog_new_entry ("Update", "Update right", NULL);

      roadmap_dialog_add_button ("Cancel", update_range_cancel);
      roadmap_dialog_add_button ("OK", update_range_apply);

      roadmap_dialog_complete (roadmap_preferences_use_keyboard ());
   }

   fill_dialog (&line, &pos, direction);
}

