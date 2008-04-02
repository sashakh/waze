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
#include <qboxlayout.h>


// Implementation of RMapDialog class
RMapDialog::RMapDialog( QWidget *parent, Qt::WFlags f ):QDialog(parent, f) {
}

RMapDialog::~RMapDialog() {
}

QMap<int, Entry *>* RMapDialog::getFrame(QString frameName) {
   QMap<int, Entry *>* frame = frames[frameName];

   if (frame == 0) {
      frame = new QMap<int, Entry*>;
      frameNames.append(QString(frameName));
      frames.insert(frameName, frame);
   }

   return frame;
}

Entry* RMapDialog::getEntry(QString frameName, QString entryName) {
   QMap<int, Entry *>* frame = frames[frameName];

   if (frame == 0) {
      return 0;
   }

   int i;
   for (i=0; i<frame->count(); i++) {
     if (frame->value(i)->getName() == entryName)
     return frame->value(i);   
   }
   return 0;

}

void RMapDialog::addTextEntry(const char* frameName, const char* name) {
   QMap<int, Entry *>* frame = getFrame(frameName);

   Entry *entry = new Entry(this, Entry::TextEntry, name);
   frame->insert(frame->count(),entry);
}

void RMapDialog::addLabelEntry(const char* frameName, const char* name) {
   QMap<int, Entry *>* frame = getFrame(frameName);

   Entry *entry = new Entry(this, Entry::LabelEntry, name);
   frame->insert(frame->count(),entry);
}

void RMapDialog::addColorEntry(const char* frameName, const char* name) {
   addTextEntry(frameName, name);
}

void RMapDialog::addChoiceEntry(const char* frameName,
                                const char* name,
                                int count,
                                int current,
                                char** labels,
                                void* values,
                                RoadMapDialogCallback callback) {

   QMap<int, Entry *>* frame = getFrame(frameName);
   char **vals = (char **)values;

   QVector<Item> items(count);
   for(int i = 0; i < count; i++) {
      items[i].label = labels[i];
      items[i].value = vals[i];
   }

   Entry *entry =
      new Entry(this, Entry::ChoiceEntry, name, items, current, callback);
   frame->insert(frame->count(),entry);
}

void RMapDialog::addListEntry(const char* frameName, const char* name) {
   QMap<int, Entry *>* frame = getFrame(frameName);

   Entry *entry = new Entry(this, Entry::ListEntry, name);
   frame->insert(frame->count(),entry);
}

void RMapDialog::setListEntryValues(const char* frameName, const char* name, 
   int count, char** labels, void** values, RoadMapDialogCallback callback) {

   Entry *entry = getEntry(frameName, name);

   if (entry == 0) {
      return;
   }

   QVector<Item> items(count);
   for(int i = 0; i < count; i++) {
      items[i].label = labels[i];
      items[i].value = values[i];
   }


   entry->setValues(items, callback);
}

void RMapDialog::addButton(const char* label, RoadMapDialogCallback callback) {
   QVector<Item> items;
   Entry *entry =
      new Entry(this, Entry::ButtonEntry, label, items, 0, callback);
   buttons.append(entry);
}

void RMapDialog::addHiddenEntry(const char* frameName, const char* name) {
   QMap<int, Entry *>* frame = getFrame(frameName);

   Entry *entry = new Entry(this, Entry::HiddenEntry, name);
   frame->insert(frame->count(),entry);
}

void RMapDialog::complete(int) {
   QVBoxLayout* main = new QVBoxLayout(this);
   QWidget* topw = 0;
   main->setSpacing(4);
   main->setMargin(2);

   if (frames.count() > 1) {
      QTabWidget* tw = new QTabWidget(this);
      
      for(int i = 0; i < frameNames.size(); 
         ++i) {

         QMap<int, Entry *>* frame = frames[frameNames.at(i)];
         QWidget* w = new QWidget(tw);
         initTab(w, frame);
         tw->addTab(w, frameNames.at(i));
      }

      topw = tw;
   } else {
      topw = new QWidget(this);
      initTab(topw, *frames.begin());
   }

   QWidget* bw = new QWidget(this);
   QHBoxLayout* btns = new QHBoxLayout(bw);
   Entry *entry;
   for(int i=0; i<buttons.size(); i++) {
      entry = buttons.at(i);
      QWidget* w = entry->create(bw);
      btns->addWidget(w);
   }

   main->addWidget(topw);
   main->addWidget(bw);
   adjustSize();

   show();
}

void* RMapDialog::getEntryValue(const char* frame, const char* name) {
   Entry *entry = getEntry(frame, name);

   if (entry == 0) {
      return 0;
   }

   return entry->getValue();
}

void RMapDialog::setEntryValue(const char* frame, const char* name, const void* data) {
   Entry *entry = getEntry(frame, name);

   if (entry == 0) {
      return;
   }

   return entry->setValue(data);
}

