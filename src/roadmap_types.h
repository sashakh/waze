/* roadmap_types.h - general type definitions used in RoadMap.
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

#ifndef _ROADMAP_TYPES__H_
#define _ROADMAP_TYPES__H_

typedef unsigned short RoadMapZip;
typedef unsigned short RoadMapString;

typedef struct {
   int longitude;
   int latitude;
} RoadMapPosition;

typedef struct {
   int first;
   int count;
} RoadMapSortedList;


/* The cfcc category codes: */

/* Category: road. */

#define ROADMAP_ROAD_FIRST       1

#define ROADMAP_ROAD_FREEWAY     1
#define ROADMAP_ROAD_RAMP        2
#define ROADMAP_ROAD_MAIN        3
#define ROADMAP_ROAD_STREET      4
#define ROADMAP_ROAD_TRAIL       5

#define ROADMAP_ROAD_LAST        5


/* Category: area. */

#define ROADMAP_AREA_FIRST       6

#define ROADMAP_AREA_PARC        6
#define ROADMAP_AREA_HOSPITAL    7
#define ROADMAP_AREA_AIRPORT     8
#define ROADMAP_AREA_STATION     9
#define ROADMAP_AREA_MALL       10

#define ROADMAP_AREA_LAST       10


/* Category: water. */

#define ROADMAP_WATER_FIRST     11

#define ROADMAP_WATER_SHORELINE 11
#define ROADMAP_WATER_RIVER     12
#define ROADMAP_WATER_LAKE      13
#define ROADMAP_WATER_SEA       14

#define ROADMAP_WATER_LAST      14

#define ROADMAP_CATEGORY_RANGE  14

#endif // _ROADMAP_TYPES__H_

