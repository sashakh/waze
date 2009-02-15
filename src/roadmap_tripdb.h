/*
 * LICENSE:
 *
 *   Copyright (c) 2008, 2009 Danny Backx
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

/**
 * @file
 * @brief maintain a database with a trip
 */

#ifndef	_ROADMAP_TRIPDB_H_
#define	_ROADMAP_TRIPDB_H_

#include "roadmap_gpx.h"

extern RoadMapList	RoadMapTripWaypointHead;
extern RoadMapList	RoadMapTripRouteHead;
extern RoadMapList	RoadMapTripTrackHead;
extern route_head	*RoadMapCurrentRoute;
extern waypoint		*RoadMapTripDest;

/*
 * @brief turning instructions
 */
enum RoadMapTurningInstruction {
	TRIP_TURN_LEFT = 0,
	TRIP_TURN_RIGHT,
	TRIP_KEEP_LEFT,
	TRIP_KEEP_RIGHT,
	TRIP_CONTINUE,
	TRIP_APPROACHING_DESTINATION,
	TRIP_LAST_DIRECTION          /**< not to be used */
};

/**
 * @brief this structure holds an entry in the route
 */
typedef struct {
	PluginLine           line;
	int                  line_direction;
	PluginStreet         street;
	RoadMapPosition      from_pos;
	RoadMapPosition      to_pos;
	RoadMapPosition      shape_initial_pos;
	int                  first_shape;
	int                  last_shape;
	RoadMapShapeItr      shape_itr;
	enum RoadMapTurningInstruction	instruction;
	int                  group_id;
	int                  distance;
	int                  cross_time;
} RoadMapTripSegment;

#define MAX_NAV_SEGMENTS 1500

void roadmap_tripdb_empty_list (void);
void roadmap_tripdb_add_waypoint_iter (RoadMapPosition pos);
void roadmap_trip_complete (void);
void roadmap_trip_add_way(RoadMapPosition from, RoadMapPosition to, enum RoadMapTurningInstruction instr);
void roadmap_trip_add_point_way(int from_point, int to_point, PluginLine line, enum RoadMapTurningInstruction instr);
void roadmap_tripdb_waypoint_iter (const waypoint *waypointp);
void roadmap_tripdb_initialize (void);
void roadmap_tripdb_remove_point (const char *name);
void roadmap_tripdb_clear(void);

#endif
