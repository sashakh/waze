/* qt_dialog.cc - A QT implementation for the RoadMap dialogs
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
 *   See qt_dialog.h
 */
#include <stdio.h>
#include "qt_dialog.h"
#include <qlayout.h>

// Implementation of RMapDialog class
RMapDialog::RMapDialog(QWidget* parent, const char* name):QDialog(parent, name) {
	setCaption(name);
}

RMapDialog::~RMapDialog() {
}

QList<Entry>* RMapDialog::getFrame(QString frameName) {
	QList<Entry>* frame = frames[frameName];

	if (frame == 0) {
		frame = new QList<Entry>;
		frameNames.append(new QString(frameName));
		frames.insert(frameName, frame);
	}

	return frame;
}

Entry* RMapDialog::getEntry(QString frameName, QString entryName) {
	QList<Entry>* frame = frames[frameName];

	if (frame == 0) {
		return 0;
	}

	Entry* entry;

	for(entry = frame->first(); entry != 0; entry = frame->next()) {
		if (entry->getName() == entryName) {
			break;
		}
	}

	return entry;
}

void RMapDialog::addTextEntry(const char* frameName, const char* name) {
	QList<Entry>* frame = getFrame(frameName);

	Entry* entry = new Entry(this, Entry::TextEntry, name);
	frame->append(entry);
}

void RMapDialog::addLabelEntry(const char* frameName, const char* name) {
	QList<Entry>* frame = getFrame(frameName);

	Entry* entry = new Entry(this, Entry::LabelEntry, name);
	frame->append(entry);
}

void RMapDialog::addColorEntry(const char* frameName, const char* name) {
	addTextEntry(frameName, name);
}

void RMapDialog::addChoiceEntry(const char* frameName, const char* name, int count,
	char** labels, void** values, RoadMapDialogCallback callback) {

	QList<Entry>* frame = getFrame(frameName);

	QVector<Item> items(count);
	for(int i = 0; i < count; i++) {
		Item* item = new Item();
		item->label = labels[i];
		item->value = values[i];
		items.insert(i, item);
	}

	Entry* entry = new Entry(this, Entry::ChoiceEntry, name, items, callback);
	frame->append(entry);
}

void RMapDialog::addListEntry(const char* frameName, const char* name) {
	QList<Entry>* frame = getFrame(frameName);

	Entry* entry = new Entry(this, Entry::ListEntry, name);
	frame->append(entry);
}

void RMapDialog::setListEntryValues(const char* frameName, const char* name, 
	int count, char** labels, void** values, RoadMapDialogCallback callback) {

	Entry* entry = getEntry(frameName, name);

	if (entry == 0) {
		return;
	}

	QVector<Item> items(count);
	for(int i = 0; i < count; i++) {
		Item* item = new Item();
		item->label = labels[i];
		item->value = values[i];
		items.insert(i, item);
	}


	entry->setValues(items, callback);
}

void RMapDialog::addButton(char* label, RoadMapDialogCallback callback) {
	QVector<Item> items;
	Entry* entry = new Entry(this, Entry::ButtonEntry, label, items, callback);
	buttons.append(entry);
}

void RMapDialog::complete(int) {
	QVBoxLayout* main = new QVBoxLayout(this);
	QWidget* topw = 0;
	main->setSpacing(4);
	main->setMargin(2);

	if (frames.count() > 1) {
		QTabWidget* tw = new QTabWidget(this);
		for(QString* name = frameNames.first(); name != 0; 
			name = frameNames.next()) {

			QList<Entry>* frame = frames[*name];
			QWidget* w = new QWidget(tw);
			initTab(w, frame);
			tw->addTab(w, *name);
		}

		topw = tw;
	} else {
		topw = new QWidget(this);
		initTab(topw, *frames.begin());
	}

	QWidget* bw = new QWidget(this);
	QHBoxLayout* btns = new QHBoxLayout(bw);
	for(Entry* entry = buttons.first(); entry != 0; entry = buttons.next()) {
		QWidget* w = entry->create(bw);
		btns->addWidget(w);
	}

	main->addWidget(topw);
	main->addWidget(bw);
	adjustSize();

	show();
}

void* RMapDialog::getEntryValue(const char* frame, const char* name) {
	Entry* entry = getEntry(frame, name);

	if (entry == 0) {
		return 0;
	}

	return entry->getValue();
}

void RMapDialog::setEntryValue(const char* frame, const char* name, const void* data) {
	Entry* entry = getEntry(frame, name);

	if (entry == 0) {
		return;
	}

	return entry->setValue(data);
}

