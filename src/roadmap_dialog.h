/* roadmap_dialog.h - manage the roadmap dialogs is used for user input.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 * DESCRIPTION:
 *
 *   This module define an API to create and manipulate dialog windows.
 *
 *   A dialog window is made of two area:
 *   The 1st area (usually at the top) is used to show framed items.
 *   The 2nd area is used to show the buttons ("OK", "Cancel", etc..).
 *
 *   Every item in a dialog has a title (usually a label to place on
 *   the left side of the widget), a type (text entry, color, choice, etc..)
 *   and a parent frame.
 *
 *   When a dialog contains several parent frames, all frames should be
 *   visible as separate entities (for example using a notebook widget).
 *
 *   Here is an example of a simple dialog asking for a name and an email
 *   address (with no keyboard attached):
 *
 *   if (roadmap_dialog_activate ("Email Address")) {
 *     roadmap_dialog_new_entry  ("Address", "Name");
 *     roadmap_dialog_new_entry  ("Address", "email");
 *     roadmap_dialog_add_button ("OK", add_this_address);
 *     roadmap_dialog_add_button ("Cancel", roadmap_dialog_hide);
 *     roadmap_dialog_complete   (NULL);
 *   }
 *
 * The application can retrieve the current values of each item using
 * the roadmap_dialog_select() and roadmap_dialog_get_data() functions.
 */

#ifndef INCLUDE__ROADMAP_DIALOG__H
#define INCLUDE__ROADMAP_DIALOG__H

typedef void (*RoadMapDialogCallback) (char *name, void *context);


/* This function activates a dialog:
 * If the dialog did not exist yet, it will create an empty dialog
 * and roadmap_dialog_activate() returns 1; the application must then
 * enumerate all the dialog's items.
 * If the dialog did exist already, it will be shown on top and
 * roadmap_dialog_activate() returns 0.
 * This function never fails. The given dialog becomes the curent dialog.
 */
int roadmap_dialog_activate (char *name, void *context);

/* Hide the given dialog, if it exists. */
void roadmap_dialog_hide (char *name);

/* Add one text entry item to the current dialog. */
void roadmap_dialog_new_entry (char *frame, char *name);

/* Add one color selection item to to the current dialog. */
void roadmap_dialog_new_color (char *frame, char *name);

/* Add one choice item (a selection box or menu).
 * The optional callback is called each time a new selection is being made,
 * not when the OK button is called--that is the job of the OK button
 * callback.
 */
void roadmap_dialog_new_choice (char *frame,
                                char *name,
                                int count,
                                char **labels,
                                void **values,
                                RoadMapDialogCallback callback);

/* Add one list item.
 * This item is similar to the choice one, except for two things:
 * 1) it uses a scrollable list widget instead of a combo box.
 * 2) the list of items shown is dynamic and can be modified
 *    (it is initially empty).
 */
void roadmap_dialog_new_list (char  *frame, char  *name);

void roadmap_dialog_show_list (char  *frame,
                               char  *name,
                               int    count,
                               char **labels,
                               void **values,
                               RoadMapDialogCallback callback);


/* Add one button to the bottom of the dialog. */
void roadmap_dialog_add_button (char *label, RoadMapDialogCallback callback);

/* When all done with building the dialog, call this to finalize and show: */
void roadmap_dialog_complete (int use_keyboard);

void roadmap_dialog_select (char *dialog);
void *roadmap_dialog_get_data (char *frame, char *name);
void  roadmap_dialog_set_data (char *frame, char *name, void *data);

#endif // INCLUDE__ROADMAP_DIALOG__H
