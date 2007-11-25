/* roadmap_fileselection.cc - A C to C++ wrapper for the QT RoadMap file dialogs
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
 *   See roadmap_fileselection.h
 */
extern "C" {

#include "roadmap_fileselection.h"
}
 
#include <qfiledialog.h>


void roadmap_fileselection_new (const char *title, const char *filter, 
   const char *path, const char *mode, RoadMapFileCallback callback) {

#ifndef QWS4
   QFileDialog *dlg = new QFileDialog(0, title );

   if (mode[0] == 'w') {
      dlg->setFileMode( QFileDialog::AnyFile );
   } else {
      dlg->setFileMode( QFileDialog::ExistingFile );
   }

   dlg->setViewMode(QFileDialog::Detail);
   if ( dlg->exec() ) {

      QString result;
      result = dlg->selectedFiles().first();
      delete dlg;
      callback (result.toLatin1().data(), mode);

   } else {
      delete dlg;
   }
#endif
}
