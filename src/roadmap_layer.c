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
#include "editor/editor_screen.h"


static RoadMapConfigDescriptor RoadMapConfigStylePretty =
                        ROADMAP_CONFIG_ITEM("Style", "Use Pretty Lines");

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
 * Some categories may be displayed using more than one pen.
 * For example, a freeway could be displayed using 3 pens:
 * border, fill, center divider.
 */
#define ROADMAP_LAYER_PENS    4

static struct roadmap_canvas_category {

    const char *name;
    
    int class_index;
    int visible;
    int pen_count;
    
    RoadMapConfigDescriptor declutter;
    RoadMapConfigDescriptor thickness;

    RoadMapPen pen[ROADMAP_LAYER_PENS];
    int delta_thickness[ROADMAP_LAYER_PENS];
    int in_use[ROADMAP_LAYER_PENS];

} *RoadMapCategory;

static int RoadMapCategoryCount = 0;
static int RoadMapMaxUsedPen = 1;


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


int roadmap_layer_max_pen(void) {
   if (! roadmap_config_match (&RoadMapConfigStylePretty, "yes")) {
      return 1;
   }
   return RoadMapMaxUsedPen;
}

int roadmap_layer_is_visible (int layer) {
    
    struct roadmap_canvas_category *category = RoadMapCategory + layer;

    if (! category->visible) {
        return 0;
    }
    return roadmap_math_declutter
                (roadmap_config_get_integer (&category->declutter));
}


