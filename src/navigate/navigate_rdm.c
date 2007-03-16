/* navigate_rdm.c - rdm implementation navigate_graph functions
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
 *   See navigate_rdm.h
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
#include "roadmap_messagebox.h"
#include "roadmap_dialog.h"
#include "roadmap_main.h"
#include "roadmap_line_route.h"

#include "navigate_main.h"
#include "navigate_traffic.h"
#include "navigate_graph.h"

//#include "astar.h"
#include "fib-1.1/fib.h"

typedef unsigned char PrevItem;

#define IN_CLOSED_LIST (1 << 7)
#define REVERSED (1 << 31)

struct successor {
   int line_id;
   RoadMapPosition to_pos;
   int cost;
   int prev_cost;
};

static PrevItem *GraphPrevList;
static PrevItem *GraphOppositePrevList;
static RoadMapPosition GoalPos;

inline void * astar_alloc(size_t size)
{
   return malloc (size);
}


int navigate_reload_data (void) {
   
   return 0;
}

 
int navigate_load_data (void) {
   return 0;
}

struct SquareGraphItem {
   int square_id;
   unsigned short lines_count;
   unsigned short nodes_count;
   unsigned short *nodes;
   unsigned short *nodes_index;
   int *lines;
   unsigned short *lines_index;
};

#define MAX_GRAPH_CACHE 50
static struct SquareGraphItem *SquareGraphCache[MAX_GRAPH_CACHE];
static int SquareGraphHead;

static struct SquareGraphItem *get_square_graph (int square_id) {

   int i;
   int slot;
   int line;
   int count;
   struct SquareGraphItem *cache;
   int cur_line = 0;
   int cur_node = 0;

   for (i=0,slot=SquareGraphHead; i<MAX_GRAPH_CACHE;
        i++, slot=((slot+1) % MAX_GRAPH_CACHE)) {
     if (!SquareGraphCache[slot]) break;
     if (SquareGraphCache[slot]->square_id == square_id) break;
   }

   if ((i < MAX_GRAPH_CACHE) && (SquareGraphCache[slot])) {
      
      return SquareGraphCache[slot];
   }

   if (i == MAX_GRAPH_CACHE) {
      /* Free a cache slot */
      free (SquareGraphCache[SquareGraphHead]->nodes);
      free (SquareGraphCache[SquareGraphHead]->nodes_index);
      free (SquareGraphCache[SquareGraphHead]->lines);
      free (SquareGraphCache[SquareGraphHead]->lines_index);
      cache = SquareGraphCache[SquareGraphHead];
      SquareGraphHead = (SquareGraphHead + 1) % MAX_GRAPH_CACHE;
   } else {
      cache = SquareGraphCache[i] =
         (struct SquareGraphItem *)malloc(sizeof(struct SquareGraphItem));
   }

   cache->square_id = square_id;
   count = 0;

   /* Count total lines */
   for (i = ROADMAP_ROAD_FIRST; i <= ROADMAP_ROAD_LAST; ++i) {

      int first_line;
      int last_line;

      if (roadmap_line_in_square
            (square_id, i, &first_line, &last_line) > 0) {

         count += (last_line - first_line + 1);
      }

      if (roadmap_line_in_square2
            (square_id, i, &first_line, &last_line) > 0) {

         count += (last_line - first_line + 1);
      }
   }

   cache->lines_count = count * 2;
   cache->lines = malloc(cache->lines_count * sizeof(int));
   cache->lines_index = calloc(cache->lines_count, sizeof(unsigned short));

   /* assume that the number of nodes equals the number of lines */
   cache->nodes_count = count;
   cache->nodes = malloc(cache->nodes_count * sizeof(unsigned short));
   cache->nodes_index = malloc(cache->nodes_count * sizeof(unsigned short));

   for (i = ROADMAP_ROAD_FIRST; i < ROADMAP_ROAD_LAST; ++i) {

      int first_line;
      int last_line;

      if (roadmap_line_in_square
            (square_id, i, &first_line, &last_line) > 0) {

         for (line = first_line; line <= last_line; line++) {

            int from_point_id;
            int to_point_id;
            int j;

            if (cur_node == cache->nodes_count) {
               cache->nodes_count *= 2;
               cache->nodes = realloc(cache->nodes,
                                cache->nodes_count * sizeof(unsigned short));
               cache->nodes_index = realloc(cache->nodes_index,
                                cache->nodes_count * sizeof(unsigned short));
            }

            roadmap_line_points (line, &from_point_id, &to_point_id);
            from_point_id &= 0xffff;

            for (j=cur_node-1; j>=0; j--) {
               if (cache->nodes[j] == from_point_id) {
                  int l = cache->nodes_index[j];
                  while (cache->lines_index[l]) l=cache->lines_index[l];
                  cache->lines_index[l] = cur_line;
                  cache->lines[cur_line] = line;
                  cur_line++;
                  break;
               }
            }

            if (j<0) {
               cache->nodes[cur_node] = from_point_id;
               cache->nodes_index[cur_node] = cur_line;
               cache->lines[cur_line] = line;
               cur_node++;
               cur_line++;
            }

            if (cur_node == cache->nodes_count) {
               cache->nodes_count *= 2;
               cache->nodes = realloc(cache->nodes,
                                cache->nodes_count * sizeof(unsigned short));
               cache->nodes_index = realloc(cache->nodes_index,
                                cache->nodes_count * sizeof(unsigned short));
            }

            if (roadmap_point_square(to_point_id) == square_id) {

               to_point_id &= 0xffff;

               for (j=cur_node-1; j>=0; j--) {
                  if (cache->nodes[j] == to_point_id) {
                     int l = cache->nodes_index[j];
                     while (cache->lines_index[l]) l=cache->lines_index[l];
                     cache->lines_index[l] = cur_line;
                     cache->lines[cur_line] = line|REVERSED;
                     cur_line++;
                     break;
                  }
               }

               if (j<0) {
                  cache->nodes[cur_node] = to_point_id;
                  cache->nodes_index[cur_node] = cur_line;
                  cache->lines[cur_line] = line|REVERSED;
                  cur_node++;
                  cur_line++;
               }

            }
         }
      }

      if (roadmap_line_in_square2
            (square_id, i, &first_line, &last_line) > 0) {

         int line_cursor;

         for (line_cursor = first_line; line_cursor <= last_line;
               ++line_cursor) {
            int to_point_id;
            int j;

            if (cur_node == cache->nodes_count) {
               cache->nodes_count *= 2;
               cache->nodes = realloc(cache->nodes,
                                cache->nodes_count * sizeof(unsigned short));
               cache->nodes_index = realloc(cache->nodes_index,
                                cache->nodes_count * sizeof(unsigned short));
            }

            line = roadmap_line_get_from_index2 (line_cursor);

            roadmap_line_to_point (line, &to_point_id);

            to_point_id &= 0xffff;

            for (j=cur_node-1; j>=0; j--) {
               if (cache->nodes[j] == to_point_id) {
                  int l = cache->nodes_index[j];
                  while (cache->lines_index[l]) l=cache->lines_index[l];
                  cache->lines_index[l] = cur_line;
                  cache->lines[cur_line] = line|REVERSED;
                  cur_line++;
                  break;
               }
            }

            if (j<0) {
               cache->nodes[cur_node] = to_point_id;
               cache->nodes_index[cur_node] = cur_line;
               cache->lines[cur_line] = line|REVERSED;
               cur_node++;
               cur_line++;
            }

         }
      }
   }

   assert(cur_line <= cache->lines_count);
   assert(cur_node <= cache->nodes_count);
   cache->nodes_count = cur_node;

   return cache;
}

