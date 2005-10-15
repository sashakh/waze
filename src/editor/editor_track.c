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

#include "../roadmap.h"
#include "../roadmap_math.h"
#include "../roadmap_gps.h"
#include "../roadmap_fuzzy.h"
#include "../roadmap_adjust.h"
#include "../roadmap_layer.h"
#include "../roadmap_locator.h"
#include "../roadmap_line.h"
#include "../roadmap_point.h"
#include "../roadmap_screen.h"
#include "../roadmap_navigate.h"

#include "editor_main.h"
#include "editor_db.h"
#include "editor_point.h"
#include "editor_line.h"
#include "editor_shape.h"
#include "editor_street.h"
#include "editor_route.h"
#include "editor_log.h"
#include "editor_trkseg.h"
#include "editor_override.h"
#include "editor_track.h"

#define FOCUS_RANGE 1000
#define GPS_POINTS_DISTANCE 10
#define MAX_POINTS_IN_SEGMENT 10000
#define MAX_GPS_TIME_GAP 600

#define SPLIT_START  0x1
#define SPLIT_END    0x2
#define SPLIT_CHECK  0x4

typedef struct {

   RoadMapGpsPosition gps_point;
   int time;

} TrackPoint;

static TrackPoint TrackPoints[MAX_POINTS_IN_SEGMENT];
static int points_count = 0;
static RoadMapGpsPosition RoadMapLatestGpsPosition;
static RoadMapPosition RoadMapLatestPosition;
static int RoadMapLastGpsTime = -1;
static int cur_active_line = 0;


typedef struct {

    int valid;
    int street;
    int plugin_id;

    int direction;

    RoadMapFuzzy fuzzyfied;

    int intersection;

    RoadMapPosition entry;

} RoadMapTracking;

typedef struct {

   int id;
   int plugin_id;

} NodeNeighbour;

#define ROADMAP_TRACKING_NULL  {0, -1, 0, 0, 0, 0, {0, 0}};

#define ROADMAP_NEIGHBOURHOUD  16

static RoadMapTracking  RoadMapConfirmedStreet = ROADMAP_TRACKING_NULL;
static RoadMapNeighbour PreviousConfirmedLine = ROADMAP_NEIGHBOUR_NULL;

static RoadMapNeighbour RoadMapConfirmedLine = ROADMAP_NEIGHBOUR_NULL;
static RoadMapNeighbour RoadMapNeighbourhood[ROADMAP_NEIGHBOURHOUD];
static NodeNeighbour cur_node = {-1, -1};

static __inline RoadMapPosition* track_point_pos (int index) {

   return (RoadMapPosition*) &TrackPoints[index].gps_point;
}

static __inline int track_point_time (int index) {

   return TrackPoints[index].time;
}

static int editor_track_add_point(RoadMapGpsPosition *point, int time) {

   if (points_count == MAX_POINTS_IN_SEGMENT) return -1;

   TrackPoints[points_count].gps_point = *point;
   TrackPoints[points_count].time = time;

   return points_count++;
}


static int editor_track_create_trkseg
            (int first_point, int last_point, int flags) {

   int trk_from;
   int trk_to;
   int first_shape = -1;
   int last_shape = -1;
   int i;
   int gps_time = track_point_time (first_point);
   RoadMapPosition *pos = track_point_pos (first_point);

   trk_from = editor_point_add (pos, 0, 0);
   trk_to = editor_point_add (track_point_pos (last_point), 0, 0);

   for (i=first_point+1; i<=(last_point-1); i++) {

      RoadMapPosition *shape_pos = track_point_pos (i);
      last_shape = editor_shape_add
                     (shape_pos->longitude - pos->longitude,
                      shape_pos->latitude - pos->latitude,
                      track_point_time (i) - gps_time);

      if (last_shape == -1) {
         editor_log (ROADMAP_ERROR, "Can't add shape point.");
         editor_log_pop ();
         return -1;
      }
      
      if (first_shape == -1) {
         first_shape = last_shape;
      }

      pos = shape_pos;
      gps_time = track_point_time (i);
   }

   return
      editor_trkseg_add (trk_from,
                         trk_to,
                         first_shape,
                         last_shape,
                         track_point_time (first_point),
                         track_point_time (last_point),
                         flags);
}


void editor_track_add_trkseg (RoadMapNeighbour *line,
                              int trkseg,
                              int direction,
                              int who) {
   
   int first;
   int last;
   int route;

   if (direction != 0) {

      short from_flags;
      short to_flags;
      
      if (line->plugin_id != EditorPluginID) {
         //TODO get roadmap route data
         route = editor_override_line_get_route (line->line);
      } else {
         route = editor_line_get_route (line->line);
      }

      if (route == -1) {
         route = editor_route_segment_add (0, 0);
         from_flags = to_flags = 0;
      } else {
         editor_route_segment_get (route, &from_flags, &to_flags);
      }

      if (direction == 1) {
         from_flags |= who;
      } else if (direction == 2) {
         to_flags |= who;
      } else {
         assert (0);
      }
      
      if (route != -1) {
         editor_route_segment_set (route, from_flags, to_flags);
      }
   }

   if (line->plugin_id != EditorPluginID) {

      editor_override_line_get_trksegs (line->line, &first, &last);
      
      if (first == -1) {
         first = last = trkseg;
      } else {
         editor_trkseg_connect (last, trkseg);
         last = trkseg;
      }
      editor_override_line_set_trksegs (line->line, first, last);

   } else {

      editor_line_get_trksegs (line->line, &first, &last);
      
      if (first == -1) {
         first = last = trkseg;
      } else {
         editor_trkseg_connect (last, trkseg);
         last = trkseg;
      }

      editor_line_set_trksegs (line->line, first, last);
   }
}


static void editor_track_get_line_points (RoadMapNeighbour *line,
                                          RoadMapPosition *from_position,
                                          RoadMapPosition *to_position) {

   if (line->plugin_id != EditorPluginID) {

      assert (roadmap_locator_activate (line->fips) != -1);
      roadmap_line_from (line->line, from_position);
      roadmap_line_to (line->line, to_position);
   } else {

      assert (editor_db_activate (line->fips) != -1);
      editor_line_get (line->line,
            from_position, to_position, NULL, NULL, NULL);
   }
}


