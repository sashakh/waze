/* qt_fileselection.h - The interface for the RoadMap/QT file dialog class.
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

#ifndef INCLUDE__ROADMAP_QT_FILESELECTION__H
#define INCLUDE__ROADMAP_QT_FILESELECTION__H

extern "C" {

#include "roadmap_fileselection.h"
 
#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_file.h"

};

#include <qfile.h>
#include <qdir.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlistview.h>
#include <qlineedit.h>

class FileSelector : public QDialog {

Q_OBJECT

public:
	FileSelector(QWidget* parent, const char* title, const char* filter, 
		const char* path, const char* mode, RoadMapFileCallback cb);

	~FileSelector();

protected:
	QComboBox* dirList;
	QListView* fileList;
	QLineEdit* fileEdit;
//	QPushButton* okButton;
//	QPishButton* cancelButton;
	QDir currentDir;
	QFile currentFile;
	RoadMapFileCallback callback;
	QString filter;
	const char* mode;

	void populateList();
	virtual void resizeEvent(QResizeEvent*);

protected slots:
	void onDirSelect(const QString& dir);
	void onFileSelect(QListViewItem* lvi);
	void accept();
};

#endif
