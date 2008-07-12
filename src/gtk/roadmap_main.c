/*
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
 */

/**
 * @file
 * @brief The main function of the RoadMap application for GTK
 */

/**
 * @defgroup GTK The GTK GUI for RoadMap
 */
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "roadmap.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_gtkcanvas.h"
#include "roadmap_gtkmain.h"
#include "roadmap_progress.h"

#include "roadmap_main.h"
#include "roadmap_time.h"


struct roadmap_main_io {
   int id;
   RoadMapIO io;
   RoadMapInput callback;
};

#define ROADMAP_MAX_IO 16
static struct roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO];


struct roadmap_main_timer {
   guint id;
   RoadMapCallback callback;
};

#define ROADMAP_MAX_TIMER 16
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];


static char *RoadMapMainTitle = NULL;

static RoadMapKeyInput RoadMapMainInput = NULL;
static GtkWidget      *RoadMapMainWindow  = NULL;
static GtkWidget      *RoadMapMainBox     = NULL;
static GtkWidget      *RoadMapCanvasBox   = NULL;
static GtkWidget      *RoadMapMainMenuBar = NULL;
static GtkWidget      *RoadMapMainToolbar = NULL;
static GtkWidget      *RoadMapMainStatus  = NULL;


static void roadmap_main_close (GtkWidget *widget,
                                GdkEvent *event,
                                gpointer data) {

   roadmap_main_exit ();
}


static void roadmap_main_activate (GtkWidget *widget, gpointer data) {

   if (data != NULL) {
      roadmap_start_do_callback(data);
   }
}


static gint roadmap_main_key_pressed (GtkWidget *w, GdkEventKey *event) {

   char *key = NULL;
   char regular_key[2];


   switch (event->keyval) {

      case GDK_Left:   key = "LeftArrow";           break;
      case GDK_Right:  key = "RightArrow";          break;
      case GDK_Up:     key = "UpArrow";             break;
      case GDK_Down:   key = "DownArrow";           break;

      case GDK_Return: key = "Enter";                 break;

      /* These binding are for the iPAQ buttons: */
      case 0x1008ff1a: key = "Special-Menu";           break;
      case 0x1008ff20: key = "Special-Calendar";       break;
      case 0xaf9:      key = "Special-Contact";        break;
      case 0xff67:     key = "Special-Start";          break;

      case 0xffbe:     key = "F1";          break;
      case 0xffbf:     key = "F2";          break;
      case 0xffc0:     key = "F3";          break;
      case 0xffc1:     key = "F4";          break;
      case 0xffc2:     key = "F5";          break;
      case 0xffc3:     key = "F6";          break;
      case 0xffc4:     key = "F7";          break;
      case 0xffc5:     key = "F8";          break;
      case 0xffc6:     key = "F9";          break;
      case 0xffc7:     key = "F10";         break;
      case 0xffc8:     key = "F11";         break;
      case 0xffc9:     key = "F12";         break;

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
      return TRUE;
   }

   return FALSE;
}


void roadmap_main_set_window_size (GtkWidget *w, int width, int height) {

   int screen_width  = gdk_screen_width();
   int screen_height = gdk_screen_height();

   if (screen_width <= width - 10 || screen_height <= height - 40) {

      /* Small screen: take it all (almost: keep room for the wm). */

      gtk_widget_set_usize (w, screen_width-10, screen_height-40);

   } else {

      gtk_widget_set_usize (w, width, height);
   }  
}


void roadmap_main_toggle_full_screen (void) {
   /* Not implemented in GTK 1.2. */
}


void roadmap_main_new (const char *title, int width, int height) {

   if (RoadMapMainBox == NULL) {

      RoadMapMainWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_widget_set_events (RoadMapMainWindow, GDK_KEY_PRESS_MASK);

      roadmap_main_set_window_size (RoadMapMainWindow, width, height);

      gtk_signal_connect (GTK_OBJECT(RoadMapMainWindow), "destroy_event",
                          GTK_SIGNAL_FUNC(roadmap_main_close),
                          RoadMapMainWindow);

      gtk_signal_connect (GTK_OBJECT(RoadMapMainWindow), "delete_event",
                          GTK_SIGNAL_FUNC(roadmap_main_close),
                          NULL);

      gtk_signal_connect (GTK_OBJECT(RoadMapMainWindow), "key_press_event",
                          GTK_SIGNAL_FUNC(roadmap_main_key_pressed),
                          NULL);

      RoadMapCanvasBox = RoadMapMainBox = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER(RoadMapMainWindow), RoadMapMainBox);
   }

   gtk_window_set_title (GTK_WINDOW(RoadMapMainWindow), title);

   if (RoadMapMainTitle != NULL) {
      free(RoadMapMainTitle);
   }
   RoadMapMainTitle = strdup (title);
}

