/* roadmap_locator.c - Locate the map to which a specific place belongs.
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

#ifndef _ROADMAP_LOCATOR__H_
#define _ROADMAP_LOCATOR__H_

#include "roadmap_types.h"

#define ROADMAP_US_OK      0
#define ROADMAP_US_NOSTATE -1
#define ROADMAP_US_NOCITY  -2
#define ROADMAP_US_NOMAP   -3

typedef int (*RoadMapInstaller) (int fips);


void roadmap_locator_declare (RoadMapInstaller download);

int roadmap_locator_by_position
        (const RoadMapPosition *position, int **fips);
int roadmap_locator_by_state (const char *state_symbol, int **fips);

int  roadmap_locator_by_city     (const char *city, const char *state);
int  roadmap_locator_activate    (int fips);
int  roadmap_locator_active      (void);

int   roadmap_locator_category_count (void);
char *roadmap_locator_category_name (int index);

void roadmap_locator_close (int fips);

RoadMapString roadmap_locator_get_state (const char *state);

#endif // _ROADMAP_LOCATOR__H_

