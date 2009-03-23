/*
 * LICENSE:
 *
 *   Copyright 2003 Pascal F. Martin
 *   Copyright (c) 2008, 2009, Danny Backx.
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
 * @brief Layer management: declutter, filtering, etc..
 *
 * This source file has a problem : it should be split into one that provides basic
 * layer functionality, and another one that cause roadmap_canvas.c to be pulled in.
 * Right now this makes it impossible to call a roadmap_layer function from
 * roadmap_line.c (and roadmap_line_get_layer() would like to use roadmap_layer_road_last()).
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_path.h"
#include "roadmap_config.h"
#include "roadmap_canvas.h"
#include "roadmap_input.h"
#include "roadmap_plugin.h"

#include "roadmap_layer.h"


/**
 * @brief This is the maximum number of layers PER CLASS. As there is no limit on
 * the number of classes, the total number of layers is actually unlimited.
 * We could have implemented support for an unlimited number of layers per
 * class, but who wants a thousand layers per map, anyway?
 * Having this limit makes life easier for everyone, and the downside is nil.
 * Note that this has no impact on the API of roadmap_layer.
 */
#define ROADMAP_MAX_LAYERS           1024

/**
 * @brief There is a maximum number of navigation modules that can be registered.
 * This is not really a limitation for now, since there is currently only
 * one navigation mode (car) and any other would be hiking, boat, plane,
 * spaceship... i.e. much less than the limit here.
 * The size cannot be changed easily: the code uses bitmaps...
 */
#define ROADMAP_MAX_NAVIGATION_MODES   (8*sizeof(int))

/**
 * @brief Some layers may be displayed using more than one pen.
 * For example, a freeway could be displayed using 3 pens:
 * border, fill, center divider.
 */
#define ROADMAP_MAX_LAYER_PENS    4


static RoadMapConfigDescriptor RoadMapConfigStylePretty =
                        ROADMAP_CONFIG_ITEM("Style", "Use Pretty Lines");

static RoadMapConfigDescriptor RoadMapConfigSkin =
                        ROADMAP_CONFIG_ITEM("Display", "Skin");

static const char   *RoadMapNavigationMode[ROADMAP_MAX_NAVIGATION_MODES];
static unsigned int  RoadMapNavigationModeCount = 0;

/* ----------------------------------------------------------- */
/**
 * @brief CLASSES
 *
 * A class represent a group of categories that have the same basic
 * properties or which come from a single source. For example, the "Road"
 * class can be searched for an address. Note however that RoadMap never
 * assumes that there is any implicit property common to the layers
 * of the same class: the organization of layers into classes is only
 * a map creator's convention.
 *
 * The lines and the polygons arrays are consecutive to each other,
 * i.e. a single array where the lines are listed first, so that we
 * can manage them as a single list, using a single index.
 */
typedef struct roadmap_layer_class {

   const char *name;
   struct roadmap_layer_class *next;

   const char *before;
   const char *after;

   short lines_count;
   short polygons_count;
   struct roadmap_layer_record *layers;

   int predecessor_count;
   struct roadmap_layer_class **predecessor;

} RoadMapClass;

static RoadMapClass *RoadMapLayerCurrentClass;


/* LAYERS. ------------------------------------------------------------ */
/**
 * @brief LAYERS
 *
 * A layer represents a group of map objects that are represented
 * using the same pen (i.e. same color, thickness) and the same
 * graphical primitive (line, polygon, etc..).
 */
typedef struct roadmap_layer_record {

    const char *name;
    
    RoadMapClass *class;

    char in_use[ROADMAP_MAX_LAYER_PENS];

    RoadMapConfigDescriptor declutter;
    RoadMapConfigDescriptor thickness;

    unsigned int pen_count;
    RoadMapPen pen[ROADMAP_MAX_LAYER_PENS];
    int delta_thickness[ROADMAP_MAX_LAYER_PENS];

    int navigation_modes;			/**< bitwise OR of the navigation modes supported */
    RoadMapConfigDescriptor speed;		/**< max speed */

} RoadMapLayer;

static unsigned int RoadMapMaxUsedPen = 1;
static unsigned int RoadMapMaxDefinedLayers = 1;