void roadmap_main_title(char *fmt, ...) {

   char newtitle[200];
   va_list ap;
   int n;

   n = snprintf(newtitle, 200, "%s", RoadMapMainTitle);
   va_start(ap, fmt);
   vsnprintf(&newtitle[n], 200 - n, fmt, ap);
   va_end(ap);

   gtk_window_set_title (GTK_WINDOW(RoadMapMainWindow), newtitle);
}


void roadmap_main_set_keyboard (RoadMapKeyInput callback) {
   RoadMapMainInput = callback;
}


RoadMapMenu roadmap_main_new_menu (const char *title) {

   return (RoadMapMenu) gtk_menu_new ();
}


void roadmap_main_free_menu (RoadMapMenu menu) {

   gtk_widget_destroy ((GtkWidget *)menu);
}

void roadmap_main_add_menu (RoadMapMenu menu, const char *label) {

   GtkWidget *menu_item;

   if (RoadMapMainMenuBar == NULL) {

      RoadMapMainMenuBar = gtk_menu_bar_new();

      gtk_box_pack_start
         (GTK_BOX(RoadMapMainBox), RoadMapMainMenuBar, FALSE, TRUE, 0);
   }

   menu_item = gtk_menu_item_new_with_label (label);
   gtk_menu_bar_append (GTK_MENU_BAR(RoadMapMainMenuBar), menu_item);

   gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_item), (GtkWidget *) menu);
}


void roadmap_main_add_menu_item (RoadMapMenu menu,
                                 const char *label,
                                 const char *tip,
                                 RoadMapCallback callback) {

   GtkWidget *menu_item;

   if (label != NULL) {

      menu_item = gtk_menu_item_new_with_label (label);
      gtk_signal_connect (GTK_OBJECT(menu_item),
                          "activate",
                          GTK_SIGNAL_FUNC(roadmap_main_activate),
                          callback);
   } else {
      menu_item = gtk_menu_item_new ();
   }
   gtk_menu_append (GTK_MENU(menu), menu_item);

   if (tip != NULL) {
      gtk_tooltips_set_tip (gtk_tooltips_new (), menu_item, tip, NULL);
   }
}


void roadmap_main_add_separator (RoadMapMenu menu) {

   roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
}


void roadmap_main_popup_menu (RoadMapMenu menu,
                              const RoadMapGuiPoint *position) {

   if (menu != NULL) {
      gtk_widget_show_all(GTK_WIDGET(menu));
      gtk_menu_popup (GTK_MENU(menu),
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      0,
                      time(NULL));
   }
}


void roadmap_main_add_toolbar (const char *orientation) {

   if (RoadMapMainToolbar == NULL) {

      int on_top = 1;
      int gtk_orientation = GTK_ORIENTATION_HORIZONTAL;

      switch (orientation[0]) {
         case 't':
         case 'T': break; /* This is the default. */

         case 'b':
         case 'B': on_top = 0; break;

         case 'l':
         case 'L': gtk_orientation = GTK_ORIENTATION_VERTICAL; break;

         case 'r':
         case 'R': on_top = 0;
                   gtk_orientation = GTK_ORIENTATION_VERTICAL;
                   break;

         default:roadmap_log (ROADMAP_FATAL,
                       "Invalid toolbar orientation %s", orientation);
      }

      if (gtk_orientation == GTK_ORIENTATION_VERTICAL) {
         /* In this case we need a new box, since the "normal" box
          * is a vertical one and we need an horizontal one.
          */
         RoadMapCanvasBox = gtk_hbox_new (FALSE, 0);
         gtk_container_add (GTK_CONTAINER(RoadMapMainBox), RoadMapCanvasBox);
      }

      RoadMapMainToolbar = gtk_toolbar_new (gtk_orientation, GTK_TOOLBAR_TEXT);

      if (on_top) {
         gtk_box_pack_start (GTK_BOX(RoadMapCanvasBox),
                             RoadMapMainToolbar, FALSE, FALSE, 0);
      } else {
         gtk_box_pack_end   (GTK_BOX(RoadMapCanvasBox),
                             RoadMapMainToolbar, FALSE, FALSE, 0);
      }
   }
}

void roadmap_main_add_tool (const char *label,
                            const char *icon,
                            const char *tip,
                            RoadMapCallback callback) {

   if (RoadMapMainToolbar == NULL) {
      roadmap_main_add_toolbar ("");
   }

   gtk_toolbar_append_item (GTK_TOOLBAR(RoadMapMainToolbar),
                            label, tip, NULL, NULL,
                            roadmap_main_activate, callback);
}


void roadmap_main_add_tool_space (void) {

   if (RoadMapMainToolbar == NULL) {
      roadmap_log (ROADMAP_FATAL, "Invalid toolbar space: no toolbar yet");
   }

   gtk_toolbar_append_space (GTK_TOOLBAR(RoadMapMainToolbar));
}


