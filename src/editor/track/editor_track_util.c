/* editor_track_util.c - misc functions used by editor track modules
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
 *   See editor_track_util.h
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_gps.h"
#include "roadmap_fuzzy.h"
#include "roadmap_layer.h"
#include "roadmap_locator.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_point.h"
#include "roadmap_navigate.h"

#include "../db/editor_db.h"
#include "../db/editor_point.h"
#include "../db/editor_line.h"
#include "../db/editor_shape.h"
#include "../db/editor_street.h"
#include "../db/editor_route.h"
#include "../db/editor_trkseg.h"
#include "../db/editor_override.h"
#include "../editor_main.h"
#include "../editor_log.h"

#include "editor_track_main.h"
#include "editor_track_util.h"

#define FOCUS_RANGE 10000

#define SPLIT_START  0x1
#define SPLIT_END    0x2
#define SPLIT_NODE   0x4
#define SPLIT_CHECK  0x8


void editor_track_util_set_focus(const RoadMapPosition *position) {

   RoadMapArea focus;

   focus.west = position->longitude - FOCUS_RANGE;
   focus.east = position->longitude + FOCUS_RANGE; 
   focus.south = position->latitude - FOCUS_RANGE;
   focus.north = position->latitude + FOCUS_RANGE;
    
   roadmap_math_set_focus (&focus);
}


void editor_track_util_release_focus() {

   roadmap_math_release_focus ();
}


/* Find a good point on the given line to start a new road:
 * Check if either end points of the line are good candidates.
 * Check recent GPS points and see if there's a good candidate.
 */
static int find_split_point (PluginLine *line,
                             const RoadMapPosition *point,
                             int last_point_id,
                             int split_type,
                             int max_distance_allowed,
                             int opposite_direction,
                             NodeNeighbour *connect_point) {
#define MAX_RECENT_POINTS 50

   RoadMapPosition from_pos;
   RoadMapPosition to_pos;
   RoadMapPosition split_pos;
   RoadMapPosition candidate_split_pos;
   RoadMapNeighbour result;
   int start_point_id = -1;
   int points_count = last_point_id + 1;
   int distance;
   int min_distance;

   editor_log_push ("find_split_point");

   editor_log
      (ROADMAP_INFO,
       "find split point for line:%d (plugin_id:%d). split_type:%d",
       roadmap_plugin_get_line_id (line), roadmap_plugin_get_id (line), split_type);

   max_distance_allowed *= editor_track_point_distance ();

   roadmap_plugin_line_from (line , &from_pos);
   roadmap_plugin_line_to (line , &to_pos);

   split_pos = candidate_split_pos = *point;

   min_distance =
      roadmap_math_distance (&candidate_split_pos, &from_pos);

   if (min_distance < max_distance_allowed) {
      split_pos = from_pos;
   }
      
   distance =
      roadmap_math_distance (&candidate_split_pos, &to_pos);
   
   if ((distance < min_distance) && (distance < max_distance_allowed)) {
      min_distance = distance;
      split_pos = to_pos;
   }

   if ((min_distance > max_distance_allowed) && !(split_type & SPLIT_NODE)) {
      /* End points are not good. Check recent gps points. */
      int i;
      RoadMapTracking candidate;
      RoadMapFuzzy current_fuzzy;
      RoadMapFuzzy best = roadmap_fuzzy_false ();
      int last_azymuth_diff = -1;

      for (i=points_count-1;
            (i > (points_count-MAX_RECENT_POINTS)) && (i > 0);
            i--) {

         RoadMapGpsPosition *current_gps_point = track_point_gps (i);
         RoadMapPosition *current_pos = track_point_pos (i);
         int steering = current_gps_point->steering;
         int current_azymuth_diff;

         if (!editor_track_util_get_distance (current_pos,
                                              line,
                                             &result)) {
            continue;
         }

         if (opposite_direction) {
            steering = steering - 180;
         }
            
         current_azymuth_diff =
            roadmap_math_delta_direction
               (steering, roadmap_math_azymuth (&result.from, &result.to));

         current_fuzzy = roadmap_navigate_fuzzify
                         (&candidate, NULL,
                          &result,
                          steering);

         if (start_point_id == -1) {
            start_point_id = i;
            split_pos = result.intersection;
            best = current_fuzzy;
            last_azymuth_diff = current_azymuth_diff;
         }
         
         if (split_type & SPLIT_START) {
            /* when looking for a start point, find the first point
             * which seems ok.
             */

            if ((current_fuzzy <= best) && roadmap_fuzzy_is_certain (best)) {
               break;
            }

            if (abs (last_azymuth_diff - current_azymuth_diff) > 10) {
               break;
            }

            last_azymuth_diff = current_azymuth_diff;

            if (current_fuzzy > best) {
               start_point_id = i;
               split_pos = result.intersection;
               best = current_fuzzy;
            }

            if (roadmap_fuzzy_is_certain (current_fuzzy)) {

               if (result.distance < editor_track_point_distance ()) {
                  break;
               }
            }

         } else if (split_type & SPLIT_END) {

            /* when looking for an end point, go backward as
             * much as possible.
             */

            if (abs (last_azymuth_diff - current_azymuth_diff) > 10) {
               break;
            }

            last_azymuth_diff = current_azymuth_diff;

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
                  editor_track_point_distance ()*50)) {
            split_pos = intersection;
         }
            
      }
#endif

      /* check end points again */
      candidate_split_pos = split_pos;
      min_distance =
         roadmap_math_distance (&candidate_split_pos, &from_pos);

      if (min_distance < max_distance_allowed) {
         split_pos = from_pos;
      }
      
      distance =
         roadmap_math_distance (&candidate_split_pos, &to_pos);
   
      if ((distance < min_distance) && (distance < max_distance_allowed)) {
         min_distance = distance;
         split_pos = to_pos;
      }
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

   if (connect_point->id == -1) {

      int from, to;
      int plugin_id = roadmap_plugin_get_id (line);

      if (plugin_id != EditorPluginID) {
         roadmap_line_points (roadmap_plugin_get_line_id (line), &from, &to);
      } else {
         editor_line_get_points (roadmap_plugin_get_line_id (line), &from, &to);
      }

      if (!roadmap_math_compare_points (&split_pos, &from_pos)) {
         connect_point->id = from;
      } else {
         assert (!roadmap_math_compare_points (&split_pos, &to_pos));
         connect_point->id = to;
      }

      connect_point->plugin_id = plugin_id;
   }

   if ((start_point_id == -1) ||
         roadmap_math_compare_points (&from_pos, &to_pos)) {

      int min_distance = 1000000;
      int distance;
      RoadMapPosition intersection;
      int i;

      for (i=points_count-1;
            (i > (points_count-MAX_RECENT_POINTS)) && (i > 0);
            i--) {
         
         distance =
            roadmap_math_get_distance_from_segment
                  (&split_pos,
                   track_point_pos (i),
                   track_point_pos (i-1),
                   &intersection);

         if (distance >= min_distance) break;
         min_distance = distance;
         start_point_id = i;
      }

      if (start_point_id > 1) {
         
         if (roadmap_math_distance
               (track_point_pos (start_point_id-1), &intersection) <=
               roadmap_math_distance
               (track_point_pos (start_point_id), &intersection)) {

            start_point_id--;
         }
      }
   }

   editor_log_pop ();
   return start_point_id;
}


