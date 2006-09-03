/* editor_track_main.h - Main file of editor track module
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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

#ifndef INCLUDE__EDITOR_TRACK_MAIN__H
#define INCLUDE__EDITOR_TRACK_MAIN__H

#include "roadmap_gps.h"
#include "roadmap_canvas.h"

typedef struct {

   int id;
   int plugin_id;

} NodeNeighbour;

#define NODE_NEIGHBOUR_NULL  {-1, -1}

typedef struct {

   int type;
   int point_id;
   
} TrackNewSegment;

#define TRACK_ROAD_REG        1
#define TRACK_ROAD_TURN       2
#define TRACK_ROAD_ROUNDABOUT 3
#define TRACK_ROAD_CONNECTION 4

#define POINT_UNKNOWN         0x1
#define POINT_GAP             0x2

void editor_track_initialize (void);
int editor_track_point_distance (void);

RoadMapPosition *track_point_pos (int index);
RoadMapGpsPosition *track_point_gps (int index);
time_t track_point_time (int index);
int editor_track_draw_current (RoadMapPen pen);
void editor_track_end (void);
void editor_track_reset (void);

#endif // INCLUDE__EDITOR_TRACK_MAIN__H

