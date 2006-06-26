/* roadmap_plugin.h - plugin interfaces
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
 * DESCRIPTION:
 *
 *   There are two types: PluginLine and PluginStreet. These types are
 *   used instead of the integer number which RoadMap uses.  A line (or a
 *   street) can now be represented by other modules (I call them plugins)
 *   and they will be responsible to draw it or do whatever is needed with
 *   it. PluginLine is a structure which among other things identifies the
 *   plugin which is responsible to the object. The roadmap_plugin layer is
 *   used when some operation is needed to be done on a Plugin type.
 *
 *   Currently, roadmap_plugin checks whether the object is a roadmap
 *   object in which case it calls the regular roadmap functions to
 *   complete the operation. Otherwise, a different plugin is responsible
 *   for it. Every plugin must register some callback functions so the
 *   roadmap_layer can call the required functions to complete the
 *   operation.
 *
 */

#ifndef INCLUDED__ROADMAP_PLUGIN__H
#define INCLUDED__ROADMAP_PLUGIN__H

#include "roadmap_types.h"
#include "roadmap_canvas.h"


/* Map access functions. --------------------------------------------------- */

typedef struct {

   int plugin_id;
   int line_id;
   int layer;
   int fips;

} PluginLine;

typedef struct {

   int plugin_id;
   int street_id;

} PluginStreet;

typedef struct {

   const char *address;
   const char *street;
   const char *city;
   PluginStreet plugin_street;

} PluginStreetProperties;

typedef struct {

   PluginLine line;
   int distance;
   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition intersection;

} RoadMapNeighbour;


#define PLUGIN_MAKE_LINE(plugin_id, line_id, layer, fips) \
   {plugin_id, line_id, layer, fips}

#define PLUGIN_LINE_NULL {-1, -1, -1, -1}
#define PLUGIN_STREET_NULL {-1, -1}
#define ROADMAP_NEIGHBOUR_NULL {PLUGIN_LINE_NULL, -1, {0,0}, {0,0}, {0,0}}


int  roadmap_plugin_same_line (const PluginLine *line1,
                               const PluginLine *line2);

int  roadmap_plugin_same_street (const PluginStreet *street1,
                                 const PluginStreet *street2);

void roadmap_plugin_get_street (const PluginLine *line, PluginStreet *street);

void roadmap_plugin_line_from (const PluginLine *line, RoadMapPosition *pos);

void roadmap_plugin_line_to (const PluginLine *line, RoadMapPosition *pos);

int  roadmap_plugin_get_id (const PluginLine *line);

int  roadmap_plugin_get_fips (const PluginLine *line);

int  roadmap_plugin_get_line_id (const PluginLine *line);

int  roadmap_plugin_get_street_id (const PluginStreet *street);

void roadmap_plugin_set_line (PluginLine *line,
                              int plugin_id,
                              int line_id,
                              int layer,
                              int fips);

void roadmap_plugin_set_street (PluginStreet *street,
                                int plugin_id,
                                int street_id);

int  roadmap_plugin_activate_db (const PluginLine *line);

int  roadmap_plugin_get_distance (RoadMapPosition *point,
                                  PluginLine *line,
                                  RoadMapNeighbour *result);


/* Hook functions definition. ---------------------------------------------- */

typedef int (*plugin_override_line_hook) (int line, int layer, int fips);

typedef int (*plugin_override_pen_hook) (int line,
                                         int layer,
                                         int fips,
                                         int pen_type,
                                         RoadMapPen *override_pen);

typedef void (*plugin_screen_repaint_hook) (int max_pen);
typedef int  (*plugin_activate_db_func) (const PluginLine *line);
typedef void (*plugin_line_pos_func) (const PluginLine *line,
                                      RoadMapPosition *pos);

typedef int (*plugin_get_distance_func) (const RoadMapPosition *point,
                                         PluginLine *line,
                                         RoadMapNeighbour *result);

typedef void (*plugin_get_street_func) (const PluginLine *line,
                                        PluginStreet *street);

typedef const char *(*plugin_street_full_name_func) (const PluginLine *line);

typedef void (*plugin_street_properties_func) (const PluginLine *line,
                                               PluginStreetProperties *props);

typedef int (*plugin_find_connected_lines_func)
                  (const RoadMapPosition *crossing,
                   PluginLine *plugin_lines,
                   int max);

typedef void (*plugin_adjust_layer_hook)
                  (int layer, int thickness, int pen_count);

typedef struct {
   const char                      *name;
   plugin_line_pos_func             line_from;
   plugin_line_pos_func             line_to;
   plugin_activate_db_func          activate_db;
   plugin_get_distance_func         get_distance;
   plugin_override_line_hook        override_line;
   plugin_override_pen_hook         override_pen;
   plugin_screen_repaint_hook       screen_repaint;
   plugin_get_street_func           get_street;
   plugin_street_full_name_func     get_street_full_name;
   plugin_street_properties_func    get_street_properties;
   plugin_find_connected_lines_func find_connected_lines;
   plugin_adjust_layer_hook         adjust_layer;

} RoadMapPluginHooks;

#define ROADMAP_PLUGIN_ID 0

#define PLUGIN_VALID(plugin)      (plugin.plugin_id >= 0)
#define INVALIDATE_PLUGIN(plugin) (plugin.plugin_id = -1)


/* Plugin control functions. ----------------------------------------------- */

int roadmap_plugin_register    (RoadMapPluginHooks *hooks);
void roadmap_plugin_unregister (int plugin_id);

int roadmap_plugin_override_line (int line, int layer, int fips);

int roadmap_plugin_override_pen (int line,
                                 int layer,
                                 int fips,
                                 int pen_type,
                                 RoadMapPen *override_pen);

void roadmap_plugin_screen_repaint (int max_pen);

const char *roadmap_plugin_street_full_name (PluginLine *line);

void roadmap_plugin_get_street_properties (PluginLine *line,
                                           PluginStreetProperties *props);

int roadmap_plugin_find_connected_lines (RoadMapPosition *crossing,
                                         PluginLine *plugin_lines,
                                         int max);

void roadmap_plugin_adjust_layer (int layer,
                                  int thickness,
                                  int pen_count);

#endif /* INCLUDED__ROADMAP_PLUGIN__H */

