/* roadmap_factory.c - The menu/toolbar/shortcut factory for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_factory.h
 */

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_main.h"

#include "roadmap_factory.h"


static RoadMapConfigDescriptor RoadMapConfigGeneralToolbar =
                        ROADMAP_CONFIG_ITEM("General", "Toolbar");


const char RoadMapFactorySeparator[] = "--separator--";

static const RoadMapFactory *RoadMapFactoryBindings = NULL;


static void roadmap_factory_keyboard (char *key) {

   const RoadMapFactory *binding;

// printf ("Keystroke: %s\n", key);
   if (RoadMapFactoryBindings == NULL) return;

   for (binding = RoadMapFactoryBindings; binding->name != NULL; ++binding) {

      if (strcmp (binding->name, key) == 0) {
         if (binding->callback != NULL) {
            (*binding->callback) ();
            break;
         }
      }
   }
}

void roadmap_factory (const RoadMapFactory *menu,
                      const RoadMapFactory *toolbar,
                      const RoadMapFactory *shortcuts) {

   char *use_toolbar = roadmap_config_get (&RoadMapConfigGeneralToolbar);


   while (menu->name != NULL) {

      if (menu->callback == NULL) {
         if (menu->name == RoadMapFactorySeparator) {
            roadmap_main_add_separator ();
         } else {
            roadmap_main_add_menu (menu->name);
         }
      } else {
         roadmap_main_add_menu_item (menu->name, menu->tip, menu->callback);
      }

      menu += 1;
   }

   if (strcmp (use_toolbar, "yes") == 0) {

      while (toolbar->name != NULL) {

         if (toolbar->callback == NULL) {
            if (toolbar->name == RoadMapFactorySeparator) {
               roadmap_main_add_tool_space ();
            }
         } else {
            roadmap_main_add_tool
               (toolbar->name, toolbar->tip, toolbar->callback);
         }

         toolbar += 1;
      }
   }

   RoadMapFactoryBindings = shortcuts;

   roadmap_main_set_keyboard (roadmap_factory_keyboard);
}

