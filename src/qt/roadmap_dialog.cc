/* roadmap_dialog.cc - A C to C++ wrapper for the QT RoadMap dialogs
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
 *   See roadmap_dialog.h
 */
extern "C" {
#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_start.h"
#include "roadmap_dialog.h"
};

#include "qt_dialog.h"

QMap<QString, RMapDialog*> dialogs;
RMapDialog* currentDialog;

int roadmap_dialog_activate(const char* name, void* context) {
	RMapDialog* dialog = dialogs[name];
	int ret = 0;

	if (dialog != 0) {
		dialog->show();
		ret = 0;
	} else {
		dialog = new RMapDialog(0, roadmap_start_get_title(name));
		ret = 1;
	}

	currentDialog = dialog;
	currentDialog->setContext(context);
	return ret;
}

void roadmap_dialog_hide(const char* name) {
//	RMapDialog* dialog = dialogs[name];

	if (currentDialog != 0) {
		currentDialog->hide();
	}
}

void roadmap_dialog_new_entry (const char *frame, const char *name) {
	currentDialog->addTextEntry(frame, name);
}

void roadmap_dialog_new_label (const char *frame, const char *name) {
	// TBD.
}

void roadmap_dialog_new_color (const char *frame, const char *name) {
	currentDialog->addColorEntry(frame, name);
}

void roadmap_dialog_new_choice (const char *frame, const char *name, int count,
	char **labels, void **values, RoadMapDialogCallback callback) {

	currentDialog->addChoiceEntry(frame, name, count, labels, values, callback);
}

void roadmap_dialog_new_list (const char  *frame, const char  *name) {
	currentDialog->addListEntry(frame, name);
}

void roadmap_dialog_show_list (const char* frame, const char* name, int count,
	char **labels, void **values, RoadMapDialogCallback callback) {

	currentDialog->setListEntryValues(frame, name, count, labels, values, callback);
}

void roadmap_dialog_add_button (char *label, RoadMapDialogCallback callback) {
	currentDialog->addButton(label, callback);
}

void roadmap_dialog_complete (int use_keyboard) {
	currentDialog->complete(use_keyboard);
}

void  roadmap_dialog_select(const char *dialog) {
	RMapDialog* d = dialogs[dialog];

	if (d != 0) {
		currentDialog = d;
	}
}
	
void *roadmap_dialog_get_data (const char *frame, const char *name) {
	return currentDialog->getEntryValue(frame, name);
}

void roadmap_dialog_set_data (const char *frame, const char *name,
                              const void *data) {
	currentDialog->setEntryValue(frame, name, data);
}
