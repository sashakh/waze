/* qt_main.h - The interface for the RoadMap/QT main class.
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
 */

#ifndef INCLUDE__ROADMAP_QT_MAIN__H
#define INCLUDE__ROADMAP_QT_MAIN__H

#include <qmainwindow.h>
#include <qmap.h>
#include <qtoolbar.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qsocketnotifier.h>
#include <qpushbutton.h>
#include <qstatusbar.h>
#include <qtimer.h>

#define ROADMAP_MAX_TIMER 16

extern "C" {

#include "roadmap.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_main.h"

};

#include "qt_canvas.h"

class RMapInput : public QObject {

Q_OBJECT

public:
	RMapInput(int fd1, RoadMapInput cb);
	~RMapInput();

protected:
	int fd;
	RoadMapInput callback;
	QSocketNotifier* nf;

protected slots:
	void fire(int);
};

class RMapCallback : public QObject {

Q_OBJECT

public:
	RMapCallback(RoadMapCallback cb);
	int same (RoadMapCallback cb);

protected slots:
	void fire();

protected:
	RoadMapCallback callback;
};

class RMapMainWindow : public QMainWindow {

Q_OBJECT

public:
	RMapMainWindow(const char* name);
	virtual ~RMapMainWindow();

	void setKeyboardCallback(RoadMapKeyInput c);

	void addMenu(const char* label);
	void addMenuItem(const char* label, const char* tip, 
		RoadMapCallback callback);

	void addMenuSeparator();

	void addTool(const char* label, const char* tip,
		RoadMapCallback callback);

	void addToolSpace(void);
	void addCanvas(void);
	void addInput(int fd, RoadMapInput callback);
	void removeInput(int fd);
	void setStatus(const char* text);

	void setTimer(int interval, RoadMapCallback callback);
	void removeTimer(RoadMapCallback callback);

protected:
	RoadMapKeyInput keyCallback;
	QPopupMenu* currentMenu;
	QMap<int, RMapInput*> inputMap;
	QToolBar* toolBar;
	RMapCanvas* canvas;

	QTimer* tm[ROADMAP_MAX_TIMER];
	RMapCallback* tcb[ROADMAP_MAX_TIMER];
	bool spacePressed;

	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void closeEvent(QCloseEvent* ev);

};

extern RMapMainWindow* mainWindow;
#endif
