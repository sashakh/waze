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
#include <qmenu.h>
#include <qsocketnotifier.h>
#include <qpushbutton.h>
#include <qstatusbar.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qevent.h>
#include <qicon.h>
#include <qsocketnotifier.h>

#define ROADMAP_MAX_TIMER 16

extern "C" {

#include "roadmap.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "roadmap_history.h"
#include "roadmap_main.h"
#include "roadmap_path.h"

   typedef void (*RoadMapQtInput) (int fd);
};

#include "qt_canvas.h"

class RMapInput : public QObject {

Q_OBJECT

public:
   RMapInput(int fd1, RoadMapQtInput cb);
   ~RMapInput();

protected:
   int fd;
   RoadMapQtInput callback;
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

class RMapTimers : public QObject {

Q_OBJECT

public:
  RMapTimers(QObject *parent = 0);
  ~RMapTimers();
  void addTimer(int interval, RoadMapCallback cb);
  void removeTimer(RoadMapCallback cb);

private:
   QTimer* tm[ROADMAP_MAX_TIMER];
   RMapCallback* tcb[ROADMAP_MAX_TIMER];

};

class RMapMainWindow : public QMainWindow {

Q_OBJECT

public:
   RMapMainWindow( QWidget *parent, Qt::WFlags f);
   virtual ~RMapMainWindow();

   void setKeyboardCallback(RoadMapKeyInput c);

   QMenu *newMenu(const char *title);
   void freeMenu(QMenu *menu);

   void addMenu(QMenu *menu, const char* label);
   void popupMenu(QMenu *menu, int x, int y);

   void addMenuItem(QMenu *menu,
                    const char* label,
                    const char* tip, 
                    RoadMapCallback callback);

   void addMenuSeparator(QMenu *menu);

   void addToolbar(const char* orientation);

   void addTool(const char* label, const char *icon, const char* tip,
      RoadMapCallback callback);

   void addToolSpace(void);
   void addCanvas(void);
   void addInput(int fd, RoadMapQtInput callback);
   void removeInput(int fd);
   void setStatus(const char* text);

   static void signalHandler(int sig);

public slots:
   void handleSignal();

private:
   QSocketNotifier *snSignal;

protected:
   RoadMapKeyInput keyCallback;
   QMap<int, RMapInput*> inputMap;
   QToolBar* toolBar;
   RMapCanvas* canvas;

   bool spacePressed;

   virtual void keyPressEvent(QKeyEvent* event);
   virtual void keyReleaseEvent(QKeyEvent* event);
   virtual void closeEvent(QCloseEvent* ev);

};

extern RMapMainWindow* mainWindow;
#endif
