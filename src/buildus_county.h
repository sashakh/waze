/* buildus_county.h - Build a county table & index for RoadMap.
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

#ifndef INCLUDED__BUILDUS_COUNTY__H
#define INCLUDED__BUILDUS_COUNTY__H

#include "roadmap_types.h"

int  buildus_county_add (int fips,
                         RoadMapString name, RoadMapString state_symbol);
void buildus_county_add_state (RoadMapString name, RoadMapString symbol);
void buildus_county_add_city (int fips, RoadMapString city);
void buildus_county_set_position (int fips, const RoadMapArea *edges);

#endif // INCLUDED__BUILDUS_COUNTY__H

