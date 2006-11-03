/* qt_canvas.h - The interface for the RoadMap/QT canvas class.
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

#ifndef INCLUDE__ROADMAP_QT_CANVAS__H
#define INCLUDE__ROADMAP_QT_CANVAS__H

#include <qwidget.h>
#include <qpen.h>
#include <qmap.h>
#include <qpixmap.h>
#include <qpainter.h>

extern "C" {

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
   
#include "roadmap_canvas.h"

   struct roadmap_canvas_pen {
      QPen* pen;
   };
};

class RMapCanvas : public QWidget {

Q_OBJECT

public:
   RMapCanvas(QWidget* parent);
   virtual ~RMapCanvas();

   RoadMapPen createPen(const char* name);
   void selectPen(RoadMapPen);
   void setPenColor(const char* color);
   void setPenThickness(int thickness);
   void erase(void);
   void drawString(RoadMapGuiPoint* position, int corner,
      const char* text);
   void drawStringAngle(RoadMapGuiPoint* position,
      int center, const char* text, int angle);
   void drawMultiplePoints(int count, RoadMapGuiPoint* points);
   void drawMultipleLines(int count, int* lines, RoadMapGuiPoint* points);
   void drawMultiplePolygons(int count, int* polygons, 
      RoadMapGuiPoint* points, int filled);
   void drawMultipleCircles(int count, RoadMapGuiPoint* centers, 
      int* radius, int filled);
   void registerButtonPressedHandler(RoadMapCanvasMouseHandler handler);
   void registerButtonReleasedHandler(RoadMapCanvasMouseHandler handler);
   void registerMouseMoveHandler(RoadMapCanvasMouseHandler handler);
   void registerMouseWheelHandler(RoadMapCanvasMouseHandler handler);

   void registerConfigureHandler(RoadMapCanvasConfigureHandler handler);
   void getTextExtents(const char* text, int* width, int* ascent, 
      int* descent, int *can_tilt);

   int getHeight();
   int getWidth();
   void refresh(void);

   void configure();

protected:
   QMap<QString, QColor*> colors;
   QMap<QString, RoadMapPen> pens;
   QPen* currentPen;
   QPixmap* pixmap;
   RoadMapCanvasConfigureHandler configureHandler;
   RoadMapCanvasMouseHandler buttonPressedHandler;
   RoadMapCanvasMouseHandler buttonReleasedHandler;
   RoadMapCanvasMouseHandler mouseMoveHandler;
   RoadMapCanvasMouseHandler mouseWheelHandler;


   void initColors();

   QColor getColor(const char* color);
   virtual void mousePressEvent(QMouseEvent*);
   virtual void mouseReleaseEvent(QMouseEvent*);
   virtual void mouseMoveEvent(QMouseEvent*);
   virtual void wheelEvent(QWheelEvent*);
   virtual void resizeEvent(QResizeEvent*);
   virtual void paintEvent(QPaintEvent*);
};

extern RMapCanvas *roadMapCanvas;
extern RoadMapCanvasMouseHandler phandler;
extern RoadMapCanvasMouseHandler rhandler;
extern RoadMapCanvasMouseHandler mhandler;
extern RoadMapCanvasMouseHandler whandler;

extern RoadMapCanvasConfigureHandler chandler;

#endif
