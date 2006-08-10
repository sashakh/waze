/* editor_marker.h - database layer
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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

#ifndef INCLUDE__EDITOR_MARKER__H
#define INCLUDE__EDITOR_MARKER__H

#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "editor_dictionary.h"

#define ED_MARKER_UPLOAD  0x1
#define ED_MARKER_DELETED 0x2

typedef struct editor_marker_type {
   const char *name;
   int (*export_marker) (const char *note);
   const char *(*update_marker) (unsigned char *flags, const char *note);
} EditorMarkerType;

typedef struct editor_db_marker_s {
   int longitude;
   int latitude;
   short steering;
   unsigned char type;
   unsigned char flags;
   EditorString note;
} editor_db_marker;


int editor_marker_add (int longitude,
                       int latitude,
                       int steering,
                       unsigned char type,
                       unsigned char flags,
                       const char *note);
   
int  editor_marker_count (void);
void editor_marker_position (int marker,
                             RoadMapPosition *position, int *steering);

const char *editor_marker_type (int marker);
const char *editor_marker_note (int marker);

unsigned char editor_marker_flags (int marker);

void editor_marker_update (int marker, unsigned char flags, const char *note);

int editor_marker_reg_type (EditorMarkerType *type);

extern roadmap_db_handler EditorMarkersHandler;

#endif // INCLUDE__EDITOR_MARKER__H

