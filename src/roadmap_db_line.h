/* roadmap_db_line.h - the format of the address line table used by RoadMap.
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
 *   The RoadMap lines are described by the following table:
 *
 *   line          the ID of the line and its from and to points.
 */

#ifndef _ROADMAP_DB_LINE__H_
#define _ROADMAP_DB_LINE__H_

#include "roadmap_types.h"

typedef struct {  /* table line */

   int from;
   int to;

} RoadMapLine;

typedef struct { /* tables bysquare1 and bysquare2 */

   int first[ROADMAP_CATEGORY_RANGE];
   int last;

} RoadMapLineBySquare;

#endif // _ROADMAP_DB_LINE__H_

