/* roadmap_main.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_iphonecanvas.h"
#include "roadmap_iphonemain.h"

#include "roadmap_main.h"
#include "roadmap_time.h"


static RoadMapApp *TheApp;
static int sArgc;
static char ** sArgv;

struct roadmap_main_io {
   NSFileHandle *fh;
   RoadMapIO io;
   RoadMapInput callback;
};

#define ROADMAP_MAX_IO 16
static struct roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO];

struct roadmap_main_timer {
   NSTimer *timer;
   RoadMapCallback callback;
};

#define ROADMAP_MAX_TIMER 16
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];

static RoadMapCallback *idle_callback = NULL;
static char *RoadMapMainTitle = NULL;

static RoadMapKeyInput RoadMapMainInput = NULL;
static UIWindow    *RoadMapMainWindow  = NULL;
static UIView      *RoadMapMainBox     = NULL;
static RoadMapCanvasView *RoadMapCanvasBox   = NULL;
static UIView      *RoadMapMainMenuBar = NULL;
static UIView      *RoadMapMainToolbar = NULL;
static UIView      *RoadMapMainStatus  = NULL;


static int iPhoneIconsInitialized = 0;


static UIView *roadmap_main_toolbar_icon (const char *icon) {

   if (icon != NULL) {

      const char *icon_file = roadmap_path_search_icon (icon);

      if (icon_file != NULL) {
         iPhoneIconsInitialized = 1;
         return NULL; // gtk_image_new_from_file (icon_file);
      }
   }
   return NULL;
}

/*
static void roadmap_main_close (UIView *widget,
                                GdkEvent *event,
                                gpointer data) {

   roadmap_main_exit ();
}


static void roadmap_main_activate (UIView *widget, gpointer data) {

   if (data != NULL) {
      (* (RoadMapCallback) data) ();
   }
}


static gint roadmap_main_key_pressed (UIView *w, GdkEventKey *event) {

   char *key = NULL;
   char regular_key[2];


   switch (event->keyval) {

      case GDK_Left:   key = "Button-Left";           break;
      case GDK_Right:  key = "Button-Right";          break;
      case GDK_Up:     key = "Button-Up";             break;
      case GDK_Down:   key = "Button-Down";           break;

      case GDK_Return: key = "Enter";                 break;

      // These binding are for the iPAQ buttons: 
      case 0x1008ff1a: key = "Button-Menu";           break;
      case 0x1008ff20: key = "Button-Calendar";       break;
      case 0xaf9:      key = "Button-Contact";        break;
      case 0xff67:     key = "Button-Start";          break;

      // Regular keyboard keys: 
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


*/

void roadmap_main_toggle_full_screen (void) {
}

void roadmap_main_new (const char *title, int width, int height) {
    [TheApp newWithTitle: title andWidth: width andHeight: height];
}


void roadmap_main_title(char *fmt, ...) {

   char newtitle[200];
   va_list ap;
   int n;

   n = snprintf(newtitle, 200, "%s", RoadMapMainTitle);
   va_start(ap, fmt);
   vsnprintf(&newtitle[n], 200 - n, fmt, ap);
   va_end(ap);

 //  gtk_window_set_title (GTK_WINDOW(RoadMapMainWindow), newtitle);
}

void roadmap_main_set_keyboard (RoadMapKeyInput callback) {
   RoadMapMainInput = callback;
}

RoadMapMenu roadmap_main_new_menu (const char *title) {

   return NULL;
}


void roadmap_main_free_menu (RoadMapMenu menu) {
     NSLog (@"roadmap_main_free_menu\n");

   
}


void roadmap_main_add_menu (RoadMapMenu menu, const char *label) {
     NSLog (@"roadmap_main_add_menu label: %s\n", label);
/*
   UIView *menu_item;

   if (RoadMapMainMenuBar == NULL) {

      RoadMapMainMenuBar = gtk_menu_bar_new();

      gtk_box_pack_start
         (GTK_BOX(RoadMapMainBox), RoadMapMainMenuBar, FALSE, TRUE, 0);
   }

   menu_item = gtk_menu_item_new_with_label (label);
   gtk_menu_shell_append (GTK_MENU_SHELL(RoadMapMainMenuBar), menu_item);

   gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_item), (UIView *) menu);
*/
}


