/* editor_track.c - street databse layer
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
 *   See editor_track.h
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_gps.h"
#include "roadmap_fuzzy.h"
#include "roadmap_navigate.h"
#include "roadmap_screen.h"

#include "../editor_main.h"
#include "../db/editor_db.h"
#include "../db/editor_point.h"
#include "../db/editor_route.h"
#include "../editor_log.h"
#include "editor_track_filter.h"
#include "editor_track_util.h"
#include "editor_track_known.h"
#include "editor_track_unknown.h"

#include "editor_track_main.h"

#define GPS_POINTS_DISTANCE "10m"
#define MAX_POINTS_IN_SEGMENT 10000

typedef struct {

   RoadMapGpsPosition gps_point;
   time_t time;

} TrackPoint;

static TrackPoint TrackPoints[MAX_POINTS_IN_SEGMENT];
static int points_count = 0;
static int cur_active_line = 0;
static RoadMapGpsPosition TrackLastPosition;

static RoadMapTracking  TrackConfirmedStreet = ROADMAP_TRACKING_NULL;
static RoadMapNeighbour TrackPreviousLine = ROADMAP_NEIGHBOUR_NULL;
static RoadMapNeighbour TrackConfirmedLine = ROADMAP_NEIGHBOUR_NULL;

static NodeNeighbour cur_node = {-1, -1};

static int is_new_track = 1;

RoadMapPosition* track_point_pos (int index) {

   return (RoadMapPosition*) &TrackPoints[index].gps_point;
}

RoadMapGpsPosition* track_point_gps (int index) {

   return &TrackPoints[index].gps_point;
}

time_t track_point_time (int index) {

   return TrackPoints[index].time;
}

static int editor_track_add_point(RoadMapGpsPosition *point, time_t time) {

   if (points_count == MAX_POINTS_IN_SEGMENT) return -1;

   TrackPoints[points_count].gps_point = *point;
   TrackPoints[points_count].time = time;

   return points_count++;
}


static void track_reset_points (int last_point_id) {

   if (last_point_id == 0) return;
   
   if (last_point_id == -1) {
      points_count = 0;
      return;
   }

   points_count -= last_point_id;

   if (points_count > 0) {
      memmove (TrackPoints, TrackPoints + last_point_id,
            sizeof(TrackPoints[0]) * points_count);
   }
}


static int create_node(int point_id, NodeNeighbour *node) {

   int new_node;

   if (node && (node->id != -1)) {

      if (node->plugin_id == ROADMAP_PLUGIN_ID) {
         new_node = editor_point_roadmap_to_editor (node->id);

         return new_node;
      }

      return node->id;
   }
   
   
   new_node = editor_point_add (track_point_pos (point_id), 0, -1);

   return new_node;
}


static int create_new_line (int gps_first_point,
                            int gps_last_point,
                            int from_point,
                            int to_point,
                            int cfcc) {

   int p_from;
   int p_to;
   int line_id;

   if (editor_track_util_create_db (track_point_pos (gps_last_point)) == -1) {

      editor_log (ROADMAP_ERROR, "create_new_line: can't create db.");
      return -1;
   }

   if (from_point != -1) {
      p_from = from_point;
      
   } else {
      
      p_from = create_node (gps_first_point, &cur_node);
      if (p_from == -1) {
         return -1;
      }
   }
      
   if (to_point != -1) {
      p_to = to_point;
   } else {

      RoadMapPosition start_pos;
      editor_point_position (p_from, &start_pos);

      /* check if first and last point are the same */
      if (!roadmap_math_compare_points
            (&start_pos, track_point_pos (gps_last_point))) {
         p_to = p_from;

      } else {
         p_to = editor_point_add (track_point_pos (gps_last_point), 0, -1);

         if (p_to == -1) {
            return -1;
         }
      }
   }

   cur_node.id = p_to;
   cur_node.plugin_id = EditorPluginID;

   line_id = editor_track_util_create_line
               (gps_first_point, gps_last_point,
                p_from, p_to, cfcc, is_new_track);

   is_new_track = 0;

   return line_id;
}


