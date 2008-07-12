/*
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

/**
 * @file
 * @brief Qt implementation for the RoadMap main function.
 */
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "qt_main.h"

static int signalFd[2];

// Implementation of RMapInput class
RMapInput::RMapInput(int fd1, RoadMapQtInput cb) {
   fd = fd1;
   callback = cb;
   nf = new QSocketNotifier(fd, QSocketNotifier::Read, 0);
   connect(nf, SIGNAL(activated(int)), this, SLOT(fire(int)));
}

RMapInput::~RMapInput() {
   if (nf) {
      delete nf;
      nf = 0;
   }
}

void RMapInput::fire(int s) {
   if (callback != 0) {
      callback(s);
   }
}

// Implementation of RMapCallback class
RMapCallback::RMapCallback(RoadMapCallback cb) {
   callback = cb;
}

void RMapCallback::fire() {
   if (callback != 0) {
      roadmap_start_do_callback(callback);
   }
}

// Implementation of RMapTimerCallback class
RMapTimerCallback::RMapTimerCallback(RoadMapCallback cb) {
   callback = cb;
}

void RMapTimerCallback::fire() {
   if (callback != 0) {
      callback();
   }
}

int  RMapTimerCallback::same(RoadMapCallback cb) {
   return (callback == cb);
}

// Implementation of RMapMainWindow class
RMapMainWindow::RMapMainWindow(const char* name, int width, int height) : QMainWindow(0, name) {
   spacePressed = false;
   setCaption(QString::fromUtf8(name));
   canvas = new RMapCanvas(this);
   setCentralWidget(canvas);
   canvas->setFocus();
   setToolBarsMovable(FALSE);
   resize(width,height);
   toolBar = 0;

   // setup the signal handling
   if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalFd))
        qFatal("Couldn't create Signal socketpair");
   snSignal = new QSocketNotifier(signalFd[1], QSocketNotifier::Read, this);
   connect(snSignal, SIGNAL(activated(int)), this, SLOT(handleSignal()));
}

RMapMainWindow::~RMapMainWindow() {
   QMap<int, RMapInput*>::Iterator it;

   for(it = inputMap.begin(); it != inputMap.end(); ++it) {
      delete it.data();
      inputMap.remove(it);
   }
}

void RMapMainWindow::setKeyboardCallback(RoadMapKeyInput c) {
   keyCallback = c;
}


QPopupMenu *RMapMainWindow::newMenu(const char *title) {

   return new QPopupMenu(this, title);
}

void RMapMainWindow::freeMenu(QPopupMenu *menu) {

   delete (menu);
}

void RMapMainWindow::addMenu(QPopupMenu *menu, const char* label) {

   menuBar()->insertItem(label, menu);
}


void RMapMainWindow::popupMenu(QPopupMenu *menu, int x, int y) {

   if (menu != NULL) menu->popup (mapToGlobal(QPoint (x, y)));
}


void RMapMainWindow::addMenuItem(QPopupMenu *menu,
                                 const char* label,
                                 const char* tip,
                                 RoadMapCallback callback) {

   RMapCallback* cb = new RMapCallback(callback);
   menu->insertItem(label, cb, SLOT(fire()));
}

void RMapMainWindow::addMenuSeparator(QPopupMenu *menu) {

   menu->insertSeparator();
}

void RMapMainWindow::addToolbar(const char* orientation) {

   if (toolBar == 0) {
      toolBar = new QToolBar(this, "map view");
#ifndef QWS
      // moveDockWindow not available on QtE v2.3.10.
      switch (orientation[0]) {
         case 't':
         case 'T': break;

         case 'b':
         case 'B': moveDockWindow (toolBar, DockBottom); break;

         case 'l':
         case 'L': moveDockWindow (toolBar, DockLeft); break;

         case 'r':
         case 'R': moveDockWindow (toolBar, DockRight); break;

         default: roadmap_log (ROADMAP_FATAL,
                        "Invalid toolbar orientation %s", orientation);
      }
#endif
      toolBar->setFocusPolicy(QWidget::NoFocus);
      toolBar->setHorizontalStretchable(TRUE);
   }
}

void RMapMainWindow::addTool(const char* label,
                             const char *icon,
                             const char* tip,
                             RoadMapCallback callback) {

#ifndef QWS
   // For some unknown reason, this toolbar crashes RoadMap
   // on the Sharp Zaurus.
   // This should be fixed and the ifndef removed.
   // Pascal: I believe this has been fixed now.

   if (toolBar == 0) {
      addToolbar("");
   }

   if (label != NULL) {
      const char *icopath=roadmap_path_search_icon(icon);
      QPushButton* b;

      if (icopath)
       b = new QPushButton(QIconSet( QPixmap(icopath) ),NULL, toolBar);
      else
       b = new QPushButton(label, toolBar);
      
      QToolTip::add( b, QString::fromUtf8(tip) );
      b->setFocusPolicy(QWidget::NoFocus);
      RMapCallback* cb = new RMapCallback(callback);

      connect(b, SIGNAL(clicked()), cb, SLOT(fire()));
   }
#endif
}  

