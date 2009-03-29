/*
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
 *   Copyright (c) 2008, 2009, Danny Backx
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
 */

/**
 * @file
 * @brief cost calculations
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_line.h"
#include "roadmap_lang.h"
#include "roadmap_start.h"
// #include "roadmap_line_route.h"
// #include "roadmap_line_speed.h"
#include "roadmap_layer.h"
#include "roadmap_dialog.h"

// #include "navigate_traffic.h"
#include "navigate.h"
#include "navigate_cost.h"

#define PENALTY_NONE  0
#define PENALTY_SMALL 1
#define PENALTY_AVOID 2

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

extern RoadMapConfigDescriptor NavigateConfigAutoZoom;

void navigate_cost_preferences (void);

/**
 * @brief used (unknown how) in cost calculation
 */
static time_t start_time;

/**
 * @brief determine a penalty for taking some kind of road
 * @param line_id
 * @param layer
 * @param prev_line_id
 * @return
 */
static int calc_penalty (int line_id, int layer, int prev_line)
{
#if 0
	if (roadmap_layer_is_primary(layer) && roadmap_config_match (&CostAvoidPrimaryCfg, "yes"))
		return PENALTY_AVOID;

	if (roadmap_layer_is_trail(layer)) {
		if (roadmap_config_match (&CostAvoidTrailCfg, "yes"))
			return PENALTY_AVOID;
		if (roadmap_config_match (&CostAvoidTrailCfg, "Long trails")) {
			int length = roadmap_line_length (line_id);
			if (length > 300)
				return PENALTY_AVOID;
		}
	}

	if (roadmap_layer_is_pedestrian(layer))
		return PENALTY_AVOID;

	if (roadmap_config_match (&PreferSameStreetCfg, "yes")) {
		/* Note the roadmap_line_get_street function doesn't work yet !! */
		if (roadmap_line_get_street (line_id) != (roadmap_line_get_street (prev_line))) {
			return PENALTY_SMALL;
		}
	}

#endif
	return PENALTY_NONE;
}

/**
 * @brief
 * @param line_id
 * @param is_reversed
 * @param cur_cost
 * @param prev_line
 * @param is_prev_reversed
 * @param node_id
 * @return the cost. In this function, this is based on the sum of a penalty and a distance, so the metric is a strange one.
 */
static int cost_shortest (int line_id, int is_reversed, int cur_cost,
                          int prev_line, int is_prev_reversed,
                          int node_id) {
   int res;
   int layer = roadmap_line_get_layer (line_id);
   int penalty = calc_penalty (line_id, layer, prev_line);

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

   res = penalty + roadmap_line_length (line_id);
   roadmap_log (ROADMAP_DEBUG, "cost_shortest(%d,%d,%d) -> %d",
		   line_id, is_reversed, cur_cost, res);
   return res;
}

/**
 * @brief
 * @param line_id
 * @param is_reversed
 * @param cur_cost
 * @param prev_line
 * @param is_prev_reversed
 * @param node_id
 * @return
 */
