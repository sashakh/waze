/* roadmap_shape.h - Manage the tiger shape points.
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

#ifndef _ROADMAP_SHAPE__H_
#define _ROADMAP_SHAPE__H_

#include "roadmap_types.h"
#include "roadmap_dbread.h"

int  roadmap_shape_in_square (int square, int *first, int *last);
int  roadmap_shape_of_line   (int line, int begin, int end,
                                        int *first, int *last);
void roadmap_shape_get_position (int shape, RoadMapPosition *position);

extern roadmap_db_handler RoadMapShapeHandler;

#endif // _ROADMAP_SHAPE__H_