void roadmap_main_set_cursor (RoadMapCursor newcursor) {

   GdkCursor *cursor = NULL;

   switch (newcursor) {

   case ROADMAP_CURSOR_NORMAL:
      cursor = NULL;
      break;

   case ROADMAP_CURSOR_WAIT:
      cursor = gdk_cursor_new(GDK_WATCH);
      break;

   case ROADMAP_CURSOR_CROSS:
      cursor = gdk_cursor_new(GDK_CROSSHAIR);
      break;

   case ROADMAP_CURSOR_WAIT_WITH_DELAY:
      return;  /* shouldn't happen */
   }

   gdk_window_set_cursor(RoadMapMainWindow->window, cursor);

   if (cursor) {
      gdk_cursor_destroy(cursor);
   }
   gdk_flush();
}

void roadmap_main_add_canvas (void) {

   gtk_box_pack_start (GTK_BOX(RoadMapCanvasBox),
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
      struct roadmap_main_io *context = (struct roadmap_main_io *) data;
      (* context->callback) (&context->io);
   }
}


void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback) {

   int i;
   int fd = io->os.file; /* All the same on UNIX. */

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.subsystem == ROADMAP_IO_INVALID) {
         RoadMapMainIo[i].io = *io;
         RoadMapMainIo[i].callback = callback;
         RoadMapMainIo[i].id =
            gtk_input_add_full (fd, GDK_INPUT_READ, roadmap_main_input,
                                NULL, &RoadMapMainIo[i], NULL);
         break;
      }
   }
}


void roadmap_main_remove_input (RoadMapIO *io) {

   int i;
   int fd = io->os.file; /* All the same on UNIX. */

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.os.file == fd) {
         gtk_input_remove (RoadMapMainIo[i].id);
         RoadMapMainIo[i].io.os.file = -1;
         RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
         break;
      }
   }
}


static gboolean roadmap_main_timeout (gpointer data) {

   RoadMapCallback callback = (RoadMapCallback) data;

   if (callback != NULL) {
      (*callback) ();
   }
   return TRUE;
}

void roadmap_main_set_periodic (int interval, RoadMapCallback callback) {

   int index;
   struct roadmap_main_timer *timer = NULL;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {
         return;
      }
      if (timer == NULL) {
         if (RoadMapMainPeriodicTimer[index].callback == NULL) {
            timer = RoadMapMainPeriodicTimer + index;
         }
      }
   }

   if (timer == NULL) {
      roadmap_log (ROADMAP_FATAL, "Timer table saturated");
   }

   timer->id = gtk_timeout_add (interval, roadmap_main_timeout, callback);
   timer->callback = callback;
}


void roadmap_main_remove_periodic (RoadMapCallback callback) {

   int index;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {

         RoadMapMainPeriodicTimer[index].callback = NULL;
         gtk_timeout_remove (RoadMapMainPeriodicTimer[index].id);

         return;
      }
   }
}


static RoadMapCallback idle_callback;
static int idle_handler_id;

static int roadmap_main_set_idle_function_helper (void *data) {
    if (idle_callback) idle_callback();
    return 1;
}

void roadmap_main_set_idle_function (RoadMapCallback callback) {

   idle_callback = callback;
   idle_handler_id = g_idle_add (roadmap_main_set_idle_function_helper, 0);

}

void roadmap_main_remove_idle_function (void) {

   g_source_remove (idle_handler_id);

}


void roadmap_main_set_status (const char *text) {

   if (RoadMapMainStatus != NULL) {
      gtk_entry_set_text (GTK_ENTRY(RoadMapMainStatus), text);
   }
}


int roadmap_main_flush (void) {

   int hadevent;
   while (gtk_events_pending ()) {
      hadevent = 1;
      if (gtk_main_iteration ()) {
         exit(0);  /* gtk_main_quit() called */
      }
   }
   return hadevent;
}


int roadmap_main_flush_synchronous (int deadline) {

   long start_time, duration;

   start_time = roadmap_time_get_millis();

   roadmap_main_flush();

   gdk_flush();

   duration = roadmap_time_get_millis() - start_time;

   if (duration > deadline) {

      roadmap_log (ROADMAP_DEBUG, "processing flush took %d", duration);

      return 0; /* Busy. */
   }

   return 1; /* Not so busy. */
}


void roadmap_main_exit (void) {

   static int exit_done;

   if (!exit_done++) {
      roadmap_start_exit ();
      gtk_main_quit();
   }
}

void roadmap_signals_init(void);

int main (int argc, char **argv) {

   int i, gtk;

   gtk = gtk_init_check (&argc, &argv);

   roadmap_option (argc, argv, 0, NULL);

   if (!gtk) {
	fprintf(stderr, "%s: cannot open X11 display\n", argv[0]);
	exit(1);
   }

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      RoadMapMainIo[i].io.os.file = -1;
      RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
   }

   roadmap_start (argc, argv);

   atexit(roadmap_main_exit);

   roadmap_signals_init();

   gtk_main();

   return 0;
}

