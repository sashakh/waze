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
RoadMapCanvasButtonHandler bhandler = 0;
RoadMapCanvasConfigureHandler chandler = 0;

// Implementation of RMapCanvas class
RMapCanvas::RMapCanvas(QWidget* parent):QWidget(parent) {
	pixmap = 0;
	currentPen = 0;
	roadMapCanvas = this;

	registerButtonHandler(bhandler);
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
		QPen* pen = new QPen();
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
	int* descent) {

	QFont defaultFont;
	QFontMetrics fm(defaultFont);

	QRect r = fm.boundingRect(text);
	*w = r.width();
	*ascent = fm.ascent();
	*descent = fm.descent();
}

void RMapCanvas::drawString(RoadMapGuiPoint* position, int corner, const char* text) {
	if (!pixmap) {
		return;
	}

	QPainter p(pixmap);
	if (currentPen != 0) {
		p.setPen(*currentPen);
	}

	int text_width;
	int text_ascent;
	int text_descent;
	int x, y;

	getTextExtents(text, &text_width, &text_ascent, &text_descent);

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

	p.drawText(x, y, text);
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

void RMapCanvas::registerButtonHandler(RoadMapCanvasButtonHandler handler) {
	buttonHandler = handler;
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
	RoadMapGuiPoint pt;

	pt.x = ev->x();
	pt.y = ev->y();

	if (buttonHandler != 0) {
		buttonHandler(&pt);
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

