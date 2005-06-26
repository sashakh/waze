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

#include "roadmap.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_main.h"

};

#ifdef QWS
#include <qpe/qpeapplication.h>
#else
#include <qapplication.h>
#endif
#include "qt_main.h"

#ifdef QWS
static QPEApplication* app;
#else
static QApplication* app;
#endif
RMapMainWindow* mainWindow;

void roadmap_main_new(const char* title, int width, int height) {

	mainWindow = new RMapMainWindow(title);

#ifdef QWS
	app->showMainWidget(mainWindow);
#else
	app->setMainWidget(mainWindow);
#endif
}

void roadmap_main_set_keyboard(RoadMapKeyInput callback) {
	if (mainWindow) {
		mainWindow->setKeyboardCallback(callback);
	}
}

void roadmap_main_add_menu(const char* label) {
	if (mainWindow) {
		mainWindow->addMenu(label);
	}
}

void roadmap_main_add_menu_item(const char* label, const char* tip,
	RoadMapCallback callback) {

	if (mainWindow) {
		mainWindow->addMenuItem(label, tip, callback);
	}
}

void roadmap_main_add_separator(void) {
	if (mainWindow) {
		mainWindow->addMenuSeparator();
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

void roadmap_main_add_canvas(void) {
// The canvas is implicitely added to the main window.
//	if (mainWindow) {
//		mainWindow->addCanvas();
//	}
}

void roadmap_main_add_status(void) {
	// nothing to be done
}

void roadmap_main_show(void) {
	if (mainWindow) {
		mainWindow->show();
	}
}

void roadmap_main_set_input(int fd, RoadMapInput callback) {
	if (mainWindow) {
		mainWindow->addInput(fd, callback);
	}
}

void roadmap_main_remove_input(int fd) {
	if (mainWindow) {
		mainWindow->removeInput(fd);
	}
}

void roadmap_main_set_serial_input    (int fd, RoadMapInput callback) {
   roadmap_main_set_input (fd, callback); /* Same thing on UNIX. */
}

void roadmap_main_remove_serial_input (int fd) {
   roadmap_main_remove_input (fd); /* Same thing on UNIX. */
}


void roadmap_main_set_periodic (int interval, RoadMapCallback callback) {

	if (mainWindow) {
		mainWindow->setTimer(interval, callback);
	}
}


void roadmap_main_remove_periodic (RoadMapCallback callback) {

	if (mainWindow) {
		mainWindow->removeTimer(callback);
	}
}


void roadmap_main_set_status(const char *text) {
	if (mainWindow) {
		mainWindow->setStatus(text);
	}
}


void roadmap_main_toggle_full_screen (void) {
	// Not yet implemented (how to do this ??)
}


void roadmap_main_flush (void) {

   if (app != NULL) {
      app->processEvents ();
   }
}


void roadmap_main_exit(void) {

	roadmap_start_exit();
	exit(0);
}

int main(int argc, char* argv[]) {
#ifdef QWS
	app = new QPEApplication(argc, argv);
#else
	app = new QApplication(argc, argv);
#endif
	roadmap_start(argc, argv);

	return app->exec();
}