int roadmap_layer_all_roads (int *layers, int size) {
    
    int i;
    int count = -1;
    
    --size; /* To match our boundary check. */
    
    for (i = RoadMapRoadClass->count - 1; i >= 0; --i) {

        int category = RoadMapRoadClass->category[i];
        if (count >= size) break;
        layers[++count] = category;
    }
    
    return count + 1;
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


int roadmap_layer_visible_lines (int *layers, int size, int pen_type) {

    int i;
    int j;
    int count = -1;
    
    --size; /* To match our boundary check. */
    
    for (i = 0; RoadMapLineClass[i].name != NULL; ++i) {

        RoadMapClass *this_class = RoadMapLineClass + i;

        for (j = this_class->count - 1; j >= 0; --j) {

            int category = this_class->category[j];

            if (pen_type >= RoadMapCategory[category].pen_count) continue;
            if (! RoadMapCategory[category].in_use[pen_type]) continue;

            if (roadmap_layer_is_visible (category)) {
                if (count >= size) goto done;
                layers[++count] = category;
            }
        }
    }
    
done:
    return count + 1;
}


void roadmap_layer_select (int layer, int pen_type) {

   roadmap_canvas_select_pen (RoadMapCategory[layer].pen[pen_type]);
}


RoadMapPen roadmap_layer_get_pen (int layer, int pen_type) {

   if (!roadmap_layer_is_visible (layer)) return NULL;
   
   if (!RoadMapCategory[layer].in_use[pen_type]) return NULL;
   
   return RoadMapCategory[layer].pen[pen_type];
}


void roadmap_layer_adjust (void) {
    
    int i;
    int j;
    int thickness;
    int future_thickness;
    struct roadmap_canvas_category *category;
    
    for (i = RoadMapCategoryCount; i > 0; --i) {
        
        if (roadmap_layer_is_visible(i)) {

            category = RoadMapCategory + i;

            thickness =
               roadmap_math_thickness
                  (roadmap_config_get_integer (&category->thickness),
                   roadmap_config_get_integer (&category->declutter),
                   category->pen_count > 1);

            editor_adjust_layer (i, thickness, category->pen_count);

            /* As a matter of taste, I do dislike roads with a filler
             * of 1 pixel. Lets force at least a filler of 2.
             */
            future_thickness = thickness;

            for (j = 1; j < category->pen_count; ++j) {

               if (category->delta_thickness[j] > 0) break;

               future_thickness =
                  future_thickness + category->delta_thickness[j];
               if (future_thickness == 1) {
                  thickness += 1;
               }
            }

            roadmap_canvas_select_pen (category->pen[0]);
            roadmap_canvas_set_thickness (thickness);
            category->in_use[0] = 1;

            for (j = 1; j < category->pen_count; ++j) {

               /* The previous thickness was already the minimum:
                * the pens that follow should not be used.
                */
               if (thickness <= 1) {
                  category->in_use[j] = 0;
                  continue;
               }

               if (category->delta_thickness[j] < 0) {

                  thickness += category->delta_thickness[j];

               } else {
                  /* Don't end with a road mostly drawn with the latter
                   * pen.
                   */
                  if (category->delta_thickness[j] >= thickness / 2) {
                     category->in_use[j] = 0;
                     thickness = 1;
                     continue;
                  }
                  thickness = category->delta_thickness[j];
               }

               /* If this pen is not visible, there is no reason
                * to draw it.
                */
               if (thickness < 1) {
                  category->in_use[j] = 0;
                  continue;
               }

               roadmap_canvas_select_pen (category->pen[j]);
               roadmap_canvas_set_thickness (thickness);
               category->in_use[j] = 1;
            }
        }
    }
}


void roadmap_layer_initialize (void) {
    
    int i;
    int j;
    RoadMapConfigDescriptor descriptor = ROADMAP_CONFIG_ITEM_EMPTY;


    if (RoadMapCategory != NULL) return;
    

    RoadMapCategoryCount =
       sizeof(RoadMapDefaultCategoryTable) / sizeof(char *);

    RoadMapCategory =
        calloc (RoadMapCategoryCount + 1, sizeof(*RoadMapCategory));

    roadmap_check_allocated(RoadMapCategory);

    roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStylePretty, "yes", "no", NULL);

    for (i = 1; i <= RoadMapCategoryCount; ++i) {

        struct roadmap_canvas_category *category = RoadMapCategory + i;

        const char *name = RoadMapDefaultCategoryTable[i-1];
        const char *class_name;
        const char *color[ROADMAP_LAYER_PENS];

        int  thickness;
        int  other_pens_length = strlen(name) + 64;
        char *other_pens = malloc(other_pens_length);

        RoadMapClass *p;


        category->name = name;
        category->visible = 1;


        /* Retrieve the class of the category. */

        descriptor.category = name;
        descriptor.name = "Class";
        descriptor.reference = NULL;

        roadmap_config_declare ("schema", &descriptor, "");
        class_name = roadmap_config_get (&descriptor);

        for (p = RoadMapClasses; p->name != NULL; ++p) {

            if (strcasecmp (class_name, p->name) == 0) {
                p->category[p->count++] = i;
                category->class_index = (int) (p - RoadMapClasses);
                break;
            }
        }


        /* Retrieve the category thickness & declutter. */

        category->thickness.category = name;
        category->thickness.name     = "Thickness";
        roadmap_config_declare
            ("schema", &category->thickness, "1");

        thickness = roadmap_config_get_integer (&category->thickness);

        category->declutter.category = name;
        category->declutter.name     = "Declutter";
        roadmap_config_declare
            ("schema", &category->declutter, "20248000000");


        /* Retrieve the first pen's color (mandatory). */

        descriptor.name = "Color";
        descriptor.reference = NULL;

        roadmap_config_declare ("schema", &descriptor, "black");
        color[0] = roadmap_config_get (&descriptor);


        /* Retrieve the category's other colors (optional). */

        for (j = 1; j < ROADMAP_LAYER_PENS; ++j) {

           snprintf (other_pens, other_pens_length, "Delta%d", j);

           descriptor.name = other_pens;
           descriptor.reference = NULL;

           roadmap_config_declare ("schema", &descriptor, "0");
           category->delta_thickness[j] =
              roadmap_config_get_integer (&descriptor);

           if (category->delta_thickness[j] == 0) break;

           snprintf (other_pens, other_pens_length, "Color%d", j);

           descriptor.name = other_pens;
           descriptor.reference = NULL;

           roadmap_config_declare ("schema", &descriptor, "");
           color[j] = roadmap_config_get (&descriptor);

           if (*color[j] == 0) break;
        }
        category->pen_count = j;
        if (j > RoadMapMaxUsedPen) RoadMapMaxUsedPen = j;


        /* Create all necessary pens. */

        category->pen[0] = roadmap_canvas_create_pen (name);

        roadmap_canvas_set_thickness
            (roadmap_config_get_integer (&category->thickness));

        if (color[0] != NULL && *(color[0]) > ' ') {
            roadmap_canvas_set_foreground (color[0]);
        }

        for (j = 1; j < category->pen_count; ++j) {

           snprintf (other_pens, other_pens_length, "%s%d", name, j);

           category->pen[j] = roadmap_canvas_create_pen (other_pens);

           if (category->delta_thickness[j] < 0) {
              thickness += category->delta_thickness[j];
           } else {
              thickness = category->delta_thickness[j];
           }
           roadmap_canvas_set_thickness (thickness);
           roadmap_canvas_set_foreground (color[j]);
        }
    }
}


void roadmap_layer_get_categories_names (char **names[], int *count) {

   *names = RoadMapDefaultCategoryTable;
   *count = 
       sizeof(RoadMapDefaultCategoryTable) / sizeof(char *);
}

