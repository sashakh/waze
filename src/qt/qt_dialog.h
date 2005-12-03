/* qt_dialog.h - The interface for the RoadMap/QT dialog class.
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

#include <qdialog.h>
#include <qwidget.h>
#include <qlist.h>
#include <qarray.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qvector.h>
#include <qlist.h>
#include <qmap.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_dialog.h"

struct Item {
	QString label;
	void* value;
};

class RMapDialog;
		
class Entry : public QObject {

Q_OBJECT

protected:
	RMapDialog* dialog;
	int type;
	QString name;
	RoadMapDialogCallback callback;
	QVector<Item> items;
   int current;
	QWidget* widget;

public:
 	enum {
		TextEntry = 1,
		ColorEntry,
		ChoiceEntry,
		ListEntry,
		ButtonEntry,
		LabelEntry,
	};

	Entry(RMapDialog* dialog, int type, QString name);
	Entry(RMapDialog* dialog, int type, QString name, QVector<Item>& items,
		int current, RoadMapDialogCallback callback);

	~Entry();

	QWidget* create(QWidget* parent);
	QString getName();

	void* getValue();
	void setValue(const void*);

	QWidget* getWidget() {
		return widget;
	}

	void setValues(QVector<Item>& items, 
		RoadMapDialogCallback callback);

protected slots:
	void run();
};

class RMapDialog : public QDialog {
Q_OBJECT

public:
	RMapDialog(QWidget* parent, const char* name);
	virtual ~RMapDialog();

	void addTextEntry(const char* frame, const char* name);
	void addLabelEntry(const char* frame, const char* name);
	void addColorEntry(const char* frame, const char* name);
	void addChoiceEntry(const char* frame,
                       const char* name,
                       int count,
                       int current,
                       char** labels,
                       void** values,
                       RoadMapDialogCallback callback);

	void addListEntry(const char* frame, const char* name);
	void setListEntryValues(const char* frame, const char* name, int count,
		char** labels, void** values, RoadMapDialogCallback callback);

	void addButton(char* label, RoadMapDialogCallback callback);
	void complete(int use_keyboard);

	void* getEntryValue(const char* frame, const char* name);
	void setEntryValue(const char* frame, const char* name, const void* data);

	void setContext(void*);
	void* getContext() {
		return context;
	}

protected:
	QMap<QString, QList<Entry> *> frames;
	QList<QString> frameNames;
	QList<Entry> buttons;
	void* context;

	QList<Entry>* getFrame(QString frameName);
	Entry* getEntry(QString frameName, QString entryName);
	void initTab(QWidget* w, QList<Entry>* entries);
};

