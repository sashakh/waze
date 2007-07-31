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

#ifndef INCLUDED__ROADMAP_POLYGON__H
#define INCLUDED__ROADMAP_POLYGON__H

#define POLYGON_SIDE_LEFT   0
#define POLYGON_SIDE_RIGHT  1

int  buildmap_polygon_add (int landid, RoadMapString cenid, int polyid);
int  buildmap_polygon_add_landmark
        (int landid, int cfcc, RoadMapString name);
int  buildmap_polygon_add_line
        (RoadMapString cenid, int polyid, int tlid, int side);

int  buildmap_polygon_use_line (int tlid);

#endif // INCLUDED__ROADMAP_POLYGON__H

