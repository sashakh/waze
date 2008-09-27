/* buildmap_city.h - Build a city table & index for BuildMap.
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

#ifndef _BUILDMAP_CITY__H_
#define _BUILDMAP_CITY__H_

#include "roadmap_types.h"

void          buildmap_city_initialize (void);
void          buildmap_city_add        (int fips, int year, RoadMapString name);
RoadMapString buildmap_city_get_name   (int fips);

void          buildmap_city_save       (void);
void          buildmap_city_summary    (void);
void          buildmap_city_reset      (void);

#endif /* _BUILDMAP_CITY__H_ */

