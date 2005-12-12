/* buildmap_point.h - Build a table of all points referenced in lines.
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

#ifndef _BUILDMAP_POINT__H_
#define _BUILDMAP_POINT__H_

void buildmap_point_initialize (void);
int  buildmap_point_add        (int longitude, int latitude, int id);

void buildmap_point_sort (void);
int  buildmap_point_get_square (int pointid);
int  buildmap_point_get_longitude (int pointid);
int  buildmap_point_get_latitude  (int pointid);
int  buildmap_point_get_sorted (int pointid);
int  buildmap_point_get_longitude_sorted (int point);
int  buildmap_point_get_latitude_sorted  (int point);
int  buildmap_point_get_square_sorted (int point);

void buildmap_point_save    (void);
void buildmap_point_summary (void);
void buildmap_point_reset   (void);

#endif // _BUILDMAP_POINT__H_

