/* navigate_graph.c - Implements a graph for routing calculation
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
 *   See navigate_graph.h
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

#include "navigate_graph.h"

#define MAX_GRAPH_CACHE 75

struct SquareGraphItem {
   int square_id;
   unsigned short lines_count;
   unsigned short nodes_count;
   unsigned short *nodes_index;
   int *lines;
   unsigned short *lines_index;
};

static struct SquareGraphItem *SquareGraphCache[MAX_GRAPH_CACHE];
static int SquareGraphHead;

static struct SquareGraphItem *get_square_graph (int square_id) {

   int i;
   int slot;
   int line;
   int lines1_count;
   int lines2_count;
   struct SquareGraphItem *cache;
   int cur_line = 0;

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
   lines1_count = 0;
   lines2_count = 0;

   /* Count total lines */
   for (i = ROADMAP_ROAD_FIRST; i <= ROADMAP_ROAD_LAST; ++i) {

      int first_line;
      int last_line;

      if (roadmap_line_in_square
            (square_id, i, &first_line, &last_line) > 0) {

         lines1_count += (last_line - first_line + 1);
      }

      if (roadmap_line_in_square2
            (square_id, i, &first_line, &last_line) > 0) {

         lines2_count += (last_line - first_line + 1);
      }
   }

   cache->lines_count = lines1_count * 2 + lines2_count;
   cache->lines = malloc(cache->lines_count * sizeof(int));
   cache->lines_index = calloc(cache->lines_count, sizeof(unsigned short));

   /* assume that the number of nodes equals the number of lines */
   cache->nodes_count = roadmap_square_points_count (square_id);
   cache->nodes_index = calloc(cache->nodes_count, sizeof(unsigned short));

   for (i = ROADMAP_ROAD_FIRST; i <= ROADMAP_ROAD_LAST; ++i) {

      int first_line;
      int last_line;

      if (roadmap_line_in_square
            (square_id, i, &first_line, &last_line) > 0) {

         for (line = first_line; line <= last_line; line++) {

            int from_point_id;
            int to_point_id;
            int l;

            roadmap_line_points (line, &from_point_id, &to_point_id);
            from_point_id &= 0xffff;

            l = cache->nodes_index[from_point_id];
            if (l) {
               l--;
               while (cache->lines_index[l]) l=cache->lines_index[l];
               cache->lines_index[l] = cur_line;
            } else {
               cache->nodes_index[from_point_id] = cur_line + 1;
            }

            cache->lines[cur_line] = line;
            cur_line++;

            if (roadmap_point_square(to_point_id) == square_id) {

               to_point_id &= 0xffff;

               l = cache->nodes_index[to_point_id];
               if (l) {
                  l--;
                  while (cache->lines_index[l]) l=cache->lines_index[l];
                  cache->lines_index[l] = cur_line;
               } else {
                  cache->nodes_index[to_point_id] = cur_line + 1;
               }
               cache->lines[cur_line] = line|REVERSED;
               cur_line++;
            }
         }
      }

      if (roadmap_line_in_square2
            (square_id, i, &first_line, &last_line) > 0) {

         int line_cursor;
         int l;

         for (line_cursor = first_line; line_cursor <= last_line;
               ++line_cursor) {
            int to_point_id;

            line = roadmap_line_get_from_index2 (line_cursor);

            roadmap_line_to_point (line, &to_point_id);

            to_point_id &= 0xffff;

            l = cache->nodes_index[to_point_id];

            if (l) {
               l--;
               while (cache->lines_index[l]) l=cache->lines_index[l];
               cache->lines_index[l] = cur_line;
            } else {
               cache->nodes_index[to_point_id] = cur_line + 1;
            }
            cache->lines[cur_line] = line|REVERSED;
            cur_line++;
         }
      }
   }

   assert(cur_line <= cache->lines_count);

   return cache;
}

int get_connected_segments (int seg_line_id, int is_seg_reversed,
                            int node_id, struct successor *successors,
                            int max, int use_restrictions,
                            PrevItem *prev_item) {

   static int res_bits[8];
   int i;
   int square;

   int count = 0;
   int index = 0;
   int res_index = 0;
   int line;
   int line_reversed;
   int seg_res_bits;
   struct SquareGraphItem *cache;

   /* Init turn restrictions bits */
   if (!res_bits[0]) for (i=0; i<8; i++) res_bits[i] = 1<<i;

   *prev_item = (PrevItem)-1;

   square = roadmap_point_square (node_id);

   cache = get_square_graph (square);

   node_id &= 0xffff;

   i = cache->nodes_index[node_id];
   assert (i > 0);
   i--;

   if (use_restrictions) {
      if (is_seg_reversed) {
         seg_res_bits = roadmap_line_route_get_restrictions (seg_line_id, 1);
      } else {
         seg_res_bits = roadmap_line_route_get_restrictions (seg_line_id, 0);
      }
   }

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
         if (roadmap_line_route_get_direction
               (seg_line_id, ROUTE_CAR_ALLOWED) == ROUTE_DIRECTION_ANY) {
            res_index++;
         }
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

      if (!use_restrictions || (res_index >= 8) ||
          !(seg_res_bits & res_bits[res_index])) {
              successors[count].line_id = line;
              successors[count].reversed = (line_reversed != 0);
              successors[count].to_point = to_point_id;
              count++;
      }
      index++;
      res_index++;
   }

   assert (*prev_item != (PrevItem)-1);
   return count;
}


int navigate_graph_get_line (int node, int line_no) {

   int square = roadmap_point_square (node);
   struct SquareGraphItem *cache = get_square_graph (square);
   int i;
   int skip;

   node &= 0xffff;

   i = cache->nodes_index[node];
   assert (i > 0);
   i--;

   skip = line_no;
   while (skip) {
      i = cache->lines_index[i];
      skip--;
   }

   return cache->lines[i];
}

