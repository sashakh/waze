/* roadmap_main.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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

#include <java/lang.h>
#include <javax/microedition/lcdui.h>
#include <javax/microedition/lcdui/game.h>
#include <command_mgr.h>
#include <form_command_mgr.h>
#include <gps_manager.h>
#include <timer_mgr.h>
#include <device_specific.h>

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_canvas.h"
#include "roadmap_messagebox.h"
#include "roadmap_dialog.h"

#include "roadmap_main.h"

static RoadMapConfigDescriptor RoadMapConfigFullScreen =
                        ROADMAP_CONFIG_ITEM("Display", "Full Screen");

extern void roadmap_canvas_configure (void);

struct roadmap_main_io {
   int active;
   RoadMapIO io;
   RoadMapInput callback;
};

#define ROADMAP_MAX_IO 16
static struct roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO];


struct roadmap_main_timer {
   RoadMapCallback callback;
};

#define ROADMAP_MAX_TIMER 10
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];


static RoadMapKeyInput RoadMapMainInput = NULL;

volatile static int command_addr = 0;
volatile static int form_command_addr = 0;
volatile static char *form_command_name[255];
volatile static int form_command_context = 0;
volatile static int should_exit = 0;
static NOPH_GpsManager_t gps_mgr = 0;
static NOPH_TimerMgr_t timer_mgr = 0;
static NOPH_GameCanvas_t Canvas;
static const char *exception_type = "";
static int FullScreenMode = 1;

static void roadmap_start_event (int event) {
   switch (event) {
   case ROADMAP_START_INIT:
      break;
   }
}

static int KeyCode = 0;

static void roadmap_main_process_key (int keyCode) {

   char *k = NULL;
   const RoadMapAction *action;

   switch (keyCode) {
   case NOPH_Canvas_KEY_NUM2:
      k = "+";
      break;
   case NOPH_Canvas_KEY_NUM4:
      k = "J";
      break;
   case NOPH_Canvas_KEY_NUM5:
      k = "R";
      break;
   case NOPH_Canvas_KEY_NUM6:
      k = "K";
      break;
   case NOPH_Canvas_KEY_NUM8:
      k = "-";
      break;
   case NOPH_Canvas_KEY_POUND:
      KeyCode = 0;
      action = roadmap_start_find_action ("togglegpsrecord");
      if (action) (*action->callback) ();
      return;
   case NOPH_Canvas_KEY_STAR:
      k = "A";
      KeyCode = 0;
      break;
   default:
      switch (NOPH_Canvas_getGameAction((NOPH_Canvas_t)Canvas, keyCode)) {
      case NOPH_Canvas_UP:
         k = "Button-Up";
         break;
      case NOPH_Canvas_FIRE:
         KeyCode = 0;
         k = "G";
         break;
      case NOPH_Canvas_RIGHT:
         k = "Button-Right";
         break;
      case NOPH_Canvas_DOWN:
         k = "Button-Down";
         break;
      case NOPH_Canvas_LEFT:
         k = "Button-Left";
         break;
      }
   }

   //roadmap_log (ROADMAP_DEBUG, "In roadmap_main_process_key, keys:%d, k:%s, RoadMapMainInput:0x%x\n", keys, k, RoadMapMainInput);

   if ((k != NULL) && (RoadMapMainInput != NULL)) {
      (*RoadMapMainInput) (k);
   }
}


void roadmap_main_toggle_full_screen (void) {

   static int RoadMapIsFullScreen = 0;

   if (RoadMapIsFullScreen) {
   } else {
   }
}

void roadmap_main_new (const char *title, int width, int height) {
}


void roadmap_main_set_keyboard (struct RoadMapFactoryKeyMap *bindings,
                                RoadMapKeyInput callback) {
   RoadMapMainInput = callback;
}


RoadMapMenu roadmap_main_new_menu (void) {

   return NULL;
}


void roadmap_main_free_menu (RoadMapMenu menu) {

}


void roadmap_main_add_menu (RoadMapMenu menu, const char *label) {

}


void roadmap_main_add_menu_item (RoadMapMenu menu,
                                 const char *label,
                                 const char *tip,
                                 RoadMapCallback callback) {

   NOPH_CommandMgr_t cm = NOPH_CommandMgr_getInstance();
   NOPH_CommandMgr_addCommand(cm, label, (void *)callback);
}


void roadmap_main_popup_menu (RoadMapMenu menu, int x, int y) {

}


void roadmap_main_add_separator (RoadMapMenu menu) {

   roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
}


void roadmap_main_add_tool (const char *label,
                            const char *icon,
                            const char *tip,
                            RoadMapCallback callback) {
}


void roadmap_main_add_tool_space (void) {

}


void roadmap_main_add_canvas (void) {
   RoadMapImage image;

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigFullScreen, NULL, "yes", "no", NULL);

   Canvas = NOPH_GameCanvas_get();

   if (!strcmp(roadmap_config_get (&RoadMapConfigFullScreen), "no"))
        FullScreenMode = 0;

   NOPH_Canvas_setFullScreenMode(Canvas, FullScreenMode);

   roadmap_log(ROADMAP_DEBUG, "In roadmap_main_add_canvas...\n");
   roadmap_canvas_configure ();

   roadmap_log(ROADMAP_DEBUG, "B4 load welcome...\n");
   image = roadmap_canvas_load_image (NULL, "/welcome.png");
   roadmap_log(ROADMAP_DEBUG, "After load welcome:%x\n", image);

   if (image) {
      RoadMapGuiPoint pos;

      RoadMapPen splash_bg = roadmap_canvas_create_pen ("SplashBG");
      roadmap_canvas_set_foreground ("#000000");
      roadmap_canvas_select_pen (splash_bg);

      roadmap_canvas_erase ();

      pos.x = (roadmap_canvas_width () - roadmap_canvas_image_width(image)) / 2;
      pos.y = (roadmap_canvas_height () - roadmap_canvas_image_height(image)) / 2;
      roadmap_canvas_draw_image (image, &pos, 0, IMAGE_NORMAL);
      roadmap_canvas_free_image (image);
      roadmap_canvas_refresh ();
   }
}


void roadmap_main_add_status (void) {

}


void roadmap_main_show (void) {

}

void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback) {

   if (io->subsystem == ROADMAP_IO_SERIAL) {
      /* We currently only support GPS input */
      if (!gps_mgr) {
         gps_mgr = NOPH_GpsManager_getInstance();
         NOPH_GpsManager_setTypeMsgs(gps_mgr,
                                     roadmap_lang_get("Internal GPS"),
                                     roadmap_lang_get("External GPS"));
      }
      RoadMapMainIo[0].io = *io;
      RoadMapMainIo[0].callback = callback;
      RoadMapMainIo[0].active = 1;
      NOPH_GpsManager_start(gps_mgr);
   }
}


