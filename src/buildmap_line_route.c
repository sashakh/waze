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
 *        unsigned char from_max_speed,
 *        unsigned char to_max_speed,
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

#include "roadmap_db_line_route.h"
#include "roadmap_hash.h"

#include "buildmap.h"
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
         unsigned char from_max_speed,
         unsigned char to_max_speed,
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
   this_route->record.from_max_speed = from_max_speed;
   this_route->record.to_max_speed = to_max_speed;
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
         one_route = Routes[i/BUILDMAP_BLOCK] + (i % BUILDMAP_BLOCK);

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

