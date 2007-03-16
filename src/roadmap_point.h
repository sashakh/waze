/* roadmap_point.h - Manage the points that define tiger lines.
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

#ifndef _ROADMAP_POINT__H_
#define _ROADMAP_POINT__H_

#include "roadmap_types.h"
#include "roadmap_dbread.h"

int  roadmap_point_in_square (int square, int *first, int *last);
void roadmap_point_position  (int point, RoadMapPosition *position);
int roadmap_point_db_id (int point);
int roadmap_point_count (void);
int roadmap_point_square (int point);

extern roadmap_db_handler RoadMapPointHandler;

#endif // _ROADMAP_POINT__H_
