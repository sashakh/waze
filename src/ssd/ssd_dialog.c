/* ssd_dialog.c - small screen devices Widgets (designed for touchscreens)
 * (requires agg)
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
 *   See ssd_dialog.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_pointer.h"
#include "roadmap_start.h"

#include "ssd_widget.h"
#include "ssd_container.h"
#include "ssd_entry.h"
#include "ssd_button.h"
#include "ssd_dialog.h"

struct ssd_dialog_item;
typedef struct ssd_dialog_item *SsdDialog;

struct ssd_dialog_item {

   struct ssd_dialog_item *next;
   struct ssd_dialog_item *activated_prev;

   char *name;

   void *context;  /* References a caller-specific context. */

   RoadMapCallback callback; /* Called before hide */

   SsdWidget container;
};

static SsdDialog RoadMapDialogWindows = NULL;
static SsdDialog RoadMapDialogCurrent = NULL;

static RoadMapGuiPoint LastPointerPoint;

static int RoadMapScreenFrozen = 0;
static int RoadMapDialogKeyEnabled = 0;

static SsdDialog ssd_dialog_get (const char *name) {

   SsdDialog child;

   child = RoadMapDialogWindows;

   while (child != NULL) {
      if (strcmp (child->name, name) == 0) {
         return child;
      }
      child = child->next;
   }

   return NULL;
}


#if 0
int ssd_dialog_activate (const char *name, void *context) {

   SsdDialog dialog = ssd_dialog_get (NULL, name);

   dialog->context = context;

   if (dialog->w != NULL) {

      /* The dialog exists already: show it on top. */

      RoadMapDialogCurrent = dialog;

      gdk_window_show (dialog->w->window);
      gdk_window_raise (dialog->w->window);
      gtk_widget_show_all (GTK_WIDGET(dialog->w));

      return 0; /* Tell the caller the dialog already exists. */
   }

   /* Create the dialog's window. */

   dialog->w = gtk_dialog_new();
#ifdef ROADMAP_USES_GPE
   displaymigration_mark_window (dialog->w);
#endif
   gtk_window_set_title (GTK_WINDOW(dialog->w), ssd_start_get_title(name));

   gtk_button_box_set_layout
      (GTK_BUTTON_BOX(GTK_DIALOG(dialog->w)->action_area),
       GTK_BUTTONBOX_SPREAD);

   return 1; /* Tell the caller this is a new, undefined, dialog. */
}


void ssd_dialog_hide (const char *name) {

   ssd_dialog_hide_window (ssd_dialog_get (NULL, name));
}


void ssd_dialog_new_entry (const char *frame, const char *name,
                               RoadMapDialogCallback callback) {

   GtkWidget *w = gtk_entry_new ();
   SsdDialog child = ssd_dialog_new_item (frame, name, w, 0);
   child->callback = callback;
   child->widget_type = SSD_WIDGET_ENTRY;

   g_signal_connect (w, "activate",
                     (GCallback) ssd_dialog_action, child);

}


void ssd_dialog_new_mul_entry (const char *frame, const char *name,
                                   RoadMapDialogCallback callback) {

   GtkWidget *w = gtk_text_view_new ();
   SsdDialog child = ssd_dialog_new_item (frame, name, w, 0);
   child->callback = callback;
   child->widget_type = SSD_WIDGET_MUL_ENTRY;
}


void ssd_dialog_new_progress (const char *frame, const char *name) {

   name = "Progress";
   GtkWidget *w = gtk_label_new (name);
   SsdDialog child = ssd_dialog_new_item (frame, name, w, 0);
   child->widget_type = SSD_WIDGET_LABEL;
}


void ssd_dialog_new_image (const char *frame, const char *name) {}


void ssd_dialog_new_password (const char *frame, const char *name) {

   GtkWidget *w = gtk_entry_new ();
   gtk_entry_set_visibility(GTK_ENTRY(w), FALSE);
   SsdDialog child = ssd_dialog_new_item (frame, name, w, 0);
   child->widget_type = SSD_WIDGET_PASSWORD;
}


void ssd_dialog_new_label (const char *frame, const char *name) {

   GtkWidget *w = gtk_label_new (name);
   SsdDialog child = ssd_dialog_new_item (frame, name, w, 0);

   child->widget_type = SSD_WIDGET_LABEL;
}


void ssd_dialog_new_color (const char *frame, const char *name) {

   ssd_dialog_new_entry (frame, name, NULL);
}