static void editor_track_get_node_pos (NodeNeighbour *node,
                                RoadMapPosition *position) {

   if (node->plugin_id != EditorPluginID) {

      roadmap_point_position (node->id, position);
   } else {

      editor_point_position (node->id, position);
   }
}


static void editor_track_set_focus(const RoadMapPosition *position) {

   RoadMapArea focus;

   focus.west = position->longitude - FOCUS_RANGE;
   focus.east = position->longitude + FOCUS_RANGE; 
   focus.south = position->latitude - FOCUS_RANGE;
   focus.north = position->latitude + FOCUS_RANGE;
    
   roadmap_math_set_focus (&focus);
}


static int editor_track_get_distance (RoadMapPosition  *point,
                                       RoadMapNeighbour *line,
                                       RoadMapNeighbour *result) {

   int res;
   editor_track_set_focus(point);

   if (line->plugin_id != EditorPluginID) {

      roadmap_locator_activate (line->fips);

      res = roadmap_street_get_distance
                     (point,
                      line->line,
                      line->cfcc,
                      result);
   } else {

      editor_db_activate (line->fips);

      res = editor_street_get_distance
                     (point,
                      line->line,
                      line->fips,
                      result);
   }
      
   roadmap_math_release_focus();

   return res;
}


static int editor_track_fuzzify
                (RoadMapTracking *tracked,
                 RoadMapNeighbour *line,
                 const RoadMapPosition *position,
                 int heading,
                 int speed) {

    RoadMapFuzzy fuzzyfied_distance;
    RoadMapFuzzy fuzzyfied_direction;
    int line_direction = 0;
    int symetric = 0;

    if ((speed < 15) && (line->distance > roadmap_fuzzy_max_distance ()/2)) {
       return roadmap_fuzzy_false ();
    }

    if (editor_db_activate (line->fips) != -1) {

       int route;
       //TODO get route from roadmap
       if (line->plugin_id != EditorPluginID) {
          route = editor_override_line_get_route (line->line);
       } else {
          route = editor_line_get_route (line->line);
       }

       if (route != -1) {
          line_direction = editor_route_get_direction (route, ED_ROUTE_CAR);
       }
    }

    fuzzyfied_distance = roadmap_fuzzy_distance(line->distance);

    if (! roadmap_fuzzy_is_acceptable(fuzzyfied_distance)) {
       return roadmap_fuzzy_false ();
    }

    if ((line_direction == 0) || (line_direction == 3)) {
       symetric = 1;
    }

    if (line_direction <= 1) {
       tracked->direction = roadmap_math_azymuth (&line->from, &line->to);
    } else {
       tracked->direction = roadmap_math_azymuth (&line->to, &line->from);
    }

    fuzzyfied_direction = roadmap_fuzzy_direction (tracked->direction,
          heading, symetric);

    if (! roadmap_fuzzy_is_acceptable (fuzzyfied_direction)) {

       return roadmap_fuzzy_false ();

    }

    return roadmap_fuzzy_and (fuzzyfied_distance, fuzzyfied_direction);
}


static int editor_track_find_street
                     (const RoadMapGpsPosition *gps_position,
                      RoadMapTracking *nominated,
                      int *found,
                      RoadMapFuzzy *best) {

   int count;
   int layers[128];
   RoadMapTracking candidate;
   int result;
   int i;

   editor_track_set_focus((RoadMapPosition*)gps_position);

   count = roadmap_layer_all_roads (layers, 128);

   count = roadmap_street_get_closest
           (&RoadMapLatestPosition, layers, count,
             RoadMapNeighbourhood, ROADMAP_NEIGHBOURHOUD);

    roadmap_math_release_focus ();

   
   for (i = 0, *best = roadmap_fuzzy_false(), *found = 0; i < count; ++i) {

      result = editor_track_fuzzify
                 (&candidate, RoadMapNeighbourhood+i,
                  (RoadMapPosition*)gps_position, gps_position->steering,
                  gps_position->speed);
      
      if (result > *best) {
         *found = i;
         *best = result;
         *nominated = candidate;
      }
    }

    return count;
}

static int editor_track_length (int first, int last) {

   int length = 0;
   int i;

   for (i=first; i<last; i++) {

      length +=
         roadmap_math_distance
            (track_point_pos (i), track_point_pos (i+1));
   }

   return length;
}

static int editor_track_end_known_segment (
                  RoadMapNeighbour *previous_line,
                  int last_point_id,
                  RoadMapNeighbour *line) {
   //TODO: add stuff
   //Notice that previous_line may not be valid (only at first)

   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition *current;
   int trkseg;
   int line_length;
   int segment_length;
   int percentage;

   editor_log_push ("editor_track_end_known_segment");

   if (editor_db_activate (line->fips) == -1) {
      editor_db_create (line->fips);
      if (editor_db_activate (line->fips) == -1) {
         editor_log_pop ();
         return 0;
      }
   }

   if (line->plugin_id != EditorPluginID) {

      assert (roadmap_locator_activate (line->fips) != -1);
      roadmap_line_from (line->line, &from);
      roadmap_line_to (line->line, &to);
      line_length = roadmap_line_length (line->line);
   } else {

      editor_line_get (line->line,
            &from, &to, NULL, NULL, NULL);
      line_length = editor_line_length (line->line);
   }

   segment_length = editor_track_length (0, last_point_id);

   editor_log (ROADMAP_INFO, "Ending line %d (plugin_id:%d). Line length:%d, Segment length:%d",
         line->line, line->plugin_id, line_length, segment_length);

   /* End current segment if we really passed through it
    * and not only touched a part of it.
    */

   assert (line_length > 0);

   if (line_length == 0) {
      editor_log (ROADMAP_ERROR, "line %d (plugin_id:%d) has length of zero.",
            line->line, line->plugin_id);
      editor_log_pop ();
      return 0;
   }

   percentage = 100 * segment_length / line_length;
   if (percentage < 80) {
      editor_log (ROADMAP_INFO, "segment is too small to consider: %d%%",
            percentage);
      trkseg = editor_track_create_trkseg (0, last_point_id, ED_TRKSEG_IGNORE);
      editor_track_add_trkseg (line, trkseg, 0, ED_ROUTE_CAR);
      editor_log_pop ();
      if (segment_length > (GPS_POINTS_DISTANCE*1.5)) {
         return 1;
      } else {
         return 0;
      }
   }

   current = track_point_pos (last_point_id);
   trkseg = editor_track_create_trkseg (0, last_point_id, 0);

   if (roadmap_math_distance (current, &to) >
       roadmap_math_distance (current, &from)) {
      
      editor_log (ROADMAP_INFO, "Updating route direction: to -> from");
      editor_track_add_trkseg (line, trkseg, 2, ED_ROUTE_CAR);
   } else {

      editor_log (ROADMAP_INFO, "Updating route direction: from -> to");
      editor_track_add_trkseg (line, trkseg, 1, ED_ROUTE_CAR);
   }


   editor_log_pop ();

   return 1;
}


