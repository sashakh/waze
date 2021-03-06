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

#ifdef ROADMAP_USES_GPE
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <libdisplaymigration/displaymigration.h>
#endif

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_gtkcanvas.h"
#include "roadmap_gtkmain.h"
#include "../editor/editor_main.h"

#include "roadmap_main.h"


struct roadmap_main_io {
   int id;
   RoadMapIO io;
   RoadMapInput callback;
   time_t start_time;
};

#define ROADMAP_MAX_IO 16
static struct roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO];


struct roadmap_main_timer {
   guint id;
   RoadMapCallback callback;
};

#define ROADMAP_MAX_TIMER 32
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];


static char *RoadMapMainTitle = NULL;

static RoadMapKeyInput RoadMapMainInput = NULL;
static GtkWidget      *RoadMapMainWindow  = NULL;
static GtkWidget      *RoadMapMainBox     = NULL;
static GtkWidget      *RoadMapMainMenuBar = NULL;
static GtkWidget      *RoadMapMainToolbar = NULL;
static GtkWidget      *RoadMapMainStatus  = NULL;


static int GtkIconsInitialized = 0;

int USING_PHONE_KEYPAD = 0;


static void roadmap_start_event (int event) {
   switch (event) {
   case ROADMAP_START_INIT:
#ifdef FREEMAP_IL
      editor_main_check_map ();
#endif
      break;
   }
}


#ifdef ROADMAP_USES_GPE

static struct gpe_icon RoadMapGpeIcons[] = {
   {"rm_destination", NULL},
   {"rm_location", NULL},
   {"rm_gps", NULL},
   {"rm_hold", NULL},
   {"rm_counterclockwise", NULL},
   {"rm_clockwise", NULL},
   {"rm_zoomin", NULL},
   {"rm_zoomout", NULL},
   {"rm_zoom1", NULL},
   {"rm_up", NULL},
   {"rm_left", NULL},
   {"rm_right", NULL},
   {"rm_down", NULL},
   {"rm_full", NULL},
   {NULL, NULL}
};


static GtkWidget *roadmap_main_toolbar_icon (const char *icon) {

   if (icon == NULL) return NULL;

   if (! GtkIconsInitialized) {
      if (gpe_load_icons (RoadMapGpeIcons) == FALSE) return NULL;
      GtkIconsInitialized = 1;
   }
   return gpe_render_icon (RoadMapMainWindow->style, gpe_find_icon (icon));
}

#else // Not GPE, i.e. standard GTK.

static GtkWidget *roadmap_main_toolbar_icon (const char *icon) {

   if (icon != NULL) {

      const char *icon_file = roadmap_path_search_icon (icon);

      if (icon_file != NULL) {
         GtkIconsInitialized = 1;
         return gtk_image_new_from_file (icon_file);
      }
   }
   return NULL;
}
#endif // ROADMAP_USES_GPE


static void roadmap_main_close (GtkWidget *widget,
                                GdkEvent *event,
                                gpointer data) {

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

      case GDK_Return: key = "Enter";                 break;

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

#ifdef ROADMAP_USES_GPE
      displaymigration_mark_window (RoadMapMainWindow);
#endif

      gtk_widget_set_events (RoadMapMainWindow, GDK_KEY_PRESS_MASK);

      roadmap_main_set_window_size (RoadMapMainWindow, width, height);

      g_signal_connect (RoadMapMainWindow, "destroy_event",
                        (GCallback)roadmap_main_close,
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
#ifdef FREEMAP_IL
   editor_main_set (1);
#endif
}


void roadmap_main_set_keyboard (struct RoadMapFactoryKeyMap *bindings,
                                RoadMapKeyInput callback) {
   RoadMapMainInput = callback;
}


#ifndef NO_MENU
RoadMapMenu roadmap_main_new_menu (void) {

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
   gtk_menu_shell_append (GTK_MENU_SHELL(RoadMapMainMenuBar), menu_item);

   gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_item), (GtkWidget *) menu);
}


void roadmap_main_add_menu_item (RoadMapMenu menu,
                                 const char *label,
                                 const char *tip,
                                 RoadMapCallback callback) {

   GtkWidget *menu_item;

   if (label != NULL) {

      menu_item = gtk_menu_item_new_with_label (label);
      g_signal_connect (menu_item, "activate",
                        (GCallback)roadmap_main_activate,
                        callback);
   } else {
      menu_item = gtk_menu_item_new ();
   }
   gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);
   
   gtk_widget_show(menu_item);
   
   if (tip != NULL) {
      gtk_tooltips_set_tip (gtk_tooltips_new (), menu_item, tip, NULL);
   }
}
#else

RoadMapMenu roadmap_main_new_menu(void)
{
	return NULL;
}

void roadmap_main_free_menu(RoadMapMenu menu)
{
}

void roadmap_main_add_menu(RoadMapMenu menu, const char *label)
{
}

void roadmap_main_add_menu_item(RoadMapMenu menu, const char *label,
				const char *tip, RoadMapCallback callback)
{
}
#endif

void roadmap_main_popup_menu (RoadMapMenu menu, int x, int y) {

   gtk_menu_popup (GTK_MENU(menu),
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   0,
                   gtk_get_current_event_time());

}


void roadmap_main_add_separator (RoadMapMenu menu) {

   roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
}