/* SETS. -------------------------------------------------------------- */
/**
 * @brief SETS
 *
 * A set represent a full configuration of classes. One set only is
 * active at any given time. The sets are always predefined (for now?).
 */
typedef struct {

   char *name;

   int class_count;
   RoadMapClass *classes;

} RoadMapSet;

static RoadMapSet RoadMapLayerSet[] = {
   {"default", 0, NULL},
   {"day",     0, NULL},
   {"night",   0, NULL},
   {NULL,      0, NULL}
};
static RoadMapSet *RoadMapLayerActiveSet = NULL;

/* PEN ATTRIBUTES */
static RoadMapLayerPenAttribute RoadMapLayerPenSetupTable[] = {
  /*
   *  Thickness and Delta are a bit special, for now leave them out
   */
  /* {"Thickness", ROADMAP_STYLE_TYPE_INT, "",  roadmap_canvas_set_thickness},*/
  /* {"Delta", ROADMAP_STYLE_TYPE_INT, "",  roadmap_canvas_set_thickness},     */
  {"Color", ROADMAP_STYLE_TYPE_STRING, "#000000", roadmap_canvas_set_foreground},
  {"Style", ROADMAP_STYLE_TYPE_STRING, "", roadmap_canvas_set_linestyle},
#if defined(ROADMAP_ADVANCED_STYLE)
  {"LineCap", ROADMAP_STYLE_TYPE_STRING, "", roadmap_canvas_set_linecap },
  {"LineJoin", ROADMAP_STYLE_TYPE_STRING, "", roadmap_canvas_set_linejoin },
  {"Opacity", ROADMAP_STYLE_TYPE_INT, "", roadmap_canvas_set_opacity },
  {"BrushColor", ROADMAP_STYLE_TYPE_STRING, "", roadmap_canvas_set_brush_color },
  {"BrushStyle", ROADMAP_STYLE_TYPE_STRING, "", roadmap_canvas_set_brush_style },
  {"isBackground", ROADMAP_STYLE_TYPE_INT, "", roadmap_canvas_set_brush_isbackground },
  {"LabelFontName", ROADMAP_STYLE_TYPE_STRING, "", roadmap_canvas_set_label_font_name },
  {"LabelFontSize", ROADMAP_STYLE_TYPE_INT, "", roadmap_canvas_set_label_font_size },
  {"LabelFontStyle", ROADMAP_STYLE_TYPE_INT, "", roadmap_canvas_set_label_font_style },
  {"LabelFontColor", ROADMAP_STYLE_TYPE_STRING, "", roadmap_canvas_set_label_font_color },
  {"LabelFontWeight", ROADMAP_STYLE_TYPE_STRING, "", roadmap_canvas_set_label_font_weight },
  {"LabelFontSpacing", ROADMAP_STYLE_TYPE_INT, "", roadmap_canvas_set_label_font_spacing },
  {"LabelBufferColor", ROADMAP_STYLE_TYPE_STRING, "", roadmap_canvas_set_label_buffer_color },
  {"LabelBufferSize", ROADMAP_STYLE_TYPE_INT, "", roadmap_canvas_set_label_buffer_size },
#endif /* ADVANCED STYLE */
};


static RoadMapSet *roadmap_layer_find_set (const char *set) {

   RoadMapSet *config;


   if (set == NULL) return NULL;

   for (config = RoadMapLayerSet; config->name != NULL; ++config) {
      if (strcasecmp(set, config->name) == 0) return config;
   }

   return NULL;
}


static RoadMapClass *roadmap_layer_find_class(RoadMapClass *list,
                                              const char *name) {

   RoadMapClass *class;

   for (class = list; class != NULL; class = class->next) {
      if (strcasecmp (name, class->name) == 0) break;
   }

   return class;
}


static int roadmap_layer_is_visible (RoadMapLayer *layer) {
    
    return roadmap_math_declutter
                (roadmap_config_get_integer (&layer->declutter));
}


unsigned int roadmap_layer_max_defined(void) {

   return RoadMapMaxDefinedLayers + 1; /* To be safe, allocate a bit more. */
}


unsigned int roadmap_layer_max_pen(void) {

   if (! roadmap_config_match (&RoadMapConfigStylePretty, "yes")) {
      return 1;
   }
   return RoadMapMaxUsedPen;
}


