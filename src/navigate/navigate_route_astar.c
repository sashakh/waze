/* navigate_route_astar.c - astar route calculation
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
 *   See navigate_route.h
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

#include "navigate_main.h"
#include "navigate_traffic.h"
#include "navigate_graph.h"
#include "navigate_cost.h"

#include "fib-1.1/fib.h"
#include "navigate_route.h"

#define IN_CLOSED_LIST (1 << 7)

#define HU_SPEED 28 /* Meters per second */
#define MAX_SUCCESSORS 100


static PrevItem *GraphPrevList;
static PrevItem *GraphOppositePrevList;
static RoadMapPosition GoalPos;


int navigate_route_reload_data (void) {
   
   return 0;
}

int navigate_route_load_data (void) {
   return 0;
}


static void make_path (PrevItem *prev_line, int line_id, int line_reversed)
{
   if (prev_line) {
      if (!line_reversed) GraphPrevList[line_id] = *prev_line;
      else GraphOppositePrevList[line_id] = *prev_line;
   }
}


static int get_to_node (int line_id, int reversed) {

   int next_node;

   if (reversed) {
      roadmap_line_from_point (line_id, &next_node);
   } else {
      roadmap_line_to_point (line_id, &next_node);
   }

   return next_node;
}


struct fibheap * make_queue(int line_id)
{
   struct fibheap *fh;
   fh = fh_makekeyheap();    
   
   fh_insertkey(fh, 0, (void *)line_id);
   
   return fh;
}

static void update_progress (int progress) {
   progress = progress * 9 / 10;
   roadmap_dialog_set_progress ("Calculating", "Progress", progress);

   roadmap_main_flush ();
}

static int astar(int start_node, int start_segment, int is_reversed,
                 int goal_node, int *route_total_cost, int recalc)
{
   int line;
   int node;
   int no_successors;
   int i;
   struct successor successors[MAX_SUCCESSORS]; 
   PrevItem prev_id;
   PrevItem *prev;
   int cur_cost;
   RoadMapPosition node_pos;
   int goal_distance = -1;
   int cur_min_distance;

   struct fibheap *q;
   int lines_count = roadmap_line_count ();
   NavigateCostFn cost_fn = navigate_cost_get ();
   int navigate_type = navigate_cost_type ();

   GraphPrevList = (PrevItem *) malloc(lines_count * sizeof(PrevItem));
   memset (GraphPrevList, (PrevItem)-1, lines_count * sizeof(PrevItem));
   GraphOppositePrevList = (PrevItem *) malloc(lines_count * sizeof(PrevItem));
   memset (GraphOppositePrevList, (PrevItem)-1, lines_count * sizeof(PrevItem));
   roadmap_point_position (goal_node, &GoalPos);
   roadmap_point_position (start_node, &node_pos);
   cur_min_distance = goal_distance =
                                roadmap_math_distance (&node_pos, &GoalPos);

   line = start_segment;
   if (is_reversed) line |= REVERSED;

   q = make_queue(line);
   make_path (NULL, start_segment, is_reversed);

   while (fh_min(q) != NULL) {
      int line_reversed;

      cur_cost = fh_minkey(q);
      line = (int )fh_extractmin(q);
      line_reversed = line & REVERSED;

      if (line_reversed) {
         line = line & ~REVERSED;
         prev = GraphOppositePrevList + line;
      } else {
         prev = GraphPrevList + line;
      }

      node = get_to_node (line, line_reversed);
      roadmap_point_position (node, &node_pos);

      if (cur_cost) {
         int dis = roadmap_math_distance (&node_pos, &GoalPos);
         //if ((dis > 10000) && (dis > (cur_min_distance * 100)))
         //     break;
         if (navigate_type == COST_FASTEST) dis = (dis / HU_SPEED);
         cur_cost -= dis;
      }

      if(node == goal_node) {
         *route_total_cost = cur_cost;
         fh_deleteheap(q);
         return line | line_reversed;
      }
      /* Insert into closed list */
      *prev |= IN_CLOSED_LIST;

      no_successors = get_connected_segments (line, line_reversed, node,
                                    successors, MAX_SUCCESSORS, 1, &prev_id);
      if (!no_successors) {
         continue;
      }

      for(i = 0; i < no_successors; i++) {
         int path_cost;
         int segment;
         int segment_cost;
         int distance_to_goal;
         int cost_to_goal;
         int total_cost;
         int is_reversed;
         int has_prev_cost = 0;
         RoadMapPosition to_pos;

         segment = successors[i].line_id;
         is_reversed = successors[i].reversed;

         if (!is_reversed && (GraphPrevList[segment] != (PrevItem)-1)) {
            if (!(GraphPrevList[segment] & IN_CLOSED_LIST)) {
               has_prev_cost = 1;
               continue;
            } else {
               continue;
            }
         }

         if (is_reversed && (GraphOppositePrevList[segment] != (PrevItem)-1)) {
            if (!(GraphOppositePrevList[segment] & IN_CLOSED_LIST)) {
               has_prev_cost = 1;
               continue;
            } else {
               continue;
            }
         }

         segment_cost = cost_fn (segment, is_reversed, cur_cost, line,
                                 line_reversed, node);

         if (segment_cost < 0) continue;

         path_cost = segment_cost + cur_cost;
         roadmap_point_position (successors[i].to_point, &to_pos);
         distance_to_goal = roadmap_math_distance (&to_pos, &GoalPos);

         if (navigate_type == COST_FASTEST) {
            cost_to_goal = (distance_to_goal / HU_SPEED);
         } else {
            cost_to_goal = distance_to_goal;
         }
         total_cost = path_cost + cost_to_goal + 1;

         if (has_prev_cost) {
            struct fibheap_el *node;
            int l = segment;

            if (is_reversed) l |= REVERSED;

            node = fh_find_data(q, (void *)l);
            assert (node != NULL);

            if (total_cost < fh_elkey(node)) {
               printf("found a shortcut! line:%d\n", l);
               //FIXME update key value in heap
            } else {
               printf("not found a shortcut total_cost:%d, node:%d\n", total_cost, fh_elkey(node));
               continue;
            }
         }

         make_path (&prev_id, segment, is_reversed);

         if (is_reversed) segment |= REVERSED;
         fh_insertkey(q, total_cost, (void *)segment);

         if ((distance_to_goal << 2 ) < (cur_min_distance << 2)) {
            cur_min_distance = distance_to_goal;
            if (!recalc) update_progress
                               (100 - (cur_min_distance * 100 / goal_distance));
         }

      }
   }
   fh_deleteheap(q);
   free(GraphPrevList);
   free(GraphOppositePrevList);
   return -1;
}


