/* buildmap_polygon.h - Build a table & index of polygons for RoadMap.
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

#ifndef _ROADMAP_POLYGON__H_
#define _ROADMAP_POLYGON__H_

#define POLYGON_SIDE_LEFT   0
#define POLYGON_SIDE_RIGHT  1

void buildmap_polygon_initialize (void);
int  buildmap_polygon_add (int landid, RoadMapString cenid, int polyid);
int  buildmap_polygon_add_landmark
        (int landid, char cfcc, RoadMapString name);
int  buildmap_polygon_add_line
        (RoadMapString cenid, int polyid, int tlid, int side);
int  buildmap_polygon_use_line (int tlid);
void buildmap_polygon_sort    (void);
void buildmap_polygon_save    (void);
void buildmap_polygon_summary (void);
void buildmap_polygon_reset   (void);

#endif // _ROADMAP_POLYGON__H_

