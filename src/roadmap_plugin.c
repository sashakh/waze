/*
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright (c) 2008, 2009, Danny Backx.
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
 * @brief roadmap_plugin.c - plugin layer
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_line.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_shape.h"
#include "roadmap_file.h"
#include "roadmap_library.h"
#include "roadmap_gps.h"
#include "roadmap_plugin.h"
#include "roadmap_messagebox.h"

#define MAX_PLUGINS 10

/**
 * @brief an internal structure to hide private fields in
 */
typedef struct RoadMapPrivateHooks {
	int			initialized;	/**< */
	RoadMapPluginHooks	*plugin;	/**< */
} RoadMapPrivateHooks;

// static RoadMapPluginHooks *hooks[MAX_PLUGINS] = {0};
static RoadMapPrivateHooks	hooks[MAX_PLUGINS];
static int PluginCount = 0;

/**
 * @brief access function for the plugin hooks structures, guards array size and initialisation
 * @param id the plugin selected
 * @return a pointer to the plugin structure
 */
static RoadMapPluginHooks *get_hooks (int id) {

   assert (id < MAX_PLUGINS);

   /* This make sure we don't call a plugin method before the plugins are initialized.
    * An exception to this : the roadmap_plugin_actions_menu function.
    */
   if (hooks[id].initialized)
	   return hooks[id].plugin;
   return NULL;
}

/**
 * @brief register a plugin
 * @param hook the plugin structure
 * @return an identifier for this plugin
 */
int roadmap_plugin_register (RoadMapPluginHooks *hook) {

   int i;

   roadmap_log(ROADMAP_WARNING, "roadmap_plugin_register(%s)", hook->name);
   if (PluginCount == 0) {
	   for (i=1; i<MAX_PLUGINS; i++) {
		   hooks[i].initialized = 0;
		   hooks[i].plugin = NULL;
	   }
   }

   if (hook->size != sizeof(RoadMapPluginHooks)) {
	   roadmap_log(ROADMAP_FATAL, "Size of hook for plugin '%s' is invalid (%d, should be %d)",
			   hook->name,
			   hook->size,
			   sizeof(RoadMapPluginHooks));
   }

   for (i=1; i<MAX_PLUGINS; i++) {
      
      if (hooks[i].plugin == NULL) {
         hooks[i].plugin= hook;
	 hooks[i].initialized = 0;
         PluginCount = i;
	 roadmap_log (ROADMAP_DEBUG, "roadmap_plugin_register(%s) -> %d",
			 hook->name, i);
         return i;
      }
   }

   roadmap_log (ROADMAP_WARNING, "roadmap_plugin_register: too many plugins");
   return -1;
}

/**
 * @brief stop using some plugin
 * @param plugin_id identifier for the plugin
 */
void roadmap_plugin_unregister (int plugin_id) {

   hooks[plugin_id].plugin = NULL;
   hooks[plugin_id].initialized = -1;
}

/**
 * @brief list the active plugins
 * @return a string that should be freed by the caller
 */
char *roadmap_plugin_list_all_plugins(void)
{
	int	i, len;
	char	*r, num[8];

	for (i=len=0; i<=PluginCount; i++) {
		if (hooks[i].initialized && hooks[i].plugin && hooks[i].plugin->name)
			len += strlen(hooks[i].plugin->name) + 10;
	}
	r = malloc(len);
	r[0] = '\0';
	for (i=0; i<=PluginCount; i++)
		if (hooks[i].initialized && hooks[i].plugin && hooks[i].plugin->name) {
			sprintf(num, "%d - ", i);
			strcat(r, num);
			strcat(r, hooks[i].plugin->name);
#ifdef	_WIN32
			strcat(r, "\r\n");
#else
			strcat(r, "\n");
#endif
		}
	return r;
}

/**
 * @brief are the two lines the same ?
 * @param line1
 * @param line2
 * @return
 */