static void editor_track_reset_points (int last_point_id) {

   if (last_point_id == 0) return;
   
   if (last_point_id == -1) {
      points_count = 0;
      return;
   }

   points_count -= last_point_id;

   memmove (TrackPoints, TrackPoints + last_point_id,
         sizeof(TrackPoints[0]) * points_count);
}


/* Find a good point on the given line to start a new road:
 * Check if either end points of the line are good candidates.
 * Check recent GPS points and see if there's a good candidate.
 */
static int
editor_track_find_split_point (RoadMapNeighbour *line,
                               const RoadMapPosition *point,
                               int split_type,
                               int max_distance_allowed,
                               NodeNeighbour *connect_point) {
#define MAX_RECENT_POINTS 50

   RoadMapPosition from_pos;
   RoadMapPosition to_pos;
   RoadMapPosition split_pos;
   RoadMapNeighbour result;
   int start_point_id = -1;
   int distance;
   int min_distance;

   editor_log_push ("editor_track_find_split_point");

   editor_log
      (ROADMAP_INFO,
       "find split point for line:%d (plugin_id:%d). split_type:%d",
       line->line, line->plugin_id, split_type);

   connect_point->id = -1;
         
   max_distance_allowed *= GPS_POINTS_DISTANCE;

   editor_track_get_line_points (line, &from_pos, &to_pos);

   min_distance =
      roadmap_math_distance (point, &from_pos);
   split_pos = from_pos;
      
   distance =
      roadmap_math_distance (point, &to_pos);
   
   if (distance < min_distance) {
      min_distance = distance;
      split_pos = to_pos;
   }

   if (min_distance > max_distance_allowed) {
      /* End points are not good. Check recent gps points. */
      int i;
      RoadMapTracking candidate;
      RoadMapFuzzy current_fuzzy;
      RoadMapFuzzy best = roadmap_fuzzy_false ();

      for (i=points_count-1;
            (i > (points_count-MAX_RECENT_POINTS)) && (i > 0);
            i--) {

         RoadMapGpsPosition *current_gps_point = &TrackPoints[i].gps_point;
         RoadMapPosition *current_pos = track_point_pos (i);

         if (!editor_track_get_distance (current_pos,
                                         line,
                                         &result)) {
            continue;
         }

         current_fuzzy = editor_track_fuzzify
                         (&candidate, &result,
                          current_pos,
                          current_gps_point->steering,
                          current_gps_point->speed);

         if (start_point_id == -1) {
            start_point_id = i;
            split_pos = result.intersection;
            best = current_fuzzy;

         }
         
         if (split_type & SPLIT_START) {
            /* when looking for a start point, find the first point
             * which seems ok.
             */

            if ((current_fuzzy <= best) && roadmap_fuzzy_is_certain (best)) {
               break;
            }

            if (current_fuzzy > best) {
               start_point_id = i;
               split_pos = result.intersection;
               best = current_fuzzy;
            }

            if (roadmap_fuzzy_is_certain (current_fuzzy)) {

               if (result.distance < GPS_POINTS_DISTANCE) {
                  break;
               }
            }

         } else if (split_type & SPLIT_END) {

            /* when looking for an end point, go backward as
             * much as possible.
             */
            if (roadmap_fuzzy_is_acceptable (current_fuzzy)) {
               start_point_id = i;
               split_pos = result.intersection;
            }

            if (!roadmap_fuzzy_is_acceptable (current_fuzzy)) {
               start_point_id = i;
               split_pos = result.intersection;
               break;
            }
         } else {
            assert (0);
         }
      }

#if 0      
      if (find_start_point && (start_point_id != (points_count-1))) {
         
         RoadMapPosition intersection;
         int res =
            roadmap_math_intersection
               ((RoadMapPosition *)&GpsPoints[start_point_id],
                (RoadMapPosition *)&GpsPoints[start_point_id+1],
                &result.from,
                &result.to,
                &intersection);
         
         if (res &&
               (roadmap_math_distance (&split_pos, &intersection) <
                  GPS_POINTS_DISTANCE*50)) {
            split_pos = intersection;
         }
            
      }
#endif

      /* check end points again */
      min_distance =
         roadmap_math_distance (&split_pos, &from_pos);

      if (min_distance < max_distance_allowed) {
         split_pos = from_pos;
      }
      
      distance =
         roadmap_math_distance (&split_pos, &to_pos);
   
      if ((distance < min_distance) && (distance < max_distance_allowed)) {
         min_distance = distance;
         split_pos = to_pos;
      }

      if (min_distance >= max_distance_allowed) {
         editor_log
            (ROADMAP_INFO,
             "End points are not good enough. Need to split. Gone back %d points to find a good position.",
             points_count - start_point_id);

         if (split_type & SPLIT_CHECK) {
            editor_log_pop ();
            return -1;
         }
      
         if (editor_line_split
                     (line,
                     (split_type & SPLIT_START ?
                      track_point_pos (0) :
                      track_point_pos (points_count-1)),
                     &split_pos,
                     &connect_point->id) == -1) {

            editor_log
               (ROADMAP_ERROR, "Can't split line.");
            editor_log_pop ();

            return -1;
         }

         connect_point->plugin_id = EditorPluginID;
      }
   }

   if (connect_point->id == -1) {

      int from, to;

      if (line->plugin_id != EditorPluginID) {
         roadmap_line_points (line->line, &from, &to);
      } else {
         editor_line_get_points (line->line, &from, &to);
      }

      if (!roadmap_math_compare_points (&split_pos, &from_pos)) {
         connect_point->id = from;
      } else {
         assert (!roadmap_math_compare_points (&split_pos, &to_pos));
         connect_point->id = to;
      }

      connect_point->plugin_id = EditorPluginID;
   }

   if (start_point_id == -1) {

      int min_distance = 1000000;
      int distance;
      int i;

      for (i=points_count-1;
            (i > (points_count-MAX_RECENT_POINTS)) && (i > 0);
            i--) {

         distance = roadmap_math_distance (track_point_pos (i),
                                           &split_pos);
         if (distance > min_distance) break;
         min_distance = distance;
         start_point_id = i;
      }
   }

   editor_log_pop ();
   return start_point_id;
}