void ssd_dialog_new_choice (const char *frame,
                                const char *name,
                                int count,
                                const char **labels,
                                void **values,
                                RoadMapDialogCallback callback) {

   int i;
   GtkWidget *w = gtk_option_menu_new ();
   SsdDialog child = ssd_dialog_new_item (frame, name, w, 0);
   GtkWidget *menu;
   GtkWidget *menu_item;
   RoadMapDialogSelection *choice;

   child->widget_type = SSD_WIDGET_CHOICE;

   if (labels == values) {
      child->data_is_string = 1;
   }

   menu = gtk_menu_new ();

   choice = (RoadMapDialogSelection *) calloc (count, sizeof(*choice));
   roadmap_check_allocated(choice);

   for (i = 0; i < count; ++i) {

      choice[i].typeid = "RoadMapDialogSelection";
      choice[i].item = child;
      choice[i].value = values[i];
      choice[i].callback = callback;

      menu_item = gtk_menu_item_new_with_label (labels[i]);
      if (child->data_is_string) {
         GtkWidget *menu_label = gtk_bin_get_child(GTK_BIN(menu_item));
         choice[i].value = gtk_label_get_text(GTK_LABEL(menu_label));
      }

      gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);

      g_signal_connect_swapped
                  (menu_item,
                   "activate",
                   (GCallback) ssd_dialog_chosen,
                   (gpointer) (choice+i));

      gtk_widget_show (menu_item);
   }
   gtk_option_menu_set_menu (GTK_OPTION_MENU(w), menu);

   if (child->choice != NULL) {
      free(child->choice);
   }
   child->choice = choice;
   child->num_choices = count;
   child->value  = choice[0].value;

}


void ssd_dialog_new_list (const char  *frame, const char  *name) {

   GtkWidget         *listbox;
   GtkListStore      *store;
   GtkTreeViewColumn *column;


   GtkWidget *scrollbox = gtk_scrolled_window_new (NULL, NULL);

   SsdDialog child =
      ssd_dialog_new_item (frame, name, scrollbox, 1);

   store = gtk_list_store_new (RM_LIST_WAYPOINT_COLUMNS, G_TYPE_STRING);
   listbox = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   g_object_unref (G_OBJECT (store));

   // gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(listbox), 0);

   gtk_scrolled_window_add_with_viewport
         (GTK_SCROLLED_WINDOW(scrollbox), listbox);

   child->w = listbox;
   child->widget_type = SSD_WIDGET_LIST;

   if (name[0] == '.') name += 1;

   column = gtk_tree_view_column_new_with_attributes
                (name, gtk_cell_renderer_text_new (),
                 "text", RM_LIST_WAYPOINT_NAME,
                 NULL);

   gtk_tree_view_append_column (GTK_TREE_VIEW (listbox), column);

   gtk_tree_selection_set_mode
       (gtk_tree_view_get_selection (GTK_TREE_VIEW (listbox)),
        GTK_SELECTION_SINGLE);
}


static int listview_sort_cmp (const void *a, const void *b) {

   RoadMapDialogSelection *c1 = (RoadMapDialogSelection *)a;
   RoadMapDialogSelection *c2 = (RoadMapDialogSelection *)b;

   return strcmp (c1->value, c2->value);
}


void ssd_dialog_show_list (const char  *frame,
                               const char  *name,
                               int    count,
                               char **labels,
                               void **values,
                               RoadMapDialogCallback callback) {

   int i;
   SsdDialog parent;
   SsdDialog child;
   RoadMapDialogSelection *choice;
   char *empty_list[1] = {""};

   GtkTreeModel *model;
   GtkTreeIter   iterator;

   parent = ssd_dialog_get (RoadMapDialogCurrent, frame);
   if (parent->w == NULL) {
      roadmap_log (ROADMAP_ERROR,
                   "list %s in dialog %s filled before built", name, frame);
      return;
   }

   child  = ssd_dialog_get (parent, name);
   if (child->w == NULL) {
      roadmap_log (ROADMAP_ERROR,
                   "list %s in dialog %s filled before finished", name, frame);
      return;
   }
   model = gtk_tree_view_get_model (GTK_TREE_VIEW(child->w));

   if (child->choice != NULL) {
      gtk_list_store_clear (GTK_LIST_STORE(model));
      free (child->choice);
      child->choice = NULL;
   }

   if (!count) {
      count = 1;
      labels = values = empty_list;
   }

   choice = (RoadMapDialogSelection *) calloc (count, sizeof(*choice));
   roadmap_check_allocated(choice);

   for (i = 0; i < count; ++i) {

      choice[i].typeid = "RoadMapDialogSelection";
      choice[i].item = child;
      choice[i].value = values[i];
      choice[i].callback = callback;
   }
   
   qsort(choice, count, sizeof(*choice), listview_sort_cmp);

   gtk_tree_selection_set_select_function
       (gtk_tree_view_get_selection (GTK_TREE_VIEW (child->w)),
        ssd_dialog_list_selected,
        (gpointer)choice,
        NULL);

   for (i = 0; i < count; ++i) {

      gtk_list_store_append (GTK_LIST_STORE(model), &iterator);
      gtk_list_store_set (GTK_LIST_STORE(model), &iterator,
                          RM_LIST_WAYPOINT_NAME, choice[i].value,
                          -1);
      if (i == 0) {
         gtk_tree_selection_select_iter
            (gtk_tree_view_get_selection(GTK_TREE_VIEW (child->w)), &iterator);
      }
   }
   
   child->choice = choice;
   child->num_choices = count;
   child->value  = choice[0].value;

   gtk_widget_show (parent->w);
}


