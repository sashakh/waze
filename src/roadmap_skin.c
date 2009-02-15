/* roadmap_skin.c - manage skins
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
 * DESCRIPTION:
 *
 */

#include <stdio.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_config.h"
#include "roadmap_screen.h"

#include "roadmap_skin.h"


#define MAX_LISTENERS 16
static RoadMapCallback RoadMapSkinListeners[MAX_LISTENERS] = {NULL};
static const char *CurrentSkin = "default";
static const char *CurrentSubSkin = "day";


static void notify_listeners (void) {
   int i;

   for (i = 0; i < MAX_LISTENERS; ++i) {

      if (RoadMapSkinListeners[i] == NULL) break;

      (RoadMapSkinListeners[i]) ();
   }

}


void roadmap_skin_register (RoadMapCallback listener) {

   int i;

   for (i = 0; i < MAX_LISTENERS; ++i) {
      if (RoadMapSkinListeners[i] == NULL) {
         RoadMapSkinListeners[i] = listener;
         break;
      }
   }
}


void roadmap_skin_set_subskin (const char *sub_skin) {
   const char *base_path = roadmap_path_preferred ("skin");
   char path[255];
   char *skin_path;
   char *subskin_path;

   CurrentSubSkin = sub_skin;

   skin_path = roadmap_path_join (base_path, CurrentSkin);
   subskin_path = roadmap_path_join (skin_path, CurrentSubSkin);

   snprintf (path, sizeof(path), "%s,%s", subskin_path, skin_path);

   roadmap_path_set ("skin", path);

   roadmap_path_free (subskin_path);
   roadmap_path_free (skin_path);

   roadmap_config_reload ("preferences");
   notify_listeners ();

   roadmap_screen_repaint ();
}


void roadmap_skin_toggle (void) {
   if (!strcmp (CurrentSubSkin, "day")) {
      roadmap_skin_set_subskin ("night");
   } else {
      roadmap_skin_set_subskin ("day");
   }
}