int roadmap_plugin_same_line (const PluginLine *line1,
                              const PluginLine *line2) {

   return ( (line1->plugin_id == line2->plugin_id) &&
            (line1->line_id == line2->line_id) &&
            (line1->fips == line2->fips) );
}

/**
 * @brief
 * @param street1
 * @param street2
 * @return
 */
int roadmap_plugin_same_street (const PluginStreet *street1,
                                const PluginStreet *street2) {

   return ( (street1->plugin_id == street2->plugin_id) &&
            (street1->street_id == street2->street_id) );
}

/**
 * @brief
 * @param line
 * @return
 */
int roadmap_plugin_get_id (const PluginLine *line) {
   return line->plugin_id;
}

/**
 * @brief
 * @param line
 * @return
 */
int roadmap_plugin_get_fips (const PluginLine *line) {
   return line->fips;
}

/**
 * @brief
 * @param line
 * @return
 */
int roadmap_plugin_get_line_id (const PluginLine *line) {
   return line->line_id;
}

/**
 * @brief
 * @param street
 * @return
 */
int roadmap_plugin_get_street_id (const PluginStreet *street) {
   return street->street_id;
}

/**
 * @brief
 * @param line
 * @param plugin_id
 * @param line_id
 * @param layer
 * @param fips
 * @return
 */
void roadmap_plugin_set_line (PluginLine *line,
                              int plugin_id,
                              int line_id,
                              int layer,
                              int fips) {

   line->plugin_id = plugin_id;
   line->line_id = line_id;
   line->layer = layer;
   line->fips = fips;
}

/**
 * @brief
 * @param street
 * @param plugin_id
 * @param street_id
 * @return
 */
void roadmap_plugin_set_street (PluginStreet *street,
                                int plugin_id,
                                int street_id) {

   street->plugin_id = plugin_id;
   street->street_id = street_id;
}

/**
 * @brief
 * @param line
 * @return
 */
int roadmap_plugin_activate_db (const PluginLine *line) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      if (roadmap_locator_activate (line->fips) != ROADMAP_US_OK) {
         return -1;
      }

      return 0;

   } else {
      RoadMapPluginHooks *hp = get_hooks (line->plugin_id);

      if (hp == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         return -1;
      }

      if (hp->activate_db != NULL) {
         return (*hp->activate_db) (line);
      }

      return 0;
   }
}


/**
 * @brief calculate the distance between a point and a line
 * @param point the point to use
 * @param line the line to use
 * @param result store results in this structure
 * @result whether something sensible was found
 */
int roadmap_plugin_get_distance (RoadMapPosition *point, PluginLine *line, RoadMapNeighbour *result)
{
   if (line->plugin_id == ROADMAP_PLUGIN_ID) {
      return roadmap_street_get_distance (point, line->line_id, line->layer, result);
   } else {
      RoadMapPluginHooks *hp = get_hooks (line->plugin_id);

      if (hp == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.", line->plugin_id);
         return 0;
      }

      if (hp->get_distance != NULL) {
         return (*hp->get_distance) (point, line, result);
      }

      return 0;
   }
}

/**
 * @brief
 * @param line
 * @param pos
 */
void roadmap_plugin_line_from (const PluginLine *line, RoadMapPosition *pos) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      roadmap_line_from (line->line_id, pos);
   } else {
      RoadMapPluginHooks *hp = get_hooks (line->plugin_id);
      
      if (hp == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         pos->longitude = pos->latitude = 0;
         return;
      }

      if (hp->line_from != NULL) {
         (*hp->line_from) (line, pos);

      } else {
         pos->longitude = pos->latitude = 0;
      }

      return;
   }
}

/**
 * @brief
 * @param line
 * @param pos
 */
