/* roadmap_object.h - manage the roadmap moving objects.
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
 * DESCRIPTION:
 *
 *   This module manages a dynamic list of objects to be displayed on the map.
 *   The objects are usually imported from external applications for which
 *   a RoadMap driver has been installed (see roadmap_driver.c).
 *
 *   These objects are dynamic and not persistent (i.e. these are not points
 *   of interest).
 */

#ifndef INCLUDE__ROADMAP_OBJECT__H
#define INCLUDE__ROADMAP_OBJECT__H

#include "roadmap_types.h"


void roadmap_object_add (const char *id,
                         const char *name,
                         const char *sprite,
                         const RoadMapPosition *position);

void roadmap_object_move (const char *id,
                          const RoadMapPosition *position);

void roadmap_object_remove (const char *id);

void roadmap_object_draw (void);

void roadmap_object_initialize (void);

#endif // INCLUDE__ROADMAP_OBJECT__H