void ssd_dialog_add_button (const char *label,
                                RoadMapDialogCallback callback) {

   SsdDialog dialog = RoadMapDialogCurrent;
   SsdDialog child;

   GtkWidget *button = gtk_button_new_with_label (label);

   child = ssd_dialog_get (dialog, label);

   child->w = button;
   child->callback = callback;
   child->widget_type = SSD_WIDGET_BUTTON;

   g_signal_connect (button, "clicked",
                     (GCallback) ssd_dialog_action, child);

   GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

   gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->action_area),
                       button, TRUE, FALSE, 0);

   gtk_widget_grab_default (button);
}


void ssd_dialog_complete (int use_keyboard) {

   int count;
   SsdDialog dialog = RoadMapDialogCurrent;
   SsdDialog frame;


   count = 0;

   for (frame = dialog->children; frame != NULL; frame = frame->next) {
      if (frame->widget_type == SSD_WIDGET_CONTAINER) {
         count += 1;
      }
   }

   if (count > 1) {

      /* There are several frames in that dialog: use a notebook widget
       * to let the user access all of them.
       */
      GtkWidget *notebook = gtk_notebook_new();

      gtk_notebook_set_scrollable (GTK_NOTEBOOK(notebook), TRUE);

      gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->vbox),
                          notebook, TRUE, TRUE, 0);

      for (frame = dialog->children; frame != NULL; frame = frame->next) {

         if (frame->widget_type == SSD_WIDGET_CONTAINER) {

            GtkWidget *label = gtk_label_new (frame->name);

            gtk_notebook_append_page (GTK_NOTEBOOK(notebook), frame->w, label);
         }
      }

   } else if (count == 1) {

      /* There is only one frame in that dialog: show it straight. */

      for (frame = dialog->children; frame != NULL; frame = frame->next) {

         if (frame->widget_type == SSD_WIDGET_CONTAINER) {

            gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->vbox),
                                frame->w, TRUE, TRUE, 0);
            break;
         }
      }

   } else {
      roadmap_log (ROADMAP_FATAL,
                   "no frame defined for dialog %s", dialog->name);
   }

   if (use_keyboard) {

      SsdDialog last_item = NULL;
      RoadMapKeyboard   keyboard  = ssd_keyboard_new ();

      gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->vbox),
                          ssd_keyboard_widget(keyboard),
                          TRUE, TRUE, 0);

      for (frame = dialog->children; frame != NULL; frame = frame->next) {

         if (frame->widget_type == SSD_WIDGET_CONTAINER) {

            SsdDialog item;

            for (item = frame->children; item != NULL; item = item->next) {

                if (item->widget_type == SSD_WIDGET_ENTRY) {

                   g_signal_connect (item->w, "button_press_event",
                                     (GCallback) ssd_dialog_selected,
                                     keyboard);

                   last_item = item;
                }
            }
         }
      }
      if (last_item != NULL) {
         ssd_keyboard_set_focus (keyboard, last_item->w);
      }
   }

   gtk_container_set_border_width
      (GTK_CONTAINER(GTK_BOX(GTK_DIALOG(dialog->w)->vbox)), 4);

   g_signal_connect (dialog->w, "destroy",
                     (GCallback) ssd_dialog_destroyed,
                     dialog);

   gtk_grab_add (dialog->w);

   ssd_main_set_window_size (dialog->w,
                                 ssd_option_width(dialog->name),
                                 ssd_option_height(dialog->name));

   gtk_widget_show_all (GTK_WIDGET(dialog->w));
}