static int editor_track_connect_roads (RoadMapNeighbour *from,
                                       RoadMapNeighbour *to,
                                       const RoadMapGpsPosition *gps_position) {

   RoadMapPosition from_position;
   RoadMapPosition to_position;
   RoadMapPosition split_position;
   RoadMapPosition position = *(RoadMapPosition *) gps_position;
   RoadMapNeighbour result;
   int from_point;
   int to_point;
   int distance;
   int min_distance;
   int split_point;
   NodeNeighbour connect_node;
   int gps_connect_point;
   int split_type = SPLIT_END;

   editor_log_push ("editor_track_connect_roads");

   editor_log
      (ROADMAP_INFO,
       "from:%d (plugin_id:%d), to:%d (plugin_id:%d), pos - lon:%d, lat:%d",
       from->line, from->plugin_id, to->line, to->plugin_id,
       gps_position->longitude, gps_position->latitude);

   //TODO: need to handle a merge of two junctions (each with two roads or more)
   gps_connect_point =
      editor_track_find_split_point
         (from, &position, SPLIT_START|SPLIT_CHECK, 3, &connect_node);

   if (gps_connect_point == -1) {
      gps_connect_point =
         editor_track_find_split_point
            (to, &position, SPLIT_END|SPLIT_CHECK, 3, &connect_node);

      if (gps_connect_point != -1) {
         RoadMapNeighbour *tmp = from;
         from = to;
         to = tmp;
         split_type = SPLIT_START;
      } else {
         gps_connect_point =
            editor_track_find_split_point
            (from, &position, SPLIT_START, 3, &connect_node);
      }
   }

   if (gps_connect_point == -1) {
      assert (0);
      return -1;
   }

   editor_track_get_node_pos (&connect_node, &position);

   editor_track_get_line_points(to, &from_position, &to_position);

   if ((roadmap_math_distance (&position, &from_position) == 0) ||
        (roadmap_math_distance (&position, &to_position) == 0)) {

      editor_log (ROADMAP_INFO, "Both lines share the same node. Done.");
      editor_log_pop ();
      return gps_connect_point;
   }

   min_distance = roadmap_math_distance (&position, &from_position);
   split_position = from_position;
   distance = roadmap_math_distance (&position, &to_position);
   if (distance < min_distance) {
      min_distance = distance;
      split_position = to_position;
   }

   if (min_distance < (GPS_POINTS_DISTANCE*5)) {

      int found;
      int count;
      int i;
      RoadMapFuzzy best;
      RoadMapTracking nominated;
      RoadMapPosition cline_from;
      RoadMapPosition cline_to;
   
      /* check if we skipped some small segment which connects both segments */
      count = editor_track_find_street (gps_position, &nominated, &found, &best);
      found = -1;

      for (i=0; i<count; i++) {

         if ((RoadMapNeighbourhood[i].line == from->line) ||
             (RoadMapNeighbourhood[i].line == to->line)) continue;
         
         editor_track_get_line_points
            (&RoadMapNeighbourhood[i], &cline_from, &cline_to);

         if ((!roadmap_math_compare_points (&cline_from, &position) &&
               !roadmap_math_compare_points (&cline_to, &split_position)) ||
             (!roadmap_math_compare_points (&cline_to, &position) &&
               !roadmap_math_compare_points (&cline_from, &split_position))) {
            found = i;
            break;
         }
      }
         
      //TODO set route information for skipped line
      if (found != -1) {

         editor_log_pop ();
         return editor_track_find_split_point
                  (to, &position, split_type, 3, &connect_node);
      }
      
      editor_log
         (ROADMAP_INFO,
          "Lines are close enough to share a node. Need to merge nodes.");

      //TODO implement
      //assert (0);

      editor_log_pop ();
      return gps_connect_point;
   }

   /* No connection. We need to split the TO line and make sure
    * that the split point is the end point of the FROM line
    */

   if (from->plugin_id != EditorPluginID) {
      from->line = editor_line_copy (from->line, from->cfcc, from->fips);

      if (from->line == -1) {
         return -1;
      }
      from->plugin_id = EditorPluginID;
   }

   editor_line_get_points (from->line, &from_point, &to_point);

   editor_point_position (from_point, &from_position);
   editor_point_position (to_point, &to_position);

   min_distance =
      roadmap_math_distance (&position, &from_position);
   split_point = from_point;
   split_position = from_position;
      
   distance =
      roadmap_math_distance (&position, &to_position);
   
   if (distance < min_distance) {
      min_distance = distance;
      split_point = to_point;
      split_position = to_position;
   }

   editor_track_set_focus(&split_position);

   if (!editor_street_get_distance
                     (&split_position,
                      to->line,
                      to->fips,
                     &result)) {
      assert (0);
      
      roadmap_math_release_focus();
      return gps_connect_point;
   }

   roadmap_math_release_focus();

   if (result.distance < 3*GPS_POINTS_DISTANCE) {

      editor_log
         (ROADMAP_INFO,
          "Need to split TO line. It is close enough to use the split position as its node");

      split_position.longitude = result.intersection.longitude;
//      split_position.longitude /= 2;
      split_position.latitude = result.intersection.latitude;
//      split_position.latitude /= 2;

      //TODO: make sure that the point is not shared
      editor_point_set_pos (split_point, &split_position);
      
      if (editor_line_split
                  (to,
                  (split_type & SPLIT_START ?
                   track_point_pos (0) :
                   track_point_pos (points_count-1)),
                  &split_position,
                  &split_point) == -1) {
         gps_connect_point = -1;
      }
      
      editor_log_pop ();
      return gps_connect_point;
   } else {

      split_point = -1;
      editor_log (ROADMAP_INFO, "Need to split TO line and connect it to position with a new line");

      if (editor_line_split
                  (to,
                  (split_type & SPLIT_START ?
                   track_point_pos (0) :
                   track_point_pos (points_count-1)),
                  &result.intersection,
                  &split_point) == -1) {

         gps_connect_point = -1;
      }

      assert(0);
      if (gps_connect_point == -1) return -1;

      //TODO: create a new line between the nodes (note that from & to may have swapped)

      editor_log_pop ();

      return gps_connect_point;
   }
}