/**
 * @brief query the layers navigable for a given navigation mode (e.g. car)
 * @param mode
 * @param layers
 * @param size
 * @return
 */
int roadmap_layer_navigable (int mode, int *layers, int size)
{
    int i;
    int mask = 1 << mode;
    int count = 0;

    RoadMapLayer *layer;
    

    if (RoadMapLayerCurrentClass == NULL) return 0;

    for (i = 1; i <= RoadMapLayerCurrentClass->lines_count; ++i) {

        layer = RoadMapLayerCurrentClass->layers + i - 1;

        if (layer->navigation_modes & mask) {

           if (roadmap_layer_is_visible (layer)) {
              if (count >= size) break;
              layers[count++] = i;
           }
        }
    }
    
    return count;
}

int roadmap_layer_visible_lines (int *layers, int size, unsigned int pen_index)
{
    int i;
    int count = 0;

    RoadMapLayer *layer;


    if (RoadMapLayerCurrentClass == NULL) return 0;

    for (i = RoadMapLayerCurrentClass->lines_count; i > 0; --i) {

        layer = RoadMapLayerCurrentClass->layers + i - 1;

        if (pen_index >= layer->pen_count) continue;
        if (! layer->in_use[pen_index]) continue;

        if (roadmap_layer_is_visible (layer)) {
           if (count >= size) goto done;
           layers[count++] = i;
        }
    }

done:
    return count;
}


void roadmap_layer_adjust (void) {
    
    int i;
    int thickness;
    int future_thickness;
    unsigned int pen_index;
    RoadMapLayer *layer;


    if (RoadMapLayerCurrentClass == NULL) return;

    for (i = RoadMapLayerCurrentClass->lines_count - 1; i >= 0; --i) {

       layer = RoadMapLayerCurrentClass->layers + i;

       if (roadmap_layer_is_visible(layer)) {

            thickness =
               roadmap_math_thickness
                  (roadmap_config_get_integer (&layer->thickness),
                   roadmap_config_get_integer (&layer->declutter),
                   layer->pen_count > 1);

            roadmap_plugin_adjust_layer (i, thickness, layer->pen_count);

            /* As a matter of taste, I do dislike roads with a filler
             * of 1 pixel. Lets force at least a filler of 2.
             */
            future_thickness = thickness;

            for (pen_index = 1; pen_index < layer->pen_count; ++pen_index) {

               if (layer->delta_thickness[pen_index] > 0) break;

               future_thickness =
                  future_thickness + layer->delta_thickness[pen_index];
               if (future_thickness == 1) {
                  thickness += 1;
               }
            }

            if (thickness > 0) {
               roadmap_canvas_select_pen (layer->pen[0]);
               roadmap_canvas_set_thickness (thickness);
            }

            layer->in_use[0] = 1;

            for (pen_index = 1; pen_index < layer->pen_count; ++pen_index) {

               /* The previous thickness was already the minimum:
                * the pens that follow should not be used.
                */
               if (thickness <= 1) {
                  layer->in_use[pen_index] = 0;
                  continue;
               }

               if (layer->delta_thickness[pen_index] < 0) {

                  thickness += layer->delta_thickness[pen_index];

               } else {
                  /* Don't end with a road mostly drawn with the latter
                   * pen.
                   */
                  if (layer->delta_thickness[pen_index] >= thickness / 2) {
                     layer->in_use[pen_index] = 0;
                     thickness = 1;
                     continue;
                  }
                  thickness = layer->delta_thickness[pen_index];
               }

               /* If this pen is not visible, there is no reason
                * to draw it.
                */
               if (thickness < 1) {
                  layer->in_use[pen_index] = 0;
                  continue;
               }

               roadmap_canvas_select_pen (layer->pen[pen_index]);
               roadmap_canvas_set_thickness (thickness);
               layer->in_use[pen_index] = 1;
            }
        }
    }
}


/**
 * @brief
 * @param layer
 * @param pen_index
 * @return
 */
