#if 1
/*
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright (c) 2008, Danny Backx.
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
 * @brief calculate navigation instructions
 */

#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_line.h"
// #include "roadmap_line_route.h"
#include "roadmap_math.h"
#include "roadmap_point.h"
#include "roadmap_street.h"
#include "roadmap_turns.h"
#include "roadmap_layer.h"

#include "navigate.h"
#include "navigate_cost.h"
#include "navigate_instr.h"

static int navigate_instr_azymuth_delta (int az1, int az2) {
   
   int delta;

   delta = az1 - az2;

   while (delta > 180)  delta -= 360;
   while (delta < -180) delta += 360;

   return delta;
}

static int navigate_instr_calc_azymuth (NavigateSegment *seg, int type) {

   RoadMapPosition start;
   RoadMapPosition end;
   RoadMapPosition *shape_pos;
   int shape;

   start = seg->from_pos;
   end   = seg->to_pos;

   if (seg->first_shape > -1) {

      int last_shape;

      if (type == LINE_START) {

         last_shape = seg->first_shape;
         shape_pos  = &end;
         *shape_pos = seg->shape_initial_pos;
      } else {

         last_shape = seg->last_shape;
         shape_pos  = &start;
         *shape_pos = seg->shape_initial_pos;
      }

      for (shape = seg->first_shape; shape <= last_shape; shape++) {

         seg->shape_itr (shape, shape_pos);
      }
   }

   return roadmap_math_azymuth (&start, &end);
}


static void navigate_fix_line_end (RoadMapPosition *position,
                                   NavigateSegment *segment,
                                   int type) {

   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition intersection;
   int smallest_distance = 0x7fffffff;
   int distance;
   int seg_shape_end = -1;
   RoadMapPosition seg_end_pos = {0, 0};
   RoadMapPosition seg_shape_initial = {0, 0};
   int i;

   if (segment->first_shape <= -1) {
      
      from = segment->from_pos;
      to = segment->to_pos;
   } else {

      to = from = segment->from_pos;

      for (i = segment->first_shape; i <= segment->last_shape; i++) {

         segment->shape_itr (i, &to);

         distance =
            roadmap_math_get_distance_from_segment
            (position, &from, &to, &intersection, NULL);

         if (distance < smallest_distance) {

            smallest_distance = distance;

            if (type == LINE_START) {

               seg_shape_end = i;
               seg_end_pos = intersection;
               seg_shape_initial = from;
            } else {

               seg_shape_end = i-1;
               seg_end_pos = intersection;
            }
         }

         from = to;
      }

      to = segment->to_pos;
   }

   distance =
      roadmap_math_get_distance_from_segment
      (position, &from, &to, &intersection, NULL);

   if (distance < smallest_distance) {

      if (type == LINE_START) {

         seg_shape_end = -1;
         seg_end_pos = intersection;
         seg_shape_initial = from;
      } else {

         seg_shape_end = segment->last_shape;
         seg_end_pos = intersection;
      }
   }

   if (type == LINE_START) {
      
      segment->from_pos = seg_end_pos;
      segment->shape_initial_pos = seg_shape_initial;
      if ((seg_shape_end < 0) || (seg_shape_end > segment->last_shape)) {
         segment->first_shape = segment->last_shape = -1;
      } else {
         segment->first_shape = seg_shape_end;
      }

   } else {

      segment->to_pos = seg_end_pos;
      if ((seg_shape_end < 0) || (seg_shape_end < segment->first_shape)) {
         segment->first_shape = segment->last_shape = -1;
      } else {
         segment->last_shape = seg_shape_end;
      }
   }
}


