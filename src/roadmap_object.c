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

#include "roadmap_object.h"


struct RoadMapObjectDescriptor {

   RoadMapDynamicString origin;

   RoadMapDynamicString id; /* Must be unique. */

   RoadMapDynamicString name; /* Name displayed, need not be unique. */

   RoadMapDynamicString sprite; /* Icon for the object on the map. */

   RoadMapGpsPosition position;

   RoadMapObjectListener listener;

   struct RoadMapObjectDescriptor *next;
   struct RoadMapObjectDescriptor *previous;
};

typedef struct RoadMapObjectDescriptor RoadMapObject;


static RoadMapObject *RoadmapObjectList = NULL;



static void roadmap_object_null_listener (RoadMapDynamicString id,
                                          const RoadMapGpsPosition *position) {}

static void roadmap_object_null_monitor (RoadMapDynamicString id) {}


static RoadMapObjectMonitor RoadMapObjectFirstMonitor =
                                      roadmap_object_null_monitor;


static RoadMapObject *roadmap_object_search (RoadMapDynamicString id) {

   RoadMapObject *cursor;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {
      if (cursor->id == id) return cursor;
   }

   return NULL;
}


void roadmap_object_add (RoadMapDynamicString origin,
                         RoadMapDynamicString id,
                         RoadMapDynamicString name,
                         RoadMapDynamicString sprite) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor == NULL) {

      cursor = calloc (1, sizeof(RoadMapObject));
      if (cursor == NULL) {
         roadmap_log (ROADMAP_ERROR, "no more memory");
         return;
      }
      cursor->origin = origin;
      cursor->id     = id;
      cursor->name   = name;
      cursor->sprite = sprite;

      cursor->listener = roadmap_object_null_listener;

      roadmap_string_lock(origin);
      roadmap_string_lock(id);
      roadmap_string_lock(name);
      roadmap_string_lock(sprite);

      cursor->previous  = NULL;
      cursor->next      = RoadmapObjectList;
      RoadmapObjectList = cursor;

      if (cursor->next != NULL) {
         cursor->next->previous = cursor;
      }

      (*RoadMapObjectFirstMonitor) (id);
   }
}


void roadmap_object_move (RoadMapDynamicString id,
                          const RoadMapGpsPosition *position) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor != NULL) {

      if ((cursor->position.longitude != position->longitude) ||
          (cursor->position.latitude  != position->latitude)  ||
          (cursor->position.altitude  != position->altitude)  ||
          (cursor->position.steering  != position->steering)  ||
          (cursor->position.speed     != position->speed)) {

         cursor->position = *position;
         (*cursor->listener) (id, position);
      }
   }
}


void roadmap_object_remove (RoadMapDynamicString id) {

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

      roadmap_string_release(cursor->origin);
      roadmap_string_release(cursor->id);
      roadmap_string_release(cursor->name);
      roadmap_string_release(cursor->sprite);
      free (cursor);
   }
}


void roadmap_object_iterate (RoadMapObjectAction action) {

   RoadMapObject *cursor;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {

      (*action) (roadmap_string_get(cursor->name),
                 roadmap_string_get(cursor->sprite),
                 &(cursor->position));
   }
}


void roadmap_object_cleanup (RoadMapDynamicString origin) {

   RoadMapObject *cursor;
   RoadMapObject *next;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = next) {

      next = cursor->next;

      if (cursor->origin == origin) {
         roadmap_object_remove (cursor->id);
      }
   }
}


RoadMapObjectListener roadmap_object_register_listener
                           (RoadMapDynamicString id,
                            RoadMapObjectListener listener) {

   RoadMapObjectListener previous;
   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor == NULL) return NULL;

   previous = cursor->listener;
   cursor->listener = listener;

   return previous;
}


RoadMapObjectMonitor roadmap_object_register_monitor
                           (RoadMapObjectMonitor monitor) {

   RoadMapObjectMonitor previous = RoadMapObjectFirstMonitor;

   RoadMapObjectFirstMonitor = monitor;
   return previous;
}

