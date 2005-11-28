/* editor_override.c - line databse layer
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
 *   See editor_override.h
 */

#include <assert.h>
#include "stdlib.h"
#include "string.h"

#include "roadmap.h"
#include "roadmap_locator.h"

#include "editor_db.h"
#include "editor_route.h"
#include "editor_override.h"

typedef struct {
   editor_db_section *index;
   editor_db_section *data;
} EditorOverrideContext;

static EditorOverrideContext *EditorOverrideActive;

static void *editor_route_map (roadmap_db *root) {

   EditorOverrideContext *context;

   roadmap_db *table;

   context = (EditorOverrideContext *) malloc (sizeof(EditorOverrideContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }

   table = roadmap_db_get_subsection (root, "index");
   context->index = (editor_db_section *) roadmap_db_get_data (table);

   table = roadmap_db_get_subsection (root, "data");
   context->data = (editor_db_section *) roadmap_db_get_data (table);

   return context;
}

static void editor_route_activate (void *context) {

   EditorOverrideActive = (EditorOverrideContext *) context;
}

static void editor_route_unmap (void *context) {

   free (context);
}


roadmap_db_handler EditorOverrideHandler = {
   "override",
   editor_route_map,
   editor_route_activate,
   editor_route_unmap
};


static void editor_override_init_index_item (void *item) {
   
   *(int *)item = -1;
}


static editor_db_override *editor_override_data (int line, int create) {

   int *index;

   if (editor_db_activate (roadmap_locator_active ()) == -1) {
      if (create) {
         editor_db_create (roadmap_locator_active ());
         if (editor_db_activate (roadmap_locator_active ()) == -1) {
            return NULL;
         }
      } else {
         return NULL;
      }
   }

   index =
      (int *) editor_db_get_item
         (EditorOverrideActive->index,
          line,
          create,
          editor_override_init_index_item);

   if ((index == NULL) || (*index == -1)) {
      
      if (!create) return NULL;

      if (index == NULL) {
         editor_db_grow ();
         index =
            (int *) editor_db_get_item
                  (EditorOverrideActive->index,
                   line,
                   create,
                   editor_override_init_index_item);

      }

      if (index == NULL) return NULL;

      if (*index == -1) {
         
         static editor_db_override data = NULL_OVERRIDE;

         *index =
            editor_db_add_item (EditorOverrideActive->data, &data);

         if (*index == -1) {
            editor_db_grow ();
            *index =
               editor_db_add_item (EditorOverrideActive->data, &data);
         }
      }

      if (*index == -1) return NULL;

   }

   return (editor_db_override *) editor_db_get_item (
                                       EditorOverrideActive->data,
                                      *index,
                                       0,
                                       NULL);
}


int editor_override_line_get_route (int line) {

   editor_db_override *data = editor_override_data (line, 0);

   if (data == NULL) return -1;

   return data->route;
}


int editor_override_line_set_route (int line, int route) {

   editor_db_override *data = editor_override_data (line, 1);

   if (data == NULL) return -1;
   
   data->route = route;

   return 0;
}


int editor_override_line_get_flags (int line) {

   editor_db_override *data = editor_override_data (line, 0);

   if (data == NULL) return 0;

   return data->flags;
}


int editor_override_line_set_flags (int line, int flags) {

   editor_db_override *data = editor_override_data (line, 1);

   if (data == NULL) return -1;
   
   data->flags = flags;

   return 0;
}


void editor_override_line_get_trksegs (int line, int *first, int *last) {

   editor_db_override *data = editor_override_data (line, 0);

   if (data == NULL) {
      *first = *last = -1;
      return;
   }

   *first = data->first_trkseg;
   *last = data->last_trkseg;
}


int editor_override_line_set_trksegs (int line, int first, int last) {

   editor_db_override *data = editor_override_data (line, 1);

   if (data == NULL) return -1;

   data->first_trkseg = first;
   data->last_trkseg = last;

   return 0;
}

