/* qt_progress.h - progress bar for qt
 *
 * LICENSE:
 *
 *   (c) Copyright 2008 Alessandro Briosi
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
 */

#ifndef INCLUDE__ROADMAP_QT_PROGRESS_H
#define INCLUDE__ROADMAP_QT_PROGRESS_H
#include <qdialog.h>

class QProgressBar;

class RMapProgressDialog : public QDialog {
Q_OBJECT
public:
   RMapProgressDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
   ~RMapProgressDialog();

   void setMaximum(int value);
   void setProgress(int value);
private:
   QProgressBar *progress;
};

#endif /* INCLUDE__ROADMAP_QT_PROGRESS_H */
