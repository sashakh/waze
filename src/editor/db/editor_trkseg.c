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

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_street.h"
#include "roadmap_navigate.h"

#include "../editor_log.h"

#include "editor_db.h"
#include "editor_point.h"
#include "editor_shape.h"
#include "editor_trkseg.h"

editor_db_trkseg_private editor_db_trkseg_private_init = {-1, -1};

static editor_db_trkseg_header *ActiveTrksegDB;

static void editor_trkseg_activate (void *context) {
   ActiveTrksegDB = (editor_db_trkseg_header *) context;
}

roadmap_db_handler EditorTrksegHandler = {
   "trkseg",
   editor_map,
   editor_trkseg_activate,
   editor_unmap
};


static void editor_trkseg_set (int trkseg,
                               int trk_from,
                               int first_shape,
                               int last_shape,
                               time_t start_time,
                               time_t end_time,
                               int flags) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (&ActiveTrksegDB->section, trkseg, 0, NULL);

   assert (track != NULL);
   if (track == NULL) return;

   track->point_from = trk_from;
   track->first_shape = first_shape;
   track->last_shape = last_shape;
   track->gps_start_time = start_time;
   track->gps_end_time = end_time;
   track->flags = flags;
}


void editor_trkseg_get_time (int trkseg,
                             time_t *start_time,
                             time_t *end_time) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (&ActiveTrksegDB->section, trkseg, 0, NULL);

   assert (track != NULL);
   if (track == NULL) return;

   *start_time = track->gps_start_time;
   *end_time = track->gps_end_time;
}


int editor_trkseg_add (int line_id,
                       int plugin_id,
                       int p_from,
                       int first_shape,
                       int last_shape,
                       time_t gps_start_time,
                       time_t gps_end_time,
                       int flags) {

   editor_db_trkseg track;
   int id;

   track.line_id            = line_id;
   track.plugin_id          = plugin_id;
   track.point_from         = p_from;
   track.first_shape        = first_shape;
   track.last_shape         = last_shape;
   track.gps_start_time     = gps_start_time;
   track.gps_end_time       = gps_end_time;
   track.flags              = flags;
   track.next_road_trkseg   = -1;
   track.next_global_trkseg = -1;

   id = editor_db_add_item (&ActiveTrksegDB->section, &track);

   if (id == -1) {
      editor_db_grow ();
      id = editor_db_add_item (&ActiveTrksegDB->section, &track);
   }

   if (id == -1) return -1;

   if ( !(flags & ED_TRKSEG_NO_GLOBAL)) {

      if (ActiveTrksegDB->header.last_global_trkseg == -1) {
         
         ActiveTrksegDB->header.last_global_trkseg = id;
      } else {

         editor_trkseg_connect_global
            (ActiveTrksegDB->header.last_global_trkseg, id);
      }

      if (ActiveTrksegDB->header.next_export == -1) {
         ActiveTrksegDB->header.next_export = id;
      }
   }

   return id;
}


void editor_trkseg_get (int trkseg,
                        int *p_from,
                        int *first_shape,
                        int *last_shape,
                        int *flags) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (&ActiveTrksegDB->section, trkseg, 0, NULL);

   if (p_from != NULL) *p_from = track->point_from;
   if (first_shape != NULL) *first_shape = track->first_shape;
   if (last_shape != NULL) *last_shape = track->last_shape;
   if (flags != NULL) *flags = track->flags;
}


void editor_trkseg_connect_roads (int previous, int next) {

   editor_db_trkseg *prev_track;

   prev_track = (editor_db_trkseg *) editor_db_get_item
                              (&ActiveTrksegDB->section, previous, 0, NULL);

   prev_track->next_road_trkseg = next;
}


void editor_trkseg_connect_global (int previous, int next) {

   editor_db_trkseg *prev_track;

   prev_track = (editor_db_trkseg *) editor_db_get_item
                              (&ActiveTrksegDB->section, previous, 0, NULL);

   prev_track->next_global_trkseg = next;
   
   if ((next != -1) &&
         (ActiveTrksegDB->header.last_global_trkseg == previous)) {

      ActiveTrksegDB->header.last_global_trkseg = next;
   }
}


int editor_trkseg_next_in_road (int trkseg) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (&ActiveTrksegDB->section, trkseg, 0, NULL);
   assert (track != NULL);

   if (track == NULL) return -1;

   return track->next_road_trkseg;
}


int editor_trkseg_next_in_global (int trkseg) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                              (&ActiveTrksegDB->section, trkseg, 0, NULL);
   assert (track != NULL);

   if (track == NULL) return -1;

   return track->next_global_trkseg;
}


