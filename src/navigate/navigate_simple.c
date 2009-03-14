#undef	HAVE_RUNTIME_CACHE
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
#include "roadmap_line_route.h"
#include "roadmap_street.h"
#include "roadmap_fuzzy.h"
#include "roadmap_county.h"

#include "roadmap_navigate.h"

#include "navigate.h"
// #include "navigate_traffic.h"
// #include "navigate_graph.h"
#include "navigate_cost.h"

#include "navigate_route.h"

#define	maxblacklist 100
static int blacklist[maxblacklist];
int nblacklist = 0;

/**
 * @brief add this line to the blacklist : never to be visited
 * @param line
 */
static void navigate_simple_blacklist_add(int line)
{
	if (nblacklist == maxblacklist)
		roadmap_log(ROADMAP_FATAL, "blacklist full");
	blacklist[nblacklist++] = line;
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

#ifdef HAVE_RUNTIME_CACHE
/*
 * Cache
 */
enum direction {
	D_NORMAL,
	D_REVERSE
};

static int cache_point_count = 0,
	   cache_line_count = 0;

struct cache_line {
	int line;
	int layer;
	int fips;
};

#define	INIT_LINES_PER_POINT	5
#define	INIT_POINTS		80

struct cache_per_point {
	int	point;
	int	num_lines, max_lines;
	struct cache_line	*lines;
};

struct cache_per_point	*cache = NULL;
int			num_cache = 0, max_cache = 0;
int			cache_square = 0;

/**
 * @brief build a cache of the line/point combinations in this square, organize per point
 * @param line
 * @param point
 * @param reverse
 */
static void cache_point(int line, int layer, int fips, int point, enum direction reverse)
{
	int	i, j;

	for (i=0; i<num_cache; i++) {
		if (cache[i].point == point) {
			for (j=0; j<cache[i].num_lines; j++) {
				if (cache[i].lines[j].line == line)
					return;
			}
			/* Found the entry for this point, but not this line, so store the line */
			if (cache[i].num_lines == cache[i].max_lines) {
				/* If necessary, expand the array first */
				cache[i].max_lines += INIT_LINES_PER_POINT;
				cache[i].lines = (struct cache_line *)realloc((void *)cache[i].lines,
						cache[i].max_lines * sizeof(struct cache_line));
			}
			cache[i].lines[cache[i].num_lines].line = line;
			cache[i].lines[cache[i].num_lines].fips = fips;
			cache[i].lines[cache[i].num_lines].layer = layer;
			cache[i].num_lines++;
			cache_line_count++;
			return;
		}
	}

	/* Haven't seen this point yet */
	if (num_cache == max_cache) {
		/* Need more space */
		max_cache += INIT_POINTS;
		cache = (struct cache_per_point *)realloc((void *)cache,
				max_cache * sizeof(struct cache_per_point));
	}

	/* New point, allocate initial array */
	cache[num_cache].point = point;
	cache[num_cache].num_lines = 0;
	cache[num_cache].max_lines = INIT_LINES_PER_POINT;
	cache[num_cache].lines = (struct cache_line *)calloc(cache[num_cache].max_lines,
			sizeof(struct cache_line));
	cache[num_cache].lines[cache[num_cache].num_lines].line = line;
	cache[num_cache].lines[cache[num_cache].num_lines].fips = fips;
	cache[num_cache].lines[cache[num_cache].num_lines].layer = layer;
	cache[num_cache].num_lines++;
	num_cache++;

	cache_point_count++;
	cache_line_count++;
}

/**
 * @brief free the cache
 */
static void free_cache(void)
{
	int	i;

	for (i=0; i<num_cache; i++) {
		free((void *)cache[i].lines);
	}

	free(cache);
	num_cache = max_cache = 0;
	cache = NULL;
	cache_point_count = cache_line_count = 0;
}

/**
 * @brief look up points and lines in this square
 * @param square
 */
static void navigate_simple_square_inventory(int square)
{
#define	maxlayers	10
	int	nlayer, layers[maxlayers], i, layer, fips,
		nlines, first, last, line, from_point, to_point;
	RoadMapPosition	pos;

	if (square == cache_square)
		return;
	free_cache();
	cache_square = square;

	nlayer = roadmap_layer_navigable(roadmap_navigate_get_mode(), layers, maxlayers);

	nlines = 0;
	for (i=0; i<nlayer; i++) {
		layer = layers[i];

		/* Query streets per layer, due to structure of RoadMap database */
		if (roadmap_line_in_square(square, layer, &first, &last)) {
			nlines += last - first + 1;

			for (line = first; line <= last; line++) {
				roadmap_line_points(line, &from_point, &to_point);
				/* We know the from_point is in the square */
				cache_point(line, layer, fips, from_point, D_NORMAL);
				/* Check whether the to-point is in the square */
				roadmap_point_position(to_point, &pos);
				if (roadmap_county_by_position(&pos, &fips, 1) == 1
				 && roadmap_square_search(&pos) == square)
					cache_point(line, layer, fips, to_point, D_REVERSE);
			}

		}

		if (roadmap_line_in_square2(square, layer, &first, &last)) {
			nlines += last - first + 1;

			for (line = first; line <= last; line++) {
				int l = roadmap_line_get_from_index2(line);
				to_point = roadmap_line_to_point(l);
				cache_point(l, layer, fips, to_point, D_NORMAL);
			}
		}
		roadmap_log (ROADMAP_DEBUG, "closeby : %d lines %d points after layer %d",
				cache_line_count, cache_point_count, layer);
	}

}
#endif	/* HAVE_RUNTIME_CACHE */

static void debug(int point, int maxlines)
{
	int	i, line_id;
	fprintf(stderr, "CloseBy from map (%d) -> ", point);
	for (i=0; (line_id = roadmap_line_point_adjacent(point, i)) && (i < maxlines); i++) {
		fprintf(stderr, "%d ", line_id);
	}
	fprintf(stderr, "\n");

#if 0
	static int once = 1;
	if (once) {
		once = 0;
		for (i=0; i<1708; i++) {
			int j;
			fprintf(stderr, "LBP[%d] -> ", i);
			for (j=0; roadmap_line_point_adjacent(i, j); j++)
				fprintf(stderr, " %d ", roadmap_line_point_adjacent(i, j));
			fprintf(stderr, "\n");
		}
	}
#endif
#if 0
	for (i=0; i<1708; i++) {
		if (roadmap_line_point_adjacent(i, 0) == 21
			&& roadmap_line_point_adjacent(i, 1) == 22
			&& roadmap_line_point_adjacent(i, 2) == 23) {
			fprintf(stderr, "YOW YOW YOW %d -> %d (21 22 23)\n", point, i);
		}
	}
#endif
}

/**
 * @brief
 * @param pos
 * @param point
 * @param line
 * @param lines
 * @param maxlines
 * @return
 */
int navigate_simple_lines_closeby(RoadMapPosition pos, int point, PluginLine *line,
		PluginLine *lines, const int maxlines)
{
#if 1
	debug(point, maxlines);
	// if (point == 455) debug(367, maxlines);
	// if (point == 461) debug(382, maxlines);
#endif
#ifndef HAVE_RUNTIME_CACHE
	/* Use the line/bypoint data in newer maps, or fail */
	int	i, line_id;
	RoadMapStreetProperties prop;

	for (i=0; (line_id = roadmap_line_point_adjacent(point, i)) && (i < maxlines); i++) {
		roadmap_street_get_properties(line_id, &prop);
		lines[i].line_id = line_id;
		// lines[i].fips = prop.fips;
		// lines[i].layer = prop.layer;
	}
	return i;
#else /* HAVE_RUNTIME_CACHE */
#define	maxlayers	10
	int	square, i, j,
		nlines, found, other, l;

	roadmap_log (ROADMAP_DEBUG, "navigate_simple_lines_closeby(%d,%d) point %d",
			pos.longitude, pos.latitude, point);

	square = roadmap_square_search(&pos);

	/* Make a point/line inventory of the whole square */
	navigate_simple_square_inventory(square);

	/* Figure out where to go now */
	for (i=0,found=0; i<num_cache && !found; i++) {
		if (cache[i].point == point) {
			found = 1;
			break;
		}
	}
	if (! found) {
		roadmap_log (ROADMAP_WARNING, "Yow : not found");
		return 0;
	}

	for (nlines=j=0; j<cache[i].num_lines; j++) {
		RoadMapStreetProperties prop;
		l = cache[i].lines[j].line;

		roadmap_street_get_properties(cache[i].lines[j].line, &prop);

		other = roadmap_line_from_point(l);
		if (other == point)
			other = roadmap_line_to_point(l);

		lines[nlines].line_id = l;
		lines[nlines].fips = cache[i].lines[j].fips;
		lines[nlines].layer = cache[i].lines[j].layer;
		nlines++;
	}
	return nlines;
#endif /* HAVE_RUNTIME_CACHE */
}

/**
 * @brief calculate the basic route.
 *
 * Called from navigate_calc_route and navigate_recalc_route.
 * Result will be placed in the "segments" parameter, number of steps in "size".
 *
 * @param from_line
 * @param from_pos
 * @param to_line
 * @param to_pos
 * @param segments
 * @param size
 * @param flags either NEW_ROUTE or RECALC_ROUTE
 * @return track time
 */
int navigate_simple_get_segments (PluginLine *from_line,
                                 RoadMapPosition from_pos,
                                 PluginLine *to_line,
                                 RoadMapPosition to_pos,
                                 NavigateSegment *segments,
                                 int *size,
                                 int *flags)
{
	static int do_this_once = 1;
	int stop;

#define	MAX_CAT		10	/* Not interesting */
#define	MAX_ITER	100	/* Max #iterations */

	int			ncategories, i, j, found;
	int			categories[MAX_CAT], dist, maxdist, bestdist, dist2, rev, oldrev;
	int			iteration, max_iterations, loop;
	int			best, t;
	int			tracktime, cost, oldcost;
	RoadMapPosition		CurrentPos, newpos, newpos2, bestnewpos;
	int			navmode;
	int			point, newpt, newpt2, bestnewpt;

	NavigateCostFn		cost_get;

	cost_get = navigate_cost_get();

	/* Query the categories we can navigate in */
	navmode = roadmap_navigate_get_mode();
	ncategories = roadmap_layer_navigable(navmode, categories, MAX_CAT);

	max_iterations = MAX_ITER;

	if (do_this_once) {
		do_this_once = 0;
#ifdef HAVE_RUNTIME_CACHE
		roadmap_log(ROADMAP_WARNING, "Use runtime cache");
#else
		roadmap_log(ROADMAP_WARNING, "Use map extension");
#endif
	}

	from_line->line_id = to_line->line_id = 0;

	/* Where are we ? */
	PluginLine	*l = roadmap_navigate_position2line(from_pos);
	roadmap_log (ROADMAP_WARNING, "from --> %s (%d)",
			l ? roadmap_plugin_street_full_name(l) : "??",
			l->line_id);
	if (l)
		*from_line = *l;
#if 0
	else
		return -1;
#endif

	l = roadmap_navigate_position2line(to_pos);
	roadmap_log (ROADMAP_WARNING, "to --> %s",
			l ? roadmap_plugin_street_full_name(l) : "??");
	if (l)
		*to_line = *l;
#if 0
	else
		return -1;
#endif

	/*
	 * Get in shape to start looking for a route
	 */
	CurrentPos = from_pos;
	tracktime = 0;
	oldcost = 0;
	bestdist = maxdist = roadmap_math_distance(&CurrentPos, &to_pos);

	/* FIX ME which of the points ? Could use roadmap_line_to_point as well. */
	point = roadmap_line_from_point(from_line->line_id);
	point = roadmap_line_to_point(from_line->line_id);

	/*
	 * Next iteration
	 */
	for (loop=iteration=0; loop < max_iterations && iteration < max_iterations; loop++) {
		roadmap_log (ROADMAP_DEBUG,
			"RouteGetSegments iteration === %d === point %d, CurrentPos %d %d",
			iteration, point,
			CurrentPos.longitude, CurrentPos.latitude);

#if 1
		roadmap_point_position(point, &newpos);
		roadmap_log (ROADMAP_DEBUG, "Point %d is at %d %d", point,
				newpos.longitude, newpos.latitude);
#endif
		int	maxlines = 10, nlines;
		PluginLine	lines[10];

		l = roadmap_navigate_position2line(CurrentPos);
		nlines = navigate_simple_lines_closeby(CurrentPos, point, l, lines, maxlines);

		roadmap_log(ROADMAP_WARNING, "lines_closeby -> %d, best dist %d", nlines, bestdist);
		if (nlines == 0) {
			point = roadmap_line_from_point(from_line->line_id);
			roadmap_log(ROADMAP_WARNING, "try point #%d", point);
			nlines = navigate_simple_lines_closeby(CurrentPos, point, l,
					lines, maxlines);

			roadmap_log(ROADMAP_WARNING, "lines_closeby -> %d, best dist %d",
					nlines, bestdist);
		}
		if (nlines == 0) {
			point = roadmap_line_to_point(from_line->line_id);
			roadmap_log(ROADMAP_WARNING, "try point #%d", point);
			nlines = navigate_simple_lines_closeby(CurrentPos, point, l,
					lines, maxlines);

			roadmap_log(ROADMAP_WARNING, "lines_closeby -> %d, best dist %d",
					nlines, bestdist);
		}
		if (nlines == 0) {
			point = 198;
			roadmap_log(ROADMAP_WARNING, "try point #%d", point);
			nlines = navigate_simple_lines_closeby(CurrentPos, point, l,
					lines, maxlines);

			roadmap_log(ROADMAP_WARNING, "lines_closeby -> %d, best dist %d",
					nlines, bestdist);
		}
#if 1
		int x;
		fprintf(stderr, "Lines closeby (point %d) -> lines {", point);
		for (x=0; x<nlines; x++)
			fprintf(stderr, "%d ", lines[x].line_id);
		fprintf(stderr, "}\n");
#endif

		bestdist = maxdist * 2;
		best = -1;

		for (i=0; i<nlines; i++) {
			RoadMapStreetProperties prop;

			roadmap_street_get_properties(lines[i].line_id, &prop);

			/* Have we already been here ? */
			for (j=0, found=0; j<iteration && found == 0; j++)
				if (lines[i].line_id == segments[j].line.line_id)
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

			newpt = roadmap_line_to_point(lines[i].line_id);
			roadmap_point_position(newpt, &newpos);
			dist = roadmap_math_distance(&newpos, &to_pos);

			roadmap_log (ROADMAP_DEBUG, "Line %d point %d pos %d %d",
					lines[i].line_id, newpt,
					newpos.longitude, newpos.latitude);

			newpt2 = roadmap_line_from_point(lines[i].line_id);
			roadmap_point_position(newpt2, &newpos2);
			dist2 = roadmap_math_distance(&newpos2, &to_pos);

			roadmap_log (ROADMAP_DEBUG, "Line %d point %d pos %d %d",
					lines[i].line_id, newpt2,
					newpos2.longitude, newpos2.latitude);

			if (point == newpt2) {
				rev = 0;
				if (dist < bestdist) {
					best = i;
					bestdist = dist;
					bestnewpt = newpt;
					bestnewpos = newpos;
				}
			} else {
				rev = 1;
				if (dist2 < bestdist) {
					best = i;
					bestdist = dist2;
					bestnewpt = newpt2;
					bestnewpos = newpos2;
				}
			}
			roadmap_log (ROADMAP_DEBUG, " line %d [%d] {%s} %d",
				i, lines[i].line_id,
				roadmap_street_get_full_name(&prop),
				rev ? dist2 : dist);
		}

		roadmap_log (ROADMAP_DEBUG, "After inner loop : best %d bestdist %d",
				best, bestdist);

		stop = 0;
		if (!stop && to_line && to_line->line_id == lines[best].line_id)
			stop++;
		if (!stop && roadmap_line_from_point(to_line->line_id) == bestnewpt)
			stop++;
		if (!stop && roadmap_line_to_point(to_line->line_id) == bestnewpt)
			stop++;
		if (stop) 
		{
			roadmap_log (ROADMAP_DEBUG, "Line match -> track time %d, dist %d",
					tracktime, bestdist);
			*size = iteration;
			return tracktime;
		}
		if (bestdist < 30) /* FIX ME */ {
			roadmap_log (ROADMAP_DEBUG, "< 30 -> tracktime %d, dist %d",
					tracktime, bestdist);
			*size = iteration;
			return tracktime;
		}
		if (bestdist == maxdist) {
			/* Not better than the previous step */
			/* We might be "there" */
			if (bestdist < 100 /* FIX ME */) {
				roadmap_log (ROADMAP_DEBUG, "Tracktime %d, dist %d",
						tracktime, bestdist);
				*size = iteration;
				return tracktime;
			}
		}
		if (best < 0) {
			if (iteration == 0) {
				roadmap_log (ROADMAP_WARNING, "On an island ?");
				return -1;
			}

			roadmap_log (ROADMAP_DEBUG, "Backtrack ..");

			iteration--;
			navigate_simple_blacklist_add(segments[iteration].line.line_id);

			point = segments[iteration].from_point;
			CurrentPos = segments[iteration].from_pos;
			tracktime -= segments[iteration].cross_time;
			oldrev = segments[iteration].line_direction;
			oldcost = segments[iteration].cross_time;
			continue;
		}


		t = /* FIX ME */ (maxdist - bestdist) / 10;

		cost = (*cost_get)(lines[best].line_id, rev, oldcost,
				(iteration > 1) ? segments[iteration-1].line.line_id : 0,
				(iteration > 1) ? oldrev : 0, 0);

		segments[iteration].from_pos = CurrentPos;
		segments[iteration].from_point = point;
		segments[iteration].distance = maxdist - bestdist;
//		segments[iteration].cross_time = t;
		segments[iteration].line = lines[best];
		segments[iteration].line_direction = rev;
		segments[iteration].cross_time = cost;

		segments[iteration].to_pos = CurrentPos = bestnewpos;
		segments[iteration].to_point = bestnewpt;
		point = bestnewpt;

		tracktime += t;
		oldcost = cost;
		iteration++;

		roadmap_log (ROADMAP_DEBUG, "To point %d (%d,%d) via line %d (rev %d)",
				point,
				CurrentPos.longitude, CurrentPos.latitude,
				lines[best].line_id,
				rev);
		roadmap_log (ROADMAP_DEBUG, "Line selected : from %d to %d",
				newpt, newpt2);
	}

	/*
	 * Clean up and return
	 */
	return -1;
}
