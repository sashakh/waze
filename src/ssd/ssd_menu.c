/* ssd_menu.c - Icons menu
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
 *   See ssd_menu.h.
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "roadmap_factory.h"
#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_text.h"

#include "ssd_menu.h"

static int button_callback (SsdWidget widget, const char *new_value) {

   RoadMapCallback callback = (RoadMapCallback) widget->context;

   (*callback)();
   return 0;
}


static const RoadMapAction *find_action
                              (const RoadMapAction *actions, const char *item) {

   while (actions->name != NULL) {
      if (strcmp (actions->name, item) == 0) return actions;
      ++actions;
   }

   return NULL;
}


static int short_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   if (ssd_widget_short_click (widget->children, point)) {
      ssd_dialog_hide (widget->name);
   }

   return 1;
}


static int long_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   if (ssd_widget_long_click (widget->children, point)) {
      ssd_dialog_hide (widget->name);
   }

   return 1;
}


static void ssd_menu_new (const char           *name,
                          const char           *items[],
                          const RoadMapAction  *actions) {

   int i;
   const char *icons[255] = {0};

   const char **menu_items =
      roadmap_factory_user_config (name, "menu", actions, icons);

   SsdWidget dialog = ssd_dialog_new (name, "תפריט ראשי",
                                      SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE);
   SsdWidget container;

   container = ssd_container_new (name, NULL, SSD_MAX_SIZE, SSD_MAX_SIZE,
                                  SSD_ALIGN_GRID);

   /* Override short and long click */
   container->short_click = short_click;
   container->long_click = long_click;
   
   if (!menu_items) menu_items = items;

   for (i = 0; menu_items[i] != NULL; ++i) {

      const char *item = menu_items[i];

      if (item == RoadMapFactorySeparator) {

      } else {

         SsdWidget text_box;
         const RoadMapAction *this_action = find_action (actions, item);
         const char *button_icon[] = {
            icons[i],
            NULL
         };

         SsdWidget w = ssd_container_new (item, NULL,
                           SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_ALIGN_CENTER);
         SsdWidget button;
         SsdWidget text;

         if (!icons[i]) continue;

         button = ssd_button_new
                    (item, item, button_icon, 1, SSD_ALIGN_CENTER|SSD_END_ROW,
                     button_callback);

         ssd_widget_set_context (button, this_action->callback);
         ssd_widget_add (w, button);

         text_box = ssd_container_new ("text_box", NULL, 60, SSD_MIN_SIZE,
                                       SSD_ALIGN_CENTER|SSD_END_ROW);

         text = ssd_text_new (item,
                              roadmap_lang_get (this_action->label_long),
                              10, /* 60,*/ SSD_ALIGN_CENTER|SSD_END_ROW);
         ssd_widget_add (text_box, text);
         ssd_widget_add (w, text_box);

         ssd_widget_add (container, w);
      }
   }

   ssd_widget_add (dialog, container);
}


void ssd_menu_activate (const char           *name,
                        const char           *items[],
                        const RoadMapAction  *actions) {

   if (ssd_dialog_activate (name, NULL)) {
      ssd_dialog_draw ();
      return;
   }

   ssd_menu_new (name, items, actions);

   ssd_dialog_activate (name, NULL);
   ssd_dialog_draw ();
}

