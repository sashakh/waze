/* editor_trkseg.c - point databse layer
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
 *   See editor_trkseg.h
 */

#include <stdlib.h>
#include <assert.h>

#include "../roadmap.h"
#include "../roadmap_math.h"
#include "../roadmap_navigate.h"

#include "editor_db.h"
#include "editor_point.h"
#include "editor_shape.h"
#include "editor_log.h"
#include "editor_trkseg.h"

static editor_db_section *ActiveTrksegDB;

static void editor_trkseg_activate (void *context) {
   ActiveTrksegDB = (editor_db_section *) context;
}

roadmap_db_handler EditorTrksegHandler = {
   "trkseg",
   editor_map,
   editor_trkseg_activate,
   editor_unmap
};


static void editor_trkseg_set (int trkseg,
                               int trk_from,
                               int trk_to,
                               int first_shape,
                               int last_shape,
                               int start_time,
                               int end_time,
                               int flags) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (ActiveTrksegDB, trkseg, 0, NULL);

   assert (track != NULL);
   if (track == NULL) return;

   track->point_from = trk_from;
   track->point_to = trk_to;
   track->first_shape = first_shape;
   track->last_shape = last_shape;
   track->gps_start_time = start_time;
   track->gps_end_time = end_time;
   track->flags = flags;
}


void editor_trkseg_get_time (int trkseg,
                             int *start_time,
                             int *end_time) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (ActiveTrksegDB, trkseg, 0, NULL);

   assert (track != NULL);
   if (track == NULL) return;

   *start_time = track->gps_start_time;
   *end_time = track->gps_end_time;
}


int editor_trkseg_add (int line_id,
                       int p_from,
                       int p_to,
                       int first_shape,
                       int last_shape,
                       int gps_start_time,
                       int gps_end_time,
                       int flags,
                       int connect) {

   editor_db_trkseg track;
   int id;

   track.line_id            = line_id;
   track.point_from         = p_from;
   track.point_to           = p_to;
   track.first_shape        = first_shape;
   track.last_shape         = last_shape;
   track.gps_start_time     = gps_start_time;
   track.gps_end_time       = gps_end_time;
   track.flags              = flags;
   track.next_road_trkseg   = -1;
   track.next_global_trkseg = -1;

   id = editor_db_add_item (ActiveTrksegDB, &track);

   if (id == -1) {
      editor_db_grow ();
      id = editor_db_add_item (ActiveTrksegDB, &track);
   }

   if (id == -1) return -1;

   if (connect == ED_TRKSEG_CONNECT_GLOBAL) {

      int last_global_trkseg = editor_db_get_current_trkseg();
      
      if (last_global_trkseg != -1) {
         editor_trkseg_connect_global (last_global_trkseg, id);

      } else {
         editor_db_update_current_trkseg (id);
      }
   }

   return id;
}


void editor_trkseg_get (int trkseg,
                        int *p_from,
                        int *p_to,
                        int *first_shape,
                        int *last_shape,
                        int *flags) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (ActiveTrksegDB, trkseg, 0, NULL);

   if (p_from != NULL) *p_from = track->point_from;
   if (p_to != NULL) *p_to = track->point_to;
   if (first_shape != NULL) *first_shape = track->first_shape;
   if (last_shape != NULL) *last_shape = track->last_shape;
   if (flags != NULL) *flags = track->flags;
}


void editor_trkseg_connect_roads (int previous, int next) {

   editor_db_trkseg *prev_track;

   prev_track = (editor_db_trkseg *) editor_db_get_item
                              (ActiveTrksegDB, previous, 0, NULL);

   prev_track->next_road_trkseg = next;
}


void editor_trkseg_connect_global (int previous, int next) {

   editor_db_trkseg *prev_track;

   prev_track = (editor_db_trkseg *) editor_db_get_item
                              (ActiveTrksegDB, previous, 0, NULL);

   prev_track->next_global_trkseg = next;
   
   if ((next != -1) && (editor_db_get_current_trkseg() == previous)) {

      editor_db_update_current_trkseg (next);
   }
}


int editor_trkseg_next_in_road (int trkseg) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (ActiveTrksegDB, trkseg, 0, NULL);
   assert (track != NULL);

   if (track == NULL) return -1;

   return track->next_road_trkseg;
}


int editor_trkseg_next_in_global (int trkseg) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (ActiveTrksegDB, trkseg, 0, NULL);
   assert (track != NULL);

   if (track == NULL) return -1;

   return track->next_global_trkseg;
}


