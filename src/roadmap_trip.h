/* roadmap_trip.h - Manage a trip: destination & waypoints.
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

#ifndef INCLUDE__ROADMAP_TRIP__H
#define INCLUDE__ROADMAP_TRIP__H

#include "roadmap_types.h"
#include "roadmap_list.h"


void  roadmap_trip_set_point (const char *name, RoadMapPosition *position);

void  roadmap_trip_set_mobile (const char *name,
                               RoadMapPosition *position,
                               int speed,
                               int direction);

void  roadmap_trip_remove_point (char *name);


void  roadmap_trip_set_focus (const char *name, int rotate);

int   roadmap_trip_is_focus_changed  (void);
int   roadmap_trip_is_focus_moved    (void);
int   roadmap_trip_is_refresh_needed (void);

int   roadmap_trip_get_orientation (void);

const RoadMapPosition *roadmap_trip_get_focus_position (void);


void  roadmap_trip_start (int rotate);

void  roadmap_trip_repaint (void);


void  roadmap_trip_clear (void);

void  roadmap_trip_initialize (void);

char *roadmap_trip_current (void);

/* In the two primitives that follow, the name is either NULL (i.e.
 * open a dialog to let the user enter one), or an explicit name.
 */
void  roadmap_trip_load (const char *name);
void  roadmap_trip_save (const char *name);

#endif // INCLUDE__ROADMAP_TRIP__H