void ssd_dialog_select (const char *dialog) {

   RoadMapDialogCurrent = ssd_dialog_get (NULL, dialog);
}


void ssd_dialog_set_focus (const char *frame, const char *name) {
}


void *ssd_dialog_get_data (const char *frame, const char *name) {

   SsdDialog this_frame;
   SsdDialog this_item;


   this_frame  = ssd_dialog_get (RoadMapDialogCurrent, frame);
   this_item   = ssd_dialog_get (this_frame, name);

   switch (this_item->widget_type) {

   case SSD_WIDGET_PASSWORD:
   case SSD_WIDGET_ENTRY:

      return (void *)gtk_entry_get_text (GTK_ENTRY(this_item->w));

   case SSD_WIDGET_LABEL:
      
      return (void *)gtk_label_get_text (GTK_LABEL(this_item->w));
      
   case SSD_WIDGET_MUL_ENTRY:
      {
         GtkTextIter start, end;
         gtk_text_buffer_get_bounds
            (gtk_text_view_get_buffer (GTK_TEXT_VIEW(this_item->w)),
	     &start, &end );
         return gtk_text_buffer_get_text
            (gtk_text_view_get_buffer (GTK_TEXT_VIEW(this_item->w)),
                                       &start, &end, TRUE);
      }
      break;
   }

   return this_item->value;
}


void  ssd_dialog_set_data (const char *frame, const char *name,
                               const void *data) {

   SsdDialog this_frame;
   SsdDialog this_item;
   int i;


   this_frame  = ssd_dialog_get (RoadMapDialogCurrent, frame);
   this_item   = ssd_dialog_get (this_frame, name);

   switch (this_item->widget_type) {

   case SSD_WIDGET_PASSWORD:
   case SSD_WIDGET_ENTRY:

      gtk_entry_set_text (GTK_ENTRY(this_item->w), (const char *)data);
      break;

   case SSD_WIDGET_MUL_ENTRY:
      {
         GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
         gtk_text_buffer_set_text (buffer, (const char *)data, strlen(data));
         gtk_text_view_set_buffer (GTK_TEXT_VIEW(this_item->w), buffer);
         g_object_unref (buffer);
      }
      break;

   case SSD_WIDGET_LABEL:

      gtk_label_set_text (GTK_LABEL(this_item->w), (const char *)data);
      break;

   case SSD_WIDGET_CHOICE:

      for (i=0; i < this_item->num_choices; i++) {
         if ((data == this_item->choice[i].value) ||
               (this_item->data_is_string &&
                !strcmp (this_item->choice[i].value, data))) {
            gtk_option_menu_set_history (GTK_OPTION_MENU(this_item->w), i);
            break;
         }
      }
      
      if ((i == this_item->num_choices) && this_item->data_is_string) {

         RoadMapDialogSelection *choice;
         GtkMenuItem *item;
         GtkWidget *menu = gtk_option_menu_get_menu
            (GTK_OPTION_MENU(this_item->w));
         GList *glist = gtk_container_get_children (GTK_CONTAINER(menu));
         int count = this_item->num_choices + 1;

         choice = (RoadMapDialogSelection *)
            realloc (this_item->choice, count * sizeof(*choice));
         roadmap_check_allocated(choice);

         for (i = 0; i < count-1; ++i) {

            item = g_list_nth_data (glist, i);

            g_signal_handlers_disconnect_by_func
                                 (item,
                                  (GCallback) ssd_dialog_chosen,
                                  this_item->choice + i);

            g_signal_connect_swapped
               (item,
                "activate",
                (GCallback) ssd_dialog_chosen,
                (gpointer) (choice+i));
         }

         this_item->choice = choice;
         g_list_free (glist);
         item = gtk_menu_item_new_with_label ((const char *)data);
         gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);

         g_signal_connect_swapped
            (item,
             "activate",
             (GCallback) ssd_dialog_chosen,
             (gpointer) (choice+i));
         memcpy (this_item->choice + this_item->num_choices,
                 this_item->choice + this_item->num_choices - 1,
                 sizeof(RoadMapDialogSelection));

         menu = gtk_bin_get_child(GTK_BIN(item));
         this_item->choice[this_item->num_choices].value = 
            gtk_label_get_text(GTK_LABEL(menu));
         this_item->num_choices++;

         gtk_widget_show (item);
         gtk_option_menu_set_history (GTK_OPTION_MENU(this_item->w), i);
      }

      break;
   }
   this_item->value = (char *)data;
}