static int editor_track_create_line (int fips,
                                     int gps_first_point,
                                     int gps_last_point,
                                     int from_point,
                                     int to_point) {

   int p_from, p_to;
   RoadMapPosition start_pos;
   int route;
   int trkseg;
   int i;

   assert (gps_first_point != gps_last_point);

   editor_log_push ("editor_track_create_line");

   if (gps_first_point == gps_last_point) {
      editor_log (ROADMAP_ERROR, "first point == gps_last_point");
      editor_log_pop ();
      return -1;
   }

   if (editor_db_activate (fips) == -1) {
      editor_db_create (fips);
      if (editor_db_activate (fips) == -1) {
         editor_log_pop ();
         return -1;
      }
   }

   if (cur_node.id != -1) {
      editor_track_get_node_pos (&cur_node, &start_pos);
   }

   if (from_point != -1) {
      p_from = from_point;
      
   } else if (cur_node.id != -1) {

      if (cur_node.plugin_id != EditorPluginID) {
         cur_node.id = editor_point_roadmap_to_editor (cur_node.id);

         if (cur_node.id == -1) {
            editor_log_pop ();
            return -1;
         }
         cur_node.plugin_id = EditorPluginID;
      }

      p_from = cur_node.id;

   } else {
      p_from = editor_point_add
         (track_point_pos (gps_first_point), 0, 0);

      if (p_from == -1) {
         editor_log_pop ();
         return -1;
      }
   }

   editor_point_position (p_from, &start_pos);

   if (to_point != -1) {
      p_to = to_point;
   } else {

      if (!roadmap_math_compare_points
            (&start_pos, track_point_pos (gps_last_point))) {
         p_to = p_from;

      } else {
         p_to =
            editor_point_add
               (track_point_pos (gps_last_point), 0, 0);

         if (p_to == -1) {
            editor_log_pop ();
            return -1;
         }
      }
   }

   cur_node.id = p_to;
   cur_node.plugin_id = EditorPluginID;


   trkseg =
      editor_track_create_trkseg
         (gps_first_point, gps_last_point, 0);

   if (trkseg == -1) {
      editor_log (ROADMAP_ERROR, "Can't create new trkseg.");
      editor_log_pop ();
      return -1;
   }

   //TODO: use avg speed to guess a good cfcc
   i = editor_line_add (p_from, p_to, trkseg, 4, 0);

   if (i == -1) {
      editor_log (ROADMAP_ERROR, "Can't create new line.");
      editor_log_pop ();
      return -1;
   }

   route = editor_route_segment_add (ED_ROUTE_CAR, 0);
   editor_line_set_route (i, route);

   roadmap_screen_refresh ();
   editor_log_pop ();
   return i;
}


typedef struct angle_pair_s {
   int index;
   int angle;
} angle_pair;


/* we want a descending list so this function inverts the real comparison */
static int angle_pair_cmp (const void *p1, const void *p2) {

   const angle_pair *pair1 = (const angle_pair *)p1;
   const angle_pair *pair2 = (const angle_pair *)p2;

   if (pair1->angle > pair2->angle) return -1;
   else if (pair1->angle < pair2->angle) return 1;
   else return 0;
}


/* Detect turns */
static void editor_track_check_line_break (int last_point,
                                    int last_azymuth,
                                    int current_point) {

#define MAX_TURN_POINTS 50

   angle_pair pairs[MAX_TURN_POINTS];
   int i;
   int j;
   int max_azymuth_diff = 0;
   int max_point_azymuth = 0;
   int max_azymuth_point = -1;
   int current_azymuth;
   
   if ((current_point - last_point + 1) > MAX_TURN_POINTS) return;

   j=0;
   for (i=last_point; i<=current_point; i++) {

      int diff;
      int prev_azymuth;

      current_azymuth = 
         roadmap_math_azymuth
            (track_point_pos (i),
             track_point_pos (i+1));

      diff = roadmap_delta_direction (last_azymuth, current_azymuth);

      if (diff > max_azymuth_diff) {
         max_azymuth_diff = diff;
      }

      prev_azymuth = 
         roadmap_math_azymuth
            (track_point_pos (i-1),
             track_point_pos (i));

      diff = roadmap_delta_direction (prev_azymuth, current_azymuth);
      pairs[j].index = i;
      pairs[j].angle = diff;
      j++;

      if (diff > max_point_azymuth) {
         max_point_azymuth = diff;
         max_azymuth_point = i;
      }
   }

   qsort (pairs, j, sizeof(angle_pair), angle_pair_cmp);

   if (max_azymuth_diff > 35) {

      int angle_sum = pairs[0].angle;
      int middle1 = pairs[0].index;
      int middle1_angle = pairs[0].angle;
      int middle2 = pairs[0].index;
      int middle2_angle = pairs[0].angle;

      j=1;

      while ((100 * angle_sum / max_azymuth_diff) < 70) {
         
         assert (j<(current_point-last_point+1));
         angle_sum += pairs[j].angle;
         if (pairs[j].index < middle1) {
            middle1 = pairs[j].index;
            middle1_angle = pairs[j].angle;
         } else if (pairs[j].index > middle2) {
            middle2 = pairs[j].index;
            middle2_angle = pairs[j].angle;
         }
         j++;
      }

      if ((middle1 != middle2) &&
            (roadmap_math_distance
               (track_point_pos (middle1),
                track_point_pos (middle2)) < (GPS_POINTS_DISTANCE*5))) {
         middle1 = middle2 = pairs[0].index;
      }
      
      if (middle1 > 0) {
         editor_track_create_line
            (roadmap_locator_active (), 0, middle1, -1, -1);
      }

      if (middle1 != middle2) {
         /* make the curve a segment of its own */
         editor_track_create_line
            (roadmap_locator_active (), middle1, middle2, -1, -1);
      }

      editor_track_reset_points (middle2);
   }
}


