/* buildmap_line_route.c - Build a line route table
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
 *   void buildmap_line_route_initialize (void);
 *   int  buildmap_line_route_add
 *       (unsigned char from_flags,
 *        unsigned char to_flags,
 *        unsigned char from_avg_speed,
 *        unsigned char to_avg_speed,
 *        unsigned short from_speed_ref,
 *        unsigned short to_speed_ref,
 *        int line);
 *   void buildmap_line_route_sort (void);
 *   int  buildmap_line_route_count (void);
 *   void buildmap_line_route_save (void);
 *   void buildmap_line_route_summary (void);
 *   void buildmap_line_route_reset   (void);
 *
 * This is a temporary scheme (no space optimizations)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "buildmap.h"
#include "roadmap_db_line_route.h"
#include "roadmap_hash.h"
#include "buildmap_line.h"
#include "buildmap_turn_restrictions.h"
#include "buildmap_line_route.h"


struct buildmap_line_route_struct {

   RoadMapLineRoute record;
   int line;
};

typedef struct buildmap_line_route_struct BuildMapRoute;

static int RoutesCount = 0;
static BuildMapRoute *Routes[BUILDMAP_BLOCK] = {NULL};

static int RouteAddCount = 0;

void buildmap_line_route_initialize (void) {

   RouteAddCount = 0;
   RoutesCount = 0;
}


int  buildmap_line_route_add
        (unsigned char from_flags,
         unsigned char to_flags,
         unsigned char from_avg_speed,
         unsigned char to_avg_speed,
         unsigned short from_speed_ref,
         unsigned short to_speed_ref,
         int line) {

   int block;
   int offset;
   BuildMapRoute *this_route;

   RouteAddCount += 1;

   block = line / BUILDMAP_BLOCK;
   offset = line % BUILDMAP_BLOCK;

   if (Routes[block] == NULL) {

      /* We need to add a new block to the table. */

      Routes[block] = calloc (BUILDMAP_BLOCK, sizeof(BuildMapRoute));
      if (Routes[block] == NULL) {
         buildmap_fatal (0, "no more memory");
      }
   }

   this_route = Routes[block] + offset;

   this_route->record.from_flags = from_flags;
   this_route->record.to_flags = to_flags;
   this_route->record.from_avg_speed = from_avg_speed;
   this_route->record.to_avg_speed = to_avg_speed;
   this_route->record.to_speed_ref = to_speed_ref;
   this_route->record.from_speed_ref = from_speed_ref;

   this_route->line = line;

   if (line >= RoutesCount) RoutesCount = line + 1;

   return RoutesCount;
}


void buildmap_line_route_sort (void) {}

void  buildmap_line_route_save (void) {

   int i;
   BuildMapRoute *one_route;
   RoadMapLineRoute  *db_route;

   buildmap_db *root;
   buildmap_db *table_data;

   int lines_at_node[50];
   int count;


   buildmap_info ("saving line route...");

   root = buildmap_db_add_section (NULL, "line_route");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   table_data = buildmap_db_add_child
                  (root, "data", RoutesCount, sizeof(RoadMapLineRoute));

   db_route = (RoadMapLineRoute *) buildmap_db_get_data (table_data);

   for (i = 0; i < RoutesCount; i++) {

      if (Routes[i/BUILDMAP_BLOCK] == NULL) {
         memset (&db_route[i], 0, sizeof (RoadMapLineRoute));

      } else {
         int j;
         int from_point_id;
         int to_point_id;
         int point_id;

         one_route = Routes[i/BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);

         buildmap_line_get_points_sorted
            (one_route->line, &from_point_id, &to_point_id);

         for (j=0; j<2; j++) {

            int res;
            int res_bit = 0;
            if (j == 0) point_id = from_point_id;
            else point_id = to_point_id;

            count = sizeof(lines_at_node) / sizeof(lines_at_node[0]);
            buildmap_line_lines_by_node (point_id, lines_at_node, &count);

            for (res=0; res<count; res++) {
               int line_from;
               int line_to;
               int line;
               int flags;
               BuildMapRoute *line_route;

               line = lines_at_node[res];
               line_route = Routes[line/BUILDMAP_BLOCK] +
                                 (line % BUILDMAP_BLOCK);
               buildmap_line_get_points_sorted (line, &line_from, &line_to);
               if (line_from == point_id) {
                  flags = line_route->record.from_flags;
               } else if (line_to == point_id) {
                  flags = line_route->record.to_flags;
               } else {
                  buildmap_fatal (0, "Error in route graph!");
               }

               if (flags & ROUTE_CAR_ALLOWED) {
                  if ((res_bit < 8) &&
                     buildmap_turn_restrictions_exists
                        (point_id, one_route->line, line)) {

                     if (j==0) one_route->record.from_turn_res |= 1<<res_bit;
                     else one_route->record.to_turn_res |= 1<<res_bit;

                  }
                  res_bit++;
                  if (res_bit > 8) {
                     buildmap_error (0, "Too many possible turns %d in node:%d", res_bit, point_id);
                  }
               }
            }
         }

         db_route[i] = one_route->record;
      }
   }
}


void buildmap_line_route_summary (void) {

   fprintf (stderr,
            "-- line route table statistics: %d lines, %d bytes used\n",
            RoutesCount, RoutesCount * sizeof(RoadMapLineRoute));
}


void buildmap_line_route_reset (void) {

   int i;

   for (i = 0; i < BUILDMAP_BLOCK; i++) {
      if (Routes[i] != NULL) {
         free (Routes[i]);
         Routes[i] = NULL;
      }
   }

   RoutesCount = 0;

   RouteAddCount = 0;
}

