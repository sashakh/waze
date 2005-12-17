/* editor_trkseg.h - database layer
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

#ifndef INCLUDE__EDITOR_TRKSEG__H
#define INCLUDE__EDITOR_TRKSEG__H

#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "editor_db.h"

/* flags */
#define ED_TRKSEG_FAKE         0x1
#define ED_TRKSEG_IGNORE       0x2
#define ED_TRKSEG_END_TRACK    0x4
#define ED_TRKSEG_NEW_TRACK    0x8
#define ED_TRKSEG_OPPOSITE_DIR 0x10
#define ED_TRKSEG_NO_GLOBAL    0x20


typedef struct editor_db_trkseg_private_s {
   int last_global_trkseg;
   int next_export;

} editor_db_trkseg_private;

typedef struct editor_db_trkseg_header_s {
   editor_db_trkseg_private header;
   editor_db_section section;
} editor_db_trkseg_header;

typedef struct editor_db_trkseg_s {
   int line_id;
   int plugin_id;
   int point_from;
   int point_to;
   int first_shape;
   int last_shape;
   int gps_start_time;
   int gps_end_time;
   int flags;
   int next_road_trkseg;
   int next_global_trkseg;
} editor_db_trkseg;

void editor_trkseg_set_line (int trkseg, int line_id, int plugin_id);

int editor_trkseg_add (int line_id,
                       int plugin_id,
                       int p_from,
                       int first_shape,
                       int last_shape,
                       time_t gps_start_time,
                       time_t gps_end_time,
                       int flags);

void editor_trkseg_get (int trkseg,
                        int *p_from,
                        int *first_shape,
                        int *last_shape,
                        int *flags);

void editor_trkseg_get_line (int trkseg, int *line_id, int *plugin_id);

void editor_trkseg_get_time (int trkseg,
                             time_t *start_time,
                             time_t *end_time);

void editor_trkseg_connect_roads (int previous, int next);
void editor_trkseg_connect_global (int previous, int next);
int editor_trkseg_next_in_road (int trkseg);
int editor_trkseg_next_in_global (int trkseg);

int editor_trkseg_split (int trkseg,
                         RoadMapPosition *line_from,
                         RoadMapPosition *line_to,
                         RoadMapPosition *split_pos);

int editor_trkseg_dup (int source_id);

int editor_trkseg_get_current_trkseg (void);
void editor_trkseg_reset_next_export (void);
int editor_trkseg_get_next_export (void);

extern roadmap_db_handler EditorTrksegHandler;
extern editor_db_trkseg_private editor_db_trkseg_private_init;

#endif // INCLUDE__EDITOR_TRKSEG__H