/* Detect roundabouts */
int editor_track_check_roundabout (int first_point, int last_point) {

   //RoadMapPosition middle;
   int length;
   int line_id;
   //int i;

   length = editor_track_length (first_point, last_point);

   if ((length < GPS_POINTS_DISTANCE*3) || (length > GPS_POINTS_DISTANCE*20)) {

      return -1;
   }

#if 0
   middle.longitude = middle.latitude = 0;
   for (i=first_point; i<last_point; i++) {

      middle.longitude += GpsPoints[i].longitude;
      middle.latitude += GpsPoints[i].latitude;
   }

   middle.longitude /= (last_point - first_point);
   middle.latitude /= (last_point - first_point);
    
   /* Fix the end of the segment before the roundabout */

   GpsPoints[first_point-1].longitude = middle.longitude;
   GpsPoints[first_point-1].latitude = middle.latitude;
#endif

   line_id =
      editor_track_create_line
         (roadmap_locator_active (), 0, first_point-1, -1, -1);

   if (line_id == -1) {
      return -1;
   }

   //TODO: add the actual rouadabout as a layer different than road
   //set node attribute as roundabout
   
   editor_track_create_line
      (roadmap_locator_active (), first_point, last_point-1, -1, cur_node.id);
      
   PreviousConfirmedLine.line = line_id;
   PreviousConfirmedLine.plugin_id = 1;
   PreviousConfirmedLine.cfcc = roadmap_locator_active ();
   editor_track_reset_points (last_point);
   cur_active_line = 0;

   //GpsPoints[0].longitude = middle.longitude;
   //GpsPoints[0].latitude = middle.latitude;

   return line_id;
}

static void editor_track_unknown_point (int point_id, int force) {

   static int last_straight_line_point = -1;
   static int last_straight_azymuth = 0;

   if (!cur_active_line) {
      /* new line */
      cur_active_line = 1;
      last_straight_line_point = -1;
      return;
   }

   if (point_id == 0) return;

   if ((point_id < 3) || (point_id < last_straight_line_point)) {
      last_straight_line_point = point_id;
      last_straight_azymuth =
         roadmap_math_azymuth
            (track_point_pos (point_id-1),
             track_point_pos (point_id));
      return;
   }

   if (force) return;

   if (roadmap_delta_direction
         (roadmap_math_azymuth
             (track_point_pos (point_id - 1),
              &RoadMapLatestPosition),
          roadmap_math_azymuth
            (track_point_pos (point_id - 3),
             track_point_pos (point_id - 2))) < 10) {

      int this_straight_azymuth;

      /* this is a straight line, save it */

      this_straight_azymuth =
         roadmap_math_azymuth
             (track_point_pos (point_id - 3),
              &RoadMapLatestPosition);

      if (last_straight_line_point == -1) {
         last_straight_line_point = point_id;
         last_straight_azymuth = this_straight_azymuth;
         return;
      }

      editor_track_check_line_break
         (last_straight_line_point, last_straight_azymuth, point_id-3);

      last_straight_line_point = point_id;
      last_straight_azymuth = this_straight_azymuth;
   }
}