static void navigate_instr_check_neighbours (NavigateSegment *seg1,
                                             NavigateSegment *seg2) {

   RoadMapPosition *junction;
   int junction_node_id;
   RoadMapNeighbour neighbours[16];
   int i;
   int count;
   int layers_count;
   int layers[128];
   int seg1_azymuth;
   int seg2_azymuth;
   int delta;
   int left_delta;
   int right_delta;
   int accuracy;
   RoadMapArea focus;

   if (seg1->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg1_azymuth  = navigate_instr_calc_azymuth (seg1, LINE_END);
      /* TODO no plugin support */
      roadmap_line_points
         (roadmap_plugin_get_line_id (&seg1->line),
          &i, &junction_node_id);
      junction = &seg1->to_pos;
   } else {
      seg1_azymuth  = 180 + navigate_instr_calc_azymuth (seg1, LINE_START);
      /* TODO no plugin support */
      roadmap_line_points
         (roadmap_plugin_get_line_id (&seg1->line),
          &junction_node_id, &i);
      junction = &seg1->from_pos;
   }

   if (seg2->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg2_azymuth  = navigate_instr_calc_azymuth (seg2, LINE_START);
   } else {
      seg2_azymuth  = 180 + navigate_instr_calc_azymuth (seg2, LINE_END);
   }

   delta = navigate_instr_azymuth_delta (seg1_azymuth, seg2_azymuth);

   left_delta = right_delta = delta;

   layers_count = roadmap_layer_navigable (0 /* car ? */, layers, 128);

   accuracy = 10;
   focus.west = junction->longitude - accuracy;
   focus.east = junction->longitude + accuracy;
   focus.north = junction->latitude + accuracy;
   focus.south = junction->latitude - accuracy;

   roadmap_math_set_focus (&focus);

   /* TODO no plugin support */
   count = roadmap_street_get_closest
           (junction,
            layers,
            layers_count,
            neighbours,
            sizeof(neighbours) / sizeof(RoadMapNeighbour));

   roadmap_math_release_focus ();

   for (i = 0; i < count; ++i) {

      PluginLine *line = &neighbours[i].line;
      RoadMapPosition from_pos;
      RoadMapPosition to_pos;
      int line_delta;
      int direction;
      
      if (roadmap_plugin_same_line (line, &seg1->line) ||
          roadmap_plugin_same_line (line, &seg2->line)) {
         continue;
      }

      roadmap_plugin_line_from (line, &from_pos);
      roadmap_plugin_line_to (line, &to_pos);
      direction = roadmap_plugin_get_direction (line, ROUTE_CAR_ALLOWED);

      if (!roadmap_math_distance (junction, &from_pos)) {

         if (direction == ROUTE_DIRECTION_AGAINST_LINE) continue;

         line_delta = navigate_instr_azymuth_delta
                        (seg1_azymuth, 
                         roadmap_math_azymuth
                            (&neighbours[i].from, &neighbours[i].to));

      } else if (!roadmap_math_distance (junction, &to_pos)) {

         if (direction == ROUTE_DIRECTION_WITH_LINE) continue;

         line_delta = navigate_instr_azymuth_delta
                        (seg1_azymuth, 
                         roadmap_math_azymuth
                            (&neighbours[i].to, &neighbours[i].from));
      } else {
         continue;
      }

      /* TODO no plugin support */
      if (roadmap_plugin_get_id (line) == ROADMAP_PLUGIN_ID) {

         if (roadmap_turns_find_restriction
               (junction_node_id,
                roadmap_plugin_get_line_id (&seg1->line),
                roadmap_plugin_get_line_id (line))) {

            continue;
         }
      }

      if (line_delta < left_delta) left_delta = line_delta;
      if (line_delta > right_delta) right_delta = line_delta;
   }

   if (((left_delta != delta) && (right_delta != delta)) ||
         ((left_delta == delta) && (right_delta == delta))) {
      seg1->instruction = NAVIGATE_CONTINUE;
      
   } else if (left_delta != delta) {
      seg1->instruction = NAVIGATE_KEEP_LEFT;
   } else {
      seg1->instruction = NAVIGATE_KEEP_RIGHT;
   }
}


static void navigate_instr_set_road_instr (NavigateSegment *seg1,
                                          NavigateSegment *seg2) {

   int seg1_azymuth;
   int seg2_azymuth;
   int delta;
   int minimum_turn_degree = 15;

   if (seg1->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg1_azymuth  = navigate_instr_calc_azymuth (seg1, LINE_END);
   } else {
      seg1_azymuth  = 180 + navigate_instr_calc_azymuth (seg1, LINE_START);
   }


   if (seg2->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg2_azymuth  = navigate_instr_calc_azymuth (seg2, LINE_START);
   } else {
      seg2_azymuth  = 180 + navigate_instr_calc_azymuth (seg2, LINE_END);
   }

   delta = navigate_instr_azymuth_delta (seg1_azymuth, seg2_azymuth);

   if (roadmap_plugin_same_street (&seg1->street, &seg2->street)) {
      minimum_turn_degree = 45;
   }

   if (delta < -minimum_turn_degree) {

      if (delta > -45) {
         seg1->instruction = NAVIGATE_KEEP_RIGHT;
      } else {
         seg1->instruction = NAVIGATE_TURN_RIGHT;
      }

   } else if (delta > minimum_turn_degree) {

      if (delta < 45) {
         seg1->instruction = NAVIGATE_KEEP_LEFT;
      } else {
         seg1->instruction = NAVIGATE_TURN_LEFT;
      }
      
   } else {

      if (!roadmap_plugin_same_street (&seg1->street, &seg2->street)) {
         navigate_instr_check_neighbours (seg1, seg2);
      } else {
         seg1->instruction = NAVIGATE_CONTINUE;
      }
   }
}

