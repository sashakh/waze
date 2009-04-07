/*
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright (c) 2008, 2009, Danny Backx.
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
 * @brief Display visual stuff to tell the user where to navigate
 *
 * @ingroup NavigatePlugin
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_line.h"
#include "roadmap_lang.h"
#include "roadmap_layer.h"
#include "roadmap_locator.h"
#include "roadmap_config.h"
#include "roadmap_start.h"
#include "roadmap_skin.h"
#include "roadmap_main.h"

#include "navigate_visual.h"

#define MAX_PEN_LAYERS 2
#define MAX_ROAD_COLORS 2

RoadMapPen	RoutePen;

/**
 * @brief
 */
void trip_create_pens (void)
{
   RoadMapPen	old;

   roadmap_log (ROADMAP_DEBUG, "trip_create_pens()");

   RoutePen = roadmap_canvas_create_pen("route");
   old = roadmap_canvas_select_pen(RoutePen);
   roadmap_canvas_set_foreground("#992222");	/* FIX ME */
   roadmap_canvas_set_thickness(10);		/* FIX ME */
   roadmap_canvas_select_pen(old);
}

/**
 * @brief
 */
void navigate_visual_initialize (void)
{
   trip_create_pens ();
   roadmap_skin_register (trip_create_pens);
}

/**
 * @brief FIX ME simplistic cache of the lines in the route
 */
static int navigate_visual_route_nlines = 0,
	   navigate_visual_route_size = 0;
static struct navigate_visual_route_cache {
	int line,
	    layer,
	    fips;
} *navigate_visual_route_cache = NULL;

/**
 * @brief clear the list of lines in the current route
 */
void navigate_visual_route_clear(void)
{
	navigate_visual_route_nlines = 0;
}

/**
 * @brief add a line to the current route cache, these will get highlighted visually
 * @param line
 * @param layer
 * @param fips
 */
void navigate_visual_route_add(int line, int layer, int fips)
{
	if (navigate_visual_route_size == navigate_visual_route_nlines) {
		navigate_visual_route_size += 100;
		navigate_visual_route_cache = (struct navigate_visual_route_cache *) realloc (
				(void *)navigate_visual_route_cache,
				navigate_visual_route_size * sizeof(struct navigate_visual_route_cache));
	}

	navigate_visual_route_cache[navigate_visual_route_nlines].line = line;
	navigate_visual_route_cache[navigate_visual_route_nlines].layer = layer;
	navigate_visual_route_cache[navigate_visual_route_nlines].fips = fips;
	navigate_visual_route_nlines++;
}

/**
 * @brief check whether we know about this line
 * @param line
 * @param layer
 * @param fips
 * @return whether this line is on the current route
 */
static int navigate_visual_line_on_route(int line, int layer, int fips)
{
	int	i;

	for (i=0; i<navigate_visual_route_nlines; i++)
		if (navigate_visual_route_cache[i].line == line
				&& navigate_visual_route_cache[i].layer == layer
				&& navigate_visual_route_cache[i].fips == fips)
			return 1;
	return 0;
}

/**
 * @brief change the visual representation of a road (show our route differently)
 * @param line
 * @param layer
 * @param fips
 * @param pen_type
 * @param override_pen a return parameter : the new pen
 * @return 1 if the caller should use a new pen
 */
int navigate_visual_override_pen (int line,
			       int layer,
			       int fips,
			       int pen_type,
			       RoadMapPen *override_pen)
{
   if (pen_type > 1) {
//	   roadmap_log (ROADMAP_DEBUG, "navigate_visual_override_pen: pen %d", pen_type);
	   return 0;
   }

   /*
    * Derive this notion from data in default/All, in this case "Class.Lines: ..."
    */
   if (! roadmap_layer_is_street(layer))
	   return 0;

   if (navigate_visual_line_on_route(line, layer, fips)) {
	   /* FIX ME need to assign a decent pen here */
	   *override_pen = RoutePen;
	   return 1;
   }

   return 0;
}

/**
 * @brief
 */
void navigate_visual_refresh (void)
{
}