RoadMapPen roadmap_layer_get_pen (int layer, unsigned int pen_index) {

   int total;

   if (RoadMapLayerCurrentClass == NULL) return NULL;

   total = RoadMapLayerCurrentClass->polygons_count
                  + RoadMapLayerCurrentClass->lines_count;

   if (layer >= 1 && layer <= total && pen_index < RoadMapMaxUsedPen) {

      RoadMapLayer *this_layer = RoadMapLayerCurrentClass->layers + layer - 1;

      if (!roadmap_layer_is_visible (this_layer)) return NULL;

      if (!this_layer->in_use[pen_index]) return NULL;

      return this_layer->pen[pen_index];
   }

   return NULL;
}


void roadmap_layer_class_first (void) {

   RoadMapLayerCurrentClass = RoadMapLayerActiveSet->classes;
}

void roadmap_layer_class_next (void) {

   if (RoadMapLayerCurrentClass != NULL) {
      RoadMapLayerCurrentClass = RoadMapLayerCurrentClass->next;
   }
}

int  roadmap_layer_select_class (const char *name) {

   RoadMapClass *selected =
      roadmap_layer_find_class (RoadMapLayerActiveSet->classes, name);

   if (selected == NULL) return 0;

   RoadMapLayerCurrentClass = selected;
   return 1;
}


const char *roadmap_layer_class_name (void) {

   if (RoadMapLayerCurrentClass == NULL) {
      return NULL;
   }
   return RoadMapLayerCurrentClass->name;
}


int roadmap_layer_names (const char *names[], int max) {

   short i;
   short total = RoadMapLayerCurrentClass->lines_count
                    + RoadMapLayerCurrentClass->polygons_count;


   if (total > max) {
      roadmap_log (ROADMAP_FATAL, "unsufficient space");
   }

   for (i = total - 1; i >= 0; --i) {
      names[i] = RoadMapLayerCurrentClass->layers[i].name;
   }

   return total;
}


void roadmap_layer_select_set (const char *name) {

   static RoadMapSet *default_set = NULL;

   RoadMapSet *set;


   if (default_set == NULL) {
      default_set = roadmap_layer_find_set ("default");
   }

   set = roadmap_layer_find_set (name);
   if (set == NULL) {

      roadmap_log (ROADMAP_ERROR, "invalid class set '%s'", name);
      set = default_set;

   } else if (set->classes == NULL) {

      set = default_set;
   }

   RoadMapLayerActiveSet = set;
   RoadMapLayerCurrentClass = set->classes;
}


/* Initialization code. ------------------------------------------- */

/**
 * @brief
 * @param config
 * @param category
 * @param id
 * @param args
 * @param max
 * @return
 */
static int roadmap_layer_decode (const char *config,
                                 const char *category, const char *id,
                                 char**args, int max) {

   int   count;
   char *buffer;
   const char *value = roadmap_config_get_from (config, category, id);

   if (value == NULL) return 0;

   /* We must allocate a new storage because we are going to split
    * the string, thus modify it.
    */
   buffer = strdup(value);
   roadmap_check_allocated(buffer);

   count = roadmap_input_split (buffer, ' ', args, max);
   if (count <= 0) {
      roadmap_log (ROADMAP_FATAL,
                   "invalid list %s.%s in class file %s",
                   category, id, roadmap_config_file(config));
   }

   return count;
}


/* This function converts each "after" string into a predecessor list. */

static void roadmap_layer_convert_after (RoadMapSet *set) {

   RoadMapClass *class;

   char *copy;
   int   i, j;
   int   cursor;
   int   count;
   char *list[ROADMAP_MAX_LAYERS];


   for (class = set->classes; class!= NULL; class = class->next) {

      class->predecessor = calloc (set->class_count, sizeof(RoadMapClass *));
      roadmap_check_allocated(class->predecessor);

      if (class->after == NULL || class->after[0] == 0) {
         class->predecessor_count = 0;
         continue;
      }

      copy = strdup(class->after);
      count = roadmap_input_split (copy, ' ', list, ROADMAP_MAX_LAYERS);

      for (cursor = 0, i = 0; cursor < count && i < count; ++i) {
         class->predecessor[cursor] =
            roadmap_layer_find_class (set->classes, list[i]);
         for (j = cursor - 1; j >= 0; --j) {
            if (class->predecessor[j] == class->predecessor[cursor]) {
               roadmap_log (ROADMAP_ERROR,
                            "duplicated 'after' class %s in class %s",
                            list[i], class->name);
               class->predecessor[cursor] = NULL;
               break;
            }
         }
         if (class->predecessor[cursor] != NULL) ++cursor;
      }
      class->predecessor_count = cursor;
      free(copy);
   }
}