void roadmap_plugin_line_to (const PluginLine *line, RoadMapPosition *pos) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      roadmap_line_to (line->line_id, pos);
   } else {
      RoadMapPluginHooks *hp = get_hooks (line->plugin_id);
      
      if (hp == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         pos->longitude = pos->latitude = 0;
         return;
      }

      if (hp->line_to != NULL) {
         (*hp->line_to) (line, pos);

      } else {
         pos->longitude = pos->latitude = 0;
      }

      return;
   }
}

/**
 * @brief
 * @param line
 * @param layer
 * @param fips
 * @return
 */
int roadmap_plugin_override_line (int line, int layer, int fips) {

   int i;

   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hp = get_hooks (i);
      if (hp == NULL) continue;

      if (hp->override_line != NULL) {

         int res = hp->override_line (line, layer, fips);

         if (res) return res;
      }
   }

   return 0;
}


/**
 * @brief
 * @param line
 * @param layer
 * @param pen_type
 * @param fips
 * @param override_pen
 * @return
 */
int roadmap_plugin_override_pen (int line,
                                 int layer,
                                 int pen_type,
                                 int fips,
                                 RoadMapPen *override_pen) {
   int i;

   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hp = get_hooks (i);
      if (hp == NULL) continue;

      if (hp->override_pen != NULL) {

         int res = hp->override_pen
                     (line, layer, fips, pen_type, override_pen);

         if (res) return res;
      }
   }

   return 0;
}


void roadmap_plugin_screen_repaint (int max_pen) {

   int i;

   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hp = get_hooks (i);
      if ((hp == NULL) || (hp->screen_repaint == NULL)) continue;

      hp->screen_repaint (max_pen);
   }
}


void roadmap_plugin_get_street (const PluginLine *line, PluginStreet *street) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      RoadMapStreetProperties properties;

      roadmap_street_get_properties (line->line_id, &properties);
      street->plugin_id = ROADMAP_PLUGIN_ID;
      street->street_id = properties.street;

   } else {
      RoadMapPluginHooks *hp = get_hooks (line->plugin_id);
      street->plugin_id = line->plugin_id;

      if (hp == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         street->street_id = -1;
         return;
      }

      if (hp->get_street != NULL) {
         (*hp->get_street) (line, street);

      } else {
         street->street_id = -1;
      }

      return;
   }
}


const char *roadmap_plugin_street_full_name (PluginLine *line) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      RoadMapStreetProperties properties;
      roadmap_street_get_properties (line->line_id, &properties);

      return roadmap_street_get_full_name (&properties);

   } else {
      RoadMapPluginHooks *hp = get_hooks (line->plugin_id);

      if (hp == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         return "";
      }

      if (hp->get_street_full_name != NULL) {
         return (*hp->get_street_full_name) (line);
      }

      return "";
   }
}

#if 0
void roadmap_plugin_get_street_properties (const PluginLine *line,
                                           PluginStreetProperties *props,
					   int type)
#else
void roadmap_plugin_get_street_properties (const PluginLine *line,
                                           PluginStreetProperties *props)
#endif
{
   if (line->plugin_id == ROADMAP_PLUGIN_ID) {
      RoadMapStreetProperties rm_properties;

#warning "roadmap_street_get_street fix needed"
#if 0
      if (type == PLUGIN_STREET_ONLY) {
	      roadmap_street_get_street (line->line_id, &rm_properties);
	      props->street = roadmap_street_get_street_name (&rm_properties);
	      props->plugin_street.plugin_id = ROADMAP_PLUGIN_ID;
	      props->plugin_street.street_id = rm_properties.street;
	      return;
      }
#endif
      roadmap_street_get_properties (line->line_id, &rm_properties);

      props->address = roadmap_street_get_street_address (&rm_properties);
      props->street = roadmap_street_get_street_name (&rm_properties);
      props->city = roadmap_street_get_city_name (&rm_properties);
      props->plugin_street.plugin_id = ROADMAP_PLUGIN_ID;
      props->plugin_street.street_id = rm_properties.street;
   
      return;

   } else {
      RoadMapPluginHooks *hp = get_hooks (line->plugin_id);

      props->address = "";
      props->street = "";
      props->city = "";

      if (hp == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         return;
      }

#if 0
      if (hp->get_street_properties != NULL) {
         (*hp->get_street_properties) (line, props, type);
#else
      if (hp->get_street_properties != NULL) {
         (*hp->get_street_properties) (line, props);
#endif
      }

      return;
   }
}

int roadmap_plugin_find_connected_lines (RoadMapPosition *crossing,
                                         PluginLine *plugin_lines,
                                         int max)
{
   int i;
   int count = 0;

   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hp = get_hooks (i);
      if (hp && hp->find_connected_lines)
	      count += hp->find_connected_lines (crossing, plugin_lines + count, max - count);
   }

   return count;
}

