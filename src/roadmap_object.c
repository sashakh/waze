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

#include "roadmap_canvas.h"

#include "roadmap_object.h"


#define ROADMAP_OBJECT_MAX_EDGE 16


struct RoadMapObjectDescriptor {

   RoadMapDynamicString origin;

   RoadMapDynamicString id; /* Must be unique. */

   RoadMapDynamicString name; /* Name displayed, need not be unique. */

   RoadMapDynamicString color;

   char                 family; // Sprite, Polygon or Circle.

   union {
      struct {
         RoadMapGpsPosition   position;
         RoadMapDynamicString id; /* Icon for the object on the map. */
      } sprite;
      struct {
         int             count;
         RoadMapPosition edge[ROADMAP_OBJECT_MAX_EDGE];
      } polygon;
      struct {
         int             radius;
         RoadMapPosition center;
      } circle;
   } view;


   RoadMapObjectListener listener;

   struct RoadMapObjectDescriptor *next;
   struct RoadMapObjectDescriptor *previous;

   struct RoadMapObjectDescriptor *hash_next;
   struct RoadMapObjectDescriptor *hash_previous;
};

typedef struct RoadMapObjectDescriptor RoadMapObject;


static RoadMapObject *RoadmapObjectList = NULL;

static RoadMapObject *RoadmapObjectHash[128];


static unsigned int roadmap_object_hashed (RoadMapDynamicString id) {

   unsigned long seed = (unsigned long) id;

   return ((((seed * 9) + (seed >> 8)) * 9) + (seed >> 16)) % 127;
}


static void roadmap_object_null_listener (RoadMapDynamicString id,
                                          const RoadMapGpsPosition *position) {}

static void roadmap_object_null_monitor (RoadMapDynamicString id) {}


static RoadMapObjectMonitor RoadMapObjectFirstMonitor =
                                      roadmap_object_null_monitor;


static RoadMapObject *roadmap_object_search (RoadMapDynamicString id) {

   int hash;
   RoadMapObject *cursor;

   hash = roadmap_object_hashed (id);

   for (cursor = RoadmapObjectHash[hash];
        cursor != NULL; cursor = cursor->hash_next) {
      if (cursor->id == id) return cursor;
   }

   return NULL;
}


static RoadMapObject *roadmap_object_add (RoadMapDynamicString origin,
                                          RoadMapDynamicString id,
                                          RoadMapDynamicString name,
                                          RoadMapDynamicString color) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor == NULL) {

      int hash = roadmap_object_hashed (id);

      cursor = calloc (1, sizeof(RoadMapObject));
      if (cursor == NULL) {
         roadmap_log (ROADMAP_ERROR, "no more memory");
         return NULL; // Cannot create.
      }

      cursor->origin = origin;
      cursor->id     = id;
      cursor->name   = name;
      cursor->color  = color;

      roadmap_string_lock(cursor->origin);
      roadmap_string_lock(cursor->id);
      roadmap_string_lock(cursor->name);
      roadmap_string_lock(cursor->color);

      cursor->listener = roadmap_object_null_listener;

      // Update the objects list.
      cursor->previous  = NULL;
      cursor->next      = RoadmapObjectList;
      RoadmapObjectList = cursor;
      if (cursor->next != NULL) {
         cursor->next->previous = cursor;
      }

      // update the hash list.
      cursor->hash_previous  = NULL;
      cursor->hash_next      = RoadmapObjectHash[hash];
      RoadmapObjectHash[hash] = cursor;
      if (cursor->hash_next != NULL) {
         cursor->hash_next->hash_previous = cursor;
      }

      return cursor;
   }

   return NULL; // The object already exists.
}


void roadmap_object_add_sprite (RoadMapDynamicString origin,
                                RoadMapDynamicString id,
                                RoadMapDynamicString name,
                                RoadMapDynamicString sprite,
				RoadMapDynamicString color) {

   RoadMapObject *cursor = roadmap_object_add (origin, id, name, color);

   if (cursor != NULL) {

      cursor->family = 'S';
      cursor->view.sprite.id = sprite;
      roadmap_string_lock(sprite);

      (*RoadMapObjectFirstMonitor) (cursor->id);
   }
}


void roadmap_object_add_polygon (RoadMapDynamicString  origin,
                                 RoadMapDynamicString  id,
                                 RoadMapDynamicString  name,
                                 RoadMapDynamicString  color,
				 int                   count,
				 const RoadMapPosition edge[]) {

   RoadMapObject *cursor = roadmap_object_add (origin, id, name, color);

   if (cursor != NULL) {

      int i;

      if (count > ROADMAP_OBJECT_MAX_EDGE) count = ROADMAP_OBJECT_MAX_EDGE;

      cursor->family = 'P';
      for (i = count - 1; i >= 0; --i) {
         cursor->view.polygon.edge[i] = edge[i];
      }
      cursor->view.polygon.count = count;

      (*RoadMapObjectFirstMonitor) (cursor->id);
   }
}