/* This function converts each "before" string into the matching 'after'
 * conditions. The rule implemented here is that if A is before B,
 * then B is after A, so A must be added to B's predecessor list.
 */
static void roadmap_layer_convert_before (RoadMapSet *set) {

   RoadMapClass *class;
   RoadMapClass *after;

   char *copy;
   int   i, j;
   int   count;
   char *list[ROADMAP_MAX_LAYERS];

   for (class = set->classes; class!= NULL; class = class->next) {

      if (class->before == NULL || class->before[0] == 0) continue;

      copy = strdup(class->before);
      count = roadmap_input_split (copy, ' ', list, ROADMAP_MAX_LAYERS);

      for (i = 0; i < count; ++i) {

         after = roadmap_layer_find_class (set->classes, list[i]);
         if (after != NULL) {

            /* The before item is a valid class: check if this other
             * class does not already list the current class.
             */
            for (j = after->predecessor_count - 1; j >= 0; --j) {
               if (after->predecessor[j] == class) break;
            }
            if (j < 0) {
               after->predecessor[after->predecessor_count++] = class;
            }
         }
      }
   }
}


static void roadmap_layer_sort (RoadMapSet *set) {

   int i, j;
   int loop = 0;
   int count = 0;
   RoadMapClass *sorted[ROADMAP_MAX_LAYERS];


   roadmap_layer_convert_after (set);
   roadmap_layer_convert_before (set);


   while (count < set->class_count) {

      RoadMapClass *class;

      if (loop++ > ROADMAP_MAX_LAYERS) {
         roadmap_log (ROADMAP_FATAL,
                      "infinite loop detected in "
                         "the classes 'before' and 'after' conditions");
      }

      for (class = set->classes; class != NULL; class = class->next) {

         /* Search if any predecessor has not been sorted yet. */

         for (i = class->predecessor_count - 1; i >= 0; --i) {

            for (j = count - 1; j >= 0; --j) {
               if (class->predecessor[i] == sorted[j]) {
                  break;
               }
            }
            if (j < 0) {
               /* This class has one predecessor that is not yet sorted.
                * We need to sort it later on.
                */
               break;
            }
         }

         if (i < 0) {
            /* All the predecessors of this class have been sorted already,
             * so we can now add this one to the sorted list too.
             */
            sorted[count++] = class;
         }
      }
   }

   /* Rebuild the class list according to the sorted order. */

   set->classes = NULL;
   for (i = count - 1; i >= 0; --i) {
      sorted[i]->next = set->classes;
      set->classes = sorted[i];
   }
}

/**
 * @brief load a "class file"
 * @param class_file which file to load
 */
