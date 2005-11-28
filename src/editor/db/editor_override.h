/* editor_override.h - database layer
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

#ifndef INCLUDE__EDITOR_OVERRIDE__H
#define INCLUDE__EDITOR_OVERRIDE__H

typedef struct editor_db_override_s {
   int first_trkseg;
   int last_trkseg;
   int route;
   int flags;
} editor_db_override;

#define NULL_OVERRIDE {-1, -1, -1, 0}

int editor_override_line_get_route (int line);
int editor_override_line_set_route (int line, int route);
int editor_override_line_get_flags (int line);
int editor_override_line_set_flags (int line, int flags);
void editor_override_line_get_trksegs (int line, int *first, int *last);
int editor_override_line_set_trksegs (int line, int first, int last);

extern roadmap_db_handler EditorOverrideHandler;

#endif // INCLUDE__EDITOR_OVERRIDE__H

