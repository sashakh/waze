/*
 * LICENSE:
 *   Copyright (c) 2008, 2009 by Danny Backx.
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
 * @brief simple (shortest path) route calculation
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_point.h"
#include "roadmap_line.h"
#include "roadmap_locator.h"
#include "roadmap_layer.h"
#include "roadmap_square.h"
#include "roadmap_math.h"
#include "roadmap_turns.h"
#include "roadmap_dialog.h"
#include "roadmap_main.h"
// #include "roadmap_line_route.h"
#include "roadmap_street.h"
#include "roadmap_fuzzy.h"
#include "roadmap_county.h"

#include "roadmap_navigate.h"
#include "roadmap_turns.h"

#include "navigate.h"
// #include "navigate_traffic.h"
// #include "navigate_graph.h"
#include "navigate_cost.h"

#include "navigate_route.h"

#define	maxblacklist 300
static int blacklist[maxblacklist];
int nblacklist = 0;

static int NavigateEndAtThisDistance = 50;	/**< If we're this close, we're there. */

/**
 * @brief add this line to the blacklist : never to be visited
 * @param line
 */
static void navigate_simple_blacklist_add(int line)
{
	int	i;

	if (nblacklist == maxblacklist)
		roadmap_log(ROADMAP_FATAL, "blacklist full");
	for (i=0; i<nblacklist; i++)
		if (blacklist[i] == line)
			return;
	blacklist[nblacklist++] = line;
	roadmap_log (ROADMAP_DEBUG, "Blacklisting line %d", line);
}

/**
 * @brief check whether this line is on the blacklist
 * @param line
 * @return true if it is
 */
static int navigate_simple_on_blacklist(int line)
{
	int i;

	for (i=0; i<nblacklist; i++)
		if (blacklist[i] == line) {
			return 1;
		}
	return 0;
}

/**
 * @brief return the lines adjacent to the given point
 * @param point input parameter
 * @param lines existing array that will be filled up
 * @param maxlines number of entries that we can write in
 * @return the number of lines found
 */
int navigate_simple_lines_closeby(int point, PluginLine *lines, const int maxlines)
{
	/* Use the line/bypoint data in newer maps, or fail */
	int	i, line_id;

	for (i=0; (line_id = roadmap_line_point_adjacent(point, i)) && (i < maxlines); i++) {
		lines[i].line_id = line_id;
		lines[i].layer = roadmap_line_get_layer(line_id);
		lines[i].fips = roadmap_line_get_fips(line_id);
	}
	return i;
}

/**
 * @brief not sure how to use this
 * @param iter
 * @return
 */
static int navigate_simple_algo_cost(NavigateIteration *iter)
{
	int	cost;

	if (iter == 0)
		return 0;
	if (iter->segment == 0)
		return 0;
	cost = navigate_cost_time(iter->next->segment->line.line_id,
		iter->next->segment->line_direction,
		iter->segment->time,
		iter->segment->line.line_id,
		iter->segment->line_direction);

	iter->next->segment->time = cost;
	return cost;
}

/**
 * @brief move one step forward from the current position
 * @param algo the algorithm pointer
 * @param stp pointer to the navigation status to work on
 * @return success (-1 is failure, 0 is backtrack, 1 is success)
 */
