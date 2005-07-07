/* roadmap_factory.h - The menu/toolbar/binding factory for RoadMap.
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

#ifndef INCLUDE__ROADMAP_FACTORY__H
#define INCLUDE__ROADMAP_FACTORY__H

#include "roadmap_main.h"


typedef struct {

   const char *name;
   const char *label_long;
   const char *label_short;
   const char *label_terse;
   const char *tip;

   RoadMapCallback callback;

} RoadMapAction;


#define ROADMAP_MENU      "/"
#define ROADMAP_SUBMENU   "//"

#define ROADMAP_MAPPED_TO " = "

extern const char RoadMapFactorySeparator[];
extern const char RoadMapFactoryHelpTopics[];

void roadmap_factory (const char          *name,
                      const RoadMapAction *actions,
                      const char          *menu[],
                      const char          *toolbar[]);

void roadmap_factory_keymap (const RoadMapAction *actions,
                             const char          *shortcuts[]);

void roadmap_factory_usage (const char *section, const RoadMapAction *action);

#endif /* INCLUDE__ROADMAP_FACTORY__H */

