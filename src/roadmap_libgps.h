/* roadmap_libgps.h - a module to interact with gpsd using its library.
 *
 * LICENSE:
 *
 *   Copyright 2017 Sasha Khapyosky
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
 *
 * DESCRIPTION:
 *
 *   This module hides the gpsd library API.
 */

#ifndef INCLUDE__ROADMAP_LIBGPS__H
#define INCLUDE__ROADMAP_LIBGPS__H

#include "roadmap_net.h"
#include "roadmap_gps.h"
#include "roadmap_input.h"

#ifdef USE_LIBGPS
#include <gps.h>
#endif

extern void roadmap_libgps_subscribe_to_navigation(RoadMapGpsdNavigation
						   navigation);
extern void roadmap_libgps_subscribe_to_satellites(RoadMapGpsdSatellite
						   satellite);
extern void roadmap_libgps_subscribe_to_dilution(RoadMapGpsdDilution dilution);

extern int roadmap_libgps_connect(RoadMapIO * io, const char *hostname,
				  const char *port);
extern int roadmap_libgps_input(void *user_context);

#endif // INCLUDE__ROADMAP_LIBGPS__H