static int navigate_simple_algo_step(NavigateAlgorithm *algo, NavigateStatus *stp)
{
	int			maxlines = 10, nlines;
	PluginLine		*l, lines[10];
	int			point, best, newpt, newpt2, bestnewpt;
	RoadMapStreetProperties	prop;
	int			bestdist, dist, dist2, rev, bestheuristic;
	int			i, found;
	NavigateIteration	*p;
	RoadMapPosition		newpos, newpos2, bestnewpos;

	NavigateSegment		*s = stp->current->segment;
	NavigateSegment		bestseg;
	RoadMapPosition		to_pos = stp->last->segment->to_pos;

	/* Figure out where to start */
	nlines = 0;
	if (s->from_point) {
		point = s->from_point;
		nlines = navigate_simple_lines_closeby(s->from_point, lines, maxlines);
	}

	if (nlines == 0) {
		l = roadmap_navigate_position2line(s->from_pos);
		point = roadmap_line_from_point(l->line_id);
		nlines = navigate_simple_lines_closeby(point, lines, maxlines);

		if (nlines == 0) {
			point = roadmap_line_to_point(l->line_id);
			nlines = navigate_simple_lines_closeby(point, lines, maxlines);
		}
	}

	if (nlines == 0) {
		/* This is too simplistic, maybe backtrack from here too ? */
		return -1;
	}

	/* Allocate the next entry */
	stp->current->next = calloc(1, sizeof(struct NavigateIteration));
	stp->current->next->prev = stp->current;

	stp->current->next->segment = calloc(1, sizeof(struct NavigateSegment));

	roadmap_point_position(point, &newpos);
	roadmap_log(ROADMAP_DEBUG, "Looking from point %d (%d %d), %d lines",
			point,
			newpos.longitude, newpos.latitude,
			nlines);

	/* We have a set of lines. Select the best one. */
	bestdist = stp->maxdist * 2;
	bestheuristic = -1;
	best = -1;

	for (i=0; i<nlines; i++) {
		roadmap_street_get_properties(lines[i].line_id, &prop); /* Only for debug */

		/* Have we already been here ? */
		for (p=stp->first, found=0; p && found == 0; p=p->next)
			if (lines[i].line_id == p->segment->line.line_id)
				found = 1;
		if (found) {
			roadmap_log (ROADMAP_DEBUG, " line %d already visited",
					lines[i].line_id);
			continue;
		}

		/* Is this line forbidden ? */
		if (navigate_simple_on_blacklist(lines[i].line_id)) {
			roadmap_log (ROADMAP_DEBUG, " line %d [%d] {%s} blacklisted",
				i, lines[i].line_id,
				roadmap_street_get_full_name(&prop));
			continue;
		}

		/* Oneway street ? */
		if (roadmap_line_get_oneway(lines[i].line_id) == ROADMAP_LINE_DIRECTION_ONEWAY) {
			roadmap_log (ROADMAP_WARNING, "Not taking oneway street %d (%s)",
					lines[i].line_id,
					roadmap_street_get_full_name(&prop));
			continue;
		}

		/* Use the turns DB : is this turn allowed ? */
		if (roadmap_turns_find_restriction (s->from_point,
			stp->current->prev ? stp->current->prev->segment->line.line_id : 0,
			stp->current->segment->line.line_id)) {

			roadmap_log (ROADMAP_WARNING, "Turn forbidden at %d from %d to %d",
				s->from_point,
				stp->current->prev ? stp->current->prev->segment->line.line_id : 0,
				stp->current->segment->line.line_id);
			continue;
		}

		stp->current->next->segment->line = lines[i];
		newpt2 = roadmap_line_from_point(lines[i].line_id);
		if (point == newpt2) {
			newpt = roadmap_line_to_point(lines[i].line_id);
			roadmap_point_position(newpt, &newpos);
			dist = roadmap_math_distance(&newpos, &to_pos);

			stp->current->next->segment->line_direction = rev = 0;
			stp->current->next->segment->from_point = newpt;
			stp->current->next->segment->from_pos = newpos;
			stp->current->next->segment->dist_from_destination = dist;
			stp->current->segment->to_point = newpt;
			stp->current->segment->to_pos = newpos;
		} else {
			roadmap_point_position(newpt2, &newpos2);
			dist2 = roadmap_math_distance(&newpos2, &to_pos);

			stp->current->next->segment->line_direction = rev = 1;
			stp->current->next->segment->from_point = newpt2;
			stp->current->next->segment->from_pos = newpos2;
			stp->current->next->segment->dist_from_destination = dist2;
			stp->current->segment->to_point = newpt2;
			stp->current->segment->to_pos = newpos2;
		}

		roadmap_log (ROADMAP_DEBUG, "Line %d (%s) point %d pos %d %d",
				lines[i].line_id,
				roadmap_street_get_full_name(&prop),
				stp->current->next->segment->from_point,
				stp->current->next->segment->from_pos.longitude,
				stp->current->next->segment->from_pos.latitude);

		stp->current->next->segment->distance = stp->current->segment->distance +
			roadmap_math_distance(&stp->current->segment->from_pos,
					&stp->current->segment->to_pos);

		(void) algo->cost_fn(stp->current);

		stp->current->next->segment->heuristic =
			stp->current->next->segment->dist_from_destination;

		if (bestheuristic < 0 || stp->current->next->segment->heuristic < bestheuristic) {
			bestseg = *stp->current->next->segment;
			best = i;
			bestdist = stp->current->next->segment->distance;
			bestnewpt = stp->current->segment->to_point;
			bestnewpos = stp->current->segment->to_pos;
			bestheuristic = stp->current->next->segment->heuristic;
			roadmap_log (ROADMAP_DEBUG,
					"Better ix %d, dist %d, pt %d, pos %d %d, hr %d",
					best, bestdist, bestnewpt,
					bestnewpos.longitude, bestnewpos.latitude,
					bestheuristic);
		}

	}

	/* Is the result really bad ? */
	if (best < 0) {
		if (stp->iteration < 2) {
			roadmap_log (ROADMAP_WARNING, "On an island ?");
			return -1;
		}

		roadmap_log (ROADMAP_DEBUG, "Backtrack ..");

		navigate_simple_blacklist_add(stp->current->segment->line.line_id);

		stp->current = stp->current->prev;
		if (stp->current == 0)
			stp->current = stp->first;
		return 0;
	}

	/* Put the data for the "best" selection back */
	stp->current->next->segment->distance = bestdist;
	stp->current->next->segment->from_point =
		stp->current->segment->to_point = bestnewpt;
	stp->current->next->segment->from_pos =
		stp->current->segment->to_pos = bestnewpos;
	stp->current->next->segment->heuristic = bestheuristic;
	*stp->current->next->segment = bestseg;

	/* Move along */
	stp->current = stp->current->next;

	/* Store cost information */
	stp->current->cost.distance = stp->current->segment->distance;
	stp->current->cost.time = stp->current->segment->time;

	roadmap_log (ROADMAP_DEBUG, "New point %d distance %d, time %d",
			bestnewpt,
			stp->current->segment->distance,
			stp->current->segment->time);
	/* return */
	return 1;
}

