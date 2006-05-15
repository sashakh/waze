/* navigate_main.h - main plugin file
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

#ifndef INCLUDE__NAVIGATE_MAIN__H
#define INCLUDE__NAVIGATE_MAIN__H

#include "roadmap_canvas.h"
#include "roadmap_plugin.h"
#include "roadmap_screen.h"

enum NavigateInstr {
   TURN_LEFT,
   TURN_RIGHT,
   KEEP_LEFT,
   KEEP_RIGHT,
   CONTINUE,
   APPROCHING_DESTINATION
};

typedef struct {
   PluginLine           line;
   int                  line_direction;
   PluginStreet         street;
   RoadMapPosition      from_pos;
   RoadMapPosition      to_pos;
   int                  first_shape;
   int                  last_shape;
   RoadMapShapeItr      shape_itr;
   enum NavigateInstr   instruction;
   
} NavigateSegment;
 
int navigate_is_enabled (void);
void navigate_main_initialize (void);
void navigate_main_set (int status);
void navigate_main_calc_route (void);

void navigate_main_screen_repaint (int max_pen);

int navigate_main_override_pen (int line,
                                int cfcc,
                                int fips,
                                int pen_type,
                                RoadMapPen *override_pen);

#endif /* INCLUDE__NAVIGATE_MAIN__H */