static void end_known_segment (int point_id,
                               RoadMapTracking *new_street,
                               RoadMapNeighbour *new_line) {

   int fips = roadmap_plugin_get_fips (&TrackConfirmedLine.line);

   if (editor_db_activate (fips) == -1) {

      editor_db_create (fips);
      if (editor_db_activate (fips) == -1) {
         roadmap_log (ROADMAP_ERROR, "Can't end known segment.");

         track_reset_points (-1);
         TrackConfirmedLine = *new_line;
         TrackConfirmedStreet = *new_street;
         return;
      }
   }

   if (new_street->valid) {

      /* we have just switched from one known street to another */

      int split_point = editor_track_util_connect_roads
                           (&TrackConfirmedLine.line,
                            &new_line->line,
                            TrackConfirmedStreet.opposite_street_direction,
                            new_street->opposite_street_direction,
                            track_point_gps (point_id),
                            points_count - 1);

      if (editor_track_known_end_segment
            (&TrackPreviousLine.line, split_point,
             &TrackConfirmedLine.line, is_new_track)) {

         is_new_track = 0;
         TrackPreviousLine = TrackConfirmedLine;
      }

      track_reset_points (split_point);

      TrackConfirmedLine = *new_line;
      TrackConfirmedStreet = *new_street;

      return;
   }


   /* we are not on a known road */

   if (TrackConfirmedStreet.valid) {

      int split_point;

      /* we just lost a known road, let's find the best point
         to start this new road from */

      split_point =
         editor_track_util_new_road_start
                     (&TrackConfirmedLine,
                      track_point_pos (point_id),
                      point_id,
                      TrackConfirmedStreet.opposite_street_direction,
                      &cur_node);

      if (split_point != -1) {
         if (editor_track_known_end_segment
               (&TrackPreviousLine.line, split_point,
                &TrackConfirmedLine.line, is_new_track)) {

            is_new_track = 0;
            TrackPreviousLine = TrackConfirmedLine;
         }
      }

      track_reset_points (split_point);

      TrackConfirmedLine = *new_line;
      TrackConfirmedStreet = *new_street;
   }

}

   
static void end_unknown_segments (TrackNewSegment *new_segments, int count) {

   int i;
   int start_point = 0;
   NodeNeighbour end_node = NODE_NEIGHBOUR_NULL;

   for (i=0; i<count; i++) {

      int type = new_segments[i].type;
      int end_point = new_segments[i].point_id;
      int end_node_id = -1;

      switch (type) {
         
         case TRACK_ROAD_TURN:

            if (editor_track_util_length (start_point, end_point) <
                  (editor_track_point_distance () * 2)) {

               RoadMapPosition pos;
               RoadMapPosition *pos1;
               RoadMapPosition *pos2;

               pos1 = track_point_pos (start_point);
               pos2 = track_point_pos (end_point);
               pos.longitude = (pos1->longitude + pos2->longitude) / 2;
               pos.latitude = (pos1->latitude + pos2->latitude) / 2;

               editor_point_set_pos (cur_node.id, &pos);

               start_point = end_point;
               continue;
            }

            break;

         case TRACK_ROAD_ROUNDABOUT:

            create_new_line
               (start_point, end_point, cur_node.id, cur_node.id, 4);

            start_point = end_point;
            continue;

            break;
      }
            
      if ((i == (count - 1)) && (TrackConfirmedStreet.valid)) {

         /* we are connecting to a known road */
         end_point =
            editor_track_util_new_road_end
                     (&TrackConfirmedLine,
                      track_point_pos (end_point),
                      points_count-1,
                      TrackConfirmedStreet.opposite_street_direction,
                      &end_node);

         end_node_id = end_node.id;
      }

      if ((i < (count -1)) || (start_point != (end_point -1))) {
         create_new_line (start_point, end_point, -1, end_node_id, 4);
      }

      start_point = end_point;
   }

   track_reset_points (start_point);
}


static void track_rec_locate_point(int point_id) {

   int i;
   int count;
   TrackNewSegment new_segments[10];

   if (!cur_active_line) {

      RoadMapTracking new_street;
      RoadMapNeighbour new_line;
         
      count = editor_track_known_locate_point
               (point_id,
                &TrackLastPosition,
                &TrackConfirmedStreet,
                &TrackConfirmedLine,
                &new_street,
                &new_line);

      if (count) {
         end_known_segment (point_id, &new_street, &new_line);

         if (!new_street.valid) {
            /* the current point does not belong to a known street */

            cur_active_line = 1;

            for (i=0; i<points_count; i++) {

               track_rec_locate_point (i);
            }
         }
      }
            
   } else {

      count = editor_track_unknown_locate_point
               (point_id,
                &TrackLastPosition,
                &TrackConfirmedStreet,
                &TrackConfirmedLine,
                new_segments,
                sizeof(new_segments) / sizeof(new_segments[0]),
                0);

      if (count) {

         if ((point_id == 0) && TrackConfirmedStreet.valid) {
            cur_active_line = 0;
            return;
         }

         assert (point_id > 0);
         end_unknown_segments (new_segments, count);

         if (TrackConfirmedStreet.valid) {

            /* the current point is a known street */
            cur_active_line = 0;
         } 

         /* After creating a new line, we need to check if the current
          * point_is still known.
          * This is not an efficent way to do it...
          */

         for (i=0; i<points_count; i++) {

            track_rec_locate_point (i);
         }
      }
   }
}