void roadmap_object_add_circle (RoadMapDynamicString   origin,
                                RoadMapDynamicString   id,
                                RoadMapDynamicString   name,
                                RoadMapDynamicString   color,
				const RoadMapPosition *center,
				int                    radius) {

   RoadMapObject *cursor = roadmap_object_add (origin, id, name, color);

   if (cursor != NULL) {

      cursor->family = 'C';
      cursor->view.circle.radius = radius;
      cursor->view.circle.center = *center;

      (*RoadMapObjectFirstMonitor) (cursor->id);
   }
}


void roadmap_object_move (RoadMapDynamicString id,
                          const RoadMapGpsPosition *position) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor != NULL) {

      switch (cursor->family) {
      case 'S':
         if ((cursor->view.sprite.position.longitude != position->longitude) ||
             (cursor->view.sprite.position.latitude  != position->latitude)  ||
             (cursor->view.sprite.position.altitude  != position->altitude)  ||
             (cursor->view.sprite.position.steering  != position->steering)  ||
             (cursor->view.sprite.position.speed     != position->speed)) {

            cursor->view.sprite.position = *position;
            (*cursor->listener) (id, position);
         }
	 break;
      case 'P':
         if ((cursor->view.polygon.edge[0].longitude != position->longitude) ||
             (cursor->view.polygon.edge[0].latitude  != position->latitude)) {

            int i;
	    int delta_latitude =
	           position->latitude - cursor->view.polygon.edge[0].latitude;
	    int delta_longitude =
	           position->longitude - cursor->view.polygon.edge[0].longitude;

	    for (i = cursor->view.polygon.count - 1; i >= 0; --i) {
	       cursor->view.polygon.edge[i].latitude += delta_latitude;
	       cursor->view.polygon.edge[i].longitude += delta_longitude;
	    }
            (*cursor->listener) (id, position);
         }
	 break;
      case 'C':
         if ((cursor->view.circle.center.longitude != position->longitude) ||
             (cursor->view.circle.center.latitude  != position->latitude)) {

            cursor->view.circle.center.latitude = position->latitude;
            cursor->view.circle.center.longitude = position->longitude;
            (*cursor->listener) (id, position);
         }
	 break;
      }
   }
}


void roadmap_object_color (RoadMapDynamicString id,
                           RoadMapDynamicString color) {

   RoadMapObject *cursor = roadmap_object_search (id);

   if (cursor != NULL) {

      roadmap_string_release(cursor->color);

      cursor->color  = color;
      roadmap_string_lock(cursor->color);
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

      if (cursor->hash_next != NULL) {
          cursor->hash_next->hash_previous = cursor->hash_previous;
      }
      if (cursor->hash_previous != NULL) {
         cursor->hash_previous->hash_next = cursor->hash_next;
      } else {
         RoadmapObjectHash[roadmap_object_hashed (id)] = cursor->hash_next;
      }

      roadmap_string_release(cursor->origin);
      roadmap_string_release(cursor->id);
      roadmap_string_release(cursor->name);
      roadmap_string_release(cursor->color);
      switch (cursor->family) {
      case 'S':
         roadmap_string_release(cursor->view.sprite.id);
	 break;
      case 'P':
      case 'C':
	 break;
      }

      free (cursor);
   }
}


void roadmap_object_iterate_sprite (RoadMapSpriteAction action) {

   RoadMapObject *cursor;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {

      if (cursor->family != 'S') continue;

      (*action) (roadmap_string_get(cursor->name),
                 roadmap_string_get(cursor->view.sprite.id),
		 NULL,
                 &(cursor->view.sprite.position));
   }
}


void roadmap_object_iterate_polygon (RoadMapPolygonAction action) {

   RoadMapObject *cursor;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {

      if (cursor->family != 'P') continue;

      (*action) (roadmap_string_get(cursor->name),
		 NULL, // FIXME: add pen.
		 cursor->view.polygon.count,
		 cursor->view.polygon.edge,
                 NULL); // FIXME: add area.
   }
}


void roadmap_object_iterate_circle (RoadMapCircleAction action) {

   RoadMapObject *cursor;

   for (cursor = RoadmapObjectList; cursor != NULL; cursor = cursor->next) {

      if (cursor->family != 'C') continue;

      (*action) (roadmap_string_get(cursor->name),
		 NULL, // FIXME: add pen.
		 cursor->view.circle.radius,
                 &(cursor->view.circle.center));
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

