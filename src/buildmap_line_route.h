/* buildmap_line_route.h - Build a line route table
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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

#ifndef _BUILDMAP_LINE_ROUTE__H_
#define _BUILDMAP_LINE_ROUTE__H_

#include "roadmap_db_line_route.h"

void buildmap_line_route_initialize (void);
int  buildmap_line_route_add
       (unsigned char from_flags,
        unsigned char to_flags,
        unsigned char from_avg_speed,
        unsigned char to_avg_speed,
        int line);

void buildmap_line_route_sort (void);

int  buildmap_line_route_count (void);

void buildmap_line_route_save    (void);
void buildmap_line_route_summary (void);
void buildmap_line_route_reset   (void);

#endif // _BUILDMAP_LINE_ROUTE__H_

