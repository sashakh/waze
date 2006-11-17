/* ssd_list.c - list view widget
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
 *   See ssd_list.h.
 */

#include <string.h>
#include <stdlib.h>

#include "ssd_dialog.h"
#include "ssd_container.h"
#include "ssd_button.h"
#include "ssd_text.h"

#include "ssd_list.h"

#define MAX_ROWS 50
#define MIN_ROW_HEIGHT 40

struct ssd_list_data {
   int alloc_rows;
   int num_rows;
   SsdWidget *rows;
   SsdCallback callback;
   int num_values;
   const char **labels;
   const void **data;
   int first_row_index;
};


static int setup_rows (SsdWidget list) {
   struct ssd_list_data *data = (struct ssd_list_data *) list->data;
   int current_index = data->first_row_index;
   int i;

   for (i=0; i<data->num_rows; i++) {
      SsdWidget row = data->rows[i];
      const char *label;

      if (current_index == data->num_values) {
         label = "";
      } else {
         label = data->labels[current_index];
         current_index++;
      }

      ssd_widget_set_value (row, "label", label);
      row = row->next;
   }
   return 0;
}


static int button_callback (SsdWidget widget, const char *new_value) {

   struct ssd_list_data *data;
   SsdWidget list = widget;

   /* Find the list main widget */
   while (strcmp(list->name, "list_container") &&
          strcmp(list->name, "scroll_container")) list = list->parent;
   list = list->parent;

   data = (struct ssd_list_data *) list->data;

   if (!strcmp(widget->name, "scroll_up")) {
      data->first_row_index -= data->num_rows;
      if (data->first_row_index < 0) data->first_row_index = 0;
      return setup_rows (list);
   }

   if (!strcmp(widget->name, "scroll_down")) {
      if ((data->first_row_index + data->num_rows) >= data->num_values) {
         return -1;
      }

      data->first_row_index += data->num_rows;
      return setup_rows (list);
   }

   return 0;
}


static int label_callback (SsdWidget widget, const char *new_value) {
   SsdWidget list = widget->parent->parent;
   SsdWidget text = ssd_widget_get (widget, "label");
   struct ssd_list_data *data;

   data = (struct ssd_list_data *) list->data;

   if (!data->callback) return 0;

   return (*data->callback) (list, text->value);
}


static void update_list_rows (SsdWidget list_container,
                              struct ssd_list_data *data) {
   SsdSize size;
   int num_rows;
   int row_height;
   int i;

   ssd_widget_container_size (list_container, &size);

   num_rows = size.height / MIN_ROW_HEIGHT;
   row_height = size.height / num_rows;

   if (data->num_rows == num_rows) return;

   if (num_rows > data->alloc_rows) {
      data->rows = realloc (data->rows, sizeof(SsdWidget) * num_rows);

      for (i=data->alloc_rows; i<num_rows; i++) {
         SsdWidget row = ssd_container_new ("rowx", NULL, SSD_MAX_SIZE,
               row_height,
               SSD_CONTAINER_BORDER|SSD_END_ROW);

         SsdWidget label = ssd_text_new ("label", "", 16, SSD_END_ROW);

         ssd_widget_set_color (row, "#000000", "#efefef");

         ssd_widget_set_callback (row, label_callback);

         ssd_widget_add (row, label);
         ssd_widget_add (list_container, row);

         data->rows[i] = row;
      }

      data->alloc_rows = num_rows;
   }

   for (i=0; i<num_rows; i++) {
      ssd_widget_set_size (data->rows[i], SSD_MAX_SIZE, row_height);
      ssd_widget_show (data->rows[i]);
   }

   for (i=num_rows; i<data->num_rows; i++) {
      ssd_widget_hide (data->rows[i]);
   }

   data->num_rows = num_rows;
}


static void resize (SsdWidget list) {

   struct ssd_list_data *data = (struct ssd_list_data *) list->data;
   SsdWidget scroll_bar = ssd_widget_get (list, "scroll_bar");
   SsdWidget button = ssd_widget_get (list, "scroll_up");
   SsdWidget scroll = ssd_widget_get (list, "scroll_container");
   SsdWidget list_container = ssd_widget_get (list, "list_container");
   SsdSize size;
   SsdSize scroll_size;

   ssd_widget_get_size (button, &size, NULL);
   ssd_widget_container_size (scroll, &scroll_size);

   ssd_widget_set_size (scroll_bar,
                        size.width,
                        scroll_size.height - size.height*2);

   update_list_rows (list_container, data);
}


