/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2008, 2009, Danny Backx.
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

/**
 * @file
 * @brief general type definitions used in RoadMap.
 */
#ifndef INCLUDED__ROADMAP_TYPES__H
#define INCLUDED__ROADMAP_TYPES__H

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

typedef struct {
   int east;
   int north;
   int west;
   int south;
} RoadMapArea;

#if defined(HAVE_TRIP_PLUGIN) || defined(HAVE_NAVIGATE_PLUGIN)
typedef unsigned char LineRouteFlag;
typedef unsigned char  LineRouteMax;
typedef unsigned short LineRouteTime;
#endif

typedef void (*RoadMapShapeItr) (int shape, RoadMapPosition *position);

#endif // INCLUDED__ROADMAP_TYPES__H