static void roadmap_layer_load_file (const char *class_file) {

    int i;
    int j;
    unsigned int pen_index;

    int lines_count;
    int polygons_count;
    char *layers[ROADMAP_MAX_LAYERS];

    RoadMapSet   *set;
    RoadMapClass *new_class;

    const char *class_config = roadmap_config_new (class_file, 0);
    const char *class_name;

    roadmap_log (ROADMAP_WARNING, "roadmap_layer_load_file(%s)", class_file);
    if (class_config == NULL) {
       roadmap_log (ROADMAP_FATAL, "cannot access class file %s", class_file);
    }

    set = roadmap_layer_find_set
             (roadmap_config_get_from (class_config, "Class", "Set"));
    if (set == NULL) {
       set = roadmap_layer_find_set ("default");
    }

    class_name = roadmap_config_get_from (class_config, "Class", "Name");
    if (class_name[0] == 0) {
       roadmap_log (ROADMAP_FATAL, "invalid class file %s", class_file);
    }

    if (roadmap_layer_find_class(set->classes, class_name) != NULL) {
       roadmap_log (ROADMAP_WARNING,
                    "class %s (set %s) redefined in %s",
                    class_name, set->name, class_file);
       return;
    }


    /* We allocate the lines (first) and the polygons (then) consecutive
     * to each other, so that we can manage them as a single list.
     */
    lines_count =
       roadmap_layer_decode
          (class_config, "Class", "Lines", layers, ROADMAP_MAX_LAYERS);
    if (lines_count <= 0) return;

    polygons_count =
       roadmap_layer_decode
          (class_config, "Class", "Polygons",
           layers + lines_count, ROADMAP_MAX_LAYERS - lines_count);
    if (polygons_count <= 0) return;


    /* Create the new class. */

    new_class =
       calloc (sizeof(RoadMapClass) + 
                  ((polygons_count + lines_count) * sizeof(RoadMapLayer)), 1);
    roadmap_check_allocated(new_class);

    roadmap_log (ROADMAP_WARNING, "Class [%s] lines %d polygons %d",
		    class_name, lines_count, polygons_count);

    new_class->name = class_name;
    new_class->lines_count = lines_count;
    new_class->polygons_count = polygons_count;

    new_class->layers = (RoadMapLayer *) (new_class + 1);

    new_class->before = 
       roadmap_config_get_from (class_config, "Class", "Before");

    new_class->after = 
       roadmap_config_get_from (class_config, "Class", "After");


    new_class->next = set->classes;
    set->classes = new_class;

    set->class_count += 1;


    if (RoadMapMaxDefinedLayers < (unsigned int) lines_count + polygons_count) {
       RoadMapMaxDefinedLayers = lines_count + polygons_count;
    }

    int n_of_callbacks = (int) sizeof(RoadMapLayerPenSetupTable) / 
                                        sizeof(RoadMapLayerPenAttribute);
  
    for (i = lines_count + polygons_count - 1; i >= 0; --i) {
        const char *svalues[ROADMAP_MAX_LAYER_PENS][n_of_callbacks];
        int ivalues[ROADMAP_MAX_LAYER_PENS][n_of_callbacks];

        RoadMapLayer *layer = new_class->layers + i;
        RoadMapConfigDescriptor descriptor = ROADMAP_CONFIG_ITEM_EMPTY;
        int  other_pen_length = strlen(layers[i]) + 64;
        static char *other_pen;
        int thickness = 1; /* suppress warning: will set again before use */

        other_pen = realloc(other_pen, other_pen_length);

	roadmap_log(ROADMAP_DEBUG, "Layer %d is %s", i, layers[i]);
        layer->name = layers[i];
        layer->class = new_class;
        layer->navigation_modes = 0;

        /* Retrieve the layer's thickness & declutter. */

        layer->thickness.category = layers[i];
        layer->thickness.name     = "Thickness";
        roadmap_config_declare (class_config, &layer->thickness, "1");
        
        layer->declutter.category = layers[i];
        layer->declutter.name     = "Declutter";
        roadmap_config_declare (class_config, &layer->declutter, "20248000000");

	layer->speed.category = layers[i];
	layer->speed.name = "Speed";
	roadmap_config_declare (class_config, &layer->speed, "120");
         
        for (pen_index = 0; pen_index < ROADMAP_MAX_LAYER_PENS; ++pen_index) {
          /* retrieve delta (thickness was taken above) */
          if (pen_index > 0) {
            const char *image;

             snprintf (other_pen, other_pen_length, "Delta%d", pen_index);

             image =
                roadmap_config_get_from (class_config, layers[i], other_pen);
             if (image == NULL || image[0] == 0) break;

             layer->delta_thickness[pen_index] = atoi(image);
          } 
           
          /* now retrieve other pen attribute values */
          for (j = 0; j < n_of_callbacks; ++j) {
            if (pen_index == 0) {
              snprintf (other_pen, other_pen_length, "%s", RoadMapLayerPenSetupTable[j].name);
            } else {
              snprintf (other_pen, other_pen_length, "%s%d", RoadMapLayerPenSetupTable[j].name, pen_index);
            }
         
            descriptor.category = layers[i];
            descriptor.name = other_pen;
            descriptor.reference = NULL;
            roadmap_config_declare (class_config, &descriptor, RoadMapLayerPenSetupTable[j].default_value);
            switch (RoadMapLayerPenSetupTable[j].type) {
              case ROADMAP_STYLE_TYPE_INT:
                 ivalues[pen_index][j] = roadmap_config_get_integer(&descriptor);
                 break; 
              case ROADMAP_STYLE_TYPE_STRING:
                 svalues[pen_index][j] = roadmap_config_get(&descriptor);
                 break;
            }
          }
        }
        layer->pen_count = pen_index;
        if (pen_index > RoadMapMaxUsedPen) RoadMapMaxUsedPen = pen_index;


        /* Create all necessary pens. */

        if (i >= lines_count) { /* This is a polygon. */
           layer->in_use[0] = 1;
        }

        for (pen_index = 0; pen_index < layer->pen_count; ++pen_index) {
           if (pen_index == 0) {
             sprintf (other_pen, "%s", layers[i]);
           } else {
             snprintf (other_pen, other_pen_length, "%s%d", layers[i], pen_index);
           }
           
           layer->pen[pen_index] = roadmap_canvas_create_pen (other_pen);

           /* set up thickness based on main thickness and delta ones */

           if (pen_index == 0) {
             thickness = roadmap_config_get_integer (&layer->thickness);
             roadmap_canvas_set_thickness (thickness);
           } else {
             if (layer->delta_thickness[pen_index] < 0) {
                thickness += layer->delta_thickness[pen_index];
             } else {
                thickness = layer->delta_thickness[pen_index];
             }
             if (thickness > 0) {
                roadmap_canvas_set_thickness (thickness);
             }
           }
          
           /* now setup the other pen attributes */ 
           for (j = 0; j < n_of_callbacks; ++j) {
             switch (RoadMapLayerPenSetupTable[j].type) {
               case ROADMAP_STYLE_TYPE_INT:
                 if (ivalues[pen_index][j] > 0) {
                   RoadMapLayerPenSetupTable[j].callback(ivalues[pen_index][j]);
                 }
                 break;
               case ROADMAP_STYLE_TYPE_STRING:
                 if (svalues[pen_index][j] != NULL && *(svalues[pen_index][j]) > ' ') {
                   RoadMapLayerPenSetupTable[j].callback((const char *)svalues[pen_index][j]);
                 }
                 break;
             }
           }

           if (i >= lines_count) { /* This is a polygon. */
              layer->in_use[pen_index] = 1;
           }
        }
    }
    

    /* Retrieve the navigation modes associated with each layer. */
    RoadMapNavigationModeCount = roadmap_layer_decode (class_config, "Class", "NavigationModes",
		    RoadMapNavigationMode, ROADMAP_MAX_NAVIGATION_MODES);
    if (RoadMapNavigationModeCount <= 0) return;

    roadmap_log (ROADMAP_WARNING, "RoadMapNavigationModeCount %d", RoadMapNavigationModeCount);

    for (i = 0; i < (int)RoadMapNavigationModeCount; ++i) {

       int j;
       int k;
       int mask = 1 << i;
       char *navigation_layers[ROADMAP_MAX_LAYERS];

       int layers_count =
          roadmap_layer_decode (class_config,
                                "Navigation", RoadMapNavigationMode[i],
                                navigation_layers, ROADMAP_MAX_LAYERS);

       roadmap_log (ROADMAP_WARNING, "Navigation %d [%s] -> %d modes",
		       i, RoadMapNavigationMode[i], layers_count);

       for (j = layers_count - 1; j >= 0; --j) {

          for (k = lines_count - 1; k >= 0; --k) {

             if (strcasecmp(layers[k], navigation_layers[j]) == 0) {
                new_class->layers[k].navigation_modes |= mask;
                break;
             }
          }
       }
    }
}