void roadmap_main_add_tool (const char *label,
                            const char *icon,
                            const char *tip,
                            RoadMapCallback callback) {

   if (RoadMapMainToolbar == NULL) {

      RoadMapMainToolbar = gtk_toolbar_new ();
      gtk_box_pack_start (GTK_BOX(RoadMapMainBox),
                          RoadMapMainToolbar, FALSE, FALSE, 0);
   }

   gtk_toolbar_append_item (GTK_TOOLBAR(RoadMapMainToolbar),
                            label, tip, NULL,
                            roadmap_main_toolbar_icon (icon),
                            (GtkSignalFunc) roadmap_main_activate, callback);

   if (gdk_screen_height() < 550)
   {
      /* When using a small screen, we want either the labels or the icons,
       * but not both (small screens are typical with PDAs).
       */
      gtk_toolbar_set_style
         (GTK_TOOLBAR(RoadMapMainToolbar),
          GtkIconsInitialized?GTK_TOOLBAR_ICONS:GTK_TOOLBAR_TEXT);
   }
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


static gboolean io_handler(GIOChannel *source, GIOCondition condition, gpointer data)
{
	if (data != NULL) {
		struct roadmap_main_io *cxt = (struct roadmap_main_io *) data;
		(* cxt->callback) (&cxt->io);
	}

	return TRUE;
}

static guint add_io_handler(int fd, GIOCondition condition, void * data)
{
	GIOChannel *channel;
	guint id;

	channel =  g_io_channel_unix_new(fd);
	if (!channel) {
		roadmap_log(ROADMAP_ERROR, "%s: Cannot create %s io channel",
			    __func__,
			    condition == G_IO_IN ? "input" : "output");
		return -1;
	}

	id = g_io_add_watch_full(channel, 0, condition, io_handler, data, NULL);

	return id;
}

static void set_io_handler(RoadMapIO *io, GIOCondition condition, RoadMapInput callback)
{
   int i;
   int fd = io->os.file; /* All the same on UNIX. */

   if (io->subsystem == ROADMAP_IO_NET)
	fd = roadmap_net_get_fd(io->os.socket);

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.subsystem == ROADMAP_IO_INVALID) {
	 io->data = &RoadMapMainIo[i];
         RoadMapMainIo[i].io = *io;
         RoadMapMainIo[i].callback = callback;
	 RoadMapMainIo[i].start_time = condition == G_IO_OUT ? time(NULL) : 0;
         RoadMapMainIo[i].id = add_io_handler(fd, condition, &RoadMapMainIo[i]);
         break;
      }
   }
}

void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback)
{
	set_io_handler(io, G_IO_IN, callback);
}

void roadmap_main_remove_input (RoadMapIO *io)
{
	struct roadmap_main_io *r = io->data;

	g_source_remove(r->id);
	r->io.os.file = -1;
	r->io.subsystem = ROADMAP_IO_INVALID;
}

void roadmap_main_set_output(RoadMapIO *io, RoadMapInput callback, BOOL is_connected)
{
	set_io_handler(io, G_IO_OUT, callback);
}

RoadMapIO *roadmap_main_output_timedout(time_t timeout)
{
	int i;

	for (i = 0; i < ROADMAP_MAX_IO; ++i)
		if (RoadMapMainIo[i].io.subsystem != ROADMAP_IO_INVALID &&
		    RoadMapMainIo[i].start_time &&
		    timeout > RoadMapMainIo[i].start_time)
			return &RoadMapMainIo[i].io;

	return NULL;
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

   timer->id = g_timeout_add(interval, roadmap_main_timeout, callback);
   timer->callback = callback;
}

void roadmap_main_remove_periodic (RoadMapCallback callback)
{
   int index;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {

         RoadMapMainPeriodicTimer[index].callback = NULL;
         g_source_remove(RoadMapMainPeriodicTimer[index].id);

         return;
      }
   }

   roadmap_log (ROADMAP_ERROR, "timer 0x%08x not found", callback);
}

static void on_auto_hide_dialog_close( int exit_code, void* context )
{
}

void roadmap_gui_minimize( void )
{
	//FreeMapNativeManager_MinimizeApplication( -1 );
}

void roadmap_gui_maximize( void )
{
	//FreeMapNativeManager_MaximizeApplication();
}

void roadmap_main_minimize( void )
{
   auto_hide_dlg(on_auto_hide_dialog_close);
}

BOOL roadmap_horizontal_screen_orientation()
{
	//return roadmap_canvas_is_landscape();
	return TRUE;
}

void roadmap_main_set_status (const char *text) {

   if (RoadMapMainStatus != NULL) {
      gtk_entry_set_text (GTK_ENTRY(RoadMapMainStatus), text);
   }
}


void roadmap_main_flush (void) {

   while (gtk_events_pending ()) {
      if (gtk_main_iteration ()) {
         exit(0);  /* gtk_main_quit() called */
      }
   }
}


void roadmap_main_exit (void) {

   static int exit_done;

   if (!exit_done++) {
      roadmap_start_exit ();
      gtk_main_quit();
   }
}

void roadmap_main_set_cursor (int cursor) {}

int main (int argc, char **argv) {

   int i;

#ifdef ROADMAP_USES_GPE
   if (! gpe_application_init (&argc, &argv)) {
      exit (1);
   }
   displaymigration_init ();
#else
   gtk_init (&argc, &argv);
#endif

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      RoadMapMainIo[i].io.os.file = -1;
      RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
   }

   roadmap_start_subscribe (roadmap_start_event);
   roadmap_start (argc, argv);

   gtk_main();

   return 0;
}

