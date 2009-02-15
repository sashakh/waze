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
 * @brief Manage a trip: destination & waypoints.
 *
 * This file is somewhat overloaded with functionality : it appears to keep track
 * of quite a number of things (trip, waypoints, starting point/destination/..,
 * focal points, ..), it draws a visual indication (green dots and slim red lines
 * between them) of the current route, .. .
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_list.h"
#include "roadmap_gpx.h"
#include "roadmap_plugin.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_street.h"
#include "roadmap_tripdb.h"
#include "roadmap_trip.h"
#include "roadmap_layer.h"
#include "roadmap_point.h"

/**
 * @brief this array holds the current route (from the navigate plugin)
 */
static RoadMapTripSegment RoadMapTripSegments[MAX_NAV_SEGMENTS];
static int RoadMapTripNumSegments = 0;			/**< number of segments */
static int RoadMapTripCurrentSegment = 0;		/**< where we are from the start */

static RoadMapPosition	RoadMapTripLastPos;

RoadMapList	RoadMapTripWaypointHead;
RoadMapList	RoadMapTripRouteHead;
RoadMapList	RoadMapTripTrackHead;

/* route following flags */
route_head *RoadMapCurrentRoute = NULL;

/* These point at waypoints in the current route. */
waypoint *RoadMapTripDest = NULL;

/**
 * @brief
 */
void roadmap_tripdb_empty_list (void)
{
	RoadMapTripNumSegments = 0;
	RoadMapTripCurrentSegment = 0;

	RoadMapTripLastPos.longitude = -1;
	RoadMapTripLastPos.latitude = -1;

	roadmap_plugin_route_clear();
}

/**
 * @brief
 * @param pos
 */
void roadmap_trip_add_waypoint_iter (RoadMapPosition pos)
{
	waypoint *wpt = waypt_new();
	wpt->pos = pos;

	waypt_add(&RoadMapTripWaypointHead, wpt);
}

/**
 * @brief call this after having added all waypoints and ways
 */
void roadmap_trip_complete (void)
{
#if 0
        ROADMAP_LIST_MOVE(&TripWaypointHead, &tmp_waypoint_list);
        ROADMAP_LIST_MOVE(&TripRouteHead, &tmp_route_list);
        ROADMAP_LIST_MOVE(&TripTrackHead, &tmp_track_list);
#endif
	RoadMapCurrentRoute = (route_head *)ROADMAP_LIST_FIRST(&RoadMapTripRouteHead);
}

/**
 * @brief inefficient way (look up everything again) to ..
 * 	add a line between two positions to the trip,
 * 	find a street that appears to match (FIX ME simple/bar heuristic here),
 * 	and add the street to the list of streets that gets highlighted
 * @param from starting point of this line
 * @param to end point of this line
 * @param instr trip instruction associated with this
 */
#define	TRIP_MAX_NEIGHBOURS	200
#define	MAX_CAT			10
void roadmap_trip_add_way(RoadMapPosition from, RoadMapPosition to, enum RoadMapTurningInstruction instr)
{
	int	num, ncategories, maxneighbours, i;
	int	categories[MAX_CAT];
	RoadMapNeighbour	neighbours[TRIP_MAX_NEIGHBOURS];

	roadmap_log (ROADMAP_DEBUG, "trip_add_way %d", RoadMapTripNumSegments);

	RoadMapTripSegments[RoadMapTripNumSegments].instruction = instr;
	RoadMapTripSegments[RoadMapTripNumSegments].from_pos = from;
	RoadMapTripSegments[RoadMapTripNumSegments].to_pos = to;
	RoadMapTripNumSegments++;

	maxneighbours = 100;

	ncategories = roadmap_layer_navigable(0 /* car ?? */, categories, MAX_CAT);

	num = roadmap_street_get_closest(&from, categories, ncategories, neighbours, maxneighbours);
	if (num == TRIP_MAX_NEIGHBOURS) {
		roadmap_log (ROADMAP_WARNING, "trip_add_way: sizing %d insufficient",
				TRIP_MAX_NEIGHBOURS);
	}

	for (i=0; i<num; i++) {
		if (roadmap_math_distance(&neighbours[i].from, &from) < 20) {
			RoadMapStreetProperties	p;
			roadmap_street_get_properties(neighbours[i].line.line_id, &p);
			/* Get a highlight */
			roadmap_plugin_route_add(neighbours[i].line.line_id,
					neighbours[i].line.layer,
					neighbours[i].line.fips);
		}

	}
}