void  ssd_dialog_set_progress (const char *frame, const char *name,
                                   int progress) {
   SsdDialog this_frame;
   SsdDialog this_item;
   int i;
   char data[100];

   this_frame  = ssd_dialog_get (RoadMapDialogCurrent, frame);
   this_item   = ssd_dialog_get (this_frame, name);

   if (this_item->widget_type != SSD_WIDGET_LABEL) return;

   snprintf(data, sizeof(data), "%d", progress);

   gtk_label_set_text (GTK_LABEL(this_item->w), (const char *)data);
}


/***********************************************************************/
#endif


static void ssd_dialog_enable_callback (void) {
   RoadMapDialogKeyEnabled = 1;
   roadmap_main_remove_periodic (ssd_dialog_enable_callback);
}


static void ssd_dialog_disable_key (void) {
   RoadMapDialogKeyEnabled = 0;
   roadmap_main_set_periodic (350, ssd_dialog_enable_callback);
}


static int ssd_dialog_pressed (RoadMapGuiPoint *point) {
   SsdWidget container = RoadMapDialogCurrent->container;
   if (!ssd_widget_find_by_pos (container, point)) {
      LastPointerPoint.x = -1;
      return 0;
   }

   if (!RoadMapDialogKeyEnabled) {
      LastPointerPoint.x = -1;
      return 1;
   }

   LastPointerPoint = *point;
   ssd_widget_pointer_down (RoadMapDialogCurrent->container, point);
   roadmap_start_redraw ();
   return 1;
}

static int ssd_dialog_short_click (RoadMapGuiPoint *point) {
   if (LastPointerPoint.x < 0) {
      SsdWidget container = RoadMapDialogCurrent->container;
      if (ssd_widget_find_by_pos (container, point)) {
         return 1;
      } else {
         return 0;
      }
   }
   ssd_widget_short_click (RoadMapDialogCurrent->container, &LastPointerPoint);
   roadmap_start_redraw ();

   return 1;
}

static int ssd_dialog_long_click (RoadMapGuiPoint *point) {
   if (!LastPointerPoint.x < 0) {
      SsdWidget container = RoadMapDialogCurrent->container;
      if (ssd_widget_find_by_pos (container, point)) {
         return 1;
      } else {
         return 0;
      }
   }
   ssd_widget_long_click (RoadMapDialogCurrent->container, &LastPointerPoint);
   roadmap_start_redraw ();

   return 1;
}

static void append_child (SsdWidget child) {

   ssd_widget_add (RoadMapDialogWindows->container, child);
}

SsdWidget ssd_dialog_new (const char *name, const char *title, int flags) {

   SsdDialog child;
   int width = SSD_MAX_SIZE;
   int height = SSD_MAX_SIZE;

   child = (SsdDialog) calloc (sizeof (*child), 1);
   roadmap_check_allocated(child);

   child->name = strdup(name);

   if (flags & SSD_DIALOG_FLOAT) {
      width = SSD_MAX_SIZE;
      height = SSD_MIN_SIZE;
   }

   child->container =
      ssd_container_new (name, title, width, height, flags);

   child->next = RoadMapDialogWindows;
   RoadMapDialogWindows = child;

   return child->container;
}


void ssd_dialog_draw (void) {

   if (!RoadMapDialogCurrent) {
      return;

   } else {
      RoadMapGuiRect rect =
         {0, 0, roadmap_canvas_width() - 1, roadmap_canvas_height() - 1};

      ssd_widget_reset_cache (RoadMapDialogCurrent->container);
      ssd_widget_draw (RoadMapDialogCurrent->container, &rect, 0);
      roadmap_canvas_refresh ();
   }
}


void ssd_dialog_new_entry (const char *name, const char *value,
                           int flags, SsdCallback callback) {

   SsdWidget child = ssd_entry_new (name, value, flags);
   append_child (child);

   ssd_widget_set_callback (child, callback);
}


SsdWidget ssd_dialog_new_button (const char *name, const char *value,
                                 const char **bitmaps, int num_bitmaps,
                                 int flags, SsdCallback callback) {

   SsdWidget child =
      ssd_button_new (name, value, bitmaps, num_bitmaps, flags, callback);
   append_child (child);

   return child;
}


