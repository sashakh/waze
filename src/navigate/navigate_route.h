/**
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright (c) 2009, Danny Backx.
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
 * @brief navigate_route.h - generic navigate functions
 */

#ifndef _NAVIGATE_ROUTE_H_
#define _NAVIGATE_ROUTE_H_

#include "navigate.h"

#define GRAPH_IGNORE_TURNS 1

#define NEW_ROUTE 1
#define RECALC_ROUTE 2

int navigate_route_reload_data (void);
int navigate_route_load_data   (void);

int navigate_route_get_segments (PluginLine *from_line,
                                 RoadMapPosition from_pos,
                                 PluginLine *to_line,
                                 RoadMapPosition to_pos,
                                 NavigateSegment *segments,
                                 int *size,
                                 int *result);

NavigateStatus navigate_route_get_initial (PluginLine *from_line,
					   RoadMapPosition from_pos,
					   PluginLine *to_line,
					   RoadMapPosition to_pos);

#endif /* _NAVIGATE_ROUTE_H_ */

