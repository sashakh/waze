/* ssd_dialog.h - small screen devices Widgets (designed for touchscreens)
 * (requires agg)
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

#ifndef __SSD_DIALOG_H_
#define __SSD_DIALOG_H_

#include "roadmap.h"
#include "roadmap_canvas.h"
#include "ssd_widget.h"

typedef int (*SsdDialogCB) (int type, const char *new_value, void *context);

SsdWidget ssd_dialog_activate (const char *name, void *context);
void ssd_dialog_hide (const char *name);
SsdWidget ssd_dialog_new (const char *name, const char *title, int flags);
void ssd_dialog_new_line (void);
void ssd_dialog_draw (void);
void ssd_dialog_new_entry (const char *name, const char *value,
                           int flags, SsdCallback callback);

SsdWidget ssd_dialog_new_button (const char *name, const char *value,
                                 const char **bitmaps, int num_bitmaps,
                                 int flags, SsdCallback callback);

const char *ssd_dialog_get_value (const char *name);
const void *ssd_dialog_get_data  (const char *name);
int ssd_dialog_set_value (const char *name, const char *value);
int ssd_dialog_set_data  (const char *name, const void *value);

void *ssd_dialog_context (void);

void ssd_dialog_hide_current (void);

void ssd_dialog_set_callback (RoadMapCallback callback);

#endif // __SSD_DIALOG_H_
