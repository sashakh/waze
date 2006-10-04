/* roadmap_gpx.c - stubs, shims, and other interfaces to the GPX library
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

#ifdef ROADMAP_USES_EXPAT
# include <expat.h>
#endif

#define ROADMAP 1
# include "gpx/defs.h"

int roadmap_gpx_read_file
        (const char *path, const char *name, RoadMapList *w, int wee, RoadMapList *r, RoadMapList *t);
int roadmap_gpx_read_waypoints
        (const char *path, const char *name, RoadMapList *waypoints, int wee);
int roadmap_gpx_read_one_track
        ( const char *path, const char *name, route_head **track);
int roadmap_gpx_read_one_route
        ( const char *path, const char *name, route_head **route);
int roadmap_gpx_write_file
        (const char *path, const char *name, RoadMapList *w, RoadMapList *r, RoadMapList *t);
int roadmap_gpx_write_waypoints
        (const char *path, const char *name, RoadMapList *waypoints);
int roadmap_gpx_write_track
        (const char *path, const char *name, route_head *track);
int roadmap_gpx_write_route
        (const char *path, const char *name, route_head *route);

