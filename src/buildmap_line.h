/* buildmap_line.h - Build a line table & index for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2009, Danny Backx
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

#ifndef INCLUDED__BUILDMAP_LINE__H
#define INCLUDED__BUILDMAP_LINE__H

int  buildmap_line_add (int tlid, int layer, int from, int to, int oneway);

int  buildmap_line_find_sorted   (int tlid);
void buildmap_line_get_position  (int line, int *longitude, int *latitude);
int  buildmap_line_get_sorted    (int line);
int  buildmap_line_get_id_sorted (int line);
void buildmap_line_get_points_sorted (int line, int *from, int *to);
int  buildmap_line_get_square_sorted (int line);
void buildmap_line_get_position_sorted
        (int line, int *longitude, int *latitude);

void buildmap_line_sort (void);

void buildmap_line_test_long (int line, int longitude, int latitude);
void buildmap_line_oneway(int way, int oneway);
void buildmap_line_layer(int way, int layer);

#endif // INCLUDED__BUILDMAP_LINE__H