static void editor_track_util_node_pos (NodeNeighbour *node,
                                        RoadMapPosition *position) {

   if (node->plugin_id != EditorPluginID) {

      roadmap_point_position (node->id, position);
   } else {

      editor_point_position (node->id, position);
   }
}



static int editor_track_util_same_node (NodeNeighbour *from_node,
                                        NodeNeighbour *to_node) {

   RoadMapPosition from_pos;
   RoadMapPosition to_pos;

   /* FIXME this does not really check if the nodes are the same.
    * It only checks if the location is equal.
    */
   editor_track_util_node_pos (from_node, &from_pos);
   editor_track_util_node_pos (to_node, &to_pos);

   if (!roadmap_math_compare_points (&from_pos, &to_pos)) return 1;
   else return 0;

}


static void adjust_connect_node (NodeNeighbour *node, PluginLine *line) {

   RoadMapPosition split_position;
   RoadMapNeighbour result;

   editor_track_util_node_pos (node, &split_position);

   //editor_track_util_set_focus(&split_position);
   if (!roadmap_plugin_get_distance (&split_position, line, &result)) {

      assert (0);
      
      //editor_track_util_release_focus ();
      node->id = -1;
      return;
   }
   //editor_track_util_release_focus ();

   assert (result.distance < 5*editor_track_point_distance ());

   split_position.longitude = result.intersection.longitude;
   split_position.latitude = result.intersection.latitude;

   if (node->plugin_id == ROADMAP_PLUGIN_ID) {
      node->id = editor_point_roadmap_to_editor (node->id);
      node->plugin_id = EditorPluginID;
   }

   //TODO: make sure that the point is not shared
   editor_point_set_pos (node->id, &split_position);
}


