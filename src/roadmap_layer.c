/* roadmap_layer.c - Layer management: declutter, filtering, etc..
 *
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_layer.h.
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_canvas.h"

#include "roadmap_layer.h"


/* The following table is a hardcoded default we use until the
 * "category" tables are created.
 */
static char *RoadMapDefaultCategoryTable[] = {
   "Freeways",
   "Ramps",
   "Highways",
   "Streets",
   "Trails",
   "Parks",
   "Hospitals",
   "Airports",
   "Stations",
   "Malls",
   "Shore",
   "Rivers",
   "Lakes",
   "Sea"
};


/* CATEGORIES.
 * A category represents a group of map objects that are represented
 * using the same pen (i.e. same color, thickness) and the same
 * graphical primitive (line, polygon, etc..).
 */
static struct roadmap_canvas_category {

    const char *name;
    
    int class_index;
    int visible;
    
    RoadMapConfigDescriptor declutter;
    RoadMapConfigDescriptor thickness;

    RoadMapPen pen;

} *RoadMapCategory;

static int RoadMapCategoryCount = 0;


/* CLASSES.
 * A class represent a group of categories that have the same basic
 * properties. For example, the "Road" class can be searched for an
 * address.
 */
typedef struct {

   char *name;

   int   count;
   char  category[128];

} RoadMapClass;

static RoadMapClass RoadMapClasses[] = {
    {"Area",    0, {0}},
    {"Road",    0, {0}},
    {"Feature", 0, {0}},
    {NULL,      0, {0}}
};

static RoadMapClass *RoadMapLineClass = &(RoadMapClasses[1]);
static RoadMapClass *RoadMapRoadClass = &(RoadMapClasses[1]);


int roadmap_layer_is_visible (int layer) {
    
    struct roadmap_canvas_category *category = RoadMapCategory + layer;

    if (! category->visible) {
        return 0;
    }
    return roadmap_math_declutter
                (roadmap_config_get_integer (&category->declutter));
}


int roadmap_layer_visible_roads (int *layers, int size) {
    
    int i;
    int count = -1;
    
    --size; /* To match our boundary check. */
    
    for (i = RoadMapRoadClass->count - 1; i >= 0; --i) {

        int category = RoadMapRoadClass->category[i];

        if (roadmap_layer_is_visible (category)) {
            if (count >= size) break;
            layers[++count] = category;
        }
    }
    
    return count + 1;
}


int roadmap_layer_visible_lines (int *layers, int size) {
   
    int i;
    int j;
    int count = -1;
    
    --size; /* To match our boundary check. */
    
    for (i = 0; RoadMapLineClass[i].name != NULL; ++i) {

        RoadMapClass *this_class = RoadMapLineClass + i;

        for (j = this_class->count - 1; j >= 0; --j) {

            int category = this_class->category[j];

            if (roadmap_layer_is_visible (category)) {
                if (count >= size) goto done;
                layers[++count] = category;
            }
        }
    }
    
done:
    return count + 1;
}


void roadmap_layer_select (int layer) {

    roadmap_canvas_select_pen (RoadMapCategory[layer].pen);
}


void roadmap_layer_adjust (void) {
    
    int i;
    struct roadmap_canvas_category *category;
    
    for (i = RoadMapCategoryCount; i > 0; --i) {
        
        if (roadmap_layer_is_visible(i)) {

            category = RoadMapCategory + i;

            roadmap_canvas_select_pen (category->pen);

            roadmap_canvas_set_thickness
                (roadmap_math_thickness
                    (roadmap_config_get_integer (&category->thickness)));
        }
    }
}


void roadmap_layer_initialize (void) {
    
    int i;
    RoadMapConfigDescriptor descriptor = ROADMAP_CONFIG_ITEM_EMPTY;


    if (RoadMapCategory != NULL) return;
    
    
    RoadMapCategoryCount =
       sizeof(RoadMapDefaultCategoryTable) / sizeof(char *);

    RoadMapCategory =
        calloc (RoadMapCategoryCount + 1, sizeof(*RoadMapCategory));

    roadmap_check_allocated(RoadMapCategory);

    for (i = 1; i <= RoadMapCategoryCount; ++i) {

        struct roadmap_canvas_category *category = RoadMapCategory + i;

        const char *name = RoadMapDefaultCategoryTable[i-1];
        const char *class_name;
        const char *color;

        RoadMapClass *p;


        category->name = name;
        category->visible = 1;
        
        descriptor.category = name;
        descriptor.name = "Class";
        descriptor.reference = NULL;

        roadmap_config_declare ("schema", &descriptor, "");
        class_name = roadmap_config_get (&descriptor);

        descriptor.name = "Color";
        descriptor.reference = NULL;

        roadmap_config_declare ("schema", &descriptor, "black");
        color = roadmap_config_get (&descriptor);
        
        category->thickness.category = name;
        category->thickness.name     = "Thickness";
        roadmap_config_declare
            ("schema", &category->thickness, "1");
        
        category->declutter.category = name;
        category->declutter.name     = "Declutter";
        roadmap_config_declare
            ("schema", &category->declutter, "20248000000");

        for (p = RoadMapClasses; p->name != NULL; ++p) {

            if (strcasecmp (class_name, p->name) == 0) {
                p->category[p->count++] = i;
                category->class_index = (int) (p - RoadMapClasses);
                break;
            }
        }

        category->pen = roadmap_canvas_create_pen (name);

        roadmap_canvas_set_thickness
            (roadmap_config_get_integer (&category->thickness));

        if (color != NULL && *color > ' ') {
            roadmap_canvas_set_foreground (color);
        }
    }
}

