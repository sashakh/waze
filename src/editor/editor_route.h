/* editor_route.h - database layer
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

#ifndef INCLUDE__EDITOR_ROUTE__H
#define INCLUDE__EDITOR_ROUTE__H

#define ED_ROUTE_CAR 0x0001

#define ED_ROUTE_STOP_LIGHT  0x8000

typedef short EditorRouteFlag;

typedef struct editor_route_segment_s {
   EditorRouteFlag from_flags;
   EditorRouteFlag to_flags;
} editor_db_route_segment;


int editor_route_segment_add
      (short from_flags, short to_flags) ;

void editor_route_segment_get (int route, short *from_flags, short *to_flags);
void editor_route_segment_set (int route, short from_flags, short to_flags);

void editor_route_segment_copy (int source_line, int plugin_id, int dest_line);

int editor_route_get_direction (int route_id, int who);

extern roadmap_db_handler EditorRouteHandler;

#endif // INCLUDE__EDITOR_ROUTE__H

