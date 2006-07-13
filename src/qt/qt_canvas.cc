/* qt_canvas.cc - A QT implementation for the RoadMap canvas
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
 *   See qt_canvas.h
 */

#include <stdio.h>
#include <stdlib.h>
#include "qt_canvas.h"


RMapCanvas *roadMapCanvas = 0;
RoadMapCanvasMouseHandler phandler = 0;
RoadMapCanvasMouseHandler rhandler = 0;
RoadMapCanvasMouseHandler mhandler = 0;
RoadMapCanvasMouseHandler whandler = 0;
RoadMapCanvasConfigureHandler chandler = 0;

// Implementation of RMapCanvas class
RMapCanvas::RMapCanvas(QWidget* parent):QWidget(parent) {
	pixmap = 0;
	currentPen = 0;
	roadMapCanvas = this;

   initColors();

   registerButtonPressedHandler(phandler);
   registerButtonReleasedHandler(rhandler);
   registerMouseMoveHandler(mhandler);
   registerMouseWheelHandler(whandler);

	registerConfigureHandler(chandler);
	setBackgroundMode(QWidget::NoBackground);
}

RMapCanvas::~RMapCanvas() {
	if (pixmap != 0) {
		delete pixmap;
		pixmap = 0;
	}

	// TODO: delete pens
}

RoadMapPen RMapCanvas::createPen(const char* name) {
	RoadMapPen p = pens[name];

	if (p == 0) {
		QPen* pen = new QPen(Qt::SolidLine/*Qt::DotLine*/);
		p = new roadmap_canvas_pen();
		p->pen = pen;
		pens.insert(name, p);
	}

	currentPen = p->pen;

	return p;
}

void RMapCanvas::selectPen(RoadMapPen p) {
	currentPen = p->pen;
}

void RMapCanvas::setPenColor(const char* color) {
	if (currentPen != 0) {
		currentPen->setColor(getColor(color));
	}

}

void RMapCanvas::setPenThickness(int thickness) {
	if (currentPen != 0) {
		currentPen->setWidth(thickness);
	}
}

void RMapCanvas::erase() {
	if (pixmap) {
		pixmap->fill(currentPen->color());
	}
}

void RMapCanvas::getTextExtents(const char* text, int* w, int* ascent,
	int* descent, int *can_tilt) {

	QFont f("Arial Bold",12);
	QFontMetrics fm(f);
	
	QRect r = fm.boundingRect(QString::fromUtf8(text));
	*w = r.width();
	*ascent = fm.ascent();
	*descent = fm.descent();
#ifdef QT_NO_ROTATE
	if (can_tilt) *can_tilt = 0;
#else
	if (can_tilt) *can_tilt = 1;
#endif

}

void RMapCanvas::drawString(RoadMapGuiPoint* position, 
		int corner, const char* text) {
	if (!pixmap) {
		return;
	}

	QPainter p(pixmap);
	if (currentPen != 0) {
		p.setPen(*currentPen);
	}
        QFont f("Arial Bold",12);
        p.setFont(f);
                
	int text_width;
	int text_ascent;
	int text_descent;
	int x, y;

	getTextExtents(text, &text_width, &text_ascent, &text_descent, NULL);

	switch (corner) {

	case ROADMAP_CANVAS_TOPLEFT:
		y = position->y + text_ascent;
		x = position->x;
		break;

	case ROADMAP_CANVAS_TOPRIGHT:
		y = position->y + text_ascent;
		x = position->x - text_width;
		break;

	case ROADMAP_CANVAS_BOTTOMRIGHT:
		y = position->y - text_descent;
		x = position->x - text_width;
		break;

	case ROADMAP_CANVAS_BOTTOMLEFT:
		y = position->y - text_descent;
		x = position->x;
		break;

	case ROADMAP_CANVAS_CENTER:
		y = position->y + (text_ascent / 2);
		x = position->x - (text_width / 2);
		break;

	default:
		return;
	}

	p.drawText(x, y, QString::fromUtf8(text));
}

void RMapCanvas::drawStringAngle(RoadMapGuiPoint* position,
		int center, const char* text, int angle) {
#ifndef QT_NO_ROTATE
	if (!pixmap) {
		return;
	}

	QPainter p(pixmap);
	if (currentPen != 0) {
		p.setPen(*currentPen);
	}

	QFont f("Arial Bold",12);

	p.setFont(f);
	p.translate(position->x,position->y);
	p.rotate((double)angle);
	p.drawText(0, 0, QString::fromUtf8(text));
#endif
}

void RMapCanvas::drawMultiplePoints(int count, RoadMapGuiPoint* points) {
	QPainter p(pixmap);
	if (currentPen != 0) {
		p.setPen(*currentPen);
	}

	QPointArray pa(count);
	for(int n = 0; n < count; n++) {
		pa.setPoint(n, points[n].x, points[n].y);
	}

	p.drawPoints(pa);
}

void RMapCanvas::drawMultipleLines(int count, int* lines, RoadMapGuiPoint* points) {
	QPainter p(pixmap);
	if (currentPen != 0) {
		p.setPen(*currentPen);
	}

	for(int i = 0; i < count; i++) {
		int count_of_points = *lines;
		QPointArray pa(count_of_points);
		for(int n = 0; n < count_of_points; n++) {
			pa.setPoint(n, points[n].x, points[n].y);
		}

		p.drawPolyline(pa);

		lines++;
		points += count_of_points;
	}
}

