/* editor_point.h - database layer
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
 */

#ifndef INCLUDE__EDITOR_POINT__H
#define INCLUDE__EDITOR_POINT__H

#include "roadmap_types.h"
#include "roadmap_dbread.h"


#define ED_POINT_SHARED 1

typedef struct editor_db_point_s {
   int longitude;
   int latitude;
   int flags;
   int roadmap_id;
} editor_db_point;

typedef struct editor_db_del_point_s {
   int roadmap_id;
   int editor_id;
} editor_db_del_point;


int editor_point_add (RoadMapPosition *position, int flags, int roadmap_id);
void editor_point_position  (int point, RoadMapPosition *position);
int editor_point_set_pos (int point, RoadMapPosition *position);

int editor_point_roadmap_to_editor (int roadmap_id);
void editor_point_roadmap_id (int point_id, int *roadmap_id);

extern roadmap_db_handler EditorPointsHandler;
extern roadmap_db_handler EditorPointsDelHandler;

#endif // INCLUDE__EDITOR_POINT__H

