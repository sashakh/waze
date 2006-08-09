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
#include <string.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_gps.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"

#include "../db/editor_db.h"
#include "../db/editor_line.h"
#include "../db/editor_trkseg.h"
#include "../db/editor_route.h"
#include "../editor_main.h"
#include "../editor_log.h"
#include "editor_track_main.h"
#include "editor_track_util.h"

#include "editor_track_known.h"

#define MAX_ENTRIES 5
#define MAX_PATHS 5

typedef struct {
   RoadMapTracking street;
   RoadMapNeighbour line;
   int point_id;
} KnownCandidateEntry;

typedef struct {
   KnownCandidateEntry entries[MAX_ENTRIES];
   int count;
} KnownCandidatePath;

static KnownCandidatePath KnownCandidates[MAX_PATHS];
static int KnownCandidatesCount;

#define ROADMAP_NEIGHBOURHOUD  16
static RoadMapNeighbour RoadMapNeighbourhood[ROADMAP_NEIGHBOURHOUD];


static int resolve_candidates (const RoadMapGpsPosition *gps_position,
                               int point_id) {

   int count;
   int c;
   int found;
   RoadMapFuzzy best = roadmap_fuzzy_false ();
   RoadMapFuzzy second_best;
   int i;
   int j;

   for (c = 0; c < KnownCandidatesCount; c++) {

      RoadMapTracking new_street;

      KnownCandidateEntry *entry =
         KnownCandidates[c].entries + KnownCandidates[c].count - 1;
   
   
      count = editor_track_util_find_street
         (gps_position, &new_street,
          &entry->street,
          &entry->line,
          RoadMapNeighbourhood,
          ROADMAP_NEIGHBOURHOUD, &found, &best, &second_best);

      if (!roadmap_fuzzy_is_good (best)) {

         /* remove this entry */
         memmove (KnownCandidates + c, KnownCandidates + c + 1,
               (KnownCandidatesCount - c - 1) * sizeof (KnownCandidatePath));

         KnownCandidatesCount--;
         c--;
         continue;
      }

      if (roadmap_fuzzy_is_good (best) &&
            roadmap_fuzzy_is_good (second_best)) {

         /* We got too many good candidates. Wait before deciding. */

         continue;
      }

      if (!roadmap_plugin_same_line
            (&entry->line.line, &RoadMapNeighbourhood[found].line)) {
         
         int p;

         if (KnownCandidates[c].count == 1) {
            p = point_id - entry->point_id;
         } else {
            p = KnownCandidates[c].entries[KnownCandidates[c].count-2].
                  point_id - entry->point_id;
         }

         if (++KnownCandidates[c].count == MAX_ENTRIES) {
            return -1;
         }

         entry++;
         entry->point_id = p;
         entry->line = RoadMapNeighbourhood[found];
         entry->street = new_street;
         entry->street.fuzzyfied = best;
         entry->street.valid = 1;
      }
   }

   /* check for duplicates */
   for (i = 0; i < KnownCandidatesCount-1; i++) {

      KnownCandidateEntry *entry1 =
         KnownCandidates[i].entries + KnownCandidates[i].count - 1;
      
      for (j = i+1; j < KnownCandidatesCount; j++) {

         KnownCandidateEntry *entry2 =
            KnownCandidates[j].entries + KnownCandidates[j].count - 1;

         if (roadmap_plugin_same_line
               (&entry2->line.line, &entry1->line.line)) {

            return -1;
         }
      }
   }
      
   return 0;
}


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
   int trkseg_plugin_id;
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
   trkseg_plugin_id = roadmap_plugin_get_id (line);
   trkseg_line_id = roadmap_plugin_get_line_id (line);
   
   if (trkseg_plugin_id == EditorPluginID) {

      line_length = editor_line_length (trkseg_line_id);
   } else {

      line_length = roadmap_line_length (trkseg_line_id);
   }

   segment_length = editor_track_util_length (0, last_point_id);

   editor_log
      (ROADMAP_INFO,
         "Ending line %d (plugin_id:%d). Line length:%d, Segment length:%d",
         trkseg_line_id, trkseg_plugin_id, line_length, segment_length);

   /* End current segment if we really passed through it
    * and not only touched a part of it.
    */

   assert (line_length > 0);

   if (line_length == 0) {
      editor_log (ROADMAP_ERROR, "line %d (plugin_id:%d) has length of zero.",
            trkseg_line_id, trkseg_plugin_id);
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
                     (trkseg_line_id, trkseg_plugin_id, 0, last_point_id,
                      flags|ED_TRKSEG_IGNORE|ED_TRKSEG_END_TRACK);

         editor_track_add_trkseg
            (line, trkseg, ROUTE_DIRECTION_NONE, ROUTE_CAR_ALLOWED);
         editor_log_pop ();
         return 1;
      } else {

         trkseg = editor_track_util_create_trkseg
                  (trkseg_line_id, trkseg_plugin_id,
                   0, last_point_id, flags|ED_TRKSEG_IGNORE);
         editor_track_add_trkseg
            (line, trkseg, ROUTE_DIRECTION_NONE, ROUTE_CAR_ALLOWED);
         editor_log_pop ();
         return 0;
      }
   }

   if (is_new_track) {
      flags |= ED_TRKSEG_NEW_TRACK;
   }

   trkseg =
      editor_track_util_create_trkseg
         (trkseg_line_id, trkseg_plugin_id, 0, last_point_id, flags);

   if (flags & ED_TRKSEG_OPPOSITE_DIR) {
      
      editor_log (ROADMAP_INFO, "Updating route direction: to -> from");
      editor_track_add_trkseg
         (line, trkseg, ROUTE_DIRECTION_AGAINST_LINE, ROUTE_CAR_ALLOWED);
   } else {

      editor_log (ROADMAP_INFO, "Updating route direction: from -> to");
      editor_track_add_trkseg
         (line, trkseg, ROUTE_DIRECTION_WITH_LINE, ROUTE_CAR_ALLOWED);
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
   RoadMapFuzzy second_best;
   
   const RoadMapPosition *position = track_point_pos (point_id);
   
   RoadMapFuzzy before;
   
   if (KnownCandidatesCount > 1) {
      
      if ((resolve_candidates (gps_position, point_id) == -1) ||
            !KnownCandidatesCount) {

         KnownCandidatesCount = 0;
         editor_track_reset ();
         return 0;
      }

      if (KnownCandidatesCount > 1) {

         return 0;
      }
   }

   if (KnownCandidatesCount == 1) {

      KnownCandidateEntry *entry = KnownCandidates[0].entries;

      if (!--KnownCandidates[0].count) {
         KnownCandidatesCount = 0;
      }

      *new_line = entry->line;
      *new_street = entry->street;
      point_id = entry->point_id;

      memmove (entry, entry+1,
            KnownCandidatesCount * sizeof(KnownCandidateEntry));

      return point_id;
   }

   if (confirmed_street->valid) {
      /* We have an existing street match: check if it is still valid. */
      
      before = confirmed_street->fuzzyfied;
        
      if (!editor_track_util_get_distance (position,
                                           &confirmed_line->line,
                                           confirmed_line)) {
         current_fuzzy = 0;
      } else {

         current_fuzzy = roadmap_navigate_fuzzify
                           (new_street,
                            confirmed_street,
                            confirmed_line,
                            confirmed_line,
                            0,
                            gps_position->steering);
      }

      if ((new_street->line_direction == confirmed_street->line_direction) &&
            ((current_fuzzy >= before) ||
            roadmap_fuzzy_is_certain(current_fuzzy))) {

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
             ROADMAP_NEIGHBOURHOUD, &found, &best, &second_best);

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
         RoadMapTracking candidate;

         if (roadmap_plugin_same_line (&RoadMapNeighbourhood[found].line,
                                       &confirmed_line->line)) {
            /* We don't have any candidates other than the current line */
            return 0;
         }

         /* We have two candidates here */

         /* current line */
         KnownCandidates[0].entries[0].street = *confirmed_street;
         KnownCandidates[0].entries[0].line = *confirmed_line;
         KnownCandidates[0].entries[0].point_id = 0;
         KnownCandidates[0].count = 1;

         /* new line */
         candidate.opposite_street_direction = 0;
         candidate.valid = 1;
         candidate.fuzzyfied = roadmap_navigate_fuzzify
                                 (&candidate, confirmed_street,
                                  confirmed_line, RoadMapNeighbourhood+found,
                                  0,
                                  gps_position->steering);
         KnownCandidates[1].entries[0].street = candidate;
         KnownCandidates[1].entries[0].line = RoadMapNeighbourhood[found];
         KnownCandidates[1].entries[0].point_id = point_id;
         KnownCandidates[1].count = 1;
         KnownCandidatesCount = 2;
         return 0;
      }
   }

   if (roadmap_fuzzy_is_good (best) &&
            roadmap_fuzzy_is_good (second_best)) {

      /* We got too many good candidates. Wait before deciding. */

      int i;
      RoadMapFuzzy result;

      for (i = 0; i < count; ++i) {
         RoadMapTracking candidate;

         result = roadmap_navigate_fuzzify
                    (&candidate,
                     confirmed_street,
                     confirmed_line,
                     RoadMapNeighbourhood+i,
                     0,
                     gps_position->steering);
      
         if (roadmap_fuzzy_is_good (result)) {

            if (roadmap_plugin_same_line (&confirmed_line->line,
                     &RoadMapNeighbourhood[i].line)) {
               
               if (result < best) {
               
                  continue;
               } else {
                  /* delay the decision as the current line is still good */
                  KnownCandidatesCount = 0;
                  return 0;
               }
            }

            candidate.opposite_street_direction = 0;
            candidate.valid = 1;
            candidate.fuzzyfied = result;
            KnownCandidates[KnownCandidatesCount].entries[0].street = candidate;
            KnownCandidates[KnownCandidatesCount].entries[0].line =
                                                   RoadMapNeighbourhood[i];

            KnownCandidates[KnownCandidatesCount].entries[0].point_id =
                                                   point_id;

            KnownCandidates[KnownCandidatesCount].count = 1;
            KnownCandidatesCount++;
         }
      }

      if (KnownCandidatesCount > 1) {

         return 0;
      } else {
         /* We only got one candidate so fall through to use it */
         KnownCandidatesCount = 0;
      }
   }

   if (roadmap_fuzzy_is_acceptable (best)) {

      if (confirmed_street->valid &&
            (!roadmap_plugin_same_line
               (&confirmed_line->line, &RoadMapNeighbourhood[found].line) ||
               (confirmed_street->opposite_street_direction !=
                new_street->opposite_street_direction) ||
               (confirmed_street->line_direction !=
                new_street->line_direction))) {

         *new_line = RoadMapNeighbourhood[found];
         new_street->valid = 1;
         new_street->fuzzyfied = best;

         return point_id;
      }

      *confirmed_line   = RoadMapNeighbourhood[found];
      *confirmed_street = *new_street;
      
      confirmed_street->fuzzyfied = best;
      confirmed_street->valid = 1;

      return 0;

   } else {

      new_street->valid = 0;

      return point_id;
   }
}

