/* qt_fileselection.cc - A QT implementation for the RoadMap file dialogs
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
 *   See qt_fileselection.h
 */
#include "qt_fileselection.h"
#include <qlayout.h>
#include <qmessagebox.h>

// Implementation of FileSelection class
FileSelector::FileSelector(QWidget* parent, const char* title, const char* flt,
   const char* path, const char* mod, RoadMapFileCallback cb): 
   QDialog(parent, title, TRUE) {

   filter = flt;
   mode = mod;
   callback = cb;
   fileEdit = 0;

   setCaption(title);

   if (QFileInfo(path).exists()) {
      currentDir.setPath(path);
   } else {
      currentDir.setPath(QDir::currentDirPath());
   }

   QVBoxLayout* l = new QVBoxLayout(this);
   l->setSpacing(4);
   l->setMargin(2);
   

   dirList = new QComboBox(this);
   dirList->setEditable(false);
   l->addWidget(dirList);

   fileList = new QListView(this);
   fileList->setSorting(2, FALSE);
   fileList->setSelectionMode(QListView::Single);
   fileList->setAllColumnsShowFocus(TRUE);
   fileList->addColumn(tr("Name"));
   fileList->addColumn(tr("Size"));
   fileList->setColumnAlignment(1, Qt::AlignRight);
   l->addWidget(fileList);

   if (strcmp(mode, "w") == 0) {
      fileEdit = new QLineEdit(this);
      l->addWidget(fileEdit);
   }

   connect(dirList, SIGNAL(activated(const QString&)), 
      this, SLOT(onDirSelect(const QString&)));
   connect(fileList, SIGNAL(clicked(QListViewItem*)), 
      this, SLOT(onFileSelect(QListViewItem*)));

   if (parent != 0) {
      resize(parent->width(), parent->height());
   }

   populateList();
}

FileSelector::~FileSelector() {
}

void FileSelector::populateList() {
   // populate dir list
   dirList->clear();

   QString dir = currentDir.canonicalPath();
   char sep = QDir::separator();

   while (dir.length() > 0) {
      dirList->insertItem(dir);

      if (dir == "/") {
         break;
      }

      int n = dir.findRev(sep);
      if (n > 0) {
         dir = dir.left(n);
      } else if (n == 0) {
         dir = dir.left(1);
      } else {
         dir = "";
      }
   }

   // populate file list
   currentDir.setFilter(QDir::All | QDir::Readable);
   currentDir.setSorting(QDir::DirsFirst);
   currentDir.setMatchAllDirs(TRUE);
   if (filter.isEmpty()) {
      currentDir.setNameFilter(filter);
   }

   const QFileInfoList* fil = currentDir.entryInfoList();
   QFileInfoListIterator it(*fil);
   QFileInfo* fi;

   fileList->clear();
   while ((fi = it.current()) != 0) {
      if (fi->fileName() != ".") {
         QString fsize, fname;

         fsize.sprintf("%10d", fi->size());
         fname = fi->fileName();
         if (fi->isDir()) {
            fname += "/";
         }

         new QListViewItem(fileList, fname, fsize);
      }
      ++it;
   }

   fileList->setSorting(2, FALSE);
}
   
   
void FileSelector::onDirSelect(const QString& dir) {
   currentDir.setPath(dir);
   populateList();
}

void FileSelector::onFileSelect(QListViewItem* lvi) {
   QString fname = lvi->text(0);
   QFileInfo fi(currentDir, fname);

   while (fi.isSymLink()) {
      fi.setFile(fi.readLink());
   }

   if (fi.isDir()) {
      currentDir.setPath(currentDir.canonicalPath() + 
         QDir::separator() + fname);

      populateList();
   } else {
      if (fileEdit != 0) {
         fileEdit->setText(fname);
      }
   }
}

void FileSelector::accept() {
   QString fname = QString::null;

   if (fileEdit != 0) {
      fname = fileEdit->text();
   } else {
      QListViewItem* lvi = fileList->selectedItem();

      if (lvi != 0) {
         fname = lvi->text(0);
      }
   }

   if (fname.isNull() || fname.isEmpty()) {
      QMessageBox::critical(this, "Error", "Please choose filename");
      return;
   }

   QFileInfo f(currentDir, fname);
   fname = f.absFilePath();

   if (strcmp(mode, "r") == 0) {
      if (!f.exists()) {
         QMessageBox::critical(this, "Error", "The file doesn't exist");
         return;
      }
   }

   if (!fname.isNull() && callback != 0) {
      callback((const char*) fname.local8Bit(), mode);
   }

   QDialog::accept();
}

void FileSelector::resizeEvent(QResizeEvent* e) {
   fileList->setColumnWidth(1,(fileList->width())/4);

   fileList->setColumnWidth(0,fileList->width() - 20 - 
      fileList->columnWidth(1));
}