int editor_track_util_new_road_start (RoadMapNeighbour *line,
                                      const RoadMapPosition *pos,
                                      int points_count,
                                      int opposite_direction,
                                      NodeNeighbour *node) {

   node->id = -1;
   return find_split_point
            (&line->line, pos, points_count,
             SPLIT_START, 2, opposite_direction, node);
}


int editor_track_util_new_road_end (RoadMapNeighbour *line,
                                    const RoadMapPosition *pos,
                                    int points_count,
                                    int opposite_direction,
                                    NodeNeighbour *node) {

   node->id = -1;
   return find_split_point
            (&line->line, pos, points_count,
             SPLIT_END, 2, opposite_direction, node);
}


int editor_track_util_get_distance (const RoadMapPosition *point,
                                    const PluginLine *line,
                                    RoadMapNeighbour *result) {
   int res;

   //editor_track_util_set_focus (point);
   roadmap_plugin_activate_db (line);

   res = roadmap_plugin_get_distance (point, line, result);
      
   //editor_track_util_release_focus ();
   return res;
}


int editor_track_util_find_street
                     (const RoadMapGpsPosition *gps_position,
                      RoadMapTracking *nominated,
                      RoadMapTracking *previous_street,
                      RoadMapNeighbour *previous_line,
                      RoadMapNeighbour *neighbourhood,
                      int max,
                      int *found,
                      RoadMapFuzzy *best) {

   RoadMapTracking candidate;
   int count;
   int layers[128];
   int layer_count;
   int result;
   int i;

   if (!previous_street->valid) {
      previous_line = NULL;
   }

   //editor_track_util_set_focus((RoadMapPosition*)gps_position);

   layer_count = roadmap_layer_all_roads (layers, 128);

   count = roadmap_street_get_closest
           ((RoadMapPosition *)gps_position, layers, layer_count,
             neighbourhood, max);

   count = roadmap_plugin_get_closest
           ((RoadMapPosition *)gps_position, layers, layer_count,
             neighbourhood, count, max);

   //editor_track_util_release_focus ();

   
   for (i = 0, *best = roadmap_fuzzy_false(), *found = 0; i < count; ++i) {

      result = roadmap_navigate_fuzzify
                 (&candidate, previous_line, neighbourhood+i,
                  gps_position->steering);
      
      if (result > *best) {
         *found = i;
         *best = result;
         *nominated = candidate;
         nominated->opposite_street_direction = 0;
      }
    }

   if (roadmap_fuzzy_is_acceptable (*best) &&
         !previous_street->opposite_street_direction) return count;

   /* search for a line in the opposite direction */
   for (i = 0; i < count; ++i) {

      result = roadmap_navigate_fuzzify
                 (&candidate, previous_line, neighbourhood+i,
                  gps_position->steering - 180);
      
      if (!roadmap_fuzzy_is_good (result)) {
         RoadMapPosition connection;
         
         if (!previous_street->valid ||
               !previous_street->opposite_street_direction) {
            continue;
         }

         /* if this line is the current line, or connected to the
          * current line, allow it
          */
         if (!roadmap_plugin_same_line (&neighbourhood[i].line,
                                        &previous_line->line)) {

            if (roadmap_fuzzy_is_certain (
                  roadmap_fuzzy_connected
                     (&neighbourhood[i], previous_line,
                      candidate.line_direction,
                      &connection))) {
               
               if (roadmap_math_distance
                     (&connection, (RoadMapPosition *) gps_position) >=
                        3*editor_track_point_distance ()) { 

                  continue;
               }
               
            } else {

               continue;
            }
         }
      }

      if (result > *best) {
         *found = i;
         *best = result;
         *nominated = candidate;
         nominated->opposite_street_direction = 1;
      }
    }

    return count;
}


