/* roadmap_db_county.h - the format of the county table used by RoadMap.
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
 *   The RoadMap counties are described by the following table:
 *
 *   county/data     the name, position and FIPS code of the county, by state.
 *   county/city     the name, position and FIPS code of each city, by state.
 *   county/bystate  pointers to the other tables for each state.
 */

#ifndef INCLUDE__ROADMAP_DB_COUNTY__H
#define INCLUDE__ROADMAP_DB_COUNTY__H

#include "roadmap_types.h"

typedef struct {  /* table county/data */

   int fips;

   short reserved;

   RoadMapString name;

   int max_longitude;
   int max_latitude;

   int min_longitude;
   int min_latitude;

} RoadMapCounty;

typedef struct {  /* table county/city */

   unsigned short fips;
   RoadMapString city;

} RoadMapCountyCity;

typedef struct {  /* table county/bystate */

   RoadMapString name;
   RoadMapString symbol;

   unsigned short first_county;
   unsigned short last_county;

   unsigned short first_city;
   unsigned short last_city;

   int max_longitude;
   int max_latitude;

   int min_longitude;
   int min_latitude;

} RoadMapCountyByState;

#endif // INCLUDE__ROADMAP_DB_COUNTY__H

