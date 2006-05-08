/* buildmap_line.h - Build a line table & index for RoadMap.
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

#ifndef _BUILDMAP_LINE__H_
#define _BUILDMAP_LINE__H_

void buildmap_line_initialize (void);
int  buildmap_line_add (int tlid, int cfcc, int from, int to);

int  buildmap_line_find_sorted   (int tlid);
void buildmap_line_get_position  (int line, int *longitude, int *latitude);
int  buildmap_line_get_sorted    (int line);
int  buildmap_line_get_id_sorted (int line);
void buildmap_line_get_points_sorted (int line, int *from, int *to);
int  buildmap_line_get_square_sorted (int line);
void buildmap_line_get_position_sorted
        (int line, int *longitude, int *latitude);
void buildmap_line_sort          (void);

void buildmap_line_test_long (int line, int longitude, int latitude);
   
void buildmap_line_save    (void);
void buildmap_line_summary (void);
void buildmap_line_reset   (void);

#endif // _BUILDMAP_LINE__H_