void roadmap_main_remove_input (RoadMapIO *io) {

   if (io->subsystem == ROADMAP_IO_SERIAL) {
      /* We currently only support GPS input */
      if (gps_mgr) NOPH_GpsManager_stop(gps_mgr);
      RoadMapMainIo[0].active = 0;
   }
}


void roadmap_main_set_periodic (int interval, RoadMapCallback callback) {

   int index;

   if (!timer_mgr) timer_mgr = NOPH_TimerMgr_getInstance();
   index = NOPH_TimerMgr_set (timer_mgr, interval);

   if ((index == -1) || (RoadMapMainPeriodicTimer[index].callback != NULL)) {
      roadmap_log (ROADMAP_FATAL, "Can't create a new timer!");
   }


   RoadMapMainPeriodicTimer[index].callback = callback;
}


void roadmap_main_remove_periodic (RoadMapCallback callback) {

   int index;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {

         RoadMapMainPeriodicTimer[index].callback = NULL;
         NOPH_TimerMgr_remove (timer_mgr, index);

         return;
      }
   }

   roadmap_log (ROADMAP_ERROR, "timer 0x%08x not found", callback);
}


void roadmap_main_set_status (const char *text) {

}


void roadmap_main_flush (void) {

}


void roadmap_main_exit (void) {

   int index;
   roadmap_log(ROADMAP_DEBUG, "b4 roadmap_start_Exit...\n");
   roadmap_start_exit ();
   roadmap_log(ROADMAP_DEBUG, "after roadmap_start_Exit...\n");
   should_exit = 1;

   /* remove all timers */
   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback != NULL) {

         RoadMapMainPeriodicTimer[index].callback = NULL;
         NOPH_TimerMgr_remove (timer_mgr, index);
      }
   }

   exit(0);
}

void roadmap_main_set_cursor (int cursor) {
   if (cursor == ROADMAP_CURSOR_WAIT) {

      if (roadmap_dialog_activate("Please wait", NULL, 1)) {
         roadmap_dialog_new_progress ("Please wait", "Please wait...");
         roadmap_dialog_complete (0);
      }

   } else if (cursor == ROADMAP_CURSOR_NORMAL) {
      roadmap_dialog_hide ("Please wait");
   }
}

static void keyPressed(int code)
{
   KeyCode = code;
   return;
}

static void keyReleased(int code)
{
  KeyCode = 0;
}

static void handler_main_loop(NOPH_Exception_t exception, void *arg)
{
  NOPH_Throwable_printStackTrace(exception);
  roadmap_messagebox(roadmap_lang_get("Error"), exception_type);

  NOPH_delete(exception);
}

