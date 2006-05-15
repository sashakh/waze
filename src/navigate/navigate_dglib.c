/* navigate_dglib.c - dglib implementation navigate_graph functions
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
 *   See navigate_dglib.h
 */

#include <stdio.h>

#include <stdlib.h>
#include "type.h"
#include "graph.h"
#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_line.h"
#include "roadmap_locator.h"
#include "roadmap_turns.h"
#include "roadmap_metadata.h"
#include "roadmap_messagebox.h"
#include "roadmap_line_route.h"

#include "navigate_main.h"
#include "navigate_graph.h"

static int fips_data_loaded = 0;
static dglGraph_s graph;
static dglSPCache_s spCache;

static int  clipper     (
                        dglGraph_s *    pgraph ,
                        dglSPClipInput_s * pIn ,
                        dglSPClipOutput_s * pOut ,
                        void *          pvarg       /* caller's pointer */
                        )
{       
   if ( roadmap_turns_find_restriction (
            dglNodeGet_Id(pgraph, pIn->pnNodeFrom),
            pIn->pnPrevEdge != NULL ?
               dglEdgeGet_Id(pgraph, pIn->pnPrevEdge) :
               (int) pvarg,
            dglEdgeGet_Id(pgraph, pIn->pnEdge))) {
            /*
            printf( "        discarder.\n" );
            */
            return 1;
        }

   return 0;
}


int navigate_load_data (void) {
   FILE *fd;
   int nret;
   char path[255];
   int fips;
   time_t map_unix_time;
   const char *sequence;

   fips = roadmap_locator_active ();
   if (fips_data_loaded == fips) return 0;

   sequence = roadmap_path_first("maps");
   if (sequence == NULL) {
      return -1;
   }

   do {
      snprintf (path, sizeof(path), "%s/usc%05d.dgl", sequence, fips);
      
      fd = fopen( path, "rb" );
      if ( fd != NULL ) break;

      sequence = roadmap_path_next("maps", sequence);

   } while (sequence != NULL);

   if ( fd == NULL ) {  
      roadmap_messagebox ("Error", "Can't find route data.");
      return -1;
   }

   nret = dglRead( & graph , fd );

   fclose( fd );

   if ( nret < 0 ) {
      roadmap_messagebox ("Error", "Can't load route data.");
      return -1;
   }
   
   map_unix_time = atoi
                     (roadmap_metadata_get_attribute ("Version", "UnixTime"));

   if ((time_t) *dglGet_Opaque (&graph) != map_unix_time) {
      roadmap_messagebox ("Error", "Navigation data is too old.");
      return -1;
   }

   dglInitializeSPCache( & graph, & spCache );
   fips_data_loaded = fips;

   return 0;
}


int navigate_get_route_segments (PluginLine *from_line,
                                 int from_point,
                                 PluginLine *to_line,
                                 int to_point,
                                 NavigateSegment *segments,
                                 int *size) {
   
   int i;
   int nret;
   int total_cost;
   dglSPReport_s *pReport;

   if (fips_data_loaded != roadmap_locator_active ()) return -1;

   /* save places for start & end lines */
   *size -= 2;

   nret = dglShortestPath (&graph, &pReport, from_point, to_point,
                           clipper, (void *)from_line->line_id, NULL);
   if (nret <= 0) return nret;

   if (pReport->cArc > *size) return -1;
   
   total_cost = pReport->nDistance;
   *size = pReport->cArc;

   /* add starting line */
   if (abs(dglEdgeGet_Id(&graph, pReport->pArc[0].pnEdge)) !=
         from_line->line_id) {

      int tmp_from;
      int tmp_to;
      
      /* FIXME no plugin support */
      roadmap_line_points (from_line->line_id, &tmp_from, &tmp_to);

      segments[0].line = *from_line;

      if (from_point == tmp_from) {
         segments[0].line_direction = ROUTE_DIRECTION_AGAINST_LINE;
      } else {
         segments[0].line_direction = ROUTE_DIRECTION_WITH_LINE;
      }

      segments++;
      (*size)++;
   }


   for(i=0; i < pReport->cArc ;i++) {
      segments[i].line.plugin_id = ROADMAP_PLUGIN_ID;
      segments[i].line.line_id = dglEdgeGet_Id(&graph, pReport->pArc[i].pnEdge);
      segments[i].line.fips = roadmap_locator_active ();
      segments[i].line.cfcc = *(unsigned char *)
         dglEdgeGet_Attr(&graph, pReport->pArc[i].pnEdge);
      if (segments[i].line.line_id < 0) {
         segments[i].line.line_id = abs(segments[i].line.line_id);
         segments[i].line_direction = ROUTE_DIRECTION_AGAINST_LINE;

      } else {
         segments[i].line_direction = ROUTE_DIRECTION_WITH_LINE;
      }

/*
      printf(  "edge[%d]: from %ld to %ld - travel cost %ld - user edgeid %ld - distance from start node %ld\n" ,
            i,
            pReport->pArc[i].nFrom,
            pReport->pArc[i].nTo,
            dglEdgeGet_Cost(&graph, pReport->pArc[i].pnEdge), * this is the cost from clip() *
            dglEdgeGet_Id(&graph, pReport->pArc[i].pnEdge),
            pReport->pArc[i].nDistance
            );
            */
   }

   /* add ending line */

   /* FIXME no plugin support */
   if (segments[i-1].line.line_id != to_line->line_id) {
      segments[i].line = *to_line;
      /* TODO destination line should have a direction */
      segments[i].line_direction = ROUTE_DIRECTION_NONE;
      (*size)++;
   }

   dglFreeSPReport(&graph, pReport);

   return total_cost;
}