void roadmap_main_popup_menu (RoadMapMenu menu,
                              const RoadMapGuiPoint *position) {
     NSLog (@"roadmap_main_popup_menu\n");

 /*  if (menu != NULL) {
      gtk_menu_popup (GTK_MENU(menu),
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      0,
                      gtk_get_current_event_time());
   }
   */
}


void roadmap_main_add_menu_item (RoadMapMenu menu,
                                 const char *label,
                                 const char *tip,
                                 RoadMapCallback callback) {

     NSLog (@"roadmap_main_add_menu_item label: %s tip: %s\n", label, tip);
/*
   UIView *menu_item;

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
   */
}


void roadmap_main_add_separator (RoadMapMenu menu) {
   NSLog (@"roadmap_main_add_seperator\n");

   roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
}


void roadmap_main_add_toolbar (const char *orientation) {
     NSLog (@"roadmap_main_add_toolbar orientation: %s\n", orientation);

    if (RoadMapMainToolbar == NULL) {
        int on_top = 1;

        //   GtkOrientation gtk_orientation = GTK_ORIENTATION_HORIZONTAL;

        // RoadMapMainToolbar = gtk_toolbar_new ();

        switch (orientation[0]) {
            case 't':
            case 'T': on_top = 1; break;

            case 'b':
            case 'B': on_top = 0; break;

            case 'l':
            case 'L': on_top = 1;
                      //            gtk_orientation = GTK_ORIENTATION_VERTICAL;
                      break;

            case 'r':
            case 'R': on_top = 0;
                      //           gtk_orientation = GTK_ORIENTATION_VERTICAL;
                      break;

            default: /*roadmap_log (ROADMAP_FATAL,
                             "Invalid toolbar orientation %s", orientation);
                             */
                             break;
        }
        /*      gtk_toolbar_set_orientation (GTK_TOOLBAR(RoadMapMainToolbar),
                gtk_orientation);

                if (gtk_orientation == GTK_ORIENTATION_VERTICAL) {
         */
        /* In this case we need a new box, since the "normal" box
         * is a vertical one and we need an horizontal one. */

        /*         RoadMapCanvasBox = gtk_hbox_new (FALSE, 0);
                   gtk_container_add (GTK_CONTAINER(RoadMapMainBox), RoadMapCanvasBox);
         */
        // }

        if (on_top) {
            // gtk_box_pack_start (GTK_BOX(RoadMapCanvasBox),
            //                     RoadMapMainToolbar, FALSE, FALSE, 0);
        } else {
            // gtk_box_pack_end   (GTK_BOX(RoadMapCanvasBox),
            //                     RoadMapMainToolbar, FALSE, FALSE, 0);
        }
    }
}

void roadmap_main_add_tool (const char *label,
                            const char *icon,
                            const char *tip,
                            RoadMapCallback callback) {
   NSLog (@"roadmap_main_add_tool label: %s icon: %s tip: %s\n", label, icon, tip);
   if (RoadMapMainToolbar == NULL) {
      roadmap_main_add_toolbar ("");
   }

/*
   gtk_toolbar_append_item (GTK_TOOLBAR(RoadMapMainToolbar),
                            label, tip, NULL,
                            roadmap_main_toolbar_icon (icon),
                            (GtkSignalFunc) roadmap_main_activate, callback);

   if (gdk_screen_height() < 550)
   {
*/
      /* When using a small screen, we want either the labels or the icons,
       * but not both (small screens are typical with PDAs). */
       
/*      gtk_toolbar_set_style
         (GTK_TOOLBAR(RoadMapMainToolbar),
          GtkIconsInitialized?GTK_TOOLBAR_ICONS:GTK_TOOLBAR_TEXT);
   }
   */
}


