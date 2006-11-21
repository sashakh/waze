/* ssd_menu.h - Icons menu
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

#ifndef __SSD_MENU_H_
#define __SSD_MENU_H_
  
#include "roadmap_factory.h"

void ssd_menu_activate (const char           *name,
                        const char           *items_file,
                        const char           *items[],
                        RoadMapCallback       callback,
                        const RoadMapAction  *actions,
                        int                   flags);

void ssd_menu_hide (const char *name);

#endif // __SSD_MENU_H_
