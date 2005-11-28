/* editor_track_known.c - handle tracks of known points
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
 * NOTE:
 * This file implements all the "dynamic" editor functionality.
 * The code here is experimental and needs to be reorganized.
 * 
 * SYNOPSYS:
 *
 *   See editor_track_unknown.h
 */

#include <assert.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_gps.h"
#include "roadmap_line.h"

#include "../db/editor_db.h"
#include "../db/editor_line.h"
#include "../db/editor_trkseg.h"
#include "../db/editor_route.h"
#include "../editor_main.h"
#include "../editor_log.h"
#include "editor_track_main.h"
#include "editor_track_util.h"

#include "editor_track_known.h"

#define ROADMAP_NEIGHBOURHOUD  16
static RoadMapNeighbour RoadMapNeighbourhood[ROADMAP_NEIGHBOURHOUD];


int editor_track_known_end_segment (PluginLine *previous_line,
                                    int last_point_id,
                                    PluginLine *line,
                                    int is_new_track) {
   //TODO: add stuff
   //Notice that previous_line may not be valid (only at first)

   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition *current;
   int trkseg;
   int trkseg_line_id;
   int line_length;
   int segment_length;
   int percentage;
   int flags = 0;

   editor_log_push ("editor_track_end_known_segment");

   assert (last_point_id != 0);
   if (!last_point_id) return 0;

   if (editor_db_activate (line->fips) == -1) {
      editor_db_create (line->fips);
      if (editor_db_activate (line->fips) == -1) {
         editor_log_pop ();
         return 0;
      }
   }

   roadmap_plugin_line_from (line, &from);
   roadmap_plugin_line_to (line, &to);
   
   if (roadmap_plugin_get_id (line) == EditorPluginID) {

      line_length = editor_line_length (roadmap_plugin_get_line_id (line));
      trkseg_line_id = roadmap_plugin_get_line_id (line);
   } else {

      line_length = roadmap_line_length (roadmap_plugin_get_line_id (line));
      trkseg_line_id = -1;
   }

   segment_length = editor_track_util_length (0, last_point_id);

   editor_log
      (ROADMAP_INFO,
         "Ending line %d (plugin_id:%d). Line length:%d, Segment length:%d",
         roadmap_plugin_get_line_id (line), roadmap_plugin_get_id (line), line_length, segment_length);

   /* End current segment if we really passed through it
    * and not only touched a part of it.
    */

   assert (line_length > 0);

   if (line_length == 0) {
      editor_log (ROADMAP_ERROR, "line %d (plugin_id:%d) has length of zero.",
            roadmap_plugin_get_line_id (line), roadmap_plugin_get_id (line));
      editor_log_pop ();
      return 0;
   }

   current = track_point_pos (last_point_id);
   if (roadmap_math_distance (current, &to) >
       roadmap_math_distance (current, &from)) {

      flags = ED_TRKSEG_OPPOSITE_DIR;
   }
      
   percentage = 100 * segment_length / line_length;
   if (percentage < 70) {
      editor_log (ROADMAP_INFO, "segment is too small to consider: %d%%",
            percentage);
      if (segment_length > (editor_track_point_distance ()*1.5)) {

         trkseg = editor_track_util_create_trkseg
                     (trkseg_line_id, 0, last_point_id,
                      flags|ED_TRKSEG_IGNORE|ED_TRKSEG_END_TRACK);
         editor_track_add_trkseg (line, trkseg, 0, ED_ROUTE_CAR);
         editor_log_pop ();
         return 1;
      } else {

         trkseg = editor_track_util_create_trkseg
                  (trkseg_line_id, 0, last_point_id, flags|ED_TRKSEG_IGNORE);
         editor_track_add_trkseg (line, trkseg, 0, ED_ROUTE_CAR);
         editor_log_pop ();
         return 0;
      }
   }

   if (is_new_track) {
      flags |= ED_TRKSEG_NEW_TRACK;
   }

   trkseg =
      editor_track_util_create_trkseg
         (trkseg_line_id, 0, last_point_id, flags);

   if (flags & ED_TRKSEG_OPPOSITE_DIR) {
      
      editor_log (ROADMAP_INFO, "Updating route direction: to -> from");
      editor_track_add_trkseg (line, trkseg, 2, ED_ROUTE_CAR);
   } else {

      editor_log (ROADMAP_INFO, "Updating route direction: from -> to");
      editor_track_add_trkseg (line, trkseg, 1, ED_ROUTE_CAR);
   }


   editor_log_pop ();

   return 1;
}


int editor_track_known_locate_point (int point_id,
                                     const RoadMapGpsPosition *gps_position,
                                     RoadMapTracking *confirmed_street,
                                     RoadMapNeighbour *confirmed_line,
                                     RoadMapTracking *new_street,
                                     RoadMapNeighbour *new_line) {

   int found;
   int count;
   int current_fuzzy;
   RoadMapFuzzy best = roadmap_fuzzy_false ();
   
   const RoadMapPosition *position = track_point_pos (point_id);
   
   RoadMapFuzzy before;
   
   if (confirmed_street->valid) {
      /* We have an existing street match: check if it is still valid. */
      
      before = confirmed_street->fuzzyfied;
        
      if (!editor_track_util_get_distance (position,
                                           &confirmed_line->line,
                                           confirmed_line)) {
         current_fuzzy = 0;
      } else {

         current_fuzzy = roadmap_navigate_fuzzify
                           (confirmed_street, confirmed_line,
                            confirmed_line,
                            gps_position->steering);
      }

      if ((current_fuzzy >= before) ||
            roadmap_fuzzy_is_certain(current_fuzzy)) {

         confirmed_street->fuzzyfied = current_fuzzy;
         return 0; /* We are on the same street. */
      }
   }
   
    /* We must search again for the best street match. */
   
   count = editor_track_util_find_street
            (gps_position, new_street,
             confirmed_street,
             confirmed_line,
             RoadMapNeighbourhood,
             ROADMAP_NEIGHBOURHOUD, &found, &best);

#if 0
   if (count && (RoadMapNeighbourhood[found].distance < 30) &&
      !roadmap_fuzzy_is_acceptable (best)) {
      /* if we are on a known street, we don't want to start a new road
       * if we are still close to it. In this case we ignore the fuzzy
       */
   } else
#endif     

   if (!roadmap_fuzzy_is_good (best) &&
         roadmap_fuzzy_is_acceptable (best) &&
         confirmed_street->valid) {
      /* We're not sure that the new line is a real match.
       * Delay the decision if we're still close to the previous road.
       */
      if (confirmed_line->distance < editor_track_point_distance ()) {
         return 0;
      }
   }

   if (roadmap_fuzzy_is_acceptable (best)) {

      if (confirmed_street->valid &&
            (!roadmap_plugin_same_line
               (&confirmed_line->line, &RoadMapNeighbourhood[found].line) ||
               (confirmed_street->opposite_street_direction !=
                new_street->opposite_street_direction))) {

         *new_line = RoadMapNeighbourhood[found];
         new_street->valid = 1;
         new_street->fuzzyfied = best;

         return 1;
      }

      *confirmed_line   = RoadMapNeighbourhood[found];
      
      confirmed_street->fuzzyfied = best;
      confirmed_street->valid = 1;

      return 0;

   } else {

      new_street->valid = 0;

      return 1;
   }
}