void RMapDialog::initTab(QWidget* tab, QList<Entry>* entries) {
	QGridLayout* grid = new QGridLayout(tab, entries->count(), 2, 2, 5);
	int i = 0;

        for(Entry* entry = entries->first(); entry != 0; entry = entries->next(), i++) {
		QWidget* w = entry->create(tab);
		QLabel* l;
      
      if (entry->getName()[0] == '.') {
         grid->addMultiCellWidget(w, i, i, 0, 1);
      } else {
         l = new QLabel(entry->getName(), tab, entry->getName());
         grid->addWidget(l, i, 0);
         grid->addWidget(w, i, 1);
      }
	}
}

void RMapDialog::setContext(void* ctx) {
	context = ctx;
}

// Implementation of Entry class
Entry::Entry(RMapDialog* dlg, int etype, QString ename) {
	dialog = dlg;
	type = etype;
	name = ename;
	callback = 0;
	widget = 0;
}

Entry::Entry(RMapDialog* dlg, int etype, QString ename, QVector<Item>& eitems,
	RoadMapDialogCallback ecallback) {

	dialog = dlg;
	type = etype;
	name = ename;
	callback = ecallback;

	items.resize(eitems.count());
	for(uint i = 0; i < eitems.count(); i++) {
		items.insert(i, eitems[i]);
	}
}

Entry::~Entry() {
	for(uint i = 0; i < items.count(); i++) {
		delete items[i];
	}
}

QWidget* Entry::create(QWidget* parent) {

	widget = 0;
	
	switch (type) {
		case TextEntry:
			widget = new QLineEdit(parent, name);
			break;

		case ColorEntry:
			widget = new QLineEdit(parent, name);
			break;

		case ChoiceEntry: 
			{
				QComboBox* cb = new QComboBox(parent, name);
				cb->setEditable(false);
	
				for(uint i = 0; i < items.count(); i++) {
					cb->insertItem(items[i]->label);
				}

				connect(cb, SIGNAL(activated(int)), this, SLOT(run()));
	
				widget = cb;
			}
			break;

		case ListEntry: {
			QListBox* lb = new QListBox(parent, name);
			widget = lb;

			// ugly hack ... 
			lb->setMinimumHeight(200);
			lb->setMinimumWidth(150);

			connect(widget, SIGNAL(highlighted(int)), this, SLOT(run()));
			}
			break;

		case ButtonEntry:
			widget = new QPushButton(name, parent, name);
			connect(widget, SIGNAL(clicked()), this, SLOT(run()));
			break;

		case LabelEntry:
			widget = new QLabel(parent, name);
         ((QLabel *)widget)->setAlignment (AlignRight|AlignVCenter|ExpandTabs);
			break;
	}

	return widget;
}

QString Entry::getName() {
	return name;
}

void* Entry::getValue() {
	void* ret = 0;

	switch (type) {
		case TextEntry: {
			QString s = ((QLineEdit*) widget)->text();
			const char* ss = s.latin1();
			ret = (void *) ss;
			}
			break;

		case ColorEntry:
			ret = (void *) (const char*) ((QLineEdit*) widget)->text().latin1();
			break;

		case ChoiceEntry:
			ret = items[((QComboBox*) widget)->currentItem()]->value;
			break;

		case ListEntry:
			ret = items[((QListBox*) widget)->currentItem()]->value;
			break;
	}

	return ret;
}

void Entry::setValue(const void* val) {
	switch (type) {
		case TextEntry:
			((QLineEdit*) widget)->setText((char*) val);
			break;

		case ColorEntry:
			((QLineEdit*) widget)->setText((char*) val);
			break;

		case ChoiceEntry: 
			items[((QComboBox*) widget)->currentItem()]->value = (char *)val;
			break;

		case ListEntry:
			items[((QListBox*) widget)->currentItem()]->value = (char *)val;
			break;

		case LabelEntry:
			((QLabel*) widget)->setText((char *)val);
			break;
	}
}

void Entry::setValues(QVector<Item>& eitems,
	RoadMapDialogCallback cb) {

	if (type != ListEntry) {
		return;
	}

	QListBox* lb = (QListBox*) widget;

   if (lb->count() > 0) lb->clear();

	items.resize(eitems.count());

	for(uint i = 0; i < eitems.count(); i++) {
		items.insert(i, eitems[i]);
		lb->insertItem(eitems[i]->label);
	}

	callback = cb;
}

void Entry::run() {
	if (callback != 0) {
		callback(name, dialog->getContext());
	}
}

