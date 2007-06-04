/* navigate_zoom.c - implement auto zoom
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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
 *   See navigate_zoom.h
 */

#include <stdlib.h>
#include <math.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_layer.h"
#include "roadmap_line_route.h"

#include "navigate_main.h"
#include "navigate_zoom.h"

void navigate_zoom_update (RoadMapPosition *pos,
                           NavigateSegment *segments,
                           int current_segment,
                           const NavigateSegment *last_group_seg,
                           int distance_to_prev) {

   const RoadMapPosition *turn_pos;
   int distance;

   if (last_group_seg->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      turn_pos = &last_group_seg->to_pos;
   } else {
      turn_pos = &last_group_seg->from_pos;
   }

   distance = roadmap_math_distance (pos, turn_pos);

   if ((current_segment > 0) &&
        (last_group_seg->group_id !=
         (segments + current_segment -1)->group_id)) {
      /* We might still be close to the previous junction. Let's make
       * sure that we don't zoom out too fast
       */

      if ((distance_to_prev < distance) && (distance > 200)) {

         distance = (distance_to_prev * (200 - distance_to_prev) + distance * distance_to_prev) / 200;

      }
   }

   roadmap_math_set_scale (distance, roadmap_canvas_height() / 2 * 3 / 4);

   roadmap_layer_adjust ();
   
}