/**
 * @brief Figure out whether the routing algorithm needs to do another iteration
 * @param stp pointer to all the navigation info
 * @return 0 to keep going, 1 to stop (reached the destination)
 */
static int navigate_simple_algo_end(NavigateStatus *stp)
{
	RoadMapPosition	p1, p2;
	int dist;

	if (stp->current->segment->from_point)
		roadmap_point_position(stp->current->segment->from_point, &p1);
	else
		p1 = stp->current->segment->from_pos;

	if (stp->last->segment->to_point)
		roadmap_point_position(stp->last->segment->to_point, &p2);
	else
		p2 = stp->last->segment->to_pos;

	dist = roadmap_math_distance(&p1, &p2);

	roadmap_log (ROADMAP_DEBUG, "Distance is now %d", dist);
	if (dist < NavigateEndAtThisDistance)
		return 1;
	return 0;	/* Keep going */
#if 0
	if (!stop && to_line && to_line->line_id == lines[best].line_id)
			stop++;
		if (!stop && roadmap_line_from_point(to_line->line_id) == bestnewpt)
			stop++;
		if (!stop && roadmap_line_to_point(to_line->line_id) == bestnewpt)
			stop++;
	return 0;	/* Nearly infinite loop */
	return 1;	/* Stop immediately */
#endif
}

/**
 * @brief structure to pass an "algorithm" to the navigation engine
 */
NavigateAlgorithm SimpleAlgo = {
	"Simple navigation",		/**< name of this algorithm */
	0,				/**< cannot go both ways */
	500,				/**< max #iterations */
	navigate_simple_algo_cost,	/**< cost function */
	navigate_simple_algo_step,	/**< step function */
	navigate_simple_algo_end	/**< end function */
};

/**
 * @brief register the algorithm
 */
void navigate_simple_initialize(void)
{
	navigate_algorithm_register(&SimpleAlgo);
}