void roadmap_plugin_adjust_layer (int layer, int thickness, int pen_count)
{
   int i;
   for (i=1; i<=PluginCount; i++) {
      RoadMapPluginHooks *hp = get_hooks (i);
      if (hp && hp->adjust_layer)
	      hp->adjust_layer (layer, thickness, pen_count);
   }
}

/**
 * @brief call plugins to tell the user where to go
 */
void roadmap_plugin_format_messages (void)
{
   int i;
   for (i=1; i<=PluginCount; i++) {
      RoadMapPluginHooks *hp = get_hooks (i);
      if (hp && hp->format_messages)
	      hp->format_messages ();
   }
}

/**
 * @brief call plugins with new information about where we are
 * This is called by roadmap_display_activate() to pass info to the plugins.
 * @param position the GPS info
 * @param line the line we're on
 * @param street the street we're on
 * @param street_has_changed boolean value to alert the plugin of a change
 */
void roadmap_plugin_update_position (const RoadMapPosition *position,
		const PluginLine *line, const PluginStreet *street,
		const int street_has_changed)
{
   int i;

   for (i=1; i<=PluginCount; i++) {
      RoadMapPluginHooks *hp = get_hooks (i);
      if (hp && hp->update_position)
	      hp->update_position (position, line, street, street_has_changed);
   }
}

/**
 * @brief
 */
void roadmap_plugin_after_refresh (void)
{
   int i;
   for (i=1; i<=PluginCount; i++) {
      RoadMapPluginHooks *hp = get_hooks (i);
      if (hp && hp->adjust_layer)
	      hp->after_refresh ();
   }
}


int roadmap_plugin_get_closest
       (const RoadMapPosition *position,
        int *categories, int categories_count,
        RoadMapNeighbour *neighbours, int count,
        int max)
{
   int i;

   for (i=1; i<=PluginCount; i++) {
      RoadMapPluginHooks *hp = get_hooks (i);
      if (hp == NULL) continue;

      if (hp->get_closest != NULL) {

         count = hp->get_closest
                     (position, categories, categories_count,
                      neighbours, count, max);
      }
   }

   return count;
}

#ifdef HAVE_NAVIGATE_PLUGIN
int roadmap_plugin_get_direction (PluginLine *line, int who)
{
#if 1
#warning implement roadmap_plugin_get_direction
	return 0;
#else
   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      return roadmap_line_route_get_direction (line->line_id, who);

   } else {
      RoadMapPluginHooks *hp = get_hooks (line->plugin_id);

      if (hp == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         return 0;
      }

      if (hp->route_direction != NULL) {
         return (*hp->route_direction) (line, who);
      }

      return 0;
   }
#endif
}
#endif

