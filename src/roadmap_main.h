/*
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

/**
 * @file
 * @brief The interface for the RoadMap main window module.
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
void roadmap_main_title(char *fmt, ...);

void roadmap_main_set_keyboard   (RoadMapKeyInput callback);


void roadmap_main_set_cursor (RoadMapCursor newcursor);

RoadMapMenu roadmap_main_new_menu (const char *title);
void roadmap_main_free_menu       (RoadMapMenu menu);

void roadmap_main_add_menu       (RoadMapMenu menu, const char *label);
void roadmap_main_popup_menu     (RoadMapMenu menu,
                                  const RoadMapGuiPoint *position);

void roadmap_main_add_menu_item  (RoadMapMenu menu,
                                  const char *label,
                                  const char *tip,
                                  RoadMapCallback callback);
void roadmap_main_add_separator  (RoadMapMenu menu);

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

void roadmap_main_set_idle_function (RoadMapCallback callback);
void roadmap_main_remove_idle_function (void);

void roadmap_main_set_status (const char *text);

void roadmap_main_toggle_full_screen (void);

int roadmap_main_flush (void);
int  roadmap_main_flush_synchronous (int deadline);

void roadmap_main_exit (void);

#endif /* INCLUDE__ROADMAP_MAIN__H */

/**
 *
 * @mainpage A Car Navigation System for Linux, UNIX, and Others
 *
 *
 * The following is a short description of RoadMap.
 * More information is available in the documentation distributed with RoadMap.
 *
 * RoadMap is an open source (GPL) program that provides a car
 * navigation for Linux, UNIX and now Windows CE and even the
 * iPhone/iPod.  It displays a map of the streets, tracks the
 * position provided by a NMEA-compliant GPS receiver, identifies
 * the street matching this GPS position, and announces the name
 * of the crossing street at the next intersection.  A trip
 * feature allows RoadMap to display routes, tracks, and provides
 * some basic navigation information (distance to the next
 * waypoint, direction, speed, etc..).  Voice messages are
 * generated that duplicate some of the screen information. 
 *
 */
