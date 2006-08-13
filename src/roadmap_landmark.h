/* roadmap_landmark.h - Personal waypoints.
 *
 * LICENSE:
 *
 *   Copyright 2005 Paul G. Fox
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

#ifndef INCLUDE__ROADMAP_LANDMARK__H
#define INCLUDE__ROADMAP_LANDMARK__H

void  roadmap_landmark_display (void);
void  roadmap_landmark_initialize (void);
void  roadmap_landmark_save (void);
void  roadmap_landmark_load (void);
void roadmap_landmark_merge(void);
int roadmap_landmark_count();
void roadmap_landmark_add(waypoint *waypointp);
void roadmap_landmark_remove(waypoint *waypointp);
RoadMapList * roadmap_landmark_list(void);
int roadmap_landmark_is_refresh_needed (void);
void roadmap_landmark_draw_waypoint
        (const waypoint *waypointp, const char *sprite);

#endif // INCLUDE__ROADMAP_LANDMARK__H

