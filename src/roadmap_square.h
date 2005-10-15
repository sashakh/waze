/* roadmap_square.h - Manage a county area, divided in small squares.
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

#ifndef _ROADMAP_SQUARE__H_
#define _ROADMAP_SQUARE__H_

#include "roadmap_types.h"
#include "roadmap_dbread.h"

#define ROADMAP_SQUARE_GLOBAL -1
#define ROADMAP_SQUARE_OTHER  -2

int   roadmap_square_count  (void);
int   roadmap_square_search (const RoadMapPosition *position);

int roadmap_square_search_near
   (const RoadMapPosition *position, int *squares, int size);

void  roadmap_square_min    (int square, RoadMapPosition *position);

void  roadmap_square_edges  (int square, RoadMapArea *edges);

int   roadmap_square_index (int square);
int   roadmap_square_from_index (int index);

int   roadmap_square_view (int *square, int size);

extern roadmap_db_handler RoadMapSquareHandler;

#endif // _ROADMAP_SQUARE__H_