void RMapDialog::initTab(QWidget* tab, QMap<int, Entry *>* entries) {
   QGridLayout* grid = new QGridLayout(tab); //, entries->count(), 2, 2, 5);

   Entry* entry;
   
   for(int i = 0; i < entries->count(); i++) {
      entry = entries->value(i);
      QWidget* w = entry->create(tab);
      QLabel* l;
      
      if (entry->getWidget() != 0) {
         if (entry->getName()[0] == '.') {
            grid->addWidget(w, i, i, 0, 1);
         } else {
            l = new QLabel(entry->getName(), tab,0 );
            
            grid->addWidget(l, i, 0);
            grid->addWidget(w, i, 1);
         }
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
   current = 0;
   value = 0;
}

Entry::Entry(RMapDialog* dlg, int etype, QString ename, QVector<Item>& eitems,
   int ecurrent, RoadMapDialogCallback ecallback) {

   dialog = dlg;
   type = etype;
   name = ename;
   current = ecurrent;
   callback = ecallback;
   value = 0;

   items.clear();
   for(int i = 0; i < eitems.size(); i++) {
      items.insert(i, eitems[i]);
   }
}

Entry::~Entry() {
}

QWidget* Entry::create(QWidget* parent) {

   widget = 0;
   
   switch (type) {
      case TextEntry:
         widget = new QLineEdit(parent);
         break;

      case ColorEntry:
         widget = new QLineEdit(parent);
         break;

      case ChoiceEntry: 
         {
            QComboBox* cb = new QComboBox(parent);
            cb->setEditable(false);
   
            for(int i = 0; i < items.count(); i++) {
               cb->addItem(items[i].label);
            }
            cb->setCurrentIndex(current);

            connect(cb, SIGNAL(activated(int)), this, SLOT(run()));
   
            widget = cb;
         }
         break;

      case ListEntry:
         {
            QListWidget* lb = new QListWidget(parent);
            widget = lb;

            // ugly hack ... 
            lb->setMinimumHeight(200);
            lb->setMinimumWidth(150);

            connect(widget, SIGNAL(itemSelectionChanged()), this, SLOT(run()));
         }
         break;

      case ButtonEntry:
         widget = new QPushButton(parent);
         widget->setObjectName(name);
         ((QPushButton *)widget)->setText(name);
         connect(widget, SIGNAL(clicked()), this, SLOT(run()));
         break;

      case LabelEntry:
         widget = new QLabel(parent);
         ((QLabel *)widget)->setAlignment (Qt::AlignRight|Qt::AlignVCenter);
         break;

      case HiddenEntry:
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
      case TextEntry:
         ret = (void *) (new QByteArray(((QLineEdit*) widget)->text().toLatin1()))->constData();
         break;

      case ColorEntry:
         ret = (void *) (new QByteArray(((QLineEdit*) widget)->text().toLatin1()))->constData();
         break;

      case ChoiceEntry:
         ret = items[((QComboBox*) widget)->currentIndex()].value;
         break;

      case ListEntry:
         if (((QListWidget*) widget)->currentRow() == -1) {
	   /* the current row is unset (well, it's -1) after a
	    * repopulate of the list, and it will cause
	    * out-of-range errors if an action is performed
	    * before another selection is done.  the right
	    * solution would be to prevent actions when there's
	    * no selection.
            */
            ((QListWidget*) widget)->setCurrentRow(0);
         }
         ret = items[((QListWidget*) widget)->currentRow()].value;
         break;

      case HiddenEntry:
         ret = (void *)value;
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
         for (int i = 0; i < items.count(); ++i) {
            if (items[i].value == val) {
               ((QComboBox*) widget)->setCurrentIndex(i);
               break;
            }
         }
         break;

      case ListEntry:
         for (int i = 0; i < items.count(); ++i) {
            if (items[i].value == val) {
               ((QListWidget*) widget)->setCurrentRow(i);
               break;
            }
         }
         break;

      case LabelEntry:
         ((QLabel*) widget)->setText((char *)val);
         break;

      case HiddenEntry:
         value = val;
         break;
   }
}

void Entry::setValues(QVector<Item>& eitems,
   RoadMapDialogCallback cb) {

   if (type != ListEntry) {
      return;
   }

   QListWidget* lb = (QListWidget*) widget;

   callback = 0;

   if (lb->count() > 0) lb->clear();

   items.clear();

   for(int i = 0; i < eitems.size(); i++) {
      items.insert(i, eitems[i]);
      lb->addItem(eitems[i].label);
   }

   callback = cb;
}

void Entry::run() {
   if (callback != 0) {
     QString* entryname;
    
     if (type == ButtonEntry)
       entryname = new QString(dialog->objectName());
     else
       entryname= new QString(this->name);

     callback(entryname->toAscii().constData(), dialog->getContext());
//      callback(name.toAscii().constData(), dialog->getContext());
   }
}

