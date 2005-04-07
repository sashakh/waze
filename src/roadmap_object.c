/* roadmap_object.c - manage the roadmap moving objects.
 *
 * LICENSE:
 *
 *   Copyright 2005 Pascal F. Martin
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
 * DESCRIPTION:
 *
 *   This module manages a dynamic list of objects to be displayed on the map.
 *   The objects are usually imported from external applications for which
 *   a RoadMap driver has been installed (see roadmap_driver.c).
 *
 *   These objects are dynamic and not persistent (i.e. these are not points
 *   of interest).
 *
 *   The implementation is not terribly optimized: there should not be too
 *   many objects.
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_sprite.h"
#include "roadmap_object.h"


struct RoadMapObjectDescriptor {

   char *id; /* Must be unique. */

   char *name; /* What's shown on display, don't need to be unique. */
   char *sprite; /* .. that is used to represent the object on screen. */

   RoadMapGpsPosition position;

   struct RoadMapObjectDescriptor *next;
   struct RoadMapObjectDescriptor *previous;
};

typedef struct RoadMapObjectDescriptor RoadMapObject;


static RoadMapObject *RoadmapObjectList = NULL;


RoadMapObject *roadmap_object_search (const char *id) {

   RoadMapObject *cursor;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {
      if (strcmp (id, cursor->id) == 0) return cursor;
   }

   return NULL;
}


void roadmap_object_add (const char *id,
                         const char *name,
                         const char *sprite) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor == NULL) {

      cursor = calloc (1, sizeof(RoadMapObject));
      if (cursor == NULL) {
         roadmap_log (ROADMAP_ERROR, "no more memory");
         return;
      }
      cursor->id = strdup(id);
      cursor->name = strdup(name);
      cursor->sprite = strdup(sprite);

      cursor->previous  = NULL;
      cursor->next      = RoadmapObjectList;
      RoadmapObjectList = cursor;

      if (cursor->next != NULL) {
         cursor->next->previous = cursor;
      }
   }
}


void roadmap_object_move (const char *id,
                          const RoadMapGpsPosition *position) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor != NULL) {
      cursor->position = *position;
   }
}


void roadmap_object_remove (const char *id) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor != NULL) {

      if (cursor->next != NULL) {
          cursor->next->previous = cursor->previous;
      }
      if (cursor->previous != NULL) {
         cursor->previous->next = cursor->next;
      } else {
         RoadmapObjectList = cursor->next;
      }

      free (cursor->id);
      free (cursor->name);
      free (cursor->sprite);
      free (cursor);
   }
}


void roadmap_object_draw (void) {

   RoadMapObject *cursor;
   RoadMapPosition position;
   RoadMapGuiPoint screen_point;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {

      position.latitude = cursor->position.latitude;
      position.longitude = cursor->position.longitude;

      if (roadmap_math_point_is_visible(&position)) {

         roadmap_math_coordinate (&position, &screen_point);
         roadmap_sprite_draw (cursor->sprite, &screen_point, 0);
      }
   }
}


void roadmap_object_initialize (void) { }

