/* roadmap_main.cc - A C to C++ wrapper for the QT RoadMap main function.
 *
 * LICENSE:
 *
 *   (c) Copyright 2003 Latchesar Ionkov
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
 *   See roadmap_main.h
 */
extern "C" {

#include <stdlib.h>
#include <signal.h>

#include "roadmap.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_main.h"
#include "roadmap_time.h"

};

#ifdef QWS
#include <qpe/qpeapplication.h>
#else
#include <qapplication.h>
#endif
#include <qcursor.h>

#include "qt_main.h"

#ifdef QWS
static QPEApplication* app;
#else
static QApplication* app;
#endif

RMapMainWindow* mainWindow;
RMapTimers* timers;


struct roadmap_main_io {
   RoadMapIO io;
   RoadMapInput callback;
};

#define ROADMAP_MAX_IO 16
static struct roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO];


static void roadmap_main_input (int fd) {

   int i;

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.os.file == fd) {
         (*RoadMapMainIo[i].callback) (&RoadMapMainIo[i].io);
         break;
      }
   }
}


void roadmap_main_new(const char* title, int width, int height) {

   mainWindow = new RMapMainWindow(title,width,height);

#ifdef QWS
   app->showMainWidget(mainWindow);
#else
   app->setMainWidget(mainWindow);
#endif
}

void roadmap_main_title(char *fmt, ...) {
   /* unimplemented */
}

void roadmap_main_set_keyboard(RoadMapKeyInput callback) {
   if (mainWindow) {
          mainWindow->setKeyboardCallback(callback);
   }
}

RoadMapMenu roadmap_main_new_menu (const char *title) {

   if (mainWindow) {
     return (RoadMapMenu) mainWindow->newMenu(title);
   } else {
      return (RoadMapMenu) NULL;
   }
}

void roadmap_main_free_menu (RoadMapMenu menu) {

   if (mainWindow) {
      mainWindow->freeMenu((QPopupMenu *)menu);
   }
}

void roadmap_main_popup_menu (RoadMapMenu menu,
                              const RoadMapGuiPoint *position) {

   if (mainWindow) {
      mainWindow->popupMenu((QPopupMenu *)menu, position->x, position->y);
   }
}

void roadmap_main_add_menu(RoadMapMenu menu, const char* label) {
   if (mainWindow) {
      mainWindow->addMenu((QPopupMenu *)menu, label);
   }
}

void roadmap_main_add_menu_item(RoadMapMenu menu,
                                const char* label,
                                const char* tip,
                                RoadMapCallback callback) {

   if (mainWindow) {
      mainWindow->addMenuItem((QPopupMenu *)menu, label, tip, callback);
   }
}

void roadmap_main_add_separator(RoadMapMenu menu) {
   if (mainWindow) {
      mainWindow->addMenuSeparator((QPopupMenu *)menu);
   }
}
                          
void roadmap_main_add_toolbar (const char *orientation) {

    if (mainWindow) {
       mainWindow->addToolbar(orientation);
    }
}

void roadmap_main_add_tool(const char* label,
                           const char *icon,
                           const char* tip,
                           RoadMapCallback callback) {

    if (mainWindow) {
       mainWindow->addTool(label, icon, tip, callback);
    }
}

void roadmap_main_add_tool_space(void) {
   if (mainWindow) {
      mainWindow->addToolSpace();
   }
}

void roadmap_main_set_cursor (RoadMapCursor newcursor) {


   if (mainWindow) {

      switch (newcursor) {

      case ROADMAP_CURSOR_NORMAL:
         mainWindow->unsetCursor ();
         break;

      case ROADMAP_CURSOR_WAIT:
         mainWindow->setCursor (QCursor(Qt::waitCursor));
         break;

      case ROADMAP_CURSOR_CROSS:
         mainWindow->setCursor (QCursor(Qt::crossCursor));
         break;

      case ROADMAP_CURSOR_WAIT_WITH_DELAY:
         return;  /* shouldn't happen */
      }
   }
}

void roadmap_main_add_canvas(void) {
// The canvas is implicitely added to the main window.
//      if (mainWindow) {
//              mainWindow->addCanvas();
//      }
}

