/* roadmap_main.c - The main function of the RoadMap application.
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
 *   int main (int argc, char **argv);
 */

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "roadmap.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_gtkcanvas.h"
#include "roadmap_gtkmain.h"

#include "roadmap_main.h"


static int RoadMapMainInputFile = -1;

static char *RoadMapMainTitle = NULL;

static RoadMapCallback RoadMapMainPeriodicCall = NULL;
static guint RoadMapMainPeriodicId;

static RoadMapKeyInput RoadMapMainInput = NULL;
static GtkWidget      *RoadMapMainWindow  = NULL;
static GtkWidget      *RoadMapMainBox     = NULL;
static GtkWidget      *RoadMapMainMenuBar = NULL;
static GtkWidget      *RoadMapCurrentMenu = NULL;
static GtkWidget      *RoadMapMainToolbar = NULL;
static GtkWidget      *RoadMapMainStatus  = NULL;


static void roadmap_main_close (GtkWidget *widget, gpointer data) {

   roadmap_main_exit ();
}


static void roadmap_main_activate (GtkWidget *widget, gpointer data) {

   if (data != NULL) {
      (* (RoadMapCallback) data) ();
   }
}


static gint roadmap_main_key_pressed (GtkWidget *w, GdkEventKey *event) {

   char *key = NULL;
   char regular_key[2];


   switch (event->keyval) {

      case GDK_Left:   key = "Button-Left";           break;
      case GDK_Right:  key = "Button-Right";          break;
      case GDK_Up:     key = "Button-Up";             break;
      case GDK_Down:   key = "Button-Down";           break;

      /* These binding are for the iPAQ buttons: */
      case 0x1008ff1a: key = "Button-Menu";           break;
      case 0x1008ff20: key = "Button-Calendar";       break;
      case 0xaf9:      key = "Button-Contact";        break;
      case 0xff67:     key = "Button-Start";          break;

      /* Regular keyboard keys: */
      default:

         if (event->keyval > 0 && event->keyval < 128) {

            regular_key[0] = event->keyval;
            regular_key[1] = 0;
            key = regular_key;
         }
         break;
   }

   if (key != NULL && RoadMapMainInput != NULL) {
      (*RoadMapMainInput) (key);
   }

   return FALSE;
}


void roadmap_main_set_window_size (GtkWidget *w, int width, int height) {

   int screen_width  = gdk_screen_width();
   int screen_height = gdk_screen_height();

   if (screen_width <= width - 10 || screen_height <= height - 40) {

      /* Small screen: take it all (almost: keep room for the wm). */

      gtk_window_resize (GTK_WINDOW(w), screen_width-10, screen_height-40);

   } else {

      gtk_window_resize (GTK_WINDOW(w), width, height);
   }  
}


void roadmap_main_toggle_full_screen (void) {

   static int RoadMapIsFullScreen = 0;

   if (RoadMapIsFullScreen) {
      gtk_window_unfullscreen (GTK_WINDOW(RoadMapMainWindow));
      RoadMapIsFullScreen = 0;
   } else {
      gtk_window_fullscreen (GTK_WINDOW(RoadMapMainWindow));
      RoadMapIsFullScreen = 1;
   }
}

void roadmap_main_new (const char *title, int width, int height) {

   if (RoadMapMainBox == NULL) {

      RoadMapMainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_widget_set_events (RoadMapMainWindow, GDK_KEY_PRESS_MASK);

      roadmap_main_set_window_size (RoadMapMainWindow, width, height);

      g_signal_connect (RoadMapMainWindow, "destroy",
                        (GCallback)gtk_widget_destroyed,
                        RoadMapMainWindow);

      g_signal_connect (RoadMapMainWindow, "delete_event",
                        (GCallback)roadmap_main_close,
                        NULL);

      g_signal_connect (RoadMapMainWindow, "key_press_event",
                        (GCallback)roadmap_main_key_pressed,
                        NULL);

      RoadMapMainBox = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER(RoadMapMainWindow), RoadMapMainBox);
   }

   gtk_window_set_title (GTK_WINDOW(RoadMapMainWindow), title);

   if (RoadMapMainTitle != NULL) {
      free(RoadMapMainTitle);
   }
   RoadMapMainTitle = strdup (title);
}


void roadmap_main_set_keyboard (RoadMapKeyInput callback) {
   RoadMapMainInput = callback;
}


void roadmap_main_add_menu (const char *label) {

   GtkWidget *menu_item;

   if (RoadMapMainMenuBar == NULL) {

      RoadMapMainMenuBar = gtk_menu_bar_new();

      gtk_box_pack_start
         (GTK_BOX(RoadMapMainBox), RoadMapMainMenuBar, FALSE, TRUE, 0);
   }

   menu_item = gtk_menu_item_new_with_label (label);
   gtk_menu_shell_append (GTK_MENU_SHELL(RoadMapMainMenuBar), menu_item);

   RoadMapCurrentMenu = gtk_menu_new ();
   gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_item), RoadMapCurrentMenu);
}