int editor_track_util_connect_roads (PluginLine *from,
                                     PluginLine *to,
                                     int from_opposite_direction,
                                     int to_opposite_direction,
                                     const RoadMapGpsPosition *gps_position,
                                     int last_point_id) {

   RoadMapPosition position = *(RoadMapPosition *) gps_position;
   NodeNeighbour from_node = NODE_NEIGHBOUR_NULL;
   NodeNeighbour to_node = NODE_NEIGHBOUR_NULL;
   NodeNeighbour connect_node = NODE_NEIGHBOUR_NULL;
   int from_point;
   int to_point;

   editor_log_push ("editor_track_connect_roads");

   editor_log
      (ROADMAP_INFO,
       "from:%d (plugin_id:%d), to:%d (plugin_id:%d), pos - lon:%d, lat:%d",
       from->line_id, from->plugin_id, to->line_id, to->plugin_id,
       gps_position->longitude, gps_position->latitude);

   //TODO: need to handle a merge of two junctions (each with two roads or more)

   from_point = find_split_point
                     (from, &position, last_point_id, SPLIT_START|SPLIT_CHECK,
                      3, from_opposite_direction, &from_node);
   to_point = find_split_point
                     (to, &position, last_point_id, SPLIT_END|SPLIT_CHECK,
                      3, to_opposite_direction, &to_node);

   if ((from_point != -1) && (to_point != -1)) {

      if (editor_track_util_same_node (&from_node, &to_node)) {

         editor_log (ROADMAP_INFO, "Both lines share the same node. Done.");
         editor_log_pop ();
         return (from_point + to_point) / 2;
         //return (from_point > to_point) ? from_point : to_point;
      }

      //TODO: need to handle a merge of two nodes (each with two roads or more)
      //Check if there's a small line connecting both nodes.
      editor_log_pop ();
      return (from_point + to_point) / 2;
   }

   if ((from_point == -1) && (to_point == -1)) {

      editor_log (ROADMAP_INFO, "Neither lines have a node. Create a node.");

      from_point = find_split_point
                   (from, &position, last_point_id, SPLIT_START,
                    3, from_opposite_direction, &from_node);

      if (from_point == -1) {
         assert (0);
         editor_log (ROADMAP_ERROR, "Can't create a connection point.");
         editor_log_pop ();
         return -1;
      }

     connect_node = from_node;
     adjust_connect_node (&connect_node, to);
     
   } else if (from_point != -1) {

     connect_node = from_node;
     adjust_connect_node (&connect_node, to);
   } else {

     connect_node = to_node;
     adjust_connect_node (&connect_node, from);
   }

   if (connect_node.id == -1) {
      editor_log (ROADMAP_ERROR, "Can't find a connection point.");
      editor_log_pop ();
      return -1;
   }

   editor_track_util_node_pos (&connect_node, &position);

   if (from_point == -1) {
      
      editor_log (ROADMAP_INFO, "'from' line has no node. Need to split.");
      
      from_point = find_split_point
                      (from, &position, last_point_id, SPLIT_START|SPLIT_NODE,
                       3, from_opposite_direction, &connect_node);
   }

   if (to_point == -1) {
      
      editor_log (ROADMAP_INFO, "'to' line has no node. Need to split.");
      
      to_point = find_split_point
                        (to, &position, last_point_id, SPLIT_END|SPLIT_NODE,
                         3, to_opposite_direction, &connect_node);
   }


   if ((from_point == -1) || (to_point == -1)) {
      editor_log (ROADMAP_ERROR, "Can't create a connection point.");
      editor_log_pop ();
      return -1;
   }

   editor_log_pop ();

   return (from_point + to_point) / 2;
}


