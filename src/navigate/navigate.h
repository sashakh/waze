/*
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright (c) 2008, 2009, Danny Backx
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
 * @brief navigate.h - main plugin file
 */

#ifndef INCLUDE__NAVIGATE_H
#define INCLUDE__NAVIGATE_H

#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_plugin.h"
#include "roadmap_screen.h"

enum NavigateInstr {
   NAVIGATE_TURN_LEFT = 0,
   NAVIGATE_TURN_RIGHT,
   NAVIGATE_KEEP_LEFT,
   NAVIGATE_KEEP_RIGHT,
   NAVIGATE_CONTINUE,
   NAVIGATE_APPROACHING_DESTINATION,
   NAVIGATE_LAST_DIRECTION
};

/**
 * @brief structure to keep iterations of a route in
 * also serves to pass them between navigate plugin and others
 */
typedef struct NavigateSegment {
   PluginLine           line;
   int                  line_direction;	/**< is the direction reversed ? */
   PluginStreet         street;
   RoadMapPosition      from_pos;
   int			from_point;
   RoadMapPosition      to_pos;
   int			to_point;
   RoadMapPosition      shape_initial_pos;
   int                  first_shape;
   int                  last_shape;
   RoadMapShapeItr      shape_itr;
   enum NavigateInstr   instruction;
   int                  group_id;

   int			dist_from_destination;	/**< how far away are we */
   int                  distance;		/**< distance from start */
   int                  time;			/**< time to reach this point */
   int			heuristic;		/**< algorithm dependent */
} NavigateSegment;
 
/* Forward declaration, required to define the list pointers */
struct NavigateIteration;

typedef struct NavigateCost {
   int          distance;	/**< distance until this point */
   int          time;		/**< time to reach this point */
//   int          heuristic;	/**< algorithm dependent heuristic about cost to get here */
} NavigateCost;

/**
 * @brief structure of one step in navigation
 * The code should maintain a doubly linked list of these
 */
typedef struct NavigateIteration {
	struct NavigateIteration	*prev, *next;
	struct NavigateSegment		*segment;
	struct NavigateCost		cost;
} NavigateIteration;

/**
 * @brief structure to store navigation status
 * to be used in the generic navigation loop
 * to be shared between the different navigation sub-functions
 * may be extended by a navigation algorithm
 */
typedef struct {
	NavigateIteration	*first, *last;
	NavigateIteration	*current;
	int			iteration;
	int			maxdist;
} NavigateStatus;

/*
 * Definitions for the algorithm fields
 */
struct NavigateAlgorithm;
typedef int (*navigate_cost_fn) (NavigateIteration *iter);
typedef int (*navigate_step_fn) (struct NavigateAlgorithm *algo, NavigateStatus *stp);
typedef int (*navigate_end_fn) (NavigateStatus *stp);

/**
 * @brief structure that defines a navigation algorithm
 * (basically a set of functions to calculate cost, next step, ..
 * that all use the same data structures)
 */
typedef struct NavigateAlgorithm {
	const char		*algorithm_name;
	int			both_ways;
	int			max_iterations;
	navigate_cost_fn	cost_fn;
	navigate_step_fn	step_fn;
	navigate_end_fn		end_fn;
} NavigateAlgorithm;

void navigate_algorithm_register(NavigateAlgorithm *algo);

int navigate_is_enabled (void);
void navigate_shutdown (void);
void navigate_initialize (void);
int  navigate_reload_data (void);
void navigate_set (int status);
int  navigate_calc_route (void);

void navigate_screen_repaint (int max_pen);

int navigate_override_pen (int line,
                                int cfcc,
                                int fips,
                                int pen_type,
                                RoadMapPen *override_pen);

void navigate_adjust_layer (int layer, int thickness, int pen_count);
void navigate_format_messages (void);
#endif /* INCLUDE__NAVIGATE_H */
