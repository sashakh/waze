/* editor_plugin.c - implement plugin interfaces
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
 *   See editor_plugin.h
 */

#include <stdlib.h>

#include "roadmap_plugin.h"

#include "db/editor_db.h"
#include "db/editor_line.h"
#include "db/editor_override.h"
#include "db/editor_street.h"

#include "editor_main.h"
#include "editor_screen.h"

#include "editor_plugin.h"


static int editor_plugin_override_line (int line, int cfcc, int fips) {

   if (editor_db_activate (fips) == -1) {
      return 0;
   }

   if (editor_override_line_get_flags (line) & ED_LINE_DELETED) {
      return 1;
   }

   return 0;
}


static int editor_plugin_activate_db (const PluginLine *line) {
   
   return editor_db_activate (roadmap_plugin_get_fips (line));
}


static void editor_plugin_line_from (const PluginLine *line,
                                     RoadMapPosition *pos) {

   int line_id = roadmap_plugin_get_line_id (line);

   editor_line_get (line_id, pos, NULL, NULL, NULL, NULL);
}


static void editor_plugin_line_to (const PluginLine *line,
                                   RoadMapPosition *pos) {

   int line_id = roadmap_plugin_get_line_id (line);

   editor_line_get (line_id, NULL, pos, NULL, NULL, NULL);
}


static void editor_plugin_get_street (const PluginLine *line,
                                      PluginStreet *street) {

   int street_id;
   int line_id = roadmap_plugin_get_line_id (line);

   if (editor_line_get_street (line_id, &street_id, NULL) == -1) {
      street_id = -1;
   }

   roadmap_plugin_set_street (street, EditorPluginID, street_id);
}


static const char *editor_plugin_street_full_name (const PluginLine *line) {

   EditorStreetProperties properties;
   int line_id = roadmap_plugin_get_line_id (line);

   editor_street_get_properties (line_id, &properties);

   return editor_street_get_full_name (&properties);
}

static void editor_plugin_street_properties
           (const PluginLine *line, PluginStreetProperties *props) {

   EditorStreetProperties properties;
   int line_id = roadmap_plugin_get_line_id (line);

   editor_street_get_properties (line_id, &properties);

   props->address = editor_street_get_street_address (&properties);
   props->street = editor_street_get_street_name (&properties);
   props->city =
      editor_street_get_street_city (&properties, ED_STREET_LEFT_SIDE);
}


static RoadMapPluginHooks editor_plugin_hooks = {

      &editor_plugin_line_from,
      &editor_plugin_line_to,
      &editor_plugin_activate_db,
      &editor_street_get_distance,
      &editor_plugin_override_line,
      &editor_screen_override_pen,
      &editor_screen_repaint,
      &editor_plugin_get_street,
      &editor_plugin_street_full_name,
      &editor_plugin_street_properties,
      &editor_street_get_connected_lines,
      &editor_screen_adjust_layer,
      &editor_street_get_closest
};


int editor_plugin_register (void) {

   return roadmap_plugin_register (&editor_plugin_hooks);
}


void editor_plugin_unregister (int plugin_id) {

   roadmap_plugin_unregister (plugin_id);
}

