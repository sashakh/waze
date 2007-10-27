/* roadmap_vii.h - a module to interact with the VII daemon.
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
 *
 * DESCRIPTION:
 *
 *   This module hides the VII specific protocol.
 */

#ifndef INCLUDE__ROADMAP_VII__H
#define INCLUDE__ROADMAP_VII__H

#include "roadmap_net.h"
#include "roadmap_input.h"


#define ROADMAP_NO_VALID_DATA    -512000000

typedef void (*RoadMapViiNavigation) (char status,
                                       int gps_time,
                                       int latitude,
                                       int longitude,
                                       int altitude,
                                       int speed,
                                       int steering);

void roadmap_vii_subscribe_to_navigation (RoadMapViiNavigation navigation);

RoadMapSocket roadmap_vii_connect (const char *name);

int roadmap_vii_decode (void *user_context,
                        void *decoder_context, char *sentence);

#endif // INCLUDE__ROADMAP_VII__H

