/* qt_main.cc - A QT implementation for the RoadMap main function.
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
 *   See qt_main.h
 */
#include "qt_main.h"

// Implementation of RMapInput class
RMapInput::RMapInput(int fd1, RoadMapInput cb) {
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
		callback();
	}
}

// Implementation of RMapMainWindow class
RMapMainWindow::RMapMainWindow(const char* name) : QMainWindow(0, name) {
	currentMenu = 0;
	spacePressed = false;
	tm = 0;
	tcb = 0;
	setCaption(name);
	canvas = new RMapCanvas(this);
	setCentralWidget(canvas);
	canvas->setFocus();
	setToolBarsMovable(FALSE);
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

void RMapMainWindow::addMenu(const char* label) {
	currentMenu = new QPopupMenu(this);

	menuBar()->insertItem(label, currentMenu);
}

void RMapMainWindow::addMenuItem(const char* label, const char* tip,
	RoadMapCallback callback) {

	if (currentMenu == 0) {
		roadmap_log (ROADMAP_FATAL, "No menu defined for menu item %s", label);
	}

	RMapCallback* cb = new RMapCallback(callback);
	currentMenu->insertItem(label, cb, SLOT(fire()));
}

void RMapMainWindow::addMenuSeparator() {
	if (currentMenu == 0) {
		roadmap_log (ROADMAP_FATAL, "No menu defined for menu separator");
	}

	currentMenu->insertSeparator();
}

void RMapMainWindow::addTool(const char* label, const char* tip,
	RoadMapCallback callback) {

#ifndef QWS
	// For some unknown reason, this toolbar crashes RoadMap
	// on the Sharp Zaurus. This should be fixed and the ifndef
	// removed.

	if (toolBar == 0) {
		toolBar = new QToolBar(this);
		toolBar->setFocusPolicy(QWidget::NoFocus);
		toolBar->setHorizontalStretchable(TRUE);
	}

	QPushButton* b = new QPushButton(label, toolBar);
	b->setFocusPolicy(QWidget::NoFocus);
	RMapCallback* cb = new RMapCallback(callback);

	connect(b, SIGNAL(clicked()), cb, SLOT(fire()));
#endif
}	

void RMapMainWindow::addToolSpace(void) {

#ifndef QWS
	// For some unknown reason, this toolbar crashes RoadMap
	// on the Sharp Zaurus. This should be fixed and the ifndef
	// removed.

	if (toolBar == 0) {
		toolBar = new QToolBar(this);
		toolBar->setFocusPolicy(QWidget::NoFocus);
		toolBar->setHorizontalStretchable(TRUE);
	}

	toolBar->addSeparator();
#endif
}


void RMapMainWindow::addCanvas(void) {
	canvas->configure();
	adjustSize();
}

void RMapMainWindow::addInput(int fd, RoadMapInput callback) {
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

void RMapMainWindow::setTimer(int interval, RoadMapCallback callback) {
	if (tm != 0) {
		roadmap_log (ROADMAP_ERROR, "no support for multiple timers");
		return;
	}

	tm = new QTimer(this);
	tcb = new RMapCallback(callback);
	connect(tm, SIGNAL(timeout()), tcb, SLOT(fire()));
	tm->start(interval, FALSE);
}

void RMapMainWindow::removeTimer(RoadMapCallback callback) {
	if (tm == 0) {
		roadmap_log (ROADMAP_ERROR, "no timer set");
		return;
	}

	tm->stop();
	delete tm;
	delete tcb;

	tm = 0;
	tcb = 0;
}

