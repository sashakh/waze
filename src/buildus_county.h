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

#ifndef _BUILDUS_COUNTY__H_
#define _BUILDUS_COUNTY__H_

#include "roadmap_types.h"

void buildus_county_initialize (void);
int  buildus_county_add (int fips,
                         RoadMapString name, RoadMapString state_symbol);
void buildus_county_add_state (RoadMapString name, RoadMapString symbol);
void buildus_county_add_city (int fips, RoadMapString city);
void buildus_county_set_position (int fips,
                                  int max_longitude, int max_latitude,
                                  int min_longitude, int min_latitude);
void buildus_county_sort (void);
void buildus_county_save (void);
void buildus_county_summary (void);

#endif // _BUILDUS_COUNTY__H_

