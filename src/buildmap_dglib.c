/* buildmap_dglib.c - Build dglib graph
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   void buildmap_dglib_initialize (void);
 *   int  buildmap_dglib_add
 *       (unsigned char from_flags,
 *        unsigned char to_flags,
 *        unsigned char from_max_speed,
 *        unsigned char to_max_speed,
 *        unsigned short from_cross_time,
 *        unsigned short to_cross_time,
 *        int line);
 *   int  buildmap_dglib_count (void);
 *   void buildmap_dglib_sort (void);
 *   void buildmap_dglib_save (void);
 *   void buildmap_dglib_summary (void);
 *   void buildmap_dglib_reset   (void);
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* temp for creating a graph file */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>


/* dglib */
#include "type.h"
#include "graph.h"

#include "buildmap.h"
#include "buildmap_line.h"
#include "buildmap_line_route.h"
#include "buildmap_dglib.h"

static dglGraph_s graph;
static int EgdesCount = 0;

void buildmap_dglib_initialize (time_t creation_time) {

   dglInt32_t opaqueset[ 16 ] = {
      creation_time, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
   };

   dglInitialize (
               & graph ,         /* graph context to initialize */
               2 ,
               0 ,     /* node attributes size */
               1 ,     /* edge attributes size */
               opaqueset         /* opaque graph parameters */
               );

   EgdesCount = 0;
}


int  buildmap_dglib_add
        (unsigned char from_flags,
         unsigned char to_flags,
         unsigned char from_max_speed,
         unsigned char to_max_speed,
         unsigned short from_cross_time,
         unsigned short to_cross_time,
         unsigned char  layer,
         int line) {

   int from;
   int to;
   int status;

   buildmap_line_get_points_sorted (line, &from, &to);

   if (from_flags & ROUTE_CAR_ALLOWED) {
      EgdesCount++;

      status = dglAddEdgeX( &graph , from , to , from_cross_time ,
                           line , NULL , NULL , &layer, 0 );
      if ( status < 0 ) {
         buildmap_fatal (0, "dglAddEdge error: %s\n", dglStrerror( &graph ) );
      }
   }

   if (to_flags & ROUTE_CAR_ALLOWED) {

      if (line == 0) {
         buildmap_error (0, "buildmap_dglib_add - can't set line as bi-directional (id = 0)");
      } else {
         EgdesCount++;

         status = dglAddEdgeX( &graph , to , from , to_cross_time ,
               -line , NULL , NULL , &layer, 0 );
         if ( status < 0 ) {
            buildmap_fatal (0, "dglAddEdge error: %s\n", dglStrerror( &graph ) );
         }
      }
   }

   return EgdesCount;
}


void buildmap_dglib_sort (void) {}

void  buildmap_dglib_save (const char *path, const char *name) {

   int nret;
   FILE *fd;
   char full_name[255];

   snprintf(full_name, sizeof(full_name), "%s/%s.dgl", path, name);

   printf( "graph flattening..." ); fflush(stdout);
   nret = dglFlatten( & graph );
   if ( nret < 0 ) {
      buildmap_fatal (0, "dglFlatten error: %s\n", dglStrerror( &graph ) );
   }
   printf( "done.\n" );

   printf( "graph write..." ); fflush(stdout);
   if ( (fd = fopen( full_name , "w")) == NULL ) {
      buildmap_fatal (0, "dgl open error.\n");
   }
   nret = dglWrite( & graph , fd );
   if ( nret < 0 ) {
      buildmap_fatal (0, "dglWrite error: %s\n", dglStrerror( &graph ) );
   }
   fclose( fd );
   printf( "done.\n" );


   printf( "graph release..." ); fflush(stdout);
   dglRelease( & graph );
   printf( "program finished.\n" );
   return;

}


void buildmap_dglib_summary (void) {

   fprintf (stderr,
            "-- dglib table statistics: %d edges, %d bytes used\n",
            EgdesCount, 0);
}


void buildmap_dglib_reset (time_t creation_time) {

   buildmap_dglib_initialize(creation_time);
}

