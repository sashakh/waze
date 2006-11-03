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
#include "qt_fileselection.h"
#include "qt_main.h"

#ifndef QWS
#include <qfiledialog.h>
#endif

void roadmap_fileselection_new (const char *title, const char *filter, 
   const char *path, const char *mode, RoadMapFileCallback callback) {

#ifdef QWS

   FileSelector fselect(mainWindow, title, filter, path, 
      mode, callback);

   fselect.exec();

#else

   QFileDialog *dlg = new QFileDialog( path, QString::null, 0, 0, TRUE );

   dlg->setCaption( QFileDialog::tr( title ) );
   if (mode[0] == 'w') {
      dlg->setMode( QFileDialog::AnyFile );
   } else {
      dlg->setMode( QFileDialog::ExistingFile );
   }

   if ( dlg->exec() == QDialog::Accepted ) {

      QString result;
      result = dlg->selectedFile();
      delete dlg;
      callback (result, mode);

   } else {
      delete dlg;
   }
#endif
}