static int missed;
static int get_connected_segments (int seg_line_id, int seg_reversed,
                                   int node_id, struct successor *successors,
                                   int max, PrevItem *prev_item) {

   static int res_bits[8];
   int i;
   int square;

   int count = 0;
   int index = 0;
   int res_index = 0;
   int line;
   int line_reversed;
   int seg_res_bits;

   /* Init turn restrictions bits */
   if (!res_bits[0]) for (i=0; i<8; i++) res_bits[i] = 1<<i;

   *prev_item = (PrevItem)-1;

   square = roadmap_point_square (node_id);

   struct SquareGraphItem *cache = get_square_graph (square);

   node_id &= 0xffff;

   for (i = 0; i < cache->nodes_count; ++i) {
      if (cache->nodes[i] == node_id) break;
   }

   assert (i < cache->nodes_count);

   if (seg_reversed) {
      seg_res_bits = roadmap_line_route_get_restrictions (seg_line_id, 1);
   } else {
      seg_res_bits = roadmap_line_route_get_restrictions (seg_line_id, 0);
   }

   i = cache->nodes_index[i];

   while (!index || i) {

      int to_point_id;
      int line_direction_allowed;

      line = cache->lines[i];
      i = cache->lines_index[i];
      line_reversed = line & REVERSED;
      if (line_reversed) line = line & ~REVERSED;

      if (line == seg_line_id) {
         *prev_item = index;
         index++;
         res_index++;
         continue;
      }

      if (!line_reversed) {
         roadmap_line_to_point (line, &to_point_id);
         line_direction_allowed =
            roadmap_line_route_get_direction (line, ROUTE_CAR_ALLOWED) &
                  ROUTE_DIRECTION_WITH_LINE;
      } else {
         roadmap_line_from_point (line, &to_point_id);
         line_direction_allowed =
            roadmap_line_route_get_direction (line, ROUTE_CAR_ALLOWED) &
                  ROUTE_DIRECTION_AGAINST_LINE;
      }

      if (!line_direction_allowed) {
         index++;
         continue;
      }

      successors[count].prev_cost = 0;
      if (!line_reversed && (GraphPrevList[line] != (PrevItem)-1)) {
         if (!(GraphPrevList[line] & IN_CLOSED_LIST)) {
            successors[count].prev_cost = 0;
            //printf("Missed an optional short cut. segment: %d\n", line);
            index++;
            res_index++;
            continue;
         } else {
            index++;
            res_index++;
            continue;
         }
      }

      if (line_reversed && (GraphOppositePrevList[line] != (PrevItem)-1)) {
         if (!(GraphOppositePrevList[line] & IN_CLOSED_LIST)) {
            successors[count].prev_cost = 0;
            //printf("Missed an optional short cut. segment: %d\n", line);
            index++;
            res_index++;
            continue;
         } else {
            index++;
            res_index++;
            continue;
         }
      }

      if ((res_index >= 8) || !(seg_res_bits & res_bits[res_index])) {
              successors[count].line_id = line|line_reversed;
              roadmap_point_position (to_point_id, &successors[count].to_pos);
              successors[count].cost =
                      roadmap_line_route_get_cross_time (line, line_reversed) + 1;
              count++;
      }
      index++;
      res_index++;
   }

   assert (*prev_item != (PrevItem)-1);
   return count;
}


