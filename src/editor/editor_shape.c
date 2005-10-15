/* editor_shape.c - point databse layer
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
 *   See editor_shape.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../roadmap.h"
#include "../roadmap_file.h"
#include "../roadmap_path.h"

#include "editor_db.h"
#include "editor_shape.h"

static editor_db_section *ActiveShapeDB;

static void editor_shape_activate (void *context) {
   ActiveShapeDB = (editor_db_section *) context;
}

roadmap_db_handler EditorShapeHandler = {
   "shape",
   editor_map,
   editor_shape_activate,
   editor_unmap
};


int editor_shape_add (short delta_longitude,
                      short delta_latitude,
                      short delta_time) {
   
   editor_db_shape shape;
   int id;

   shape.delta_longitude = delta_longitude;
   shape.delta_latitude = delta_latitude;
   shape.delta_time = delta_time;

   id = editor_db_add_item (ActiveShapeDB, &shape);

   if (id == -1) {
      editor_db_grow ();
      id = editor_db_add_item (ActiveShapeDB, &shape);
   }

   return id;
}


void editor_shape_position (int shape, RoadMapPosition *position) {

   editor_db_shape *shape_st = editor_db_get_item (ActiveShapeDB, shape, 0, 0);
   assert(shape_st != NULL);

   position->longitude += shape_st->delta_longitude;
   position->latitude  += shape_st->delta_latitude;
}


void editor_shape_time (int shape, int *time) {

   editor_db_shape *shape_st = editor_db_get_item (ActiveShapeDB, shape, 0, 0);
   assert(shape_st != NULL);

   *time += shape_st->delta_time;
}


void editor_shape_adjust_point (int shape,
                                int lon_diff,
                                int lat_diff,
                                int time_diff) {

   editor_db_shape *shape_st = editor_db_get_item (ActiveShapeDB, shape, 0, 0);
   assert(shape_st != NULL);

   shape_st->delta_longitude += lon_diff;
   shape_st->delta_latitude += lat_diff;
   shape_st->delta_time += time_diff;
}
