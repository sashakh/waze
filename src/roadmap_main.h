/* roadmap_main.h - The interface for the RoadMap main window module.
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
 */

#ifndef INCLUDE__ROADMAP_MAIN__H
#define INCLUDE__ROADMAP_MAIN__H

#include "roadmap.h"
#include "roadmap_gui.h"

#include "roadmap_io.h"
#include "roadmap_spawn.h"

typedef void (* RoadMapKeyInput) (char *key);
typedef void (* RoadMapInput)    (RoadMapIO *io);

void roadmap_main_new (const char *title, int width, int height);

void roadmap_main_set_keyboard   (RoadMapKeyInput callback);

void roadmap_main_add_menu       (const char *label);
void roadmap_main_add_menu_item  (const char *label,
                                  const char *tip,
                                  RoadMapCallback callback);
void roadmap_main_add_separator  (void);

/* The orientation must be either "top", "bottom", "left", "right" or ""
 * ("" means the default orientation).
 */
void roadmap_main_add_toolbar    (const char *orientation);

void roadmap_main_add_tool       (const char *label,
                                  const char *icon,
                                  const char *tip,
                                  RoadMapCallback callback);
void roadmap_main_add_tool_space (void);

/* The canvas must be created after the menu and the toolbar. */
void roadmap_main_add_canvas     (void);
void roadmap_main_add_status     (void);

void roadmap_main_show (void);

void roadmap_main_set_input    (RoadMapIO *io, RoadMapInput callback);
void roadmap_main_remove_input (RoadMapIO *io);

void roadmap_main_set_periodic (int interval, RoadMapCallback callback);
void roadmap_main_remove_periodic (RoadMapCallback callback);

void roadmap_main_set_status (const char *text);

void roadmap_main_toggle_full_screen (void);

void roadmap_main_flush (void);

void roadmap_main_exit (void);

#endif /* INCLUDE__ROADMAP_MAIN__H */

