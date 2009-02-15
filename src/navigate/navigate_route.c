/*
 * LICENSE:
 *
 *   Copyright 2008 by Danny Backx.
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
 * @brief entry points for route calculation, to become a user centric plugin mechanism 
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_point.h"
#include "navigate.h"
#include "navigate/navigate_simple.h"
#include "navigate_route.h"

/**
 * @brief calculate the basic route.
 *
 * Called from navigate_calc_route and navigate_recalc_route.
 * Result will be placed in the "segments" parameter, number of steps in "size".
 *
 * @param from_line
 * @param from_pos
 * @param to_line
 * @param to_pos
 * @param segments
 * @param size
 * @param flags either NEW_ROUTE or RECALC_ROUTE
 * @return track time
 */
int navigate_route_get_segments (PluginLine *from_line,
                                 RoadMapPosition from_pos,
                                 PluginLine *to_line,
                                 RoadMapPosition to_pos,
                                 NavigateSegment *segments,
                                 int *size,
                                 int *flags)
{
	return navigate_simple_get_segments(from_line, from_pos, to_line, to_pos,
			segments, size, flags);
}

/**
 * @brief
 * @return
 */
int navigate_route_reload_data (void)
{
	return 0;
}

/**
 * @brief
 * @return
 */
int navigate_route_load_data (void)
{
   return 0;
}