void ssd_dialog_new_line (void) {

   SsdWidget last = RoadMapDialogWindows->container->children;

   while (last->next) last=last->next;

   last->flags |= SSD_END_ROW; 
}


SsdWidget ssd_dialog_activate (const char *name, void *context) {

   SsdDialog prev = NULL;
   SsdDialog current = RoadMapDialogCurrent;
   SsdDialog dialog = ssd_dialog_get (name);

   if (!dialog) {
      return NULL; /* Tell the caller this is a new, undefined, dialog. */
   }

   while (current && strcmp(current->name, name)) {
      prev = current;
      current = current->activated_prev;
   }

   if (current) {
      if (prev) {
         prev->activated_prev = current->activated_prev;
         current->activated_prev = RoadMapDialogCurrent;
         RoadMapDialogCurrent = current;
      }
      return current->container;
   }

   dialog->context = context;

   dialog->activated_prev = RoadMapDialogCurrent;

   if (!RoadMapDialogCurrent) {
      /* Grab pointer hooks */
      roadmap_pointer_register_pressed
         (ssd_dialog_pressed, POINTER_HIGHEST);
      roadmap_pointer_register_short_click
         (ssd_dialog_short_click, POINTER_HIGHEST);
      roadmap_pointer_register_long_click
         (ssd_dialog_long_click, POINTER_HIGHEST);
   }

   RoadMapDialogCurrent = dialog;

   if (!RoadMapScreenFrozen && !(dialog->container->flags & SSD_DIALOG_FLOAT)) {
      roadmap_start_screen_refresh (0);
      RoadMapScreenFrozen = 1;
   }

   ssd_dialog_disable_key ();
   return dialog->container; /* Tell the caller the dialog already exists. */
}


void ssd_dialog_hide (const char *name) {

   SsdDialog prev = NULL;
   SsdDialog dialog = RoadMapDialogCurrent;

   while (dialog && strcmp(dialog->name, name)) {
      prev = dialog;
      dialog = dialog->activated_prev;
   }

   if (!dialog) {
      return;
   }

   if (prev == NULL) {
      RoadMapDialogCurrent = RoadMapDialogCurrent->activated_prev;
   } else {
      prev->activated_prev = dialog->activated_prev;
   }

   if (RoadMapDialogCurrent) {
      ssd_dialog_disable_key ();
   } else {
      roadmap_pointer_unregister_pressed     (ssd_dialog_pressed);
      roadmap_pointer_unregister_short_click (ssd_dialog_short_click);
      roadmap_pointer_unregister_long_click  (ssd_dialog_long_click);
   }

   if (dialog->callback) {
      (*dialog->callback) ();
   }

   if (RoadMapScreenFrozen) {
      dialog = RoadMapDialogCurrent;
      while (dialog) {
         if ( !(dialog->container->flags & SSD_DIALOG_FLOAT)) {
            ssd_dialog_draw ();
            return;
         }
         dialog = dialog->activated_prev;
      }
   }

   RoadMapScreenFrozen = 0;
   roadmap_start_screen_refresh (1);
}


void ssd_dialog_hide_current (void) {

   if (!RoadMapDialogCurrent) {
      roadmap_log (ROADMAP_FATAL,
         "Trying to hide a dialog, but no active dialogs exist");
   }

   ssd_dialog_hide (RoadMapDialogCurrent->name);
}


const char *ssd_dialog_get_value (const char *name) {

   return ssd_widget_get_value (RoadMapDialogCurrent->container, name);
}


const void *ssd_dialog_get_data (const char *name) {

   return ssd_widget_get_data (RoadMapDialogCurrent->container, name);
}


int ssd_dialog_set_value (const char *name, const char *value) {

   return ssd_widget_set_value (RoadMapDialogCurrent->container, name, value);
}


int ssd_dialog_set_data  (const char *name, const void *value) {
   return ssd_widget_set_data (RoadMapDialogCurrent->container, name, value);
}


void ssd_dialog_set_callback (RoadMapCallback callback) {

   if (!RoadMapDialogCurrent) {
      roadmap_log (ROADMAP_FATAL,
         "Trying to set a dialog callback, but no active dialogs exist");
   }

   RoadMapDialogCurrent->callback = callback;
}


void *ssd_dialog_context (void) {
   if (!RoadMapDialogCurrent) {
      roadmap_log (ROADMAP_FATAL,
         "Trying to get dialog context, but no active dialogs exist");
   }

   return RoadMapDialogCurrent->context;
}


