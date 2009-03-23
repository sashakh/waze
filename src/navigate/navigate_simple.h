/*
 * LICENSE:
 *
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
 * @brief API of the "simple" route calculation
 */

#ifndef _NAVIGATE_SIMPLE_H_
#define _NAVIGATE_SIMPLE_H_

#include "navigate.h"

#define GRAPH_IGNORE_TURNS 1

int navigate_simple_get_segments (PluginLine *from_line,
                                 RoadMapPosition from_pos,
                                 PluginLine *to_line,
                                 RoadMapPosition to_pos,
                                 NavigateSegment *segments,
                                 int *size,
                                 int *result);

#endif /* _NAVIGATE_SIMPLE_H_ */