void RMapCanvas::drawMultiplePolygons(int count, int* polygons, 
		RoadMapGuiPoint* points, int filled) {

	QPainter p(pixmap);
	if (currentPen != 0) {

		if (filled) {
			p.setPen(/* *currentPen*/ QPen(QPen::NoPen));
			p.setBrush(QBrush(currentPen->color()));
		} else {
			p.setPen(*currentPen);
		}

	}

	for(int i = 0; i < count; i++) {
		int count_of_points = *polygons;

		QPointArray pa(count_of_points);
		for(int n = 0; n < count_of_points; n++) {
			pa.setPoint(n, points[n].x, points[n].y);
		}

		p.drawPolygon(pa);

		polygons++;
		points += count_of_points;
	}
}

void RMapCanvas::drawMultipleCircles(int count, RoadMapGuiPoint* centers,
		int* radius, int filled) {

	QPainter p(pixmap);
	if (currentPen != 0) {
		if (filled) {
			p.setPen(*currentPen);
			p.setBrush(QBrush(currentPen->color()));
		} else {
			p.setPen(*currentPen);
		}

	}

	for(int i = 0; i < count; i++) {
		int r = radius[i];

		p.drawEllipse(centers[i].x - r, centers[i].y - r, 2*r, 2*r);
		if (filled) {
			p.drawChord(centers[i].x - r + 1,
				centers[i].y - r + 1,
				2 * r, 2 * r, 0, 16*360);
		}
	}
}

void RMapCanvas::registerButtonPressedHandler(RoadMapCanvasMouseHandler handler) {
  buttonPressedHandler = handler;
}

void RMapCanvas::registerButtonReleasedHandler(RoadMapCanvasMouseHandler handler) {
  buttonReleasedHandler = handler;
}

void RMapCanvas::registerMouseMoveHandler(RoadMapCanvasMouseHandler handler) {
  mouseMoveHandler = handler;
}

void RMapCanvas::registerMouseWheelHandler(RoadMapCanvasMouseHandler handler) {
  mouseWheelHandler = handler;
}


void RMapCanvas::registerConfigureHandler(RoadMapCanvasConfigureHandler handler) {
	configureHandler = handler;
}

int RMapCanvas::getHeight() {
	return height();
}

int RMapCanvas::getWidth() {
	return width();
}

void RMapCanvas::refresh(void) {
	update();
}

void RMapCanvas::mousePressEvent(QMouseEvent* ev) {

   int button;
   RoadMapGuiPoint pt;

   switch (ev->button()) {
      case LeftButton:  button = 1; break;
      case MidButton:   button = 2; break;
      case RightButton: button = 3; break;
      default:          button = 0; break;
   }
   pt.x = ev->x();
   pt.y = ev->y();

   if (buttonPressedHandler != 0) {
      buttonPressedHandler(button, &pt);
   }
}

void RMapCanvas::mouseReleaseEvent(QMouseEvent* ev) {

   int button;
   RoadMapGuiPoint pt;

   switch (ev->button()) {
      case LeftButton:  button = 1; break;
      case MidButton:   button = 2; break;
      case RightButton: button = 3; break;
      default:          button = 0; break;
   }
   pt.x = ev->x();
   pt.y = ev->y();

   if (buttonReleasedHandler != 0) {
      buttonReleasedHandler(button, &pt);
   }
}

void RMapCanvas::mouseMoveEvent(QMouseEvent* ev) {

   RoadMapGuiPoint pt;

   pt.x = ev->x();
   pt.y = ev->y();

   if (mouseMoveHandler != 0) {
      mouseMoveHandler(0, &pt);
   }
}

void RMapCanvas::wheelEvent (QWheelEvent* ev) {

   int direction;

   RoadMapGuiPoint pt;

   pt.x = ev->x();
   pt.y = ev->y();

   direction = ev->delta();
   direction = (direction > 0) ? 1 : ((direction < 0) ? -1 : 0);

   if (mouseWheelHandler != 0) {
      mouseWheelHandler(direction, &pt);
   }
}

void RMapCanvas::resizeEvent(QResizeEvent* ev) {
	configure();
}

void RMapCanvas::paintEvent(QPaintEvent* ev) {

	bitBlt(this, QPoint(0,0), pixmap, QRect(0, 0, pixmap->width(), pixmap->height()));
}

void RMapCanvas::configure() {
	if (pixmap != 0) {
		delete pixmap;
	}

	pixmap = new QPixmap(width(), height());

	if (configureHandler != 0) {
		configureHandler();
	}
}

QColor RMapCanvas::getColor(const char* color) {
	QColor *c = colors[color];

	if (c == 0) {
		c = new QColor(color);
      colors.insert(color, c);
	}

	return *c;
}

void RMapCanvas::initColors() {
#ifdef QWS
// It seems that QPE does not have predefined named colors.
// Temporary fix is to hard-code some. Better solution
// is to read rgb.txt??

   colors.insert("black", new QColor(0, 0, 0));
   colors.insert("blue", new QColor(0, 0, 255));
   colors.insert("DarkGrey", new QColor(169, 169, 169));
   colors.insert("green", new QColor(0, 255, 0));
   colors.insert("grey", new QColor(190, 190, 190));
   colors.insert("IndianRed", new QColor(205, 92, 92));
   colors.insert("LightBlue", new QColor(173, 216, 230));
   colors.insert("LightSlateBlue", new QColor(132, 112, 255));
   colors.insert("LightYellow", new QColor(255, 255, 224));
   colors.insert("red", new QColor(255, 0, 0));
   colors.insert("white", new QColor(255, 255, 255));
   colors.insert("yellow", new QColor(255, 255, 0));
#endif
}

