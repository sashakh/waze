/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2008, Danny Backx.
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
 * @brief roadmap_factory.h - the menu/toolbar/binding factory for RoadMap.
 */

#ifndef INCLUDE__ROADMAP_FACTORY__H
#define INCLUDE__ROADMAP_FACTORY__H

#include "roadmap_main.h"
#include "roadmap_gui.h"
#include "roadmap_config.h"


/**
 * @brief table used to create the menu, accelarators, ..
 */
typedef struct {
   const char *name;
         char *label_long;
   const char *label_short;
         char *label_terse;
   const char *tip;
   const char *key;  /**< filled in at run-time, based on key mappings */
   RoadMapCallback callback;
} RoadMapAction;


/* ROADMAP_MENU must be a substring of ROADMAP_SUBMENU */
#define ROADMAP_MENU            "/"
#define ROADMAP_SUBMENU         "//"
#define ROADMAP_INVOKE_SUBMENU  "->"

#define ROADMAP_MAPPED_TO       " = "

extern const char RoadMapFactorySeparator[];
extern const char RoadMapFactoryHelpTopics[];

RoadMapAction *roadmap_factory_find_action_or_menu
                          (RoadMapAction *actions, const char *item);

void roadmap_factory (const char          *name,
                            RoadMapAction *actions,
                      const char          *menu[],
                      const char          *toolbar[]);

void roadmap_factory_keymap (RoadMapAction *actions,
                             const char    *shortcuts[]);

void roadmap_factory_popup (const char *title,
                            const RoadMapGuiPoint *position);

void roadmap_factory_usage (const char *section, const RoadMapAction *action);

#endif /* INCLUDE__ROADMAP_FACTORY__H */

