/*
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright (c) 2008, 2009, Danny Backx
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
 * @brief navigate.h - main plugin file
 */

#ifndef INCLUDE__NAVIGATE_H
#define INCLUDE__NAVIGATE_H

#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_plugin.h"
#include "roadmap_screen.h"

enum NavigateInstr {
   NAVIGATE_TURN_LEFT = 0,
   NAVIGATE_TURN_RIGHT,
   NAVIGATE_KEEP_LEFT,
   NAVIGATE_KEEP_RIGHT,
   NAVIGATE_CONTINUE,
   NAVIGATE_APPROACHING_DESTINATION,
   NAVIGATE_LAST_DIRECTION
};

/**
 * @brief structure to keep iterations of a route in
 * also serves to pass them between navigate plugin and others
 */
typedef struct {
   PluginLine           line;
   int                  line_direction;	/**< is the direction reversed ? */
   PluginStreet         street;
   RoadMapPosition      from_pos;
   int			from_point;
   RoadMapPosition      to_pos;
   int			to_point;
   RoadMapPosition      shape_initial_pos;
   int                  first_shape;
   int                  last_shape;
   RoadMapShapeItr      shape_itr;
   enum NavigateInstr   instruction;
   int                  group_id;
   int                  distance;
   int                  cross_time;
   
} NavigateSegment;
 
int navigate_is_enabled (void);
void navigate_shutdown (void);
void navigate_initialize (void);
int  navigate_reload_data (void);
void navigate_set (int status);
int  navigate_calc_route (void);

void navigate_screen_repaint (int max_pen);

int navigate_override_pen (int line,
                                int cfcc,
                                int fips,
                                int pen_type,
                                RoadMapPen *override_pen);

void navigate_adjust_layer (int layer, int thickness, int pen_count);
void navigate_format_messages (void);
#endif /* INCLUDE__NAVIGATE_H */

