/*
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

/**
 * @file
 * @brief roadmap_navigate.h - basic navigation engine.
 */
#ifndef INCLUDED__ROADMAP_NAVIGATE__H
#define INCLUDED__ROADMAP_NAVIGATE__H

#include "roadmap_gps.h"
#include "roadmap_plugin.h"

void roadmap_navigate_disable (void);
void roadmap_navigate_enable  (void);

int roadmap_navigate_retrieve_line (const RoadMapArea *focus,
                                    const RoadMapPosition *position,
                                    PluginLine *line,
                                    int *distance);

void roadmap_navigate_locate (const RoadMapGpsPosition *gps_position);

void roadmap_navigate_initialize (void);

#if defined(HAVE_TRIP_PLUGIN) || defined(HAVE_NAVIGATE_PLUGIN)
typedef struct {
	void (*update) (RoadMapPosition *position, PluginLine *current);
	void (*get_next_line) (PluginLine *current, int direction, PluginLine *next);
} RoadMapNavigateRouteCB;
#endif

#include "roadmap_fuzzy.h"
typedef struct {
    RoadMapFuzzy direction;
    RoadMapFuzzy distance;
    RoadMapFuzzy connected;
} RoadMapDebug;
/**
 * @brief
 */
typedef struct {
    int valid;			/**< */
    PluginStreet street;	/**< */
    int direction;		/**< */
    RoadMapFuzzy fuzzyfied;	/**< */
    PluginLine intersection;	/**< */
    RoadMapPosition entry;	/**< */
    RoadMapDebug debug;		/**< */
} RoadMapTracking;
/* static */ int roadmap_navigate_fuzzify (RoadMapTracking *tracked,
                 RoadMapNeighbour *line, int direction);
/* static */ int roadmap_navigate_get_neighbours (const RoadMapArea *focus,
               const RoadMapPosition *position,
               RoadMapNeighbour *neighbours, int max,
	       int navigation_mode);
PluginLine *roadmap_navigate_position2line(RoadMapPosition pos);
int roadmap_navigate_get_mode(void);
int roadmap_navigate_find_intersection (const RoadMapGpsPosition *position,
		PluginLine *found, int navigation_mode);
#endif // INCLUDED__ROADMAP_NAVIGATE__H