void RMapMainWindow::addToolSpace(void) {

#ifndef QWS
   // For some unknown reason, this toolbar crashes RoadMap
   // on the Sharp Zaurus. This should be fixed and the ifndef
   // removed.

   addTool (NULL, NULL, NULL, NULL);

   toolBar->addSeparator();
#endif
}


void RMapMainWindow::addCanvas(void) {
   canvas->configure();
   adjustSize();
}

void RMapMainWindow::addInput(int fd, RoadMapQtInput callback) {
   RMapInput* rmi = new RMapInput(fd, callback);
   inputMap.insert(fd, rmi);
}

void RMapMainWindow::removeInput(int fd) {
   RMapInput* rmi = inputMap[fd];

   if (rmi != 0) {
      inputMap.remove(fd);
      delete rmi;
   }
}

void RMapMainWindow::setStatus(const char* text) {
   statusBar()->message(text);
}

void RMapMainWindow::keyReleaseEvent(QKeyEvent* event) {
   int k = event->key();

   if (k == ' ') {
      spacePressed = false;
   }

   event->accept();
}

void RMapMainWindow::keyPressEvent(QKeyEvent* event) {
   char* key = 0;
   char regular_key[2];
   int k = event->key();

   switch (k) {
      case ' ':
         spacePressed = true;
         break;

      case Key_Left:
         if (spacePressed) {
            key = "Button-Calendar";
         } else {
            key = "Button-Left";
         }
         break;

      case Key_Right:
         if (spacePressed) {
            key = "Button-Contact";
         } else {
            key = "Button-Right";
         }
         break;

      case Key_Up:
         key = "Button-Up";
         break;

      case Key_Down:
         key = "Button-Down";
         break;

      default:
         if (k>0 && k<128) {
            regular_key[0] = k;
            regular_key[1] = 0;
            key = regular_key;
         }
   }

   if (key!=0 && keyCallback!=0) {
      keyCallback(key);
   }

   event->accept();
}

void RMapMainWindow::closeEvent(QCloseEvent* ev) {
   roadmap_main_exit();
   ev->accept();
}

void RMapMainWindow::signalHandler(int sig)
{
  write(signalFd[0], &sig, sizeof(sig));
}

void RMapMainWindow::handleSignal()
{
  snSignal->setEnabled(false);
  int tmp;
  read(signalFd[1], &tmp, sizeof(tmp));
  QString action;
  switch (tmp) {
    case SIGTERM: action="SIGTERM"; break;
    case SIGINT : action="SIGINT"; break;
    case SIGHUP : action="SIGHUP"; break;
    case SIGQUIT: action="SIGQUIT"; break;
  }
  roadmap_log(ROADMAP_WARNING,"received signal %s",action.latin1());
  roadmap_main_exit();
  snSignal->setEnabled(true);
}

// Implementation of the RMapTimers class
RMapTimers::RMapTimers (QObject *parent, const char *name)
  : QObject(parent, name)
{
   memset(tm, 0, sizeof(tm));
   memset(tcb, 0, sizeof(tcb));
}

RMapTimers::~RMapTimers()
{
   for (int i = 0 ; i < ROADMAP_MAX_TIMER; ++i) {
     if (tm[i] != 0)
       delete tm[i];
     if (tcb[i] != 0)
       delete tcb[i];
   }
}

void RMapTimers::addTimer(int interval, RoadMapCallback callback) {

   int empty = -1;

   for (int i = 0; i < ROADMAP_MAX_TIMER; ++i) {
      if (tm[i] == 0) {
         empty = i;
         break;
      } else if (tcb[i]->same(callback)) {
         return;
      }
   }

   if (empty < 0) {
      roadmap_log (ROADMAP_ERROR, "too many timers");
   }

   tm[empty] = new QTimer(this);
   tcb[empty] = new RMapTimerCallback(callback);
   connect(tm[empty], SIGNAL(timeout()), tcb[empty], SLOT(fire()));
   tm[empty]->start(interval, FALSE);
}

void RMapTimers::removeTimer(RoadMapCallback callback) {

   int found = -1;

   for (int i = 0; i < ROADMAP_MAX_TIMER; ++i) {
      if (tcb[i] != 0) {
         if (tcb[i]->same(callback)) {
            found = i;
            break;
         }
      }
   }
   if (found < 0) return;

   tm[found]->stop();
   delete tm[found];
   delete tcb[found];

   tm[found] = 0;
   tcb[found] = 0;
}