/**
 * @brief
 * @param position
 * @param segment
 * @param type
 * @return
 */
int navigate_instr_calc_length (const RoadMapPosition *position,
                                const NavigateSegment *segment,
                                int type) {

   int total_length = 0;
   int result = 0;

   result =
      roadmap_math_calc_line_length (position,
                                     &segment->from_pos, &segment->to_pos,
                                     segment->first_shape, segment->last_shape,
                                     segment->shape_itr,
                                     &total_length);

   if (type == LINE_START) {
      
      return result;
   } else {

      return total_length - result;
   }
}


/**
 * @brief
 * @param segments
 * @param count
 * @param src_pos
 * @param dst_pos
 * @return
 */
int navigate_instr_prepare_segments (NavigateSegment *segments,
                                     int count,
                                     RoadMapPosition *src_pos,
                                     RoadMapPosition *dst_pos) {

   int i;
   int group_id = 0;
   NavigateSegment *segment;
   int cur_cost;
   int prev_line_id;
   int is_prev_reversed;

   for (i=0; i < count; i++) {

      roadmap_plugin_get_line_points (&segments[i].line,
                                      &segments[i].from_pos,
                                      &segments[i].to_pos,
                                      &segments[i].first_shape,
                                      &segments[i].last_shape,
                                      &segments[i].shape_itr);

      segments[i].shape_initial_pos.longitude = segments[i].from_pos.longitude;
      segments[i].shape_initial_pos.latitude = segments[i].from_pos.latitude;

      roadmap_plugin_get_street (&segments[i].line, &segments[i].street);
   }

   for (i=0; i < count - 1; i++) {

      navigate_instr_set_road_instr (&segments[i], &segments[i+1]);
   }

   segments[i].instruction = NAVIGATE_APPROACHING_DESTINATION;

   if (segments[0].line_direction == ROUTE_DIRECTION_WITH_LINE) {
      navigate_fix_line_end (src_pos, &segments[0], LINE_START);
   } else {
      navigate_fix_line_end (src_pos, &segments[0], LINE_END);
   }

   if (segments[i].line_direction == ROUTE_DIRECTION_WITH_LINE) {
      navigate_fix_line_end (dst_pos, &segments[i], LINE_END);
   } else {
      navigate_fix_line_end (dst_pos, &segments[i], LINE_START);
   }

   /* assign group ids */
   segment = segments;
   while (segment < segments + count) {

      int group_count = 0;

      while (segment->instruction == NAVIGATE_CONTINUE) {

         NavigateSegment *prev = segment;

         /* Check if the previous segment is the last */
         if (prev == (segments + count - 1)) {
            break;
         }

         segment++;

         if (!roadmap_plugin_same_street (&prev->street, &segment->street)) {
            segment = prev;
            break;
         }

         group_count++;
      }

      while (group_count >= 0) {
         /* (segment - group_count)->instruction = segment->instruction; */
         (segment - group_count)->group_id = group_id;
         group_count--;
      }

      segment++;
      group_id++;
   }

   /* Calculate lengths and ETA for each segment */
   segment = segments;
   cur_cost = 0;
   prev_line_id = -1;
   is_prev_reversed = 0;
   navigate_cost_reset ();

   while (segment < segments + count) {

      segment->cross_time = 
         navigate_cost_time (segment->line.line_id,
                 segment->line_direction != ROUTE_DIRECTION_WITH_LINE,
                 cur_cost,
                 prev_line_id,
                 is_prev_reversed);

      cur_cost += segment->cross_time;
      prev_line_id = segment->line.line_id;
      is_prev_reversed = segment->line_direction != ROUTE_DIRECTION_WITH_LINE;

      if ((segment == segments) || (segment == (segments + count -1))) {

         if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
            segment->distance =
               navigate_instr_calc_length
                  (&segment->from_pos, segment, LINE_END);

         } else {
            segment->distance =
               navigate_instr_calc_length
                  (&segment->to_pos, segment, LINE_START);
         }

         /* adjust cross time using the line length */
         segment->cross_time =
            (int) (1.0 * segment->cross_time * segment->distance /
               (roadmap_line_length (segment->line.line_id)+1));
      } else {

         segment->distance = roadmap_line_length (segment->line.line_id);
      }

      segment++;
   }

   return 0;
}
#endif
