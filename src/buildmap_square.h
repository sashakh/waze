/* buildmap_square.h - Divide the area in more manageable squares.
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

#ifndef _BUILDMAP_SQUARE__H_
#define _BUILDMAP_SQUARE__H_

void  buildmap_square_initialize
         (int minlongitude, int maxlongitude,
          int minlatitude,  int maxlatitude);
int   buildmap_square_add  (int longitude, int latitude);

void  buildmap_square_sort (void);
short buildmap_square_get_sorted (int squareid);
int   buildmap_square_get_count (void);
void  buildmap_square_get_reference_sorted
         (int square, int *longitude, int *latitude);

void buildmap_square_set_first_point_sorted (int square, int point);
int buildmap_square_first_point_sorted (int square);
void buildmap_square_set_first_shape_sorted (int square, int shape);
int buildmap_square_first_shape_sorted (int shape);

int buildmap_square_is_long_line (int from_point, int to_point,
		  		                      int longitude, int latitude);

void buildmap_square_save    (void);
void buildmap_square_summary (void);
void buildmap_square_reset   (void);

#endif // _BUILDMAP_SQUARE__H_

