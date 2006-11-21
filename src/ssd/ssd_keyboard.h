/* ssd_keyboard.h - Full screen keyboard
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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

#ifndef __SSD_KEYBOARD_H_
#define __SSD_KEYBOARD_H_
 
#define SSD_KEYBOARD_LETTERS 1
#define SSD_KEYBOARD_DIGITS  2

/* key types */
#define SSD_KEYBOARD_OK 0x1

#include "ssd_dialog.h"

void ssd_keyboard_show (int type, const char *title, const char *value,
                        SsdDialogCB callback, void *context);

void ssd_keyboard_hide (int type);

#endif // __SSD_KEYBOARD_H_