//#define  DEBUG_TIME
static void wait_for_events(void)
{
#ifdef DEBUG_TIME
  int start_time = NOPH_System_currentTimeMillis();
  int end_time;
#endif

  while (!should_exit) {
     if (command_addr) break; /* Menu command */
     if (NOPH_FormCommandMgr_getCallBackNotif((int *)&form_command_addr, (void *)form_command_name, (void *)&form_command_context)) break;
     if (gps_mgr) {
#ifdef DEBUG_TIME
        int has_data = 0;
#endif
	exception_type = "GPS processing";
        while (RoadMapMainIo[0].active &&
              (NOPH_GpsManager_read(gps_mgr, 0, 0) != 0)) {
#ifdef DEBUG_TIME
           if (!has_data) {
              //roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: got gps data...\n");
              has_data = 1;
           }
#endif
           /* GPS data is ready */
           RoadMapInput callback = RoadMapMainIo[0].callback;
           if (callback) (*callback)(&RoadMapMainIo[0].io);
        }
#ifdef DEBUG_TIME
        if (has_data) {
           end_time = NOPH_System_currentTimeMillis();
	   if ((end_time - start_time) > 50)
		roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: finished processing GPS data in %d ms\n",
                  end_time - start_time);
           start_time = end_time;
        }
#endif
     }

     if (timer_mgr) {
        int index;
#ifdef DEBUG_TIME
        int has_timer = 0;
#endif
	exception_type = "Periodic timer";
        while ((index = NOPH_TimerMgr_getExpired(timer_mgr)) != -1) {
#ifdef DEBUG_TIME
           if (!has_timer) {
              //roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: executing timer callbacks...\n");
              has_timer = 1;
           }
#endif
           (*RoadMapMainPeriodicTimer[index].callback) ();
        }
#ifdef DEBUG_TIME
        if (has_timer) {
           index = NOPH_System_currentTimeMillis();
	   if ((index - start_time) >= 50)
              roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: finished callbacks in %d ms\n", index - start_time);
           start_time = index;
        }
#endif
     }

     if (KeyCode != 0) break;

     NOPH_Thread_sleep( 10 );
#ifdef DEBUG_TIME
     end_time = NOPH_System_currentTimeMillis();
     if ((end_time - start_time) > 100) roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: Slept for %d ms!\n", end_time - start_time);
     start_time = end_time;
#endif
  }
}

#define SLEEP_PERIOD 100
int main (int argc, char **argv) {

   int i;

   NOPH_DeviceSpecific_init();

   NOPH_Canvas_registerKeyPressedCallback(keyPressed);
   NOPH_Canvas_registerKeyReleasedCallback(keyReleased);

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      RoadMapMainIo[i].io.os.file = -1;
      RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
   }

   roadmap_start_subscribe (roadmap_start_event);
   roadmap_start (argc, argv);

   /*
   if (NOPH_exception) {
      NOPH_Throwable_printStackTrace(NOPH_exception);
      exit(1);
   }
*/
   NOPH_CommandMgr_setResultMem(NOPH_CommandMgr_getInstance(), (int *)&command_addr);
//   NOPH_FormCommandMgr_setCallBackNotif((int *)&form_command_addr, (void *)form_command_name, (void *)&form_command_context);

NOPH_try(handler_main_loop, NULL) {

   /* The main game loop */
   while(!should_exit)
   {
#ifdef DEBUG_TIME
      int start_time;
      int end_time;
#endif

      /* Wait for an event */
      wait_for_events();

#ifdef DEBUG_TIME
      start_time = NOPH_System_currentTimeMillis();
#endif
      if (command_addr) {
#ifdef DEBUG_TIME
         roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: processing command...\n");
#endif
	 exception_type = "command";
         ((RoadMapCallback)command_addr)();
         command_addr = 0;
#ifdef DEBUG_TIME
         end_time = NOPH_System_currentTimeMillis();
         roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: finished processing command in %d ms\n",
                  end_time - start_time);
         start_time = end_time;
#endif
      }

      if (form_command_addr) {
#ifdef DEBUG_TIME
         roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: processing form command...\n");
         roadmap_log (ROADMAP_ERROR,
            "Form command: addr:%x, name:%s, context:%x",
            form_command_addr, form_command_name, form_command_context);
#endif
	 exception_type = "Form callback";
         ((RoadMapDialogCallback)form_command_addr)((char *)form_command_name,
                                          (void *)form_command_context);
         form_command_addr = 0;
#ifdef DEBUG_TIME
         end_time = NOPH_System_currentTimeMillis();
         roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: finished processing form command in %d ms\n",
                  end_time - start_time);
         start_time = end_time;
#endif
      }

#ifdef DEBUG_TIME
      roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: processing key command...\n");
#endif
      if (KeyCode) {
	 exception_type = "Key command";
         roadmap_main_process_key (KeyCode);
         NOPH_Thread_sleep( 30 );
      }
#ifdef DEBUG_TIME
      end_time = NOPH_System_currentTimeMillis();
      roadmap_log(ROADMAP_DEBUG, "MAIN LOOP: finished processing key command in %d ms\n",
            end_time - start_time);
      start_time = end_time;
#endif
   }
} NOPH_catch();

   roadmap_log(ROADMAP_DEBUG, "Main loop exiting...\n");
   //exit(0);
   return 0;
}

