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
#include <gps_manager.h>

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_canvas.h"

#include "roadmap_main.h"

extern void roadmap_canvas_configure (void);

struct roadmap_main_io {
   int id;
   RoadMapIO io;
   RoadMapInput callback;
};

#define ROADMAP_MAX_IO 16
static struct roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO];


struct roadmap_main_timer {
   int id;
   RoadMapCallback callback;
};

#define ROADMAP_MAX_TIMER 16
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];


static char *RoadMapMainTitle = NULL;

static RoadMapKeyInput RoadMapMainInput = NULL;

volatile static int command_addr = 0;
static NOPH_GpsManager_t gps_mgr = 0;

static void roadmap_start_event (int event) {
   switch (event) {
   case ROADMAP_START_INIT:
      break;
   }
}


static void roadmap_main_process_key (int keys) {

   char *k = NULL;

   if (keys & NOPH_GameCanvas_LEFT_PRESSED)
           k = "J";
   else if (keys & NOPH_GameCanvas_RIGHT_PRESSED)
           k = "K";
   else if (keys & NOPH_GameCanvas_UP_PRESSED)
           k = "+";
   else if (keys & NOPH_GameCanvas_DOWN_PRESSED)
           k = "-";
   else if (keys & NOPH_GameCanvas_FIRE_PRESSED)
           k = "Q";
   else if (keys & NOPH_GameCanvas_GAME_A_PRESSED)
           k = "Button-Left";
   else if (keys & NOPH_GameCanvas_GAME_B_PRESSED)
           k = "Button-Right";
   else if (keys & NOPH_GameCanvas_GAME_C_PRESSED)
           k = "Button-Up";
   else if (keys & NOPH_GameCanvas_GAME_D_PRESSED)
           k = "Button-Down";

   roadmap_log (ROADMAP_DEBUG, "In roadmap_main_process_key, keys:%d, k:%s, RoadMapMainInput:0x%x\n",
                keys, k, RoadMapMainInput);

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
   roadmap_canvas_configure ();
}


void roadmap_main_add_status (void) {

}


void roadmap_main_show (void) {

}

void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback) {

   if (io->subsystem == ROADMAP_IO_SERIAL) {
      /* We currently only support GPS input */
      if (!gps_mgr) gps_mgr = NOPH_GpsManager_getInstance();
      RoadMapMainIo[0].io = *io;
      RoadMapMainIo[0].callback = callback;
      NOPH_GpsManager_start(gps_mgr);
   }

#if 0
   int i;
   int fd = io->os.serial;

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.subsystem == ROADMAP_IO_INVALID) {
         RoadMapMainIo[i].io = *io;
         RoadMapMainIo[i].callback = callback;
         RoadMapMainIo[i].id = 0;
         break;
      }
   }
#endif
}


void roadmap_main_remove_input (RoadMapIO *io) {

   if (io->subsystem == ROADMAP_IO_SERIAL) {
      /* We currently only support GPS input */
      if (gps_mgr) NOPH_GpsManager_stop(gps_mgr);
      RoadMapMainIo[0].callback = NULL;
      RoadMapMainIo[0].io.subsystem = ROADMAP_IO_INVALID;
   }

#if 0
   int i;
   int fd = io->os.file; /* All the same on UNIX. */

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.os.file == fd) {
         RoadMapMainIo[i].io.os.file = -1;
         RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
         break;
      }
   }
#endif
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

   timer->id = 0;
   timer->callback = callback;
}


void roadmap_main_remove_periodic (RoadMapCallback callback) {

   int index;

   for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

      if (RoadMapMainPeriodicTimer[index].callback == callback) {

         RoadMapMainPeriodicTimer[index].callback = NULL;

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

   roadmap_start_exit ();
}

void roadmap_main_set_cursor (int cursor) {}

/* Wait until some key is pressed */
static int wait_for_events(NOPH_GameCanvas_t canvas)
{
  static int counter = 0;
  int out = 0;

  while (1) {
     counter++;
     if (command_addr) break; /* Menu command */
     if (gps_mgr) {
       while ((RoadMapMainIo[0].io.subsystem != ROADMAP_IO_INVALID) &&
       	      (NOPH_GpsManager_read(gps_mgr, 0, 0) != 0)) {
         /* GPS data is ready */
         RoadMapInput callback = RoadMapMainIo[0].callback;
         if (callback) (*callback)(&RoadMapMainIo[0].io);
       }
     }
     if ((out = NOPH_GameCanvas_getKeyStates(canvas)) != 0) break;
     if ((counter % 300) == 0) {
       int i;
       for (i=0; i<ROADMAP_MAX_TIMER; i++) {
         if (RoadMapMainPeriodicTimer[i].callback) {
	   (*RoadMapMainPeriodicTimer[i].callback) ();
	 }  
       }
     }

     NOPH_Thread_sleep( 10 );
  }

  return out;
}

#define SLEEP_PERIOD 100
int main (int argc, char **argv) {

   int i;
   int should_exit = 0;
   NOPH_GameCanvas_t canvas;

   printf ("**** test: %d\n", strlen("sdfsdf"));

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      RoadMapMainIo[i].io.os.file = -1;
      RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
   }

   canvas = NOPH_GameCanvas_get();

   roadmap_start_subscribe (roadmap_start_event);
   roadmap_start (argc, argv);

   if (NOPH_exception) {
      NOPH_Throwable_printStackTrace(NOPH_exception);
      exit(1);
   }

   NOPH_CommandMgr_setResultMem(NOPH_CommandMgr_getInstance(), &command_addr);

   /* The main game loop */
   while(!should_exit)
   {
      int keys;

      /* Wait for a keypress */
      keys = wait_for_events(canvas);

      if (command_addr) {
        ((RoadMapCallback)command_addr)();
	command_addr = 0;
      }

      roadmap_main_process_key (keys);
   }

   return 0;
}

