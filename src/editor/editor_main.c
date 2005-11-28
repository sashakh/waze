/* editor_main.c - main plugin file
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
 *   See editor_main.h
 */

#include "../roadmap.h"
#include "../roadmap_pointer.h"
#include "../roadmap_plugin.h"
#include "../roadmap_layer.h"

#include "editor_screen.h"
#include "track/editor_track.h"
#include "editor_plugin.h"
#include "editor_main.h"

int EditorEnabled = 0;
int EditorPluginID = -1;


int editor_is_enabled (void) {
   return EditorEnabled;
}


void editor_main_initialize (void) {

   editor_screen_initialize ();
   editor_track_initialize ();
   editor_main_set (1);

   EditorPluginID = editor_plugin_register ();

   roadmap_layer_adjust ();
}


void editor_main_set (int status) {

   if (status && EditorEnabled) {
      return;
   } else if (!status && !EditorEnabled) {
      return;
   }

   EditorEnabled = status;

   editor_screen_set (status);
}



