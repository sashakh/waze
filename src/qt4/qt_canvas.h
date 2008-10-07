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
#include <qevent.h>

extern "C" {

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
   
#include "roadmap_canvas.h"

   struct roadmap_canvas_pen {
      QPen* pen;
      QFont* font;
#if defined(ROADMAP_ADVANCED_STYLE)
      QBrush* brush;
      int capitalize;
      int background;
      QColor* fontcolor;
      QColor* buffercolor;
      int buffersize;
#endif
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
   void setPenLineStyle(const char* style);
   void setPenThickness(int thickness);
   void setFontSize(int size);

#if defined(ROADMAP_ADVANCED_STYLE)
   int getPenThickness(RoadMapPen p);
   void setPenOpacity(int opacity);
   void setPenLighter(int factor);
   void setPenDarker(int factor);
   void setPenLineStyle(int style);
   void setPenLineCapStyle(int cap);
   void setPenLineJoinStyle(int join);
   void setBrushColor(const char *color);
   void setBrushStyle(int style);
   void setBrushOpacity(int opacity);
   void setFontName(const char *name);
   void setFontColor(const char *color);
   void setFontWeight(int weight);
   void setFontSpacing(int spacing);
   void setFontItalic(int italic);
   void setFontStrikeOut(int strikeout);
   void setFontUnderline(int underline);
   void setFontCapitalize(int capitalize);
   void setBrushAsBackground(int background);
   void setFontBufferColor(const char *color);
   void setFontBufferSize(int size);
   int getFontBufferSize(RoadMapPen pen);
#endif /* ROADMAP_ADVANCED_STYLE */

   void erase(void);
   void setupPainterPen(QPainter &p);
   void drawString(RoadMapGuiPoint* position, int corner,
      const char* text);
   void drawStringAngle(RoadMapGuiPoint* position,
      int center, const char* text, int angle);
   void drawMultiplePoints(int count, RoadMapGuiPoint* points);
   void drawMultipleLines(int count, int* lines, RoadMapGuiPoint* points,
                          int fast_draw);
   void drawMultiplePolygons(int count, int* polygons, 
      RoadMapGuiPoint* points, int filled, int fast_draw);
   void drawMultipleCircles(int count, RoadMapGuiPoint* centers, 
      int* radius, int filled, int fast_draw);
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
   RoadMapPen currentPen;
   RoadMapPen basePen;
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
