/* editor_line.h - database layer
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

#ifndef INCLUDE__EDITOR_LINE__H
#define INCLUDE__EDITOR_LINE__H

#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "roadmap_street.h"
#include "roadmap_plugin.h"

#define ED_LINE_DELETED         0x1 /* flag */
#define ED_LINE_EXPLICIT_SPLIT  0x2 /* flag */
#define ED_LINE_DIRTY           0x4 /* flag */

typedef struct editor_db_line_s {
   int point_from;
   int point_to;
   int first_trkseg;
   int last_trkseg;
   int cfcc;
   int flags;
   int street;
   int range;
   int route;
} editor_db_line;

//int editor_line_del_add (int line_id, int cfcc);
//void editor_line_del_remove (int line_id);
//int editor_line_del_find (int line_id, int cfcc);

int editor_line_add
         (int p_from,
          int p_to,
          int trkseg,
          int cfcc,
          int flag);

void editor_line_get (int line,
                      RoadMapPosition *from,
                      RoadMapPosition *to,
                      int *trkseg,
                      int *cfcc,
                      int *flag);

void editor_line_get_points (int line, int *from, int *to);

int editor_line_length (int line);

void editor_line_modify_properties (int line,
                                    int cfcc,
                                    int flags);

int editor_line_get_count (void);

int editor_line_copy (int line, int cfcc, int fips);

int editor_line_split (PluginLine *line,
                       RoadMapPosition *previous_point, 
                       RoadMapPosition *split_position,
                       int *split_point);

int editor_line_get_street (int line, int *street, int *range);

int editor_line_set_street (int line, int *street, int *range);

int editor_line_get_route (int line);
int editor_line_set_route (int line, int route);
void editor_line_get_trksegs (int line, int *first, int *last);
void editor_line_set_trksegs (int line, int first, int last);
int editor_line_get_cross_time (int line, int direction);
int editor_line_mark_dirty (int line_id);

extern roadmap_db_handler EditorLinesHandler;
extern roadmap_db_handler EditorLinesDelHandler;

#endif // INCLUDE__EDITOR_LINE__H