int editor_track_util_create_trkseg (int line_id,
                                     int plugin_id,
                                     int first_point,
                                     int last_point,
                                     int flags) {

   int trk_from;
   int first_shape = -1;
   int last_shape = -1;
   int trkseg_id;
   int i;
   time_t gps_time = track_point_time (first_point);
   RoadMapPosition *pos = track_point_pos (first_point);

   trk_from = editor_point_add (pos, 0, -1);

   for (i=first_point; i<=last_point; i++) {

      RoadMapPosition *shape_pos = track_point_pos (i);
      last_shape = editor_shape_add
                     (shape_pos->longitude - pos->longitude,
                      shape_pos->latitude - pos->latitude,
                      (short) (track_point_time (i) - gps_time));

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

   trkseg_id = editor_trkseg_add (line_id,
                                  plugin_id,
                                  trk_from,
                                  first_shape,
                                  last_shape,
                                  track_point_time (first_point),
                                  track_point_time (last_point),
                                  flags);

   if (trkseg_id == -1) return -1;

   //last_global_trkseg = trkseg_id;

   return trkseg_id;
}


void editor_track_add_trkseg (PluginLine *line,
                              int trkseg,
                              int direction,
                              int who) {
   
   int first;
   int last;
   int route;

   int plugin_id = roadmap_plugin_get_id (line);

   if (direction != ROUTE_DIRECTION_NONE) {

      LineRouteFlag from_flags = 0;
      LineRouteFlag to_flags = 0;
      LineRouteMax speed_limit = 0;
      
      if (plugin_id == ROADMAP_PLUGIN_ID) {
         route = editor_override_line_get_route
                     (roadmap_plugin_get_line_id (line));
      } else {
         route = editor_line_get_route (roadmap_plugin_get_line_id (line));
      }

      if (route == -1) {
         
         if (plugin_id == ROADMAP_PLUGIN_ID) {
            roadmap_line_route_get_flags
               (roadmap_plugin_get_line_id (line),
                &from_flags, &to_flags);

            roadmap_line_route_get_speed_limit
               (roadmap_plugin_get_line_id (line),
                &speed_limit, &speed_limit);
         }

         route = editor_route_segment_add
                  (from_flags, to_flags, speed_limit, speed_limit);
      } else {
         editor_route_segment_get
            (route, &from_flags, &to_flags, &speed_limit, &speed_limit);
      }

      if (direction == ROUTE_DIRECTION_WITH_LINE) {
         from_flags |= who;
      } else if (direction == ROUTE_DIRECTION_AGAINST_LINE) {
         to_flags |= who;
      } else {
         assert (0);
      }
      
      if (route != -1) {
         editor_route_segment_set
            (route, from_flags, to_flags, speed_limit, speed_limit);
      }
   }

   if (plugin_id != EditorPluginID) {

      editor_override_line_get_trksegs
         (roadmap_plugin_get_line_id (line), &first, &last);
      
      if (first == -1) {
         first = last = trkseg;
      } else {
         editor_trkseg_connect_roads (last, trkseg);
         last = trkseg;
      }
      editor_override_line_set_trksegs
         (roadmap_plugin_get_line_id (line), first, last);

   } else {

      editor_line_get_trksegs
         (roadmap_plugin_get_line_id (line), &first, &last);
      
      if (first == -1) {
         first = last = trkseg;
      } else {
         editor_trkseg_connect_roads (last, trkseg);
         last = trkseg;
      }

      editor_line_set_trksegs
         (roadmap_plugin_get_line_id (line), first, last);
   }
}


int editor_track_util_create_line (int gps_first_point,
                                   int gps_last_point,
                                   int from_point,
                                   int to_point,
                                   int cfcc,
                                   int is_new_track) {

   int route;
   int trkseg;
   int trkseg2;
   int line_id;
   PluginLine line;
   int fips = editor_db_locator (track_point_pos (gps_last_point));
   int trk_flags = 0;

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

   /* we skip the first and last points of the GPS track */
   trkseg =
      editor_track_util_create_trkseg
         (-1, EditorPluginID, gps_first_point+1, gps_last_point-1,
          ED_TRKSEG_FAKE|ED_TRKSEG_NO_GLOBAL);

   if (trkseg == -1) {
      editor_log (ROADMAP_ERROR, "Can't create new trkseg.");
      editor_log_pop ();
      return -1;
   }

   line_id = editor_line_add (from_point, to_point, trkseg, cfcc, 0);

   if (line_id == -1) {
      editor_log (ROADMAP_ERROR, "Can't create new line.");
      editor_log_pop ();
      return -1;
   }

   editor_trkseg_set_line (trkseg, line_id, EditorPluginID);

   /* this is the second trkseg of the line */
   if (is_new_track) {
      trk_flags |= ED_TRKSEG_NEW_TRACK;
   }
      
   trkseg2 =
      editor_track_util_create_trkseg
         (line_id, EditorPluginID, gps_first_point, gps_last_point, trk_flags);

   if (trkseg2 == -1) {
      editor_log (ROADMAP_ERROR, "Can't create new trkseg.");
      editor_log_pop ();
      return -1;
   }

   editor_trkseg_connect_roads (trkseg, trkseg2);
   editor_line_set_trksegs (line_id, trkseg, trkseg2);

   route = editor_route_segment_add (ROUTE_CAR_ALLOWED, 0, 0, 0);
   editor_line_set_route (line_id, route);

   roadmap_plugin_set_line (&line, EditorPluginID, line_id, cfcc, fips);

   editor_log_pop ();
   return line_id;
}


int editor_track_util_length (int first, int last) {

   int length = 0;
   int i;

   for (i=first; i<last; i++) {

      length +=
         roadmap_math_distance
            (track_point_pos (i), track_point_pos (i+1));
   }

   return length;
}


int editor_track_util_create_db (const RoadMapPosition *pos) {

   int fips = editor_db_locator (pos);

   if (editor_db_activate (fips) == -1) {
      editor_db_create (fips);
      if (editor_db_activate (fips) == -1) {
         editor_log_pop ();
         return -1;
      }
   }

   return fips;
}

