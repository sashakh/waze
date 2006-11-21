/* ssd_entry.c - entry widget
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
 *   See ssd_entry.h.
 */

#include <string.h>
#include <stdlib.h>

#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_text.h"
#include "ssd_button.h"
#include "ssd_keyboard.h"

#include "ssd_entry.h"


static int keyboard_callback (int type, const char *new_value, void *context) {
   if (type == SSD_KEYBOARD_OK) {
      ((SsdWidget)context)->set_value ((SsdWidget)context, new_value);
      ssd_keyboard_hide (SSD_KEYBOARD_LETTERS);
   }
   return 1;
}


static int edit_callback (SsdWidget widget, const char *new_value) {

   const char *value;

   /* Get the entry widget */
   widget = widget->parent;

   value = widget->get_value (widget);

   ssd_keyboard_show (SSD_KEYBOARD_LETTERS, "", value, keyboard_callback,
                        widget);

   return 1;
}


static int set_value (SsdWidget widget, const char *value) {
   return ssd_widget_set_value (widget, "Text", value);
}


const char *get_value (SsdWidget widget) {
   return ssd_widget_get_value (widget, "Text");
}


SsdWidget ssd_entry_new (const char *name,
                         const char *value,
                         int flags) {

   const char *edit_button[] = {"edit"};

   SsdWidget entry =
      ssd_container_new (name, NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, flags);

   SsdWidget text_box =
      ssd_container_new ("text_box", NULL, SSD_MIN_SIZE,
                         SSD_MIN_SIZE, SSD_CONTAINER_BORDER|SSD_ALIGN_VCENTER);

   entry->get_value = get_value;
   entry->set_value = set_value;

   entry->bg_color = NULL;

   text_box->callback = edit_callback;
   text_box->bg_color = NULL;

   ssd_widget_add (text_box, ssd_text_new ("Text", value, -1, 0));
   ssd_widget_add (entry, text_box);
   ssd_widget_add (entry,
         ssd_button_new ("edit_button", "", edit_button, 1,
                         SSD_ALIGN_VCENTER, edit_callback));

   return entry;
}