void roadmap_main_add_menu_item (const char *label,
                                 const char *tip,
                                 RoadMapCallback callback) {

   GtkWidget *menu_item;

   if (RoadMapCurrentMenu == NULL) {
      roadmap_log (ROADMAP_FATAL, "No menu defined for menu item %s", label);
   }

   if (label != NULL) {

      menu_item = gtk_menu_item_new_with_label (label);
      g_signal_connect (menu_item, "activate",
                        (GCallback)roadmap_main_activate,
                        callback);
   } else {
      menu_item = gtk_menu_item_new ();
   }
   gtk_menu_shell_append (GTK_MENU_SHELL(RoadMapCurrentMenu), menu_item);

   if (tip != NULL) {
      gtk_tooltips_set_tip (gtk_tooltips_new (), menu_item, tip, NULL);
   }
}


void roadmap_main_add_separator (void) {

   roadmap_main_add_menu_item (NULL, NULL, NULL);
}


void roadmap_main_add_tool (const char *label,
                            const char *tip,
                            RoadMapCallback callback) {

   if (RoadMapMainToolbar == NULL) {

      RoadMapMainToolbar = gtk_toolbar_new ();
      gtk_box_pack_start (GTK_BOX(RoadMapMainBox),
                          RoadMapMainToolbar, FALSE, FALSE, 0);
   }

   gtk_toolbar_append_item (GTK_TOOLBAR(RoadMapMainToolbar),
                            label, tip, NULL, NULL,
                            (GtkSignalFunc) roadmap_main_activate, callback);
}


void roadmap_main_add_tool_space (void) {

   if (RoadMapMainToolbar == NULL) {
      roadmap_log (ROADMAP_FATAL, "Invalid toolbar space: no toolbar yet");
   }

   gtk_toolbar_append_space (GTK_TOOLBAR(RoadMapMainToolbar));
}


void roadmap_main_add_canvas (void) {

   gtk_box_pack_start (GTK_BOX(RoadMapMainBox),
                       roadmap_canvas_new (), TRUE, TRUE, 2);
}


void roadmap_main_add_status (void) {

   RoadMapMainStatus = gtk_entry_new ();

   gtk_editable_set_editable (GTK_EDITABLE(RoadMapMainStatus), FALSE);
   gtk_entry_set_text (GTK_ENTRY(RoadMapMainStatus), "Initializing..");

   gtk_box_pack_end (GTK_BOX(RoadMapMainBox),
                     RoadMapMainStatus, FALSE, FALSE, 0);
}


void roadmap_main_show (void) {

   if (RoadMapMainWindow != NULL) {
      gtk_widget_show_all (RoadMapMainWindow);
   }
}


static void roadmap_main_input
               (gpointer data, gint source, GdkInputCondition conditions) {

   if (data != NULL) {
      (* (RoadMapInput)data) (source);
   }
}


void roadmap_main_set_input (int fd, RoadMapInput callback) {

   if (RoadMapMainInputFile >= 0) {
      roadmap_main_remove_input (RoadMapMainInputFile);
   }

   RoadMapMainInputFile =
      gtk_input_add_full (fd, GDK_INPUT_READ, roadmap_main_input,
                          NULL, callback, NULL);
}


void roadmap_main_remove_input (int fd) {

   gtk_input_remove (RoadMapMainInputFile);
}


static gboolean roadmap_main_timeout (gpointer data) {

   RoadMapCallback callback = (RoadMapCallback) data;

   if (callback != NULL) {
      (*callback) ();
   }
   return TRUE;
}

void roadmap_main_set_periodic (int interval, RoadMapCallback callback) {

   RoadMapMainPeriodicCall = callback;
   RoadMapMainPeriodicId =
      gtk_timeout_add (interval, roadmap_main_timeout, callback);
}


void roadmap_main_remove_periodic (RoadMapCallback callback) {

   if (RoadMapMainPeriodicCall != callback) {
      roadmap_log (ROADMAP_ERROR,
                   "no support for multiple periodic callbacks");
      return;
   }
   gtk_timeout_remove (RoadMapMainPeriodicId);
   RoadMapMainPeriodicId = 0;
}


void roadmap_main_set_status (const char *text) {

   if (RoadMapMainStatus != NULL) {
      gtk_entry_set_text (GTK_ENTRY(RoadMapMainStatus), text);
   }
}


void roadmap_main_exit (void) {

   roadmap_start_exit ();
   gtk_main_quit();
}


const char *roadmap_main_get_title (const char *name) {

   static char *RoadMapMainTitleBuffer = NULL;

   if (RoadMapMainTitle != NULL) {

      int length = strlen(RoadMapMainTitle) + strlen(name) + 4;

      if (RoadMapMainTitleBuffer != NULL) {
         free(RoadMapMainTitleBuffer);
      }
      RoadMapMainTitleBuffer = malloc (length);

      if (RoadMapMainTitleBuffer != NULL) {
         strcpy (RoadMapMainTitleBuffer, RoadMapMainTitle);
         strcat (RoadMapMainTitleBuffer, " ");
         strcat (RoadMapMainTitleBuffer, name);
      }
      return RoadMapMainTitleBuffer;
   }
   return name;
}


int main (int argc, char **argv) {

   gtk_init (&argc, &argv);

   roadmap_start (argc, argv);

   gtk_main();

   return 0;
}