int editor_trkseg_split (int trkseg,
                         RoadMapPosition *line_from,
                         RoadMapPosition *line_to,
                         RoadMapPosition *split_position) {

   RoadMapArea focus;
   RoadMapNeighbour result;
   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition intersection;
   int split_shape_point;
   int i;
   int new_trkseg_id;
   int trk_from;
   time_t trk_start_time;
   time_t trk_end_time;
   time_t shape_time;
   time_t split_time;
   int first_shape;
   int last_shape;
   int flags;
   int distance;
   int smallest_distance = 0x7fffffff;

   editor_trkseg_get_time (trkseg, &trk_start_time, &trk_end_time);
   editor_trkseg_get
      (trkseg, &trk_from, &first_shape, &last_shape, &flags);

   split_shape_point = -1;

   if (first_shape == -1) {
      int trk_middle_time;

      editor_point_position (trk_from, &from);

      trk_middle_time =
         (trk_start_time + trk_end_time) / 2;

      editor_trkseg_set
            (trkseg,
             trk_from,
             -1,
             -1,
             trk_start_time,
             trk_middle_time,
             flags);

      i = editor_point_add (&from, 0, -1);

      return
         editor_trkseg_add
            (-1, -1, i, -1, -1, trk_middle_time, trk_end_time,
             flags|ED_TRKSEG_NO_GLOBAL);
   }

   focus.west = split_position->longitude - 1000;
   focus.east = split_position->longitude + 1000; 
   focus.south = split_position->latitude - 1000;
   focus.north = split_position->latitude + 1000;
    
   //roadmap_math_set_focus (&focus);

   editor_point_position (trk_from, &to);

   split_time = shape_time = trk_start_time;

   if (flags & ED_TRKSEG_OPPOSITE_DIR) {
      from = *line_to;
   } else {
      from = *line_from;
   }

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

   //roadmap_math_release_focus ();

   if (smallest_distance == 0x7fffffff) {
      editor_log (ROADMAP_ERROR, "split position is too far from target line!");
      assert (0);
      return -1;
   }

   if (split_shape_point == first_shape) {
      /* the split is before any trkseg points */

      editor_point_position (trk_from, &from);
         
      i = editor_point_add (&from, 0, -1);

      new_trkseg_id =
         editor_trkseg_add
            (-1, -1, i, first_shape, last_shape, split_time, trk_end_time,
             flags|ED_TRKSEG_NO_GLOBAL);

      editor_trkseg_set
         (trkseg,
          trk_from,
          -1,
          -1,
          trk_start_time,
          split_time,
          flags);

      return new_trkseg_id;
   }

   if ((split_shape_point == last_shape) &&
         !roadmap_math_compare_points (&result.to, &result.intersection)) {

      /* the split is after the trkseg points */
         
      i = editor_point_add (&result.to, 0, -1);
      new_trkseg_id =
         editor_trkseg_add
            (-1, -1, i, -1, -1, split_time, trk_end_time,
             flags|ED_TRKSEG_NO_GLOBAL);

      editor_trkseg_set
         (trkseg,
          trk_from,
          first_shape,
          last_shape,
          trk_start_time,
          split_time,
          flags);

      return new_trkseg_id;
   }


   editor_shape_time (split_shape_point, &split_time);

   i = editor_point_add (&result.to, 0, -1);
   new_trkseg_id =
      editor_trkseg_add
         (-1,
          -1,
          i,
          split_shape_point,
          last_shape,
          split_time,
          trk_end_time,
          flags|ED_TRKSEG_NO_GLOBAL);

   editor_shape_set_point
      (split_shape_point, 0, 0, 0);

   editor_trkseg_set
      (trkseg,
       trk_from,
       first_shape,
       split_shape_point-1,
       trk_start_time,
       split_time,
       flags);

   return new_trkseg_id;
}


void editor_trkseg_set_line (int trkseg, int line_id, int plugin_id) {
   
   editor_db_trkseg *track;

   while (trkseg != -1) {
      track = (editor_db_trkseg *) editor_db_get_item
                                 (&ActiveTrksegDB->section, trkseg, 0, NULL);

      assert (track != NULL);
      if (track == NULL) return;

      track->line_id = line_id;
      track->plugin_id = plugin_id;

      trkseg = track->next_road_trkseg;
   }
}


void editor_trkseg_get_line (int trkseg, int *line_id, int *plugin_id) {

   editor_db_trkseg *track;

   track = (editor_db_trkseg *) editor_db_get_item
                                 (&ActiveTrksegDB->section, trkseg, 0, NULL);

   assert (track != NULL);

   *line_id = track->line_id;
   *plugin_id = track->plugin_id;
}


int editor_trkseg_dup (int source_id) {

   editor_db_trkseg *track;
   RoadMapPosition from;
   RoadMapPosition shape_pos;
   time_t start_time;
   time_t shape_time;
   int new_from_point;
   int new_first_shape = -1;
   int new_last_shape = -1;
   int i;

   track = (editor_db_trkseg *) editor_db_get_item
                              (&ActiveTrksegDB->section, source_id, 0, NULL);

   if (track == NULL) return -1;

   editor_point_position (track->point_from, &from);

   new_from_point = editor_point_add (&from, 0, -1);
   
   if (new_from_point == -1) return -1;

   if (track->first_shape != -1) {

      shape_pos = from;
      start_time = track->gps_start_time;

      for (i=track->first_shape; i<=track->last_shape; i++) {
         editor_shape_position (i, &shape_pos);
         editor_shape_time (i, &shape_time);
         new_last_shape =
               editor_shape_add
                        (shape_pos.longitude - from.longitude,
                         shape_pos.latitude - from.latitude,
                         (short)(shape_time - start_time));

         if (new_last_shape == -1) {
            editor_log
               (ROADMAP_ERROR,
                "Can't create new shape. Line creation is aborted.");
            editor_log_pop ();
            return -1;
         }

         if (new_first_shape == -1) {
            new_first_shape = new_last_shape;
         }

         from = shape_pos;
         start_time = shape_time;
      }
   }
   
   return
      editor_trkseg_add
         (track->line_id,
          track->plugin_id,
          new_from_point,
          new_first_shape,
          new_last_shape,
          track->gps_start_time,
          track->gps_end_time,
          track->flags);
}


int editor_trkseg_get_current_trkseg (void) {

   return ActiveTrksegDB->header.last_global_trkseg;
}


void editor_trkseg_reset_next_export (void) {

   ActiveTrksegDB->header.next_export = -1;
}

int editor_trkseg_get_next_export (void) {

   return ActiveTrksegDB->header.next_export;
}

void editor_trkseg_set_next_export (int id) {

   ActiveTrksegDB->header.next_export = id;
}

