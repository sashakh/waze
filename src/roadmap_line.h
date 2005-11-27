/* roadmap_line.h - Manage the tiger lines.
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

#ifndef INCLUDED__ROADMAP_LINE__H
#define INCLUDED__ROADMAP_LINE__H

#include "roadmap_types.h"
#include "roadmap_dbread.h"

int  roadmap_line_in_square (int square, int cfcc, int *first, int *last);
int  roadmap_line_in_square2 (int square, int cfcc, int *first, int *last);
int  roadmap_line_get_from_index2 (int index);

void roadmap_line_points (int line, int *from, int *to);
void roadmap_line_from (int line, RoadMapPosition *position);
void roadmap_line_to   (int line, RoadMapPosition *position);

int  roadmap_line_length (int line);
int  roadmap_line_count (void);

extern roadmap_db_handler RoadMapLineHandler;

#endif // INCLUDED__ROADMAP_LINE__H