static int cost_fastest (int line_id, int is_reversed, int cur_cost,
                         int prev_line, int is_prev_reversed,
                         int node_id) {

   int length, res;
   int layer = roadmap_line_get_layer (line_id);

   int penalty = 0;
   float m_s;	/**< speed in m/s */

   if (node_id != -1) {
      penalty = calc_penalty (line_id, layer, prev_line);

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

   m_s = roadmap_layer_speed(layer) / 3.6;	/* FIX ME needs to be configurable */

   res = (int) (length / m_s) + 1 + cur_cost;
   roadmap_log (ROADMAP_DEBUG, "cost_fastest(%d,%d,%d) -> %d",
		   line_id, is_reversed, cur_cost, res);
   return res;
}

/**
 * @brief
 * @param line_id
 * @param is_reversed
 * @param cur_cost
 * @param prev_line
 * @param is_prev_reversed
 * @param node_id
 * @return
 */
static int cost_fastest_traffic (int line_id, int is_reversed, int cur_cost,
                                 int prev_line, int is_prev_reversed,
                                 int node_id) {

   int cross_time, res;
   int layer = roadmap_line_get_layer (line_id);
   int penalty = PENALTY_NONE;
   
   if (node_id != -1) penalty = calc_penalty (line_id, layer, prev_line);

#if 1
#warning need to replace roadmap_line_speed
#else
   cross_time = roadmap_line_speed_get_cross_time_at (line_id, is_reversed,
                       start_time + cur_cost);

   if (!cross_time) cross_time =
         roadmap_line_speed_get_avg_cross_time (line_id, is_reversed);
#endif

   switch (penalty) {
      case PENALTY_AVOID:
         res = cross_time + 3600;
      case PENALTY_SMALL:
         res = cross_time + 60;
      case PENALTY_NONE:
      default:
         res = cross_time;
   }
   roadmap_log (ROADMAP_DEBUG, "cost_fastest_traffic(%d,%d,%d) -> %d",
		   line_id, is_reversed, cur_cost, res);
   return res;

}

/**
 * @brief
 */
void navigate_cost_reset (void)
{
   start_time = time(NULL);
}

/**
 * @brief Look up which function to use (fastest/fastest with traffic/shortest) according to config
 * @return the function requested
 */
NavigateCostFn navigate_cost_get (void)
{

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

/**
 * @brief
 * @param line_id
 * @param is_reversed
 * @param cur_cost
 * @param prev_line_id
 * @param is_prev_reversed
 * @return
 */
int navigate_cost_time (int line_id, int is_reversed, int cur_cost,
                        int prev_line_id, int is_prev_reversed)
{
#if 0
   return cost_fastest_traffic (line_id, is_reversed, cur_cost,
                                prev_line_id, is_prev_reversed, -1);
#else
   return cost_fastest (line_id, is_reversed, cur_cost,
                                prev_line_id, is_prev_reversed, -1);
#endif
}

/**
 * @brief Query the way in which we calculate a route's cost
 * @return
 */
int navigate_cost_type (void) {
   if (roadmap_config_match(&CostTypeCfg, "Fastest")) {
      return COST_FASTEST;
   } else {
      return COST_SHORTEST;
   }
}

#include "roadmap_dialog.h"

/**
 * @brief
 */
static void navigate_save_cost_config (void)
{
	roadmap_config_set (&CostUseTrafficCfg,
		(const char *)roadmap_dialog_get_data ("Preferences", "Use traffic statistics"));
	roadmap_config_set (&CostTypeCfg,
		(const char *)roadmap_dialog_get_data ("Preferences", "Type"));
	roadmap_config_set (&CostAvoidPrimaryCfg,
		(const char *)roadmap_dialog_get_data ("Preferences", "Avoid primaries"));
	roadmap_config_set (&PreferSameStreetCfg,
		(const char *)roadmap_dialog_get_data ("Preferences", "Prefer same street"));
	roadmap_config_set (&CostAvoidTrailCfg,
		(const char *)roadmap_dialog_get_data ("Preferences", "Avoid trails"));
	roadmap_config_set (&NavigateConfigAutoZoom,
		(const char *)roadmap_dialog_get_data ("Preferences", "Auto zoom"));
	roadmap_dialog_hide ("route_prefs");
}

/**
 * @brief
 */
static void button_ok (const char *name, void *data)
{
	navigate_save_cost_config();
}

/**
 * @brief
 */
static void button_recalc (const char *name, void *data)
{
	navigate_save_cost_config ();
	navigate_calc_route ();
}

static const char *yesno_label[2];
static const char *yesno[2];
static const char *type_label[2];
static const char *type_value[2];
static const char *trails_label[3];
static const char *trails_value[3];

/**
 * @brief
 */
static void create_dialog (void)
{ 
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

   roadmap_dialog_new_choice ("Preferences", "Use traffic statistics",
                              2,
			      0,
                              yesno_label,
                              (void **)yesno,
                              NULL);
        
   roadmap_dialog_new_choice ("Preferences", "Type",
                              2,
			      0,
                              type_label,
                              (void **)type_value,
                              NULL);
        
   roadmap_dialog_new_choice ("Preferences",
                              "Prefer same street",
                              2,
			      0,
                              yesno_label,
                              (void **)yesno,
                              NULL);
        
   roadmap_dialog_new_choice ("Preferences",
                              "Avoid primaries",
                              2,
			      0,
                              yesno_label,
                              (void **)yesno,
                              NULL);
        
   roadmap_dialog_new_choice ("Preferences",
                              "Avoid trails",
                              3,
			      0,
                              trails_label,
                              (void **)trails_value,
                              NULL);
        
   roadmap_dialog_new_choice ("Preferences",
                              "Auto zoom",
                              2,
			      0,
                              yesno_label,
                              (void **)yesno,
                              NULL);

   roadmap_dialog_add_button ("Ok", button_ok);
   roadmap_dialog_add_button ("Recalculate", button_recalc);
}

/**
 * @brief
 */
void navigate_cost_preferences (void)
{
	const char *value;

	if (roadmap_dialog_activate ("route_prefs", NULL)) {
		create_dialog ();
	}

	if (roadmap_config_match(&CostUseTrafficCfg, "yes"))
		value = yesno[0];
	else
		value = yesno[1];
	roadmap_dialog_set_data ("Preferences", "Use traffic statistics", (void *) value);

	if (roadmap_config_match(&CostTypeCfg, "Fastest"))
		value = type_value[0];
	else
		value = type_value[1];
	roadmap_dialog_set_data ("Preferences", "Type", (void *) value);

	if (roadmap_config_match(&CostAvoidPrimaryCfg, "yes"))
		value = yesno[0];
	else
		value = yesno[1];
	roadmap_dialog_set_data ("Preferences", "Avoid primaries", (void *) value);

	if (roadmap_config_match(&PreferSameStreetCfg, "yes"))
		value = yesno[0];
	else
		value = yesno[1];
	roadmap_dialog_set_data ("Preferences", "Prefer same street", (void *) value);

	if (roadmap_config_match(&CostAvoidTrailCfg, "yes"))
		value = trails_value[0];
	else if (roadmap_config_match(&CostAvoidTrailCfg, "no"))
		value = trails_value[1];
	else
		value = trails_value[2];
	roadmap_dialog_set_data ("Preferences", "Avoid trails", (void *) value);

	if (roadmap_config_match(&NavigateConfigAutoZoom, "yes"))
		value = yesno[0];
	else
		value = yesno[1];
	roadmap_dialog_set_data ("Preferences", "Auto zoom", (void *) value);

	roadmap_dialog_activate ("route_prefs", NULL);
}

/**
 * @brief
 */
void navigate_cost_initialize (void)
{
	roadmap_log (ROADMAP_DEBUG, "navigate_cost_initialize");

	roadmap_config_declare_enumeration ("preferences", &CostUseTrafficCfg,
			"yes", "no", NULL);
	roadmap_config_declare_enumeration ("preferences", &CostTypeCfg,
			"Fastest", "Shortest", NULL);
	roadmap_config_declare_enumeration ("preferences", &CostAvoidPrimaryCfg,
			"no", "yes", NULL);
	roadmap_config_declare_enumeration ("preferences", &PreferSameStreetCfg,
			"no", "yes", NULL);
	roadmap_config_declare_enumeration ("preferences", &CostAvoidTrailCfg,
			"yes", "no", "Long trails", NULL);
#if 0
	roadmap_start_add_action ("traffic",
			"Routing preferences",
			NULL,
			NULL,
			"Change routing preferences",
			navigate_cost_preferences);
#endif
}