/**
 * @brief Load the configuration files
 */
void roadmap_layer_load (void)
{
    char  *class_path;
    char **classes;
    char **cursor;
    int    count;

    const char *path;
    const char *skin;
    RoadMapSet *set;


    if (RoadMapLayerActiveSet != NULL) return;
    

    /* Load all the class files from all the possible class directories. */

    skin = roadmap_config_get (&RoadMapConfigSkin);

    for (path = roadmap_path_first("config");
         path != NULL;
         path = roadmap_path_next("config", path)) {
       
       class_path = roadmap_path_join (path, skin);

       if (roadmap_path_is_directory(class_path)) {
    
           classes = roadmap_path_list(class_path, "");

           for (cursor = classes; *cursor != NULL; ++cursor) {
              char *file = roadmap_path_join (class_path, *cursor);
              roadmap_layer_load_file (file);
           }
           roadmap_path_list_free (classes);
       }

       roadmap_path_free(class_path);
    }

    for (count = 0, set = RoadMapLayerSet; set->name != NULL; ++set) {
       roadmap_layer_sort(set);
       count += set->class_count;
    }

    if (count <= 0) {
       roadmap_log (ROADMAP_FATAL, "could not load any class");
    }

    roadmap_layer_select_set ("default");
}


/**
 * @brief add a navigation mode (car, bike, ..)
 * @param name the name of this mode
 * @return the id of this navigation mode
 */