SsdWidget ssd_list_new (const char *name, int width, int height, int flags) {

   const char *scroll_up_icons[]   = {"up.bmp"};
   const char *scroll_down_icons[] = {"down.bmp"};
   SsdSize   scroll_size;
   SsdSize   button_size;
   SsdWidget list;
   SsdWidget scroll;
   SsdWidget scroll_up;
   SsdWidget scroll_down;
   SsdWidget scroll_bar;
   struct ssd_list_data *data =
      (struct ssd_list_data *)calloc (1, sizeof(*data));

   SsdWidget list_container = ssd_container_new (name, NULL, width, height,
                                                 SSD_CONTAINER_BORDER);

   list_container->data = data;

   scroll_up   = ssd_button_new ("scroll_up", "", scroll_up_icons, 1,
                                 SSD_ALIGN_CENTER|SSD_END_ROW, button_callback);
   scroll_down = ssd_button_new ("scroll_down", "", scroll_down_icons, 1,
                               SSD_ALIGN_CENTER|SSD_END_ROW, button_callback);

   ssd_widget_get_size (scroll_up, &button_size, NULL);

   scroll = ssd_container_new ("scroll_container", NULL,
                               SSD_MIN_SIZE,
                               SSD_MAX_SIZE,
                               SSD_CONTAINER_BORDER|SSD_ALIGN_RIGHT);

   ssd_widget_set_color (scroll, "#000000", "#efefef");

   ssd_widget_add (scroll, scroll_up);

   scroll_bar = ssd_container_new ("scroll_bar", NULL,
                                   button_size.width,
                                   scroll_size.height - button_size.height*2,
                                   SSD_CONTAINER_BORDER|SSD_END_ROW);

   ssd_widget_add (scroll, scroll_bar);
   ssd_widget_add (scroll, scroll_down);

   list = ssd_container_new ("list_container", NULL,
                             SSD_MAX_SIZE,
                             SSD_MAX_SIZE, SSD_CONTAINER_BORDER);

   ssd_widget_set_color (list, "#000000", "#000000");

   ssd_widget_add (list_container, scroll);
   ssd_widget_add (list_container, list);

   return list_container;
}


void ssd_list_populate (SsdWidget list, int count, const char **labels,
                        const void **values, SsdCallback callback) {

   struct ssd_list_data *data = (struct ssd_list_data *) list->data;

   data->num_values = count;
   data->labels = labels;
   data->data = values;
   data->first_row_index = 0;
   data->callback = callback;

   setup_rows (list);
}


/*** Generic list dialog implementation ***/

typedef struct ssd_list_context {
   SsdDialogCB   callback;
   void *context;
} SsdListContext;


static int list_callback (SsdWidget widget, const char *new_value) {
   SsdWidget dialog = widget->parent;
   SsdListContext *context = (SsdListContext *)dialog->context;

   return (*context->callback) (0, new_value, context->context);
}


void ssd_list_show (const char *title, int count, const char **labels,
                    const void **values,
                    SsdDialogCB callback, void *context) {

   static SsdListContext list_context;

   SsdWidget list;
   SsdWidget dialog = ssd_dialog_activate ("generic_list", NULL);

   list_context.callback = callback;
   list_context.context = context;

   if (!dialog) {
      dialog = ssd_dialog_new ("generic_list", "",
            SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE);

      list = ssd_list_new ("list", SSD_MAX_SIZE, SSD_MAX_SIZE, 0);

      ssd_widget_add (dialog, list);
      ssd_dialog_activate ("generic_list", NULL);
   }

   dialog->set_value (dialog, title);
   ssd_widget_set_context (dialog, &list_context);

   list = ssd_widget_get (dialog, "list");
   ssd_widget_reset_cache (list->parent);
   resize (list);

   ssd_list_populate (list, count, labels, NULL, list_callback);

   ssd_dialog_draw ();
}


void ssd_list_hide (void) {
   ssd_dialog_hide ("generic_list");
}

