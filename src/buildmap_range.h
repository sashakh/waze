/* buildmap_range.h - Build a street address range table & index for RoadMap.
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

#ifndef INCLUDED__BUILDMAP_RANGE__H
#define INCLUDED__BUILDMAP_RANGE__H

#include "roadmap_types.h"

void buildmap_range_merge (int frleft,  int toleft,
                           int frright, int toright,
                           int *from,   int *to);

int  buildmap_range_add
        (int line, int street,
         int fradd, int toadd, RoadMapZip zip, RoadMapString city);
void buildmap_range_add_no_address (int line, int street);
void buildmap_range_add_place      (RoadMapString place, RoadMapString city);

#endif // INCLUDED__BUILDMAP_RANGE__H