static void make_path (PrevItem *prev_line, int line_id)
{
   int line_reversed = line_id & REVERSED;
   if (line_reversed) line_id = line_id & ~REVERSED;

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


/*
A* maintains a set of partial solutions, i.e. paths through the graph starting at the start node, stored in a priority queue. The priority assigned to a path x is determined by the function f(x) = g(x) + h(x)

Here, g(x) is the cost of the path so far, i.e. the weight of the edges followed so far. h(x) is the heuristic estimate of the minimal cost to reach the goal from x. For example, if "cost" is taken to mean distance travelled, the straight line distance between two points on a map is a heuristic estimate of the distance to be travelled.

The lower f(x), the higher the priority (so a min-heap could be used to implement the queue).
*/
/* the key is an int, so limited to routes which are different by more than 1 metre. */
static int min_distance = 10000000;
static int seg_count;
static int nodes_count;
static int paths_count;

#define HU_SPEED 50

static void cancel_calc (const char *name, void *data) {
}

static void show_progress_dialog (void) {

   if (roadmap_dialog_activate ("Route calc", NULL, 1)) {

      roadmap_dialog_new_label ("Calculating", "Calculting route, please wait...");
      roadmap_dialog_new_progress ("Calculating", "Progress");

      roadmap_dialog_add_button ("Cancel", cancel_calc);

      roadmap_dialog_complete (0);
   }

   roadmap_dialog_set_progress ("Calculating", "Progress", 0);

   roadmap_main_flush ();
}


static void update_progress (int progress) {
   roadmap_dialog_set_progress ("Calculating", "Progress", progress);

   roadmap_main_flush ();
}

#define MAX_SUCCESSORS 100

static int astar(int start_node, int start_segment, int goal_node,
                 int *route_total_cost, int recalc)
{
   int line;
   int line_reversed;
   int node;
   int segment;
   int no_successors;
   int i;
   struct successor successors[MAX_SUCCESSORS]; 
   PrevItem prev_id;
   PrevItem *prev;
   int cur_cost;
   RoadMapPosition node_pos;
   int goal_distance = -1;
   int cur_min_distance;

   struct fibheap * q = make_queue(start_segment);
   int lines_count = roadmap_line_count ();

   GraphPrevList = (PrevItem *) malloc(lines_count * sizeof(PrevItem));
   memset (GraphPrevList, (PrevItem)-1, lines_count * sizeof(PrevItem));
   GraphOppositePrevList = (PrevItem *) malloc(lines_count * sizeof(PrevItem));
   memset (GraphOppositePrevList, (PrevItem)-1, lines_count * sizeof(PrevItem));
   roadmap_point_position (goal_node, &GoalPos);
   roadmap_point_position (start_node, &node_pos);
   cur_min_distance = goal_distance =
                                roadmap_math_distance (&node_pos, &GoalPos);

   make_path (NULL, start_segment);
   missed = 0;

   min_distance = 10000000;
   nodes_count = 0;
   seg_count = 0;
   paths_count = 0;

   while (fh_min(q) != NULL)
   {
      cur_cost = fh_minkey(q);
      line = (int )fh_extractmin(q);
      line_reversed = line & REVERSED;
      if (line_reversed) line = line & ~REVERSED;
      
      if (!line_reversed) prev = GraphPrevList + line;
      else prev = GraphOppositePrevList + line;

      node = get_to_node (line, line_reversed);
      roadmap_point_position (node, &node_pos);

      if (cur_cost) {
         int dis = roadmap_math_distance (&node_pos, &GoalPos);
         //if ((dis > 10000) && (dis > (cur_min_distance * 100)))
              //break;
         cur_cost -= dis * 3.6 / HU_SPEED;
      }

      if(node == goal_node)
      {
         *route_total_cost = cur_cost;
         fh_deleteheap(q);
         printf("nodes: %d, segments:%d, paths: %d missed:%d\n", nodes_count, seg_count, paths_count, missed);
         return line | line_reversed;
      }
      nodes_count++;
      /* Insert into closed list */
      *prev |= IN_CLOSED_LIST;

      no_successors = get_connected_segments (line, line_reversed, node,
                                    successors, MAX_SUCCESSORS, &prev_id);
      seg_count += no_successors;
      if (!no_successors) {
         paths_count--;
         continue;
      }

      for(i = 0; i < no_successors; i++)
      {
         int path_cost;
         int distance_to_goal;
         int cost_to_goal;
         int total_cost;

         segment = successors[i].line_id;
         path_cost = successors[i].cost + cur_cost;
         distance_to_goal =
                roadmap_math_distance (&successors[i].to_pos, &GoalPos);

         if (distance_to_goal < min_distance) {
                 printf("Current distance: %d\n", distance_to_goal);
                 min_distance = distance_to_goal;
         }

         cost_to_goal = distance_to_goal * 3.6 / HU_SPEED;
         total_cost = path_cost + cost_to_goal + 1;

         if (successors[i].prev_cost) {
            int reversed = segment & REVERSED;
            int line = segment;
            struct fibheap_el *node;

            if (reversed) {
               line = line & ~REVERSED;               
            }
            node = fh_find_data(q, (void *)segment);
            assert (node != NULL);

            if (total_cost < fh_elkey(node)) {
               printf("found a shortcut! line:%d\n", segment);
            } else {
               printf("not found a shortcut total_cost:%d, node:%d\n", total_cost, fh_elkey(node));
               continue;
            }
         }
         make_path (&prev_id, segment);
         fh_insertkey(q, total_cost, (void *)segment);

         if (distance_to_goal < cur_min_distance) {
            cur_min_distance = distance_to_goal;
            if (!recalc) update_progress
                               (100 - (cur_min_distance * 100 / goal_distance));
         } 
         paths_count++;
      }
   }
   fh_deleteheap(q);
   free(GraphPrevList);
   free(GraphOppositePrevList);
   return -1;
}

int navigate_get_route_segments (PluginLine *from_line,
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

   //FIXME add plugin support
   line = from_line->line_id;
   roadmap_line_points (line, &line_from_point, &line_to_point);
   if (from_point == line_from_point) line = from_line->line_id|REVERSED;
   if (!(*flags & RECALC_ROUTE)) show_progress_dialog ();

   line = astar
            (from_point, line, to_point, &total_cost, *flags & RECALC_ROUTE);

   *flags = 0;

   if (line == -1) {
      if (!(*flags & RECALC_ROUTE)) roadmap_dialog_hide ("Route calc");
      return -1;
   }

   line_reversed = line & REVERSED;
   if (line_reversed) line = line & ~REVERSED;
   
   i=0;

   /* FIXME no plugin support */
   if (line != to_line->line_id) {

      int from_point;
      int to_point;
      i++;
      segments[*size - i].line = *to_line;
      roadmap_line_points (to_line->line_id, &from_point, &to_point);
      
      /* FIXME */
      //if (from_point == dglNodeGet_Id(&graph, &pReport->nDestinationNode)) {
      if (!line_reversed) {
         
         segments[*size - i].line_direction = ROUTE_DIRECTION_WITH_LINE;
      } else {

         segments[*size - i].line_direction = ROUTE_DIRECTION_AGAINST_LINE;
      }
   }


   printf("Route: ");
   while (1) {
      int prev_node;
      int square;
      PrevItem *prev;
      struct SquareGraphItem *cache;
      int j;
      int skip;
      i++;

      segments[*size-i].line.plugin_id = ROADMAP_PLUGIN_ID;
      segments[*size-i].line.line_id = line;
      printf("%d, ", line);
      segments[*size-i].line.fips = roadmap_locator_active ();
      segments[*size-i].line.cfcc = 5;
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
      if (line == from_line->line_id) break;

      if (!line_reversed) prev = GraphPrevList + line;
      else prev = GraphOppositePrevList + line;

      square = roadmap_point_square (prev_node);

      cache = get_square_graph (square);
      prev_node &= 0xffff;

      for (j=0; j<cache->nodes_count; j++) {
         if (cache->nodes[j] == prev_node) break;
      }
      assert (j<cache->nodes_count);

      j = cache->nodes_index[j];

      skip = (*prev & ~IN_CLOSED_LIST);
      while (skip) {
         j = cache->lines_index[j];
         skip--;
      }

      line = cache->lines[j];

      line_reversed = line & REVERSED;
      if (line_reversed) line = line & ~REVERSED;
      if (line_reversed) line_reversed = 0;
      else line_reversed = 1;
   }
   printf("\n");

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
   if (!(*flags & RECALC_ROUTE)) roadmap_dialog_hide ("Route calc");

   return total_cost + 1;
}


