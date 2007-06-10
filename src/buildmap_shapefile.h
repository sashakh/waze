/* buildmap_shapefile.h - Read shapefiles.
 *
 * LICENSE:
 *
 *   Copyright 2004 Stephen Woodbridge
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

#ifndef INCLUDED__BUILDMAP_SHAPEFILE__H
#define INCLUDED__BUILDMAP_SHAPEFILE__H

void buildmap_shapefile_dmti_process (const char *source,
                                 const char *county,
                                 int verbose);

void buildmap_shapefile_rnf_process (const char *source,
                                     const char *county,
                                     int verbose);

void buildmap_shapefile_dcw_process (const char *source,
                                     const char *county,
                                     int verbose);

void buildmap_shapefile_state_process (const char *source,
                                     const char *county,
                                     int verbose);

void buildmap_shapefile_province_process (const char *source,
                                     const char *county,
                                     int verbose);

#endif // INCLUDED__BUILDMAP_SHAPEFILE__H