int navigate_route_get_segments (PluginLine *from_line,
                                 int from_point,
                                 PluginLine *to_line,
                                 int to_point,
                                 NavigateSegment *segments,
                                 int *size,
                                 int *flags) {
   
   int i;
   int total_cost;
   int line_from_point;
   int line_to_point;
   int line;
   int line_reversed;
   int start_line_reversed;
   int recalc = *flags & RECALC_ROUTE;

   *flags = 0;

   //FIXME add plugin support
   line = from_line->line_id;
   roadmap_line_points (line, &line_from_point, &line_to_point);
   if (line_from_point == line_to_point) start_line_reversed = 0;
   else if (from_point == line_from_point) start_line_reversed = 1;
   else start_line_reversed = 0;

   line = astar (from_point, line, start_line_reversed, to_point, &total_cost,
                 recalc);

   if (line == -1) {
      return -1;
   }
   
   line_reversed = line & REVERSED;
   
   if (line_reversed) {
      line = line & ~REVERSED;
      line_reversed = 1;
   }

   i=0;

   /* FIXME no plugin support */
   if (line != to_line->line_id) {

      int tmp;
      int line_to;
      int last_line_from;
      int last_line_reversed = 0;
      i++;
      segments[*size - i].line = *to_line;
      if (!line_reversed) roadmap_line_points (line, &tmp, &line_to);
      else roadmap_line_points (line, &line_to, &tmp);

      roadmap_line_points (to_line->line_id, &last_line_from, &tmp);
      if (last_line_from != line_to) last_line_reversed = 1;
      
      if (!last_line_reversed) {
         
         segments[*size - i].line_direction = ROUTE_DIRECTION_WITH_LINE;
      } else {

         segments[*size - i].line_direction = ROUTE_DIRECTION_AGAINST_LINE;
      }
   }


   //printf("Route: ");
   while (1) {
      int prev_node;
      PrevItem *prev;
      i++;

      segments[*size-i].line.plugin_id = ROADMAP_PLUGIN_ID;
      segments[*size-i].line.line_id = line;
      //printf("%d, ", line);
      segments[*size-i].line.fips = roadmap_locator_active ();
      segments[*size-i].line.cfcc = roadmap_line_cfcc (line);
      if (line_reversed) {
         segments[*size-i].line_direction = ROUTE_DIRECTION_AGAINST_LINE;

      } else {
         segments[*size-i].line_direction = ROUTE_DIRECTION_WITH_LINE;
      }

      if (line_reversed) {
         roadmap_line_to_point (line, &prev_node);
      } else {
         roadmap_line_from_point (line, &prev_node);
      }

      /* Check if we got to the first line */
      if ((line == from_line->line_id) &&
          (line_reversed == start_line_reversed)) break;

      if (!line_reversed) prev = GraphPrevList + line;
      else prev = GraphOppositePrevList + line;

      line = navigate_graph_get_line (prev_node, *prev & ~IN_CLOSED_LIST);

      line_reversed = line & REVERSED;

      if (line_reversed) {
         line = line & ~REVERSED;
         line_reversed = 0;
      } else {
         line_reversed = 1;
      }
   }
   //printf("\n");

   /* add starting line */
   if (segments[*size-i].line.line_id != from_line->line_id) {

      int tmp_from;
      int tmp_to;
      i++;
      
      /* FIXME no plugin support */
      roadmap_line_points (from_line->line_id, &tmp_from, &tmp_to);

      segments[*size-i].line = *from_line;

      if (from_point == tmp_from) {
         segments[*size-i].line_direction = ROUTE_DIRECTION_AGAINST_LINE;
      } else {
         segments[*size-i].line_direction = ROUTE_DIRECTION_WITH_LINE;
      }
   }

   assert(i < *size);

   memmove(segments, segments+*size-i, sizeof(segments[0]) * i);

   *size = i;

   free(GraphPrevList);
   free(GraphOppositePrevList);

   return total_cost + 1;
}