static void editor_track_locate_known_road (int point_id) {

   int found;
   int count;
   int current_fuzzy;
   RoadMapFuzzy best;
   
   const RoadMapGpsPosition *gps_position = &TrackPoints[point_id].gps_point;
   const RoadMapPosition *position = track_point_pos (point_id);
   
   static RoadMapTracking nominated;
   
   RoadMapFuzzy before;
   
   if (RoadMapConfirmedStreet.valid) {
      /* We have an existing street match: check if it is still valid. */
      
      before = RoadMapConfirmedStreet.fuzzyfied;
      
        
      if (!editor_track_get_distance (&RoadMapLatestPosition,
                                      &RoadMapConfirmedLine,
                                      &RoadMapConfirmedLine)) {
         current_fuzzy = 0;
      } else {

         current_fuzzy = editor_track_fuzzify
                           (&RoadMapConfirmedStreet, &RoadMapConfirmedLine,
                            position,
                            gps_position->steering,
                            gps_position->speed);
      }

      if ((current_fuzzy >= before) ||
            roadmap_fuzzy_is_certain(current_fuzzy)) {
         return; /* We are on the same street. */
      }
   }
   
    /* We must search again for the best street match. */
   
   count = editor_track_find_street (gps_position, &nominated, &found, &best);
#if 0
   if (count && (RoadMapNeighbourhood[found].distance < 30) &&
      !roadmap_fuzzy_is_acceptable (best)) {
      /* if we are on a known street, we don't want to start a new road
       * if we are still close to it. In this case we ignore the fuzzy
       */
   } else
#endif     

   if (!roadmap_fuzzy_is_good (best) && roadmap_fuzzy_is_acceptable (best)) {
      /* We're not sure that the new line is a real match.
       * Delay the decision if we're still close to the previous road.
       */
      if (RoadMapConfirmedLine.distance < (GPS_POINTS_DISTANCE*3)) {
         return;
      }

   } else if (roadmap_fuzzy_is_acceptable (best)) {

      if (RoadMapConfirmedStreet.valid &&
            (RoadMapConfirmedLine.line != RoadMapNeighbourhood[found].line)) {

         int split_point = editor_track_connect_roads
                              (&RoadMapConfirmedLine,
                               &RoadMapNeighbourhood[found],
                               &TrackPoints[point_id-1].gps_point);

            if (editor_track_end_known_segment
                  (&PreviousConfirmedLine, split_point,
                   &RoadMapConfirmedLine)) {

               PreviousConfirmedLine = RoadMapConfirmedLine;
            }

         editor_track_reset_points (split_point);
      }

      RoadMapConfirmedLine   = RoadMapNeighbourhood[found];
      RoadMapConfirmedStreet = nominated;
      
      RoadMapConfirmedStreet.valid = 1;
      RoadMapConfirmedStreet.plugin_id = RoadMapConfirmedLine.plugin_id;
      RoadMapConfirmedStreet.fuzzyfied = best;
      RoadMapConfirmedStreet.intersection = -1;
      
      RoadMapConfirmedStreet.street = RoadMapConfirmedLine.line;

   } else {

      if (RoadMapConfirmedStreet.valid) {

         int split_point;
         int i;

         /* we just lost a known road, let's find the best point
            to start this new road from */

         split_point =
            editor_track_find_split_point
                        (&RoadMapConfirmedLine,
                         track_point_pos (point_id),
                         SPLIT_START,
                         2, &cur_node);

         if (split_point != -1) {
            if (editor_track_end_known_segment
                  (&PreviousConfirmedLine, split_point,
                   &RoadMapConfirmedLine)) {

               PreviousConfirmedLine = RoadMapConfirmedLine;
            }
         }

         editor_track_reset_points (split_point);

         for (i=0; i<points_count; i++) {
            editor_track_unknown_point (i, 0);
         }
      } else {

         editor_track_reset_points (point_id);
         editor_track_unknown_point(0, 0);
      }
      RoadMapConfirmedStreet.valid = 0;
      RoadMapConfirmedLine.line = -1;
   }
}


/* Find if the current point matches a point on the current new line.
 * (Detect loops)
 */
static int editor_track_match_distance_from_current
                    (const RoadMapGpsPosition *gps_point,
                     int last_point,
                     RoadMapPosition *best_intersection,
                     int *match_point) {

   int i;
   int distance;
   int smallest_distance;
   int azymuth;
   RoadMapPosition intersection;

   smallest_distance = 0x7fffffff;

   for (i = 0; i < last_point; i++) {

      if (roadmap_math_line_is_visible
            (track_point_pos (i),
             track_point_pos (i+1))) {

         distance =
            roadmap_math_get_distance_from_segment
               ((RoadMapPosition *)gps_point,
                track_point_pos (i),
                track_point_pos (i+1),
                &intersection);

         if (distance < smallest_distance) {
            smallest_distance = distance;
            *best_intersection = intersection;
            *match_point = i;
            azymuth = roadmap_math_azymuth
                (track_point_pos (i),
                track_point_pos (i+1));
         }
      }
   }

   return (smallest_distance < GPS_POINTS_DISTANCE) &&
            (roadmap_delta_direction (azymuth, gps_point->steering) < 10);
}


static void track_rec_locate_unknown_road (int point_id) {

   int found;
   int count;
   
   RoadMapFuzzy best;
   
   const RoadMapGpsPosition *gps_position = &TrackPoints[point_id].gps_point;
   
   RoadMapTracking nominated;
   
   /* let's see if we got to a known street line */
   count = editor_track_find_street (gps_position, &nominated, &found, &best);

   if (roadmap_fuzzy_is_good (best) ||
         RoadMapConfirmedStreet.valid) {

      int split_point;
      NodeNeighbour node;
      /* we found an existing road, let's close the new road */
      
      /* delay ending the line until we find the best location to end it */
      if (roadmap_fuzzy_is_good (best) &&
               ( !RoadMapConfirmedStreet.valid ||
                 RoadMapNeighbourhood[found].distance <
                  RoadMapConfirmedLine.distance)) {

         RoadMapConfirmedLine   = RoadMapNeighbourhood[found];
         RoadMapConfirmedStreet = nominated;
         RoadMapConfirmedStreet.valid = 1;
         RoadMapConfirmedStreet.plugin_id = RoadMapConfirmedLine.plugin_id;
         RoadMapConfirmedStreet.fuzzyfied = best;
         RoadMapConfirmedStreet.intersection = -1;
         RoadMapConfirmedStreet.street = RoadMapConfirmedLine.line;

         if (RoadMapConfirmedLine.distance > 10) {
            editor_track_unknown_point(point_id, 1);

            return;
         }
      }

      //TODO: add intersection check to find_split_point
      split_point =
         editor_track_find_split_point
                        (&RoadMapConfirmedLine,
                        track_point_pos (point_id),
                        SPLIT_END,
                        2,
                        &node);
      if (split_point != -1) {
      //FIXME:
#if 0
      editor_track_end_unknown_segment
         (NULL,
          split_point,
          &RoadMapConfirmedLine);
#endif

         if (node.plugin_id != EditorPluginID) {
            node.id = editor_point_roadmap_to_editor (node.id);
            
            if (cur_node.id == -1) {
               editor_log
                  (ROADMAP_ERROR,
                   "track_rec_locate_unknown_road - can't create new road.");
               return;
            }
            node.plugin_id = EditorPluginID;
         }

         PreviousConfirmedLine.line =
                     editor_track_create_line
                                       (RoadMapConfirmedLine.fips,
                                        0,
                                        split_point,
                                        -1,
                                        node.id);

         cur_node = node;
      }

      PreviousConfirmedLine.plugin_id = 1;
      PreviousConfirmedLine.cfcc = RoadMapConfirmedLine.cfcc;
      editor_track_reset_points (split_point);
      cur_active_line = 0;
      
   } else {

      RoadMapPosition intersection;
      int match_point;

      RoadMapConfirmedStreet.valid = 0;
      RoadMapConfirmedLine.line = -1;

      /* see if we can match the position to the current line */
      if ((point_id > 4)  &&
            editor_track_match_distance_from_current
                  (&TrackPoints[point_id].gps_point,
                   point_id-3,
                   &intersection,
                   &match_point)) {

         //TODO roundabout - is this needed?
         //GpsPoints[point_id].longitude = intersection.longitude;
         //GpsPoints[point_id].latitude = intersection.latitude;

         if (editor_track_check_roundabout (match_point, point_id) == -1) {

            /* it's not a roundabout, so just close the new road */

            PreviousConfirmedLine.line =
               editor_track_create_line (roadmap_locator_active (), 0, point_id, -1, -1);

            PreviousConfirmedLine.plugin_id = 1;
            PreviousConfirmedLine.cfcc = RoadMapConfirmedLine.cfcc;
            editor_track_reset_points (point_id);
            cur_active_line = 0;
         }
      } else {
         editor_track_unknown_point(point_id, 0);
      }
   }
}