void roadmap_main_add_status(void) {
   // nothing to be done
}

void roadmap_main_show(void) {
   if (mainWindow) {
      mainWindow->show();
   }
}

void roadmap_main_set_input(RoadMapIO *io, RoadMapInput callback) {

   if (mainWindow) {

      int i;

      /* All the same on UNIX. */
      mainWindow->addInput(io->os.file, roadmap_main_input);

      for (i = 0; i < ROADMAP_MAX_IO; ++i) {
         if (RoadMapMainIo[i].io.subsystem == ROADMAP_IO_INVALID) {
            RoadMapMainIo[i].io = *io;
            RoadMapMainIo[i].callback = callback;
            break;
         }
      }
   }
}

void roadmap_main_remove_input(RoadMapIO *io) {

   int i;
   int fd = io->os.file; /* All the same on UNIX. */

   if (mainWindow) {
      mainWindow->removeInput(fd);
   }

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      if (RoadMapMainIo[i].io.os.file == fd) {
         RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
         RoadMapMainIo[i].io.os.file = -1;
         break;
      }
   }
}


void roadmap_main_set_periodic (int interval, RoadMapCallback callback) {

   if (timers) {
      timers->addTimer(interval, callback);
   } 
}


void roadmap_main_remove_periodic (RoadMapCallback callback) {

   if (timers) {
      timers->removeTimer(callback);
   }
}

RoadMapCallback idle_callback;

void roadmap_main_set_idle_function (RoadMapCallback callback) {

#if FIXME
   /* under QT, a timeout of 0 means the routine will run "right after"
    * the event queue is emptied.
    */
   if (timers) {
      idle_callback = callback;
      timers->addTimer(0, callback);
   }
#else
   callback();
#endif
}

void roadmap_main_remove_idle_function (void) {
#if FIXME
   if (timers) {
      timers->removeTimer(idle_callback);
   }
#endif
}


void roadmap_main_set_status(const char *text) {
   if (mainWindow) {
      mainWindow->setStatus(text);
   }
}


void roadmap_main_toggle_full_screen (void) {
   // Not yet implemented (how to do this ??)
}


int roadmap_main_flush (void) {

   if (app && app->hasPendingEvents ()) {
        app->processEvents ();
        return 1;
   }

   return 0;
}


int roadmap_main_flush_synchronous (int deadline) {

   if (app != NULL) {

      long start_time, duration;

      start_time = roadmap_time_get_millis();

      app->processEvents ();
      app->syncX();

      duration = roadmap_time_get_millis() - start_time;

      if (duration > deadline) {

         roadmap_log (ROADMAP_DEBUG, "processing flush took %d", duration);

         return 0; /* Busy. */
      }
   }
   return 1;
}


void roadmap_main_exit(void) {

   roadmap_start_exit();
   exit(0);
}

static int roadmap_main_signals_init()
{
  struct sigaction signala;

  memset(&signala, 0, sizeof (struct sigaction));

  signala.sa_handler = RMapMainWindow::signalHandler;
  sigemptyset(&signala.sa_mask);
  signala.sa_flags |= SA_RESTART;

  if (sigaction(SIGHUP, &signala, 0) > 0)
    return 1;
  if (sigaction(SIGTERM, &signala, 0) > 0)
    return 2;
  if (sigaction(SIGINT, &signala, 0) > 0)
    return 3;
  if (sigaction(SIGQUIT, &signala, 0) > 0)
    return 4;
  return 0;
}

int main(int argc, char* argv[]) {

   int i;

#ifdef QWS
   app = new QPEApplication(argc, argv);
#else
   app = new QApplication(argc, argv);
#endif

   roadmap_option (argc, argv, 0, NULL);

   for (i = 0; i < ROADMAP_MAX_IO; ++i) {
      RoadMapMainIo[i].io.subsystem = ROADMAP_IO_INVALID;
      RoadMapMainIo[i].io.os.file = -1;
   }

   timers = new RMapTimers(app,"timersHandler");
  
   roadmap_main_signals_init();

   roadmap_start(argc, argv);

   return app->exec();
}