void roadmap_main_add_tool_space (void) {
   NSLog (@"roadmap_main_add_tool_space\n");
   if (RoadMapMainToolbar == NULL) {
   //   roadmap_log (ROADMAP_FATAL, "Invalid toolbar space: no toolbar yet");
   }

/*
   gtk_toolbar_append_space (GTK_TOOLBAR(RoadMapMainToolbar));
   */
}

static unsigned long roadmap_main_busy_start;

void roadmap_main_set_cursor (RoadMapCursor newcursor) {
   NSLog (@"roadmap_main_set_cursor\n");
}

void roadmap_main_busy_check(void) {

   if (roadmap_main_busy_start == 0)
      return;

   if (roadmap_time_get_millis() - roadmap_main_busy_start > 1000) {
      roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);
   }
}

void roadmap_main_add_canvas (void) {
//    NSLog ("roadmap_main_add_canvas\n");
    struct CGRect rect = [UIHardware fullScreenApplicationContentRect];
    rect.origin.x = rect.origin.y = 0.0f;

    RoadMapCanvasBox = [[RoadMapCanvasView alloc] initWithFrame: rect];
    [RoadMapMainBox addSubview: RoadMapCanvasBox];
}

void roadmap_main_add_status (void) {
   NSLog (@"roadmap_main_add_status\n");
}


void roadmap_main_show (void) {
   NSLog (@"roadmap_main_show\n");

   if (RoadMapMainWindow != NULL) {
   }
}

void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback) {
    [TheApp setInput: io andCallback: callback];
}

void roadmap_main_remove_input (RoadMapIO *io) {
    [TheApp removeInput: io];
}

void roadmap_main_set_periodic (int interval, RoadMapCallback callback) {
    [TheApp setPeriodic: (interval*0.001) andCallback: callback];
}


void roadmap_main_remove_periodic (RoadMapCallback callback) {
   [TheApp removePeriodic: callback];
}


void roadmap_main_set_status (const char *text) {
//   NSLog (@"roadmap_main_set_status text: %s\n", text);
}

void roadmap_main_set_idle_function (RoadMapCallback callback) {
    idle_callback = callback;
    [TheApp setPeriodic: 0.00 andCallback: callback];
}

void roadmap_main_remove_idle_function (void) {
   [TheApp removePeriodic: idle_callback];
   idle_callback = NULL;
}


int roadmap_main_flush (void) {
 /*  if ([[NSNotificationCenter defaultCenter] isEmpty])

      return 0;

   return 1;
   */
//   NSLog (@"roadmap_main_flush\n");
/*
   while (gtk_events_pending ()) {
      if (gtk_main_iteration ()) {
         exit(0);  // gtk_main_quit() called 
      }
   }
   */
   return 0;
}


int roadmap_main_flush_synchronous (int deadline) {
   NSLog (@"roadmap_main_flush_synchronous\n");

   long start_time, duration;

   start_time = roadmap_time_get_millis();

  /* while (gtk_events_pending ()) {
      if (gtk_main_iteration ()) {
         exit(0); 
      }
   }
   */
//   gdk_flush();

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
     //XXX UIApplicationquit
   }
}

int main (int argc, char **argv) {
    int i;
    int j = 0;
    sArgc = argc;
    sArgv = (char **)malloc(argc * (sizeof (char*)));
    for (i=0; i<argc; i++)
    {
        if (strcmp(argv[i], "--launchedFromSB") != 0) {
            sArgv[i] = strdup(argv[j]);
            j++;
        }
        else
           sArgc--;
    }
    roadmap_option (sArgc, sArgv, 0, NULL);
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    return UIApplicationMain(sArgc, sArgv, [RoadMapApp class]);
    [pool release];
    return 0;
}

@implementation RoadMapApp

