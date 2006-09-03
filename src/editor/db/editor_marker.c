/* editor_marker.c - marker databse layer
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
 *
 * SYNOPSYS:
 *
 *   See editor_marker.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_path.h"

#include "../editor_log.h"

#include "editor_db.h"
#include "editor_marker.h"

#define MAX_MARKER_TYPES 10

static EditorMarkerType *MarkerTypes[MAX_MARKER_TYPES];
static int MarkerTypesCount;

static editor_db_section *ActiveMarkersDB;
static EditorDictionary   ActiveNoteDictionary;

static void editor_marker_activate (void *context) {
   ActiveMarkersDB = (editor_db_section *) context;
   ActiveNoteDictionary = editor_dictionary_open ("notes");
}

roadmap_db_handler EditorMarkersHandler = {
   "marker",
   editor_map,
   editor_marker_activate,
   editor_unmap
};


int editor_marker_add(int longitude,
                      int latitude,
                      int steering,
                      time_t time,
                      unsigned char type,
                      unsigned char flags,
                      const char *note) {
   
   editor_db_marker marker;
   int id;

   marker.longitude = longitude;
   marker.latitude  = latitude;
   marker.steering  = steering;
   marker.time      = time;
   marker.type      = type;
   marker.flags     = flags | ED_MARKER_DIRTY;

   if (note == NULL) {
      marker.note = ROADMAP_INVALID_STRING;
   } else {
      
      marker.note = editor_dictionary_add
                           (ActiveNoteDictionary, note, strlen(note));
   }

   id = editor_db_add_item (ActiveMarkersDB, &marker);

   if (id == -1) {
      editor_db_grow ();
      id = editor_db_add_item (ActiveMarkersDB, &marker);
   }

   /* FIXME this is a temporary solution for my bad DB design */
   editor_db_check_grow ();
   return id;
}


int  editor_marker_count(void) {

   if (!ActiveMarkersDB) return 0;

   return ActiveMarkersDB->num_items;
}


void editor_marker_position(int marker,
                            RoadMapPosition *position, int *steering) {

   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   position->longitude = marker_st->longitude;
   position->latitude = marker_st->latitude;

   if (steering) *steering = marker_st->steering;
}


const char *editor_marker_type(int marker) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   return MarkerTypes[marker_st->type]->name;
}
   

const char *editor_marker_note(int marker) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   if (marker_st->note == ROADMAP_INVALID_STRING) {
      return "";
   } else {

      return editor_dictionary_get (ActiveNoteDictionary, marker_st->note);
   }
}


time_t editor_marker_time(int marker) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   return marker_st->time;
}


unsigned char editor_marker_flags(int marker) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   assert(marker_st != NULL);

   return marker_st->flags;
}


void editor_marker_update(int marker, unsigned char flags,
                          const char *note) {

   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);

   int dirty = 0;
   assert(marker_st != NULL);

   if (note == NULL) {
      if (marker_st->note != ROADMAP_INVALID_STRING) {
         dirty++;
         marker_st->note = ROADMAP_INVALID_STRING;
      }
   } else if ((marker_st->note == ROADMAP_INVALID_STRING) ||
               strcmp(editor_dictionary_get (ActiveNoteDictionary,
                                                marker_st->note),
                      note)) {

      dirty++;
      marker_st->note = editor_dictionary_add
                           (ActiveNoteDictionary, note, strlen(note));
   }

   if ((marker_st->flags & ~ED_MARKER_DIRTY) !=
                  (flags & ~ED_MARKER_DIRTY)) {
      dirty++;
   }

   marker_st->flags = flags;

   if (dirty) marker_st->flags |= ED_MARKER_DIRTY;
}


int editor_marker_reg_type(EditorMarkerType *type) {

   int id = MarkerTypesCount;

   if (MarkerTypesCount == MAX_MARKER_TYPES) return -1;

   MarkerTypes[MarkerTypesCount] = type;
   MarkerTypesCount++;

   return id;
}


void editor_marker_voice_file(int marker, char *file, int size) {
   char *path = roadmap_path_join (roadmap_path_user (), "markers");
   char file_name[100];
   char *full_name;

   roadmap_path_create (path);
   snprintf (file_name, sizeof(file_name), "voice_%d.wav", marker);

   full_name = roadmap_path_join (path, file_name);
   strncpy (file, full_name, size);
   file[size-1] = '\0';

   roadmap_path_free (full_name);
   roadmap_path_free (path);
}


int editor_marker_export(int marker, const char **description,
                         const char *keys[MAX_ATTR],
                         char       *values[MAX_ATTR],
                         int *count) {
   
   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   
   assert(marker_st != NULL);
   
   return MarkerTypes[marker_st->type]->export_marker (marker,
                                                       description,
                                                       keys,
                                                       values,
                                                       count);
}


int editor_marker_verify(int marker, unsigned char *flags, const char **note) {

   editor_db_marker *marker_st =
      editor_db_get_item (ActiveMarkersDB, marker, 0, 0);
   
   assert(marker_st != NULL);
   
   return MarkerTypes[marker_st->type]->update_marker (marker, flags, note);
}

