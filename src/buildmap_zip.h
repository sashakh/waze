/* buildmap_zip.h - Build a zip code table for RoadMap.
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

#ifndef INCLUDED__BUILDMAP_ZIP__H
#define INCLUDED__BUILDMAP_ZIP__H

#include "roadmap_types.h"

void buildmap_zip_set_county (int county_code);

RoadMapZip buildmap_zip_add (int zip, int longitude, int latitude);

int  buildmap_zip_get_zip_code  (RoadMapZip index);
int  buildmap_zip_get_longitude (RoadMapZip index);
int  buildmap_zip_get_latitude  (RoadMapZip index);

RoadMapZip buildmap_zip_locate (int zip);

#endif // INCLUDED__BUILDMAP_ZIP__H

