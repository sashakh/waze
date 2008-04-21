/* qt_progress.cc - progress bar for qt
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
#include <qboxlayout.h>
#include <qprogressbar.h>
#include <qapplication.h>

#include "qt_progress.h"

RMapProgressDialog::RMapProgressDialog(QWidget * parent, Qt::WindowFlags f)
:  QDialog(parent, f | Qt::CustomizeWindowHint)
{
    progress = new QProgressBar(this);
    setWindowTitle("please wait...");
    QVBoxLayout *vl = new QVBoxLayout;
    progress->setMinimumSize(QSize(150, 0));
    vl->addWidget(progress);
    setLayout(vl);
    adjustSize();
    setModal(false);
}

RMapProgressDialog::~RMapProgressDialog()
{
}

void
RMapProgressDialog::setMaximum(int value)
{
    if (progress->maximum() != value)
	progress->setMaximum(value);
}

void
RMapProgressDialog::setProgress(int value)
{
    if (progress->value() != value)
	progress->setValue(value);
    qApp->processEvents();
}