#if defined(HAVE_TRIP_PLUGIN) || defined(HAVE_NAVIGATE_PLUGIN)
void roadmap_plugin_get_line_points (const PluginLine *line,
                                     RoadMapPosition  *from_pos,
                                     RoadMapPosition  *to_pos,
                                     int              *first_shape,
                                     int              *last_shape,
                                     RoadMapShapeItr  *shape_itr)
{
   roadmap_plugin_line_from (line, from_pos);
   roadmap_plugin_line_to (line, to_pos);

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {
#warning "roadmap_line_shapes needed"
#if 0
      roadmap_line_shapes (line->line_id, -1, first_shape, last_shape);
      *shape_itr = &roadmap_shape_get_position;
#endif
   } else {
      RoadMapPluginHooks *hp = get_hooks (line->plugin_id);
      
      if (hp == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         *first_shape = *last_shape = -1;
         *shape_itr   = NULL;
         return;
      }

      //FIXME implement for plugins
#if 0
      if (hp->line_shapes != NULL) {
         (*hp->line_shapes) (line, first_shape, last_shape, shape_itr);

      } else {
#else
         {
#endif   
         *first_shape = *last_shape = -1;
         *shape_itr   = NULL;
      }

      return;
   }
}
#endif

/*
 * roadmap_plugin_initialize should be here, but it's moved to roadmap_start.c
 * because otherwise all the tools like buildmap fail to build.
 */
int roadmap_plugin_get_line_cfcc (const PluginLine *line) {
	   return line->layer;
}

#ifdef HAVE_EDITOR_PLUGIN
int roadmap_plugin_calc_length (const RoadMapPosition *position,
		const PluginLine *line,
		int *total_length)
{
	RoadMapPosition line_from_pos;
	RoadMapPosition line_to_pos;
	int first_shape;
	int last_shape;
	RoadMapShapeItr shape_itr;
	roadmap_plugin_get_line_points (line, &line_from_pos, &line_to_pos,
			&first_shape, &last_shape, &shape_itr);
	return roadmap_math_calc_line_length (position,
			&line_from_pos, &line_to_pos,
			first_shape, last_shape,
			shape_itr, total_length);
} 
#endif

/**
 * @brief call the "factory" handler to register actions and menu items
 * Note that this function avoids using get_hooks() so it can work before plugin initialize.
 *
 * @param handler the handler function from roadmap_factory.
 */
void roadmap_plugin_actions_menu(roadmap_plugin_action_menu_handler handler)
{
	int i;

	for (i=1; i<=PluginCount; i++) {
		RoadMapPluginHooks *hp = hooks[i].plugin;
		if (hp == NULL)
			continue;
		/*
		 * Only call this if both actions and menu are supplied.
		 * This matches the current implementation of roadmap_factory.c where
		 * plugin menus can only use their own actions.
		 */
		if (hp->actions == NULL || hp->menu == NULL)
			continue;
		(*handler)(hp->actions, hp->menu);
	}
}

void roadmap_plugin_initialize_all_plugins(void)
{
	int			id;
	RoadMapPluginHooks	*p;

	for (id=1; id<= PluginCount; id++) {
		p = hooks[id].plugin;
		if (!p)
			continue;
		roadmap_log(ROADMAP_DEBUG, "roadmap_plugin_initialize(%s)", p->name);
		if (p->initialize)
			p->initialize();
		hooks[id].initialized = 1;
	}
}

/**
 * @brief plugins that maintain a route should clear it now
 */
void roadmap_plugin_route_clear(void)
{
	int			id;
	RoadMapPluginHooks	*p;

	for (id=1; id<= PluginCount; id++) {
		p = hooks[id].plugin;
		if (!p)
			continue;
		if (p->route_clear)
			p->route_clear();
	}
}

/**
 * @brief plugins that maintain a route should add this line to it
 * @param line
 * @param layer
 * @param fips
 */
void roadmap_plugin_route_add(int line, int layer, int fips)
{
	int			id;
	RoadMapPluginHooks	*p;

	for (id=1; id<= PluginCount; id++) {
		p = hooks[id].plugin;
		if (!p)
			continue;
		if (p->route_add)
			p->route_add(line, layer, fips);
	}
}