/**
 * @brief efficient approach to add a line between two points to the trip, and get it highlighted
 * @param from_point starting point of this line
 * @param to_point end point of this line
 * @param line the line
 * @param instr trip instruction associated with this
 */
#define	TRIP_MAX_NEIGHBOURS	200
#define	MAX_CAT			10
void roadmap_trip_add_point_way(int from_point, int to_point, PluginLine line, enum RoadMapTurningInstruction instr)
{
	roadmap_log (ROADMAP_DEBUG, "trip_add_way2 %d, line %d",
			RoadMapTripNumSegments, line.line_id);

	RoadMapTripSegments[RoadMapTripNumSegments].instruction = instr;
	roadmap_point_position(from_point, &RoadMapTripSegments[RoadMapTripNumSegments].from_pos);
	roadmap_point_position(to_point, &RoadMapTripSegments[RoadMapTripNumSegments].to_pos);
	RoadMapTripNumSegments++;

	roadmap_plugin_route_add(line.line_id, line.layer, line.fips);
}

/**
 * @brief function to call from iterator, meaning it gets called for all trip waypoints loaded
 * Need to convert two consecutive calls (each with a waypoint) into one line.
 * @param waypointp pointer to the current waypoint
 */
void roadmap_tripdb_waypoint_iter (const waypoint *waypointp)
{
    if (RoadMapTripLastPos.longitude == -1) {
	    RoadMapTripLastPos = waypointp->pos;
	    return;
    }

    roadmap_trip_add_way(RoadMapTripLastPos,
		    waypointp->pos,
		    (waypointp == RoadMapTripDest) ? TRIP_APPROACHING_DESTINATION : TRIP_CONTINUE);

    /* remember this waypoint for next line */
    RoadMapTripLastPos = waypointp->pos;
    return;
}


/**
 * @brief
 */
void roadmap_tripdb_initialize (void)
{
    ROADMAP_LIST_INIT(&RoadMapTripWaypointHead);
    ROADMAP_LIST_INIT(&RoadMapTripRouteHead);
}

/**
 * @brief
 * @param name
 */
void roadmap_tripdb_remove_point (const char *name)
{
#warning reimplement roadmap_trip_remove_point
#if 0   
	RoadMapTripPoint *result;
	if (name == NULL) {
		roadmap_trip_remove_dialog ();
		return;
	}

	result = roadmap_trip_search (name);
	if (result == NULL) {
		roadmap_log (ROADMAP_ERROR, "cannot delete: point %s not found", name);
		return;
	}

	if (result->predefined) {
		roadmap_log (ROADMAP_ERROR, "cannot delete: point %s is predefined", name);
		return;
	}

	if (RoadMapTripFocus == result || RoadMapTripDeparture == result
			|| RoadMapTripDestination == result) {
		roadmap_trip_unfocus ();
	}

	if (roadmap_math_point_is_visible (&result->map)) {
		roadmap_trip_refresh_needed();
	}

	roadmap_list_remove (&result->link);
	free (result->id);
	free (result->sprite);
	free(result);

	RoadMapTripModified = 1;
	roadmap_screen_refresh();
#endif
}

/**
 *  * @brief clear the current trip
 *   */
void roadmap_tripdb_clear (void)
{
	waypt_flush_queue (&RoadMapTripWaypointHead);
	route_flush_queue (&RoadMapTripRouteHead);
	route_flush_queue (&RoadMapTripTrackHead);

	roadmap_trip_set_modified(1);

	RoadMapCurrentRoute = NULL;
	roadmap_trip_unset_route_focii ();

	roadmap_trip_refresh_needed();
}
