/* roadmap_polygon.h - the format of the polygon table used by RoadMap.
 *
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
 *
 * SYNOPSYS:
 *
 * The RoadMap polygons are described by the following tables:
 *
 * polygon/head     for each polygon, the category and list of lines.
 * polygon/points   the list of points defining the polygons border.
 */

#ifndef INCLUDED__ROADMAP_DB_POLYGON__H
#define INCLUDED__ROADMAP_DB_POLYGON__H

#include "roadmap_types.h"

typedef struct {  /* table polygon/head */

   unsigned short first;
   unsigned short count;

   RoadMapString name;
   char  cfcc;
   unsigned char filler;

   /* TBD: replace these with a RoadMapArea (not compatible!). */
   int   north;
   int   west;
   int   east;
   int   south;

} RoadMapPolygon;

typedef struct {  /* table polygon/points */

   int point;

} RoadMapPolygonPoint;

#endif // INCLUDED__ROADMAP_DB_POLYGON__H

