/* editor_route.c - route databse layer
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
 *
 * SYNOPSYS:
 *
 *   See editor_route.h
 */

#include <assert.h>
#include "stdlib.h"
#include "string.h"

#include "roadmap.h"
#include "roadmap_line_route.h"

#include "../editor_log.h"

#include "editor_db.h"
#include "editor_override.h"
#include "editor_line.h"

#include "editor_route.h"

static editor_db_section *ActiveSegmentRouteDB;

static void editor_route_activate (void *context) {

   ActiveSegmentRouteDB = (editor_db_section *) context;
}


roadmap_db_handler EditorRouteHandler = {
   "route",
   editor_map,
   editor_route_activate,
   editor_unmap
};


void editor_route_segment_copy (int source_line,
                                int plugin_id,
                                int dest_line) {

   int route_id;
   editor_db_route_segment *route;

   if (!plugin_id) {

      route_id = editor_override_line_get_route (source_line);
   } else {
      route_id = editor_line_get_route (source_line);
   }
      
   if (route_id != -1) {

      route = (editor_db_route_segment *) editor_db_get_item (
                                          ActiveSegmentRouteDB,
                                          route_id,
                                          0,
                                          NULL);

      assert (route != NULL);

      if (route == NULL) {
         editor_log
            (ROADMAP_ERROR,
            "Can't find route information for route_id:", route_id);
         return;
      }

      route_id =
         editor_route_segment_add
                        (route->from_flags,
                         route->to_flags,
                         route->from_speed_limit,
                         route->to_speed_limit);
   } else {

      LineRouteFlag from_flags = 0;
      LineRouteFlag to_flags = 0;
      LineRouteMax  from_speed_limit = 0;
      LineRouteMax  to_speed_limit = 0;

      if (plugin_id != 0) return; /* No route info */
      
      if (!roadmap_line_route_get_flags
            (source_line, &from_flags, &to_flags) &&
         !roadmap_line_route_get_speed_limit
            (source_line, &from_speed_limit, &to_speed_limit)) {

         return;
      }

      route_id =
         editor_route_segment_add
                        (from_flags,
                         to_flags,
                         from_speed_limit,
                         to_speed_limit);
   }

   if (route_id < 0) {
      editor_log
         (ROADMAP_ERROR,
          "Can't create route information.");
      return;
   }

   editor_line_set_route (dest_line, route_id);
}


int editor_route_segment_add (LineRouteFlag from_flags,
                              LineRouteFlag to_flags,
                              LineRouteMax from_speed_limit,
                              LineRouteMax to_speed_limit) {

   editor_db_route_segment route;
   int id;

   route.from_flags = from_flags;
   route.to_flags = to_flags;
   route.from_speed_limit = from_speed_limit;
   route.to_speed_limit = to_speed_limit;

   id = editor_db_add_item (ActiveSegmentRouteDB, &route);

   if (id == -1) {
      editor_db_grow ();
      id = editor_db_add_item (ActiveSegmentRouteDB, &route);
   }

   return id;
}


/* Returns:
 * 0 - No direction.
 * 1 - from is allowed
 * 2 - to is allowed
 * 3 - both ways allowed
 */
int editor_route_get_direction (int route_id, int who) {

   int direction = 0;
   editor_db_route_segment *route;

   if (route_id == -1) return 0;

   route = (editor_db_route_segment *) editor_db_get_item (
                                       ActiveSegmentRouteDB,
                                       route_id,
                                       0,
                                       NULL);

   assert (route != NULL);

   if (route == NULL) {
      editor_log
         (ROADMAP_ERROR,
          "Can't find route information for route_id:", route_id);
      return 0;
   }

   if (route->from_flags & who) direction |= 0x1;
   if (route->to_flags & who) direction |= 0x2;
      
   return direction;
}


void editor_route_segment_get (int route_id,
                               LineRouteFlag *from_flags,
                               LineRouteFlag *to_flags,
                               LineRouteMax *from_speed_limit,
                               LineRouteMax *to_speed_limit) {

   editor_db_route_segment *route;

   if (route_id == -1) {
      *from_flags = *to_flags = 0;
      return;
   }

   route = (editor_db_route_segment *) editor_db_get_item (
                                       ActiveSegmentRouteDB,
                                       route_id,
                                       0,
                                       NULL);

   assert (route != NULL);

   if (route == NULL) {
      editor_log
         (ROADMAP_ERROR,
          "Can't find route information for route_id:", route_id);
      *from_flags = *to_flags = 0;
      return;
   }

   if (from_flags  != NULL) *from_flags  = route->from_flags;
   if (to_flags    != NULL) *to_flags    = route->to_flags;
   if (from_speed_limit != NULL) *from_speed_limit = route->from_speed_limit;
   if (to_speed_limit != NULL) *to_speed_limit = route->to_speed_limit;

}


void editor_route_segment_set (int route_id,
                               LineRouteFlag from_flags,
                               LineRouteFlag to_flags,
                               LineRouteMax from_speed_limit,
                               LineRouteMax to_speed_limit) {

   editor_db_route_segment *route;

   assert (route_id != -1);

   if (route_id == -1) {
      return;
   }

   route = (editor_db_route_segment *) editor_db_get_item (
                                       ActiveSegmentRouteDB,
                                       route_id,
                                       0,
                                       NULL);

   assert (route != NULL);

   if (route == NULL) {
      editor_log
         (ROADMAP_ERROR,
          "Can't find route information for route_id:", route_id);
      return;
   }

   route->from_flags = from_flags;
   route->to_flags = to_flags;
   route->from_speed_limit = from_speed_limit;
   route->to_speed_limit = to_speed_limit;
}


