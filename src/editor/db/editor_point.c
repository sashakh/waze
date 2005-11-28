/* editor_point.c - point databse layer
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
 *   See editor_point.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_point.h"

#include "../editor_log.h"

#include "editor_db.h"
#include "editor_point.h"

static editor_db_section *ActivePointsDB;
static editor_db_section *ActivePointsDelDB;

static void editor_points_activate (void *context) {
   ActivePointsDB = (editor_db_section *) context;
}

static void editor_points_del_activate (void *context) {
   ActivePointsDelDB = (editor_db_section *) context;
}

roadmap_db_handler EditorPointsHandler = {
   "points",
   editor_map,
   editor_points_activate,
   editor_unmap
};

roadmap_db_handler EditorPointsDelHandler = {
   "points_del",
   editor_map,
   editor_points_del_activate,
   editor_unmap
};


int editor_point_add (RoadMapPosition *position, int flags, int roadmap_id) {
   
   editor_db_point point;
   int id;

   point.longitude = position->longitude;
   point.latitude = position->latitude;
   point.flags = flags;
   point.roadmap_id = roadmap_id;
   id = editor_db_add_item (ActivePointsDB, &point);

   if (id == -1) {
      editor_db_grow ();
      id = editor_db_add_item (ActivePointsDB, &point);
   }

   return id;
}


void editor_point_position (int point, RoadMapPosition *position) {

   editor_db_point *point_st = editor_db_get_item (ActivePointsDB, point, 0, 0);
   assert(point_st != NULL);

   position->longitude = point_st->longitude;
   position->latitude = point_st->latitude;
}


int editor_point_set_pos (int point, RoadMapPosition *position) {

   editor_db_point *point_st = editor_db_get_item (ActivePointsDB, point, 0, 0);
   assert(point_st != NULL);

   if (point_st->flags & ED_POINT_SHARED) {
      position->longitude = point_st->longitude;
      position->latitude = point_st->latitude;
      return -1;
   }
      
   point_st->longitude = position->longitude;
   point_st->latitude = position->latitude;

   return 0;
}


int editor_point_roadmap_to_editor (int roadmap_id) {

   editor_db_del_point *del_point_ptr;
   editor_db_del_point del_point;
   int ed_point_id;
   RoadMapPosition point_pos;

   int begin = -1;
   int end;

   end = editor_db_get_item_count (ActivePointsDelDB);

   if (end > 0) {

      while (end - begin > 1) {

         int middle = (begin + end) / 2;
         del_point_ptr = editor_db_get_item (ActivePointsDelDB, middle, 0, 0);
         assert (del_point_ptr != NULL);
      
         if (roadmap_id < del_point_ptr->roadmap_id) {
            end = middle;
         } if (roadmap_id > del_point_ptr->roadmap_id) {
            begin = middle;
         } else {
            end = middle;
            break;
         }
      }

      if (roadmap_id == del_point_ptr->roadmap_id) {

         return del_point_ptr->editor_id;
      }
   }

   roadmap_point_position (roadmap_id, &point_pos);
   
   ed_point_id = editor_point_add (&point_pos, ED_POINT_SHARED, roadmap_id);

   if (ed_point_id == -1) {
      editor_log
         (ROADMAP_ERROR,
          "editor_point_roadmap_to_editor - Can't insert new point");
      return -1;
   }
   
   del_point.roadmap_id = roadmap_id;
   del_point.editor_id = ed_point_id;

   if (editor_db_insert_item (ActivePointsDelDB, &del_point, end) == -1) {

      editor_db_grow ();
      if (editor_db_insert_item (ActivePointsDelDB, &del_point, end) == -1) {

         editor_log
            (ROADMAP_ERROR,
             "editor_point_roadmap_to_editor - Can't insert new item");
         return -1;
      }
   }

   return ed_point_id;
}


void editor_point_roadmap_id (int point_id, int *roadmap_id) {

   editor_db_point *point_st =
      editor_db_get_item (ActivePointsDB, point_id, 0, 0);
   assert(point_st != NULL);

   *roadmap_id = point_st->roadmap_id;
}

