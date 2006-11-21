/* ssd_choice.c - combo box widget
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
 *   See ssd_choice.h.
 */

#include <string.h>
#include <stdlib.h>

#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_list.h"
#include "ssd_button.h"
#include "ssd_text.h"

#include "ssd_choice.h"

struct ssd_choice_data {
   SsdCallback callback;
   int num_values;
   const char **labels;
   const void **values;
};


static int list_callback (int type, const char *new_value, void *context) {
   ((SsdWidget)context)->set_value ((SsdWidget)context, new_value);
   ssd_list_hide ();
   return 1;
}


static int choice_callback (SsdWidget widget, const char *new_value) {

   struct ssd_choice_data *data;

   widget = widget->parent;

   data = (struct ssd_choice_data *)widget->data;

   ssd_list_show ("", data->num_values, data->labels,
                  NULL, list_callback, widget);

   return 1;
}


static const char *get_value (SsdWidget widget) {
   return ssd_widget_get_value (widget, "Label");
}


static const void *get_data (SsdWidget widget) {
   struct ssd_choice_data *data = (struct ssd_choice_data *)widget->data;
   const char *value = get_value (widget);
   int i;

   for (i=0; i<data->num_values; i++) {
      if (!strcmp(value, data->labels[i])) break;
   }

   if (i == data->num_values) return NULL;

   return data->values[i];
}


static int set_value (SsdWidget widget, const char *value) {
   struct ssd_choice_data *data = (struct ssd_choice_data *)widget->data;

   if ((data->callback) && !(*data->callback) (widget, value)) {
      return 0;
   }

   return ssd_widget_set_value (widget, "Label", value);
}


static int set_data (SsdWidget widget, const void *value) {
   struct ssd_choice_data *data = (struct ssd_choice_data *)widget->data;
   int i;

   for (i=0; i<data->num_values; i++) {
      if (data->values[i] == value) break;
   }

   if (i == data->num_values) return -1;

   return ssd_widget_set_value (widget, "Label", data->labels[i]);
}


SsdWidget ssd_choice_new (const char *name, int count,
                          const char **labels,
                          const void **values,
                          int flags,
                          SsdCallback callback) {

   const char *edit_button[] = {"edit"};

   struct ssd_choice_data *data =
      (struct ssd_choice_data *)calloc (1, sizeof(*data));

   SsdWidget choice =
      ssd_container_new (name, NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, flags);

   SsdWidget text_box =
      ssd_container_new ("text_box", NULL, SSD_MIN_SIZE,
                         SSD_MIN_SIZE, SSD_CONTAINER_BORDER|SSD_ALIGN_VCENTER);

   data->callback = callback;
   data->num_values = count;
   data->labels = labels;
   data->values = values;

   choice->get_value = get_value;
   choice->get_data = get_data;
   choice->set_value = set_value;
   choice->set_data = set_data;
   choice->data = data;
   choice->bg_color = NULL;

   text_box->callback = choice_callback;
   text_box->bg_color = NULL;


   ssd_widget_add (text_box, ssd_text_new ("Label", labels[0], -1, 0));
   ssd_widget_add (choice, text_box);
   ssd_widget_add (choice,
         ssd_button_new ("edit_button", "", edit_button, 1,
                         SSD_ALIGN_VCENTER, choice_callback));

   return choice;
}


