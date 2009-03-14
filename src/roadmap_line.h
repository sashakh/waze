/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 * @brief define functions that work on lines
 *
 * Lines have two endpoints, nothing in between.
 *
 * A line A-B should not be in a RoadMap map if it is a street that intersects with street C-D.
 *
 *     A ---- C ------ B
 *            |
 *           /
 *         D
 * In this case, A-B should not be a line in the map, but A-C, C-B and C-D should be.
 *
 * There are two higher level concepts also in RoadMap : streets and shapes.
 *
 * A street is something you'll recognize in real life : it has a name, and consists of
 * some pieces that aren't even always adjacent.
 *
 * A shape is a RoadMap concept to represent the curves followed by a non-straight line.
 *
 * Lines are basic elements for navigation : they're interesting because we know there's
 * nothing between the two endpoints.
 */

#ifndef INCLUDED__ROADMAP_LINE__H
#define INCLUDED__ROADMAP_LINE__H

#include "roadmap_types.h"
#include "roadmap_dbread.h"

int  roadmap_line_in_square (int square, int cfcc, int *first, int *last);
int  roadmap_line_in_square2 (int square, int cfcc, int *first, int *last);
int  roadmap_line_get_from_index2 (int index);
int  roadmap_line_long (int index, int *line_id, RoadMapArea *area, int *cfcc);

void roadmap_line_points (int line, int *from, int *to);
void roadmap_line_from (int line, RoadMapPosition *position);
void roadmap_line_to   (int line, RoadMapPosition *position);

int  roadmap_line_length (int line);
int  roadmap_line_count (void);

extern roadmap_db_handler RoadMapLineHandler;

extern int roadmap_line_point_adjacent(int point, int ix);

int roadmap_line_from_point (int line);
int roadmap_line_to_point (int line);

#endif // INCLUDED__ROADMAP_LINE__H
