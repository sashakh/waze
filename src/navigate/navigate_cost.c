/* navigate_cost.c - cost calculations
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
 * SYNOPSYS:
 *
 *   See navigate_cost.h
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_line.h"
#include "roadmap_lang.h"
#include "roadmap_start.h"
#include "roadmap_line_route.h"

#include "navigate_traffic.h"
#include "navigate_main.h"
#include "navigate_cost.h"

#define PENALTY_NONE  0
#define PENALTY_SMALL 1
#define PENALTY_AVOID 2

static time_t start_time;

static RoadMapConfigDescriptor CostUseTrafficCfg =
                  ROADMAP_CONFIG_ITEM("Routing", "Use traffic statistics");
static RoadMapConfigDescriptor CostTypeCfg =
                  ROADMAP_CONFIG_ITEM("Routing", "Type");
static RoadMapConfigDescriptor PreferSameStreetCfg =
                  ROADMAP_CONFIG_ITEM("Routing", "Prefer same street");
static RoadMapConfigDescriptor CostAvoidPrimaryCfg =
                  ROADMAP_CONFIG_ITEM("Routing", "Avoid primaries");
static RoadMapConfigDescriptor CostAvoidTrailCfg =
                  ROADMAP_CONFIG_ITEM("Routing", "Avoid trails");

static void cost_preferences (void);

static int calc_penalty (int line_id, int cfcc, int prev_line_id) {

   switch (cfcc) {
      case ROADMAP_ROAD_FREEWAY:
      case ROADMAP_ROAD_PRIMARY:
      case ROADMAP_ROAD_SECONDARY:
         if (roadmap_config_match (&CostAvoidPrimaryCfg, "yes"))
            return PENALTY_AVOID;
         break;
      case ROADMAP_ROAD_4X4:
      case ROADMAP_ROAD_TRAIL:
         if (roadmap_config_match (&CostAvoidTrailCfg, "yes"))
            return PENALTY_AVOID;

         if (roadmap_config_match (&CostAvoidTrailCfg, "Long trails")) {
            int length = roadmap_line_length (line_id);

            if (length > 300) return PENALTY_AVOID;
         }
         break;
      case ROADMAP_ROAD_PEDESTRIAN:
      case ROADMAP_ROAD_WALKWAY:
         return PENALTY_AVOID;
   }

   if (roadmap_config_match (&PreferSameStreetCfg, "yes")) {
      if (roadmap_line_get_street (line_id) !=
         (roadmap_line_get_street (prev_line_id))) {
         
         return PENALTY_SMALL;
      }
   }

   return PENALTY_NONE;
}


static int cost_shortest (int line_id, int is_revesred, int cur_cost,
                          int prev_line_id, int is_prev_reversed,
                          int node_id) {

   int cfcc = roadmap_line_cfcc (line_id);
   int penalty = calc_penalty (line_id, cfcc, prev_line_id);

   switch (penalty) {
      case PENALTY_AVOID:
         penalty = 100000;
         break;
      case PENALTY_SMALL:
         penalty = 500;
         break;
      case PENALTY_NONE:
         penalty = 0;
         break;
   }

   return penalty + roadmap_line_length (line_id);
}

static int cost_fastest (int line_id, int is_revesred, int cur_cost,
                         int prev_line_id, int is_prev_reversed,
                         int node_id) {

   int length;
   int cfcc = roadmap_line_cfcc (line_id);

   int penalty = 0;
   float m_s;

   if (node_id != -1) {
      penalty = calc_penalty (line_id, cfcc, prev_line_id);

      switch (penalty) {
         case PENALTY_AVOID:
            penalty = 100000;
            break;
         case PENALTY_SMALL:
            penalty = 500;
            break;
         case PENALTY_NONE:
            penalty = 0;
            break;
      }
   }

   length = penalty + roadmap_line_length (line_id);

   switch (cfcc) {
      case ROADMAP_ROAD_FREEWAY:
         m_s = (float)(100 / 3.6);
         break;
      case ROADMAP_ROAD_PRIMARY:
         m_s = (float)(75 / 3.6);
         break;
      case ROADMAP_ROAD_SECONDARY:
      case ROADMAP_ROAD_RAMP:
         m_s = (float)(65 / 3.6);
         break;
      case ROADMAP_ROAD_MAIN:
         m_s = (float)(30 / 3.6);
         break;
      default:
         m_s = (float)(20 / 3.6);
         break;
   }

   return (int) (length / m_s) + 1;
}

static int cost_fastest_traffic (int line_id, int is_revesred, int cur_cost,
                                 int prev_line_id, int is_prev_reversed,
                                 int node_id) {

   int cross_time;
   int cfcc = roadmap_line_cfcc (line_id);
   int penalty = PENALTY_NONE;
   
   if (node_id != -1) penalty = calc_penalty (line_id, cfcc, prev_line_id);

   cross_time = roadmap_line_route_get_cross_time_at (line_id, is_revesred,
                       start_time + cur_cost);

   if (!cross_time) cross_time =
         roadmap_line_route_get_avg_cross_time (line_id, is_revesred);

   switch (penalty) {
      case PENALTY_AVOID:
         return cross_time + 3600;
      case PENALTY_SMALL:
         return cross_time + 60;
      case PENALTY_NONE:
      default:
         return cross_time;
   }

}

void navigate_cost_reset (void) {
   start_time = time(NULL);
}

NavigateCostFn navigate_cost_get (void) {

   if (navigate_cost_type () == COST_FASTEST) {
      if (roadmap_config_match(&CostUseTrafficCfg, "yes")) {
         return &cost_fastest_traffic;
      } else {
         return &cost_fastest;
      }
   } else {
      return &cost_shortest;
   }
}

int navigate_cost_time (int line_id, int is_revesred, int cur_cost,
                        int prev_line_id, int is_prev_reversed) {

   return cost_fastest_traffic (line_id, is_revesred, cur_cost,
                                prev_line_id, is_prev_reversed, -1);
}

void navigate_cost_initialize (void) {

   roadmap_config_declare_enumeration
      ("preferences", &CostUseTrafficCfg, "yes", "no", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &CostTypeCfg, "Fastest", "Shortest", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &CostAvoidPrimaryCfg, "no", "yes", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &PreferSameStreetCfg, "no", "yes", NULL);

   roadmap_config_declare_enumeration
       ("preferences", &CostAvoidTrailCfg, "yes", "no", "Long trails", NULL);
   roadmap_start_add_action ("traffic", "Routing preferences", NULL, NULL,
      "Change routing preferences",
      cost_preferences);
}

int navigate_cost_type (void) {
   if (roadmap_config_match(&CostTypeCfg, "Fastest")) {
      return COST_FASTEST;
   } else {
      return COST_SHORTEST;
   }
}

/**** temporary dialog ****/