static void track_rec_locate(time_t gps_time,
                             const RoadMapGpsPrecision *dilution,
                             const RoadMapGpsPosition* gps_position) {

   static struct GPSFilter *filter;
   const RoadMapGpsPosition *filtered_gps_point;
   RoadMapPosition context_save_pos;
   int context_save_zoom;
   int point_id;
   int res;
   
   if (filter == NULL) {

      filter = editor_track_filter_new 
         (roadmap_math_distance_convert ("1000m", NULL),
          600, /* 10 minutes */
          roadmap_math_distance_convert ("10m", NULL));
   }

   roadmap_math_get_context (&context_save_pos, &context_save_zoom);
   roadmap_math_set_context ((RoadMapPosition *)gps_position, 20);
   editor_track_util_set_focus ((RoadMapPosition *)gps_position);

   res = editor_track_filter_add (filter, gps_time, dilution, gps_position);

   if (res == ED_TRACK_END) {

      /* The current point is too far from the previous point, or
       * the time from the previous point is too long.
       * This is probably a new GPS track.
       */

      editor_track_end ();
      goto restore;
   }

   while ((filtered_gps_point = editor_track_filter_get (filter)) != NULL) {

      TrackLastPosition = *filtered_gps_point;
      
      point_id = editor_track_add_point(&TrackLastPosition, gps_time);

      if (point_id == -1) {
         //TODO: new segment
         assert(0);
      }

      roadmap_fuzzy_set_cycle_params (40, 150);
      
      track_rec_locate_point (point_id);
   }

restore:
   editor_track_util_release_focus ();
   roadmap_math_set_context (&context_save_pos, context_save_zoom);
}


static void
editor_gps_update (time_t gps_time,
                   const RoadMapGpsPrecision *dilution,
                   const RoadMapGpsPosition *gps_position) {

   if (editor_is_enabled()) {

      track_rec_locate(gps_time, dilution, gps_position);
   }
}


void editor_track_initialize (void) {

   roadmap_gps_register_listener(editor_gps_update);
}


int editor_track_point_distance (void) {

   static int distance = -1;

   if (distance == -1) {

      distance = roadmap_math_distance_convert (GPS_POINTS_DISTANCE, NULL);
   }

   return distance;
}


void editor_track_end (void) {

   if (points_count > 1) {

      if (cur_active_line) {

         TrackNewSegment segment;

         segment.point_id = points_count - 1;
         segment.type = TRACK_ROAD_REG;

         end_unknown_segments (&segment, 1);
         cur_active_line = 0;
      } else {

         if (TrackConfirmedStreet.valid) {
            editor_track_known_end_segment
               (&TrackPreviousLine.line,
                points_count - 1,
                &TrackConfirmedLine.line,
                is_new_track);
         }
      }
   }

   TrackConfirmedStreet.valid = 0;
   track_reset_points (points_count);
   cur_node.id = -1;
   is_new_track = 1;
}


static void editor_track_shape_position (int shape, RoadMapPosition *position) {

   assert (shape < points_count);

   *position = *track_point_pos (shape);
}


int editor_track_draw_current (RoadMapPen pen) {

   RoadMapPosition *from;
   RoadMapPosition *to;
   int first_shape = -1;
   int last_shape = -2;

   if (!cur_active_line) return 0;
   if (points_count < 2) return 0;

   if (pen == NULL) return 0;

   from = track_point_pos (0);
   to = track_point_pos (points_count-1);

   if (points_count > 2) {

      first_shape = 1;
      last_shape = points_count - 2;
   }

   roadmap_screen_draw_one_line
               (from, to, 0, from, first_shape, last_shape,
                editor_track_shape_position, pen);

   return 1;
}