int editor_trkseg_split (int trkseg, RoadMapPosition *split_position) {

   RoadMapArea focus;
   RoadMapNeighbour result;
   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition intersection;
   int split_shape_point;
   int i;
   int new_trkseg_id;
   int trk_from;
   int trk_to;
   int trk_start_time;
   int trk_end_time;
   int shape_time;
   int split_time;
   int first_shape;
   int last_shape;
   int flags;
   int distance;
   int smallest_distance = 0x7fffffff;

   editor_trkseg_get_time (trkseg, &trk_start_time, &trk_end_time);
   editor_trkseg_get
      (trkseg, &trk_from, &trk_to, &first_shape, &last_shape, &flags);

   split_shape_point = -1;

   if (first_shape == -1) {
      int p;
      int trk_middle_time;

      flags |= ED_TRKSEG_FAKE;

      editor_point_position (trk_from, &from);
      editor_point_position (trk_to, &to);

      trk_middle_time =
         (trk_start_time + trk_end_time) / 2;

      p = editor_point_add (split_position, 0, 0);

      editor_trkseg_set
            (trkseg,
             trk_from,
             p,
             -1,
             -1,
             trk_start_time,
             trk_middle_time,
             flags);

      p = editor_point_add (split_position, 0, 0);

      return
         editor_trkseg_add
            (-1, p, trk_to, -1, -1, trk_middle_time, trk_end_time, flags, 0);
   }

   focus.west = split_position->longitude - 1000;
   focus.east = split_position->longitude + 1000; 
   focus.south = split_position->latitude - 1000;
   focus.north = split_position->latitude + 1000;
    
   roadmap_math_set_focus (&focus);

   editor_point_position (trk_from, &from);

   split_time = shape_time = trk_start_time;
   to = from;

   for (i=first_shape; i<=last_shape; i++) {
         
      editor_shape_position (i, &to);
      
      distance =
         roadmap_math_get_distance_from_segment
            (split_position, &from, &to, &intersection);

      if (distance < smallest_distance) {
         smallest_distance = distance;
         result.from = from;
         result.to = to;
         result.intersection = intersection;
         split_shape_point = i;
         /* split_time is the time of the previous shape */
         split_time = shape_time;
      }

      editor_shape_time (i, &shape_time);
      from = to;
   }

   editor_point_position (trk_to, &to);

   distance =
      roadmap_math_get_distance_from_segment
         (split_position, &from, &to, &intersection);

   if (distance < smallest_distance) {
      smallest_distance = distance;
      result.from = from;
      result.to = to;
      result.intersection = intersection;
      split_shape_point = last_shape+1;
      split_time = shape_time;
   }

   roadmap_math_release_focus ();

   if (smallest_distance == 0x7fffffff) {
      editor_log (ROADMAP_ERROR, "split position is too far from target line!");
      assert (0);
      return -1;
   }

   if (split_shape_point > last_shape) {

      int p;
         
      p = editor_point_add (&result.from, 0, 0);
      new_trkseg_id =
         editor_trkseg_add
            (-1, p, trk_to, -1, -1, split_time, trk_end_time, flags, 0);

      p = editor_point_add (&result.from, 0, 0);
      editor_trkseg_set
         (trkseg,
          trk_from,
          p,
          first_shape,
          last_shape,
          trk_start_time,
          split_time,
          flags);

   } else {
   
      RoadMapPosition *pos;
      int lon_diff;
      int lat_diff;
      int time_diff;
      int p;

/*
      if (roadmap_math_distance (&result.from, &result.intersection) <
          roadmap_math_distance (&result.to, &result.intersection)) {

         pos = &result.from;
         
         lon_diff = 0;
         lat_diff = 0;
         time_diff = 0;
      } else {
         pos = &result.to;

         lon_diff = result.from.longitude - pos->longitude;
         lat_diff = result.from.latitude - pos->latitude;

         time_diff = split_time;
         editor_shape_time (split_shape_point, &split_time);
         time_diff -= split_time;
      }
*/
      pos = &result.to;

      lon_diff = result.from.longitude - pos->longitude;
      lat_diff = result.from.latitude - pos->latitude;
      time_diff = split_time;
      editor_shape_time (split_shape_point, &split_time);

      if (!lon_diff && !lat_diff) {
         /* from point equals to first shape point. */
         split_shape_point++;

         lon_diff = result.to.longitude - pos->longitude;
         lat_diff = result.to.latitude - pos->latitude;
         editor_shape_time (split_shape_point, &split_time);
      }

      time_diff -= split_time;

      if (split_shape_point > last_shape) {
         split_shape_point = last_shape = -1;
      }

      p = editor_point_add (pos, 0, 0);
      new_trkseg_id =
         editor_trkseg_add
            (-1,
             p,
             trk_to,
             split_shape_point,
             last_shape,
             split_time,
             trk_end_time,
             flags,
             0);

      editor_shape_adjust_point
         (split_shape_point, lon_diff, lat_diff, time_diff);

      p = editor_point_add (pos, 0, 0);
      if (split_shape_point == first_shape) {
         split_shape_point = first_shape = -1;
      }
      editor_trkseg_set
         (trkseg,
          trk_from,
          p,
          first_shape,
          split_shape_point-1,
          trk_start_time,
          split_time,
          flags);
   }

   return new_trkseg_id;
}


void editor_trkseg_set_line (int trkseg, int line_id) {
   
   editor_db_trkseg *track;

   while (trkseg != -1) {
      track = (editor_db_trkseg *) editor_db_get_item
                                 (ActiveTrksegDB, trkseg, 0, NULL);

      assert (track != NULL);
      if (track == NULL) return;

      track->line_id = line_id;

      trkseg = track->next_road_trkseg;
   }
}


int editor_trkseg_get_line (int trkseg) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                                 (ActiveTrksegDB, trkseg, 0, NULL);

   assert (track != NULL);

   return track->line_id;
}