static void track_rec_locate(int gps_time,
                             const RoadMapGpsPrecision *dilution,
                             const RoadMapGpsPosition* gps_position) {

   static int first_point = 1;
   static RoadMapGpsPosition normalized_gps_point;
   static int last_azymuth = 0;
   int point_id;
   int interpolate_point = 1;
   int distance;
   int azymuth;
   
   //if (gps_position->speed < roadmap_gps_speed_accuracy()) return;
   if (gps_position->speed == 0) return;
   
   if (first_point) {
      first_point = 0;
      RoadMapLatestGpsPosition = *gps_position;
      RoadMapLastGpsTime = gps_time;
      normalized_gps_point = *gps_position;
      cur_node.id = -1;
      return;
   }

   if ((RoadMapLatestGpsPosition.latitude == gps_position->latitude) &&
       (RoadMapLatestGpsPosition.longitude == gps_position->longitude)) return;
   
   if ((roadmap_math_distance
            ( (RoadMapPosition *) &RoadMapLatestGpsPosition,
              (RoadMapPosition*) gps_position) >= 1000) ||
         (gps_time < RoadMapLastGpsTime) ||
         ((gps_time - RoadMapLastGpsTime) > MAX_GPS_TIME_GAP))
   {

      if (cur_active_line) {
         editor_track_create_line (roadmap_locator_active (), 0,
                                    points_count-1, -1, -1);
         cur_active_line = 0;
      }

      points_count = 0;

      RoadMapConfirmedStreet.valid = 0;
      PreviousConfirmedLine.line = -1;
      RoadMapConfirmedLine.line = -1;

      first_point = 1;

      track_rec_locate (gps_time, dilution, gps_position);
      return;
   }

   RoadMapLastGpsTime = gps_time;

   /* make sure that distance between GPS point is constant */

   if (interpolate_point) {
      normalized_gps_point.longitude =
         (normalized_gps_point.longitude +
          gps_position->longitude) / 2;
            
      normalized_gps_point.latitude =
         (normalized_gps_point.latitude +
         gps_position->latitude) / 2;

      normalized_gps_point.speed =
          gps_position->speed;

      normalized_gps_point.steering =
          gps_position->steering;
   } else {
      normalized_gps_point = *gps_position;
   }

   distance = roadmap_math_distance
               ((RoadMapPosition *) &RoadMapLatestGpsPosition,
                (RoadMapPosition *) &normalized_gps_point);
   
   if (distance < GPS_POINTS_DISTANCE) {

      interpolate_point = 1;
      return;
   }

   azymuth = roadmap_math_azymuth
               ((RoadMapPosition *) &normalized_gps_point,
                (RoadMapPosition *) &RoadMapLatestGpsPosition);

   if (roadmap_delta_direction (azymuth, last_azymuth) > 90) {

      RoadMapLatestGpsPosition = normalized_gps_point;
      last_azymuth = azymuth;
      return;
   }

   last_azymuth = azymuth;

   while (roadmap_math_distance
            ( (RoadMapPosition *) &RoadMapLatestGpsPosition,
              (RoadMapPosition*) &normalized_gps_point) >=
            (GPS_POINTS_DISTANCE)) {

      RoadMapGpsPosition interpolated_point = normalized_gps_point;

      while (roadmap_math_distance
            ( (RoadMapPosition*) &RoadMapLatestGpsPosition,
              (RoadMapPosition*) &interpolated_point ) >=
                                 (GPS_POINTS_DISTANCE*2)) {

            interpolated_point.longitude =
               (interpolated_point.longitude +
                RoadMapLatestGpsPosition.longitude) / 2;
            
            interpolated_point.latitude =
               (interpolated_point.latitude +
               RoadMapLatestGpsPosition.latitude) / 2;
      }

      RoadMapLatestGpsPosition = interpolated_point;
      
      point_id = editor_track_add_point(&RoadMapLatestGpsPosition, gps_time);

      if (point_id == -1) {
         //TODO: new segment
         assert(0);
      }

      roadmap_fuzzy_start_cycle ();
      
      roadmap_adjust_position
         (&RoadMapLatestGpsPosition, &RoadMapLatestPosition);
      
      if (!cur_active_line) {
         editor_track_locate_known_road(point_id);
      } else {
         track_rec_locate_unknown_road(point_id);
      }
   }
}


static void
editor_gps_update (int gps_time,
                   const RoadMapGpsPrecision *dilution,
                   const RoadMapGpsPosition *gps_position) {

   if (editor_is_enabled()) {
      track_rec_locate(gps_time, dilution, gps_position);
   }
}


void editor_track_initialize (void) {

   roadmap_gps_register_listener(editor_gps_update);
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