int roadmap_layer_declare_navigation_mode (const char *name) {

   if (RoadMapNavigationModeCount >= ROADMAP_MAX_NAVIGATION_MODES) {
      roadmap_log (ROADMAP_FATAL, "too many navigation modes");
   }
   RoadMapNavigationMode[RoadMapNavigationModeCount] = strdup(name);

   return RoadMapNavigationModeCount++;
}


void roadmap_layer_initialize (void) {

    roadmap_config_declare_enumeration
       ("preferences", &RoadMapConfigStylePretty, "yes", "no", NULL);

    roadmap_config_declare ("preferences", &RoadMapConfigSkin, "default");
}

/**
 * @brief returns true if this layer is a street
 * 	Based on the list of lines (Class.Lines) in default/All .
 * 	The code relies on the fact that Lines have the lowest numbers,
 * 	this is how they're loaded.
 * @param layer the layer's integer value
 * @return true if this is a street / line
 */
int roadmap_layer_is_street(int layer)
{
	if (RoadMapLayerCurrentClass == NULL) {
	    roadmap_log (ROADMAP_FATAL, "roadmap_layer_is_street : no current class");
	    return 0;
	}
	return (layer <= RoadMapLayerCurrentClass->lines_count);
}

/**
 * @brief get the id of the first "road" type (assumption: they're consecutive)
 * @return id of the first road type (whatever the first may be, this is actually meaningless)
 */
int roadmap_layer_road_first(void)
{
	return 0;
}

/**
 * @brief get the id of the next road type (whatever "next" may mean)
 * @param layer the current road type
 * @return the next road type
 */
int roadmap_layer_road_next(int layer)
{
	return layer + 1;
}

/**
 * @brief
 * @return
 */
int roadmap_layer_count_roads(void)
{
	if (RoadMapLayerCurrentClass == NULL) {
	    roadmap_log (ROADMAP_FATAL, "roadmap_layer_count_roads : no current class");
	    return 0;
	}
	return RoadMapLayerCurrentClass->lines_count;
}

/**
 * @brief
 */
int roadmap_layer_road_last(void)
{
	if (RoadMapLayerCurrentClass == NULL) {
	    roadmap_log (ROADMAP_FATAL, "roadmap_layer_road_last : no current class");
	    return 0;
	}
	return RoadMapLayerCurrentClass->lines_count;
}

/**
 * @brief
 * @return
 */
int roadmap_layer_last(void)
{
	if (RoadMapLayerCurrentClass == NULL) {
	    roadmap_log (ROADMAP_FATAL, "roadmap_layer_last : no current class");
	    return 0;
	}
	return RoadMapLayerCurrentClass->lines_count;
}

/**
 * @brief returns the value of the Speed parameter of this layer, from default/All.
 * @param layer
 * @return speed, assumed by calling code to be in km/h (FIX ME)
 */
int roadmap_layer_speed(int layer)
{
	RoadMapLayer		*TheLayer;
	RoadMapConfigItem	*r;

	if (RoadMapLayerCurrentClass == NULL) {
	    roadmap_log (ROADMAP_FATAL, "roadmap_layer_road_last : no current class");
	    return 0;
	}

	TheLayer = RoadMapLayerCurrentClass->layers + layer - 1;
	return roadmap_config_get_integer (&TheLayer->speed);
}
