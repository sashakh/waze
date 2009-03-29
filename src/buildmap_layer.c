/*
 * LICENSE:
 *
 *   Copyright 2003 Pascal F. Martin
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

/**
 * @file
 * @brief Layer management: list, identify.
 *
 * This module is only concerned with retrieving which layers are
 * defined for a given class. It is used by the data source decoders
 * to filter out unwanted layers from the data source.
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap_config.h"

#include "buildmap.h"
#include "buildmap_layer.h"


#define BUILDMAP_LAYER_MAX   1024

static char *BuildMapLineLayerList[BUILDMAP_LAYER_MAX];
static int   BuildMapLineLayerCount = 0;

static char *BuildMapPolygonLayerList[BUILDMAP_LAYER_MAX];
static int   BuildMapPolygonLayerCount = 0;

/**
 * @brief look up the layer number, for a given layer name
 * @param name a layer name (string)
 * @return numeric representation of a layer
 */
int buildmap_layer_get (const char *name) {

   int i;

   for (i = 0; i < BuildMapLineLayerCount; ++i) {
      if (strcasecmp(BuildMapLineLayerList[i], name) == 0) {
         return i+1;
      }
   }

   for (i = 0; i < BuildMapPolygonLayerCount; ++i) {
      if (strcasecmp(BuildMapPolygonLayerList[i], name) == 0) {
         return BuildMapLineLayerCount + i + 1;
      }
   }

   return 0;
}


/* Initialization code. ------------------------------------------- */
/**
 * @brief
 * @param text
 * @param field
 * @param max
 * @return
 */
static int buildmap_layer_split (char *text, char *field[], int max) {

   int   i;
   char *p;

   field[0] = text;
   p = strchr (text, ' ');

   for (i = 1; p != NULL && *p != 0; ++i) {

      *p = 0;
      if (i >= max) return -1;

      field[i] = ++p;
      p = strchr (p, ' ');
   }
   return i;
}

/**
 * @brief
 * @param config
 * @param id
 * @param args
 * @param max
 * @return
 */
static int buildmap_layer_decode (const char *config,
                                  const char *id, char**args, int max) {

   int   count;
   char *buffer;

   if (max <= 0) {
      buildmap_fatal (0, "too many layers in class file %s",
                         roadmap_config_file(config));
   }

   /* We must allocate a new storage because we are going to split
    * the string, thus modify it.
    */
   buffer = strdup(roadmap_config_get_from (config, "Class", id));
   if (buffer == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   count = buildmap_layer_split (buffer, args, max);
   if (count <= 0) {
      buildmap_fatal (0, "invalid layer list %s in class file %s",
                         id,
                         roadmap_config_file(config));
   }

   return count;
}

/**
 * @brief load a class file into memory
 * @param class_file path to the class file to load, usually "default/All" .
 */
void buildmap_layer_load (const char *class_file) {

    const char *config;


    roadmap_config_initialize ();

    config = roadmap_config_new (class_file, 0);
    if (config == NULL) {
       buildmap_fatal (0, "cannot access class file %s", class_file);
    }

    BuildMapLineLayerCount =
       buildmap_layer_decode
          (config, "Lines", BuildMapLineLayerList, BUILDMAP_LAYER_MAX);

    if (BuildMapLineLayerCount <= 0) {
       buildmap_fatal (0, "cannot decode line layers in class %s", class_file);
    }

    BuildMapPolygonLayerCount =
       buildmap_layer_decode
          (config, "Polygons", BuildMapPolygonLayerList, BUILDMAP_LAYER_MAX);

    if (BuildMapPolygonLayerCount <= 0) {
       buildmap_fatal
          (0, "cannot decode polygon layers in class %s", class_file);
    }
}