-(void) newWithTitle: (const char *)title andWidth: (int) width andHeight: (int) height
{
    if (RoadMapMainWindow == NULL) {
        struct CGRect rect = [UIHardware fullScreenApplicationContentRect];
        rect.origin.x = rect.origin.y = 0.0f;
        RoadMapMainWindow = [[UIWindow alloc] initWithContentRect: rect];
        RoadMapMainBox = [[UIView alloc] initWithFrame: rect];
        [RoadMapMainWindow orderFront: self];
        [RoadMapMainWindow makeKey: self];
        [RoadMapMainWindow _setHidden: NO];
        [RoadMapMainWindow setContentView: RoadMapMainBox];
        UINavigationBar *nav = [[UINavigationBar alloc] initWithFrame: CGRectMake(
                0.0f, 0.0f, 320.0f, 48.0f)];
        [nav showButtonsWithLeftTitle: @"Map" rightTitle: nil leftBack: NO];
        [nav setBarStyle: 0];
        [nav setDelegate: self];

        [RoadMapMainBox addSubview: nav]; 
    }


    if (RoadMapMainTitle != NULL) {
        free(RoadMapMainTitle);
    }
    RoadMapMainTitle = strdup (title);
}
- (void) periodicCallback: (NSTimer *) timer
{
   int i;
   for (i = 0; i < ROADMAP_MAX_TIMER; ++i) {
      if (RoadMapMainPeriodicTimer[i].timer == timer) {
         (* RoadMapMainPeriodicTimer[i].callback) ();
         break;
      }
   }
}

-(void) setPeriodic: (float) interval andCallback: (RoadMapCallback) callback
{
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

   timer->callback = callback;
   timer->timer = [NSTimer scheduledTimerWithTimeInterval: interval
                     target: self
                     selector: @selector(periodicCallback:)
                     userInfo: nil
                     repeats: YES];
}

-(void) removePeriodic: (RoadMapCallback) callback
{
   int index;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {

         RoadMapMainPeriodicTimer[index].callback = NULL;
         [RoadMapMainPeriodicTimer[index].timer invalidate];
         return;
      }
   }
}

-(void) ioCallback: (id) notify
{
   NSFileHandle *fh = [notify object];
   int i;
   int fd = [fh fileDescriptor];

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.os.file == fd) {
         (* RoadMapMainIo[i].callback) (&RoadMapMainIo[i].io);
            [RoadMapMainIo[i].fh waitForDataInBackgroundAndNotify];
         break;
      }
   }

}

-(void) setInput: (RoadMapIO*) io andCallback: (RoadMapInput) callback
{
    int i;
    int fd = io->os.file; /* All the same on UNIX. */
    NSFileHandle *fh = [[NSFileHandle alloc] initWithFileDescriptor: fd];

    for (i = 0; i < ROADMAP_MAX_IO; ++i) {
        if (RoadMapMainIo[i].io.subsystem == ROADMAP_IO_INVALID) {
            RoadMapMainIo[i].io = *io;
            RoadMapMainIo[i].callback = callback;
            RoadMapMainIo[i].fh = fh;
            [[NSNotificationCenter defaultCenter]
                addObserver: self
                selector:@selector(ioCallback:)
                name:NSFileHandleDataAvailableNotification
                object:fh];
            [fh waitForDataInBackgroundAndNotify];
            break;
        }
    }
}

-(void) removeInput: (RoadMapIO*) io
{
    int i;
    int fd = io->os.file; /* All the same on UNIX. */

    for (i = 0; i < ROADMAP_MAX_IO; ++i) {
        if (RoadMapMainIo[i].io.os.file == fd) {
            [[NSNotificationCenter defaultCenter]
                removeObserver: self
                name:NSFileHandleDataAvailableNotification
                object:RoadMapMainIo[i].fh];
            RoadMapMainIo[i].io.os.file = -1;
            RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
            break;
        }
    }
}

- (void) applicationDidFinishLaunching: (id) unused
{
    TheApp = self;
    int i;
    for (i = 0; i < ROADMAP_MAX_IO; ++i) {
        RoadMapMainIo[i].io.os.file = -1;
        RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
    }

    roadmap_start (sArgc, sArgv);

//    atexit(roadmap_main_exit);
}

- (void)applicationWillTerminate;
{
    roadmap_main_exit();
    [TheApp release];
}

@end