#ifdef SSD

#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_button.h"

static int button_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "OK") | !strcmp(widget->name, "Recalculate")) {
      roadmap_config_set (&CostUseTrafficCfg,
                           (const char *)ssd_dialog_get_data ("traffic"));
      roadmap_config_set (&CostTypeCfg,
                           (const char *)ssd_dialog_get_data ("type"));
      roadmap_config_set (&CostAvoidPrimaryCfg,
                           (const char *)ssd_dialog_get_data ("avoidprime"));
      roadmap_config_set (&PreferSameStreetCfg,
                           (const char *)ssd_dialog_get_data ("samestreet"));
      roadmap_config_set (&CostAvoidTrailCfg,
                           (const char *)ssd_dialog_get_data ("avoidtrails"));
      ssd_dialog_hide_current ();

      if (!strcmp(widget->name, "Recalculate")) {
         navigate_main_calc_route ();
      }
      return 1;

   }

   ssd_dialog_hide_current ();
   return 1;
}

static const char *yesno_label[2];
static const char *yesno[2];
static const char *type_label[2];
static const char *type_value[2];
static const char *trails_label[3];
static const char *trails_value[3];

static void create_ssd_dialog (void) {

   SsdWidget box;
   SsdWidget dialog = ssd_dialog_new ("route_prefs",
                      roadmap_lang_get ("Routing preferences"),
                      SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE);


   if (!yesno[0]) {
      yesno[0] = "Yes";
      yesno[1] = "No";
      yesno_label[0] = roadmap_lang_get (yesno[0]);
      yesno_label[1] = roadmap_lang_get (yesno[1]);
      type_value[0] = "Fastest";
      type_value[1] = "Shortest";
      type_label[0] = roadmap_lang_get (type_value[0]);
      type_label[1] = roadmap_lang_get (type_value[1]);
      trails_value[0] = "Yes";
      trails_value[1] = "No";
      trails_value[2] = "Long trails";
      trails_label[0] = roadmap_lang_get (trails_value[0]);
      trails_label[1] = roadmap_lang_get (trails_value[1]);
      trails_label[2] = roadmap_lang_get (trails_value[2]);
   }

   box = ssd_container_new ("traffic group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_add (box,
      ssd_text_new ("traffic_label",
                     roadmap_lang_get ("Use traffic statistics"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

    ssd_widget_add (box,
         ssd_choice_new ("traffic", 2,
                         (const char **)yesno_label,
                         (const void **)yesno,
                         SSD_END_ROW, NULL));
        
   ssd_widget_add (dialog, box);

   box = ssd_container_new ("type group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_add (box,
      ssd_text_new ("type_label",
                     roadmap_lang_get ("Type"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

    ssd_widget_add (box,
         ssd_choice_new ("type", 2,
                         (const char **)type_label,
                         (const void **)type_value,
                         SSD_END_ROW, NULL));
        
   ssd_widget_add (dialog, box);

   box = ssd_container_new ("samestreet group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_add (box,
      ssd_text_new ("samestreet_label",
                     roadmap_lang_get ("Prefer same street"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

    ssd_widget_add (box,
         ssd_choice_new ("samestreet", 2,
                         (const char **)yesno_label,
                         (const void **)yesno,
                         SSD_END_ROW, NULL));
        
   ssd_widget_add (dialog, box);

   box = ssd_container_new ("avoidprime group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_add (box,
      ssd_text_new ("avoidprime_label",
                     roadmap_lang_get ("Avoid primaries"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

    ssd_widget_add (box,
         ssd_choice_new ("avoidprime", 2,
                         (const char **)yesno_label,
                         (const void **)yesno,
                         SSD_END_ROW, NULL));
        
   ssd_widget_add (dialog, box);

   box = ssd_container_new ("avoidtrails group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_add (box,
      ssd_text_new ("avoidtrails_label",
                     roadmap_lang_get ("Avoid trails"),
                    -1, SSD_TEXT_LABEL|SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE));

    ssd_widget_add (box,
         ssd_choice_new ("avoidtrails", 3,
                         (const char **)trails_label,
                         (const void **)trails_value,
                         SSD_END_ROW, NULL));
        
   ssd_widget_add (dialog, box);

   ssd_widget_add (dialog,
      ssd_button_label ("OK", roadmap_lang_get ("Ok"),
                        SSD_ALIGN_CENTER|SSD_START_NEW_ROW, button_callback));
   ssd_widget_add (dialog,
      ssd_button_label ("Recalculate", roadmap_lang_get ("Recalculate"),
                        SSD_ALIGN_CENTER, button_callback));
}

void cost_preferences (void) {

   const char *value;

   if (!ssd_dialog_activate ("route_prefs", NULL)) {
      create_ssd_dialog ();
      ssd_dialog_activate ("route_prefs", NULL);
   }

   if (roadmap_config_match(&CostUseTrafficCfg, "yes")) value = yesno[0];
   else value = yesno[1];
   ssd_dialog_set_data ("traffic", (void *) value);

   if (roadmap_config_match(&CostTypeCfg, "Fastest")) value = type_value[0];
   else value = type_value[1];
   ssd_dialog_set_data ("type", (void *) value);

   if (roadmap_config_match(&CostAvoidPrimaryCfg, "yes")) value = yesno[0];
   else value = yesno[1];
   ssd_dialog_set_data ("avoidprime", (void *) value);

   if (roadmap_config_match(&PreferSameStreetCfg, "yes")) value = yesno[0];
   else value = yesno[1];
   ssd_dialog_set_data ("samestreet", (void *) value);

   if (roadmap_config_match(&CostAvoidTrailCfg, "yes")) value = trails_value[0];
   else if (roadmap_config_match(&CostAvoidTrailCfg, "no")) value = trails_value[1];
   else value = trails_value[2];
   ssd_dialog_set_data ("avoidtrails", (void *) value);
}

#else
   void traffic_preferences (void) {}
#endif

