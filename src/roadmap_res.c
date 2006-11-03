/* roadmap_res.c - Resources manager (Bitmap, voices, etc')
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
 *   See roadmap_res.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap_canvas.h"
#include "roadmap_hash.h"
#include "roadmap_list.h"
#include "roadmap_path.h"

#include "roadmap_res.h"

#define BLOCK_SIZE 100

const char *ResourceName[] = {
   "bitmap_res"
};

struct resource_slot {
   const char *name;
   void *data;
};

typedef struct roadmap_resource {
   RoadMapHash *hash;
   struct resource_slot *slots;
   int count;
   int max;
   int used_mem;
   int max_mem;
} RoadMapResource;


static RoadMapResource Resources[MAX_RESOURCES];

static void allocate_resource (unsigned int type) {
   RoadMapResource *res = &Resources[type];

   res->hash = roadmap_hash_new (ResourceName[type], BLOCK_SIZE);

   res->slots = malloc (BLOCK_SIZE * sizeof (*res->slots));
   res->max = BLOCK_SIZE;
}


static void *load_resource (unsigned int type, unsigned int flags,
                            const char *name, int *mem) {

   char *path;

   switch (type) {
   case RES_BITMAP:
      path = roadmap_path_join (roadmap_path_user(), "icons");
      *mem = 0;

      return roadmap_canvas_load_image (path, name);
   }

   return NULL;
}


static void *find_resource (unsigned int type, const char *name) {
   int hash;
   int i;
   RoadMapResource *res = &Resources[type];

   if (!res->count) return NULL;
   
   hash = roadmap_hash_string (name);

   for (i = roadmap_hash_get_first (res->hash, hash);
        i >= 0;
        i = roadmap_hash_get_next (res->hash, i)) {

      if (!strcmp(name, res->slots[i].name)) {
         
         return res->slots[i].data;
      }
   }

   return NULL;
}


void *roadmap_res_get (unsigned int type, unsigned int flags,
                       const char *name) {

   void *data;
   int mem;
   RoadMapResource *res = &Resources[type];

   data = find_resource (type, name);

   if (data) return data;

   if (!Resources[type].count) allocate_resource (type);

   //TODO implement grow (or old deletion)
   if (Resources[type].count == Resources[type].max) return NULL;

   data = load_resource (type, flags, name, &mem);

   if (!data) return NULL;

   res->slots[res->count].data = data;
   res->slots[res->count].name = name;

   res->used_mem += mem;

   roadmap_hash_add (res->hash, roadmap_hash_string (name), res->count);
   res->count++;

   return data;
}

