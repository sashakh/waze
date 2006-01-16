/* buildmap_line.h - Build a line table & index for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2004 Stephen Woodbridge
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

#ifndef INCLUDED__BUILDMAP_PLACE__H
#define INCLUDED__BUILDMAP_PLACE__H

#define BUILDMAP_MAX_PLACE_CFCC  10

int  buildmap_place_add (int name, int cfcc, int point);

int  buildmap_place_get_sorted  (int place);
int buildmap_place_get_name_sorted (int place);
int buildmap_place_find_sorted (int name);
void buildmap_place_get_position 
        (int place, int *longitude, int *latitude);
int buildmap_place_get_point_sorted (int place);
void buildmap_place_get_position_sorted
        (int place, int *longitude, int *latitude);
int buildmap_place_get_square_sorted (int place);

#endif /* INCLUDED__BUILDMAP_PLACE__H */

