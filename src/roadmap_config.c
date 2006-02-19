/* roadmap_config.c - A module to handle all RoadMap configuration issues.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_config.h.
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_math.h"
#include "roadmap_config.h"


typedef struct RoadMapConfigEnumRecord RoadMapConfigEnum;

struct RoadMapConfigEnumRecord {

   RoadMapConfigEnum *next;

   char *value;
};


#define ROADMAP_CONFIG_CLEAN     0
#define ROADMAP_CONFIG_SHARED    1
#define ROADMAP_CONFIG_DIRTY     2
#define ROADMAP_CONFIG_READONLY  3

typedef struct RoadMapConfigRecord     RoadMapConfig;

struct RoadMapConfigItemRecord {

    RoadMapConfigItem *next;
    RoadMapConfig     *file;

    const char *category;
    const char *name;
    const char *default_value;
    char *value;

    unsigned char  type;
    unsigned char  state; /* CLEAN, SHARED, DIRTY. */
    unsigned char  cached_valid;
    int            cached_value;

    union {
        RoadMapConfigEnum *enumeration_values;
    } detail;
};


struct RoadMapConfigRecord {

   struct RoadMapConfigRecord *next;

   char              *name;
   char              *file_name;
   char               required;
   char               state; /* CLEAN, SHARED or DIRTY. */
   unsigned short     count;
   RoadMapConfigItem *first_item;
};

static RoadMapConfig *RoadMapConfigFiles = NULL;


struct RoadMapConfigPredefined {
   char *name;
   char  required;
};

static struct RoadMapConfigPredefined RoadMapConfigPredefinedFiles[] = {
   {"session",     0},
   {"preferences", 0},
   {NULL, 0}
};


static RoadMapConfig *roadmap_config_search_file (const char *name) {

   RoadMapConfig *file;

   for (file = RoadMapConfigFiles; file != NULL; file = file->next) {
      if (strcmp (name, file->name) == 0) {
         return file;
      }
   }

   return NULL;
}


static RoadMapConfig *roadmap_config_find_file (const char *name) {

   RoadMapConfig *file = roadmap_config_search_file (name);

   if (file == NULL) {
      roadmap_log (ROADMAP_FATAL, "%s: not a valid config file name", name);
   }
   return file;
}


static RoadMapConfigItem *roadmap_config_search_item
                                (RoadMapConfig *file,
                                 RoadMapConfigDescriptor *descriptor) {

    RoadMapConfigItem *item;

    if (descriptor == NULL) {
        return NULL;
    }
    
    for (item = file->first_item; item != NULL; item = item->next) {

      if ((strcmp (item->name, descriptor->name) == 0) &&
          (strcmp (item->category, descriptor->category) == 0)) {
         return item;
      }
   }
   return NULL;
}


static RoadMapConfigItem *roadmap_config_retrieve
                                (RoadMapConfigDescriptor *descriptor) {

    RoadMapConfig *file;
    RoadMapConfigItem *item;

    if (descriptor == NULL) {
        return NULL;
    }
    
    if (descriptor->reference == NULL) {
        
        for (file = RoadMapConfigFiles; file != NULL; file = file->next) {

            item = roadmap_config_search_item (file, descriptor);
            if (item != NULL) {
                descriptor->reference = item;
                break;
            }
        }
    }

   return descriptor->reference;
}


static RoadMapConfigItem *roadmap_config_new_item
                             (RoadMapConfig *file,
                              RoadMapConfigDescriptor *descriptor,
                              const char    *default_value,
                              unsigned char  item_type) {

    RoadMapConfigItem *new_item;

    new_item = roadmap_config_search_item (file, descriptor);

    if (new_item != NULL) {

        /* Not so new. Update the type & default value, if any. */

        if ((default_value[0] != 0) && (new_item->default_value[0] == 0)) {
            new_item->default_value = strdup(default_value);
        }

        if (item_type != ROADMAP_CONFIG_STRING && new_item->type != item_type) {

            if (new_item->type != ROADMAP_CONFIG_STRING) {
                roadmap_log (ROADMAP_ERROR,
                             "type conflict for %s.%s: %d replaced by %d",
                             descriptor->category,
                             descriptor->name,
                             new_item->type, item_type);
            } else {
                new_item->type = item_type;
            }
        }

    } else {

        new_item = calloc (1, sizeof(RoadMapConfigItem));

        roadmap_check_allocated(new_item);

        new_item->name     = descriptor->name;
        new_item->category = descriptor->category;
        new_item->file     = file;

        if (default_value[0] == 0) {
            new_item->default_value = "";
        } else {
            new_item->default_value = default_value;
        }

        new_item->value = NULL;
        new_item->state = ROADMAP_CONFIG_CLEAN;
        new_item->type  = item_type;
   
        new_item->cached_valid = 0;

        new_item->next = file->first_item;
        file->first_item = new_item;
        file->count += 1;
    }

   descriptor->reference = new_item;
   
   return new_item;
}


static void roadmap_config_add_enumeration_value (RoadMapConfigItem *item,
                                                  const char *value) {

   RoadMapConfigEnum *new_value;
   RoadMapConfigEnum *last = item->detail.enumeration_values;

   new_value = malloc (sizeof(RoadMapConfigEnum));
   roadmap_check_allocated(new_value);

   new_value->next = NULL;
   new_value->value = strdup(value);

   if (last == NULL) {

      item->detail.enumeration_values = new_value;

   } else {

      while (last->next != NULL) last = last->next;

      last->next = new_value;
   }
}


void roadmap_config_declare (const char *config,
                             RoadMapConfigDescriptor *descriptor,
                             const char *default_value) {

   RoadMapConfig *file = roadmap_config_find_file (config);

   roadmap_config_new_item
      (file, descriptor, default_value, ROADMAP_CONFIG_STRING);
}


void roadmap_config_declare_enumeration (const char *config,
                                         RoadMapConfigDescriptor *descriptor,
                                         const char *enumeration_value, ...) {

   char *p;
   va_list ap;
   RoadMapConfig *file = roadmap_config_find_file (config);
   RoadMapConfigItem *item;
   RoadMapConfigEnum *next;
   RoadMapConfigEnum *enumerated;


   item = roadmap_config_new_item
             (file, descriptor, enumeration_value, ROADMAP_CONFIG_ENUM);

   /* Replace the enumeration list. */

   for (enumerated = item->detail.enumeration_values;
        enumerated != NULL;
        enumerated = next) {
      next = enumerated->next;
      free (enumerated->value);
      free (enumerated);
   }
   item->detail.enumeration_values = NULL;

   roadmap_config_add_enumeration_value (item, enumeration_value);

   va_start(ap, enumeration_value);
   for (p = va_arg(ap, char *); p != NULL; p = va_arg(ap, char *)) {
      roadmap_config_add_enumeration_value (item, p);
   }
   va_end(ap);
}


void roadmap_config_declare_color (const char *config,
                                   RoadMapConfigDescriptor *descriptor,
                                   const char *default_value) {

   RoadMapConfig *file = roadmap_config_find_file (config);

   roadmap_config_new_item
      (file, descriptor, default_value, ROADMAP_CONFIG_COLOR);
}


void roadmap_config_declare_distance (const char *config,
                                      RoadMapConfigDescriptor *descriptor,
                                      const char *default_value) {

   RoadMapConfig *file = roadmap_config_find_file (config);

   roadmap_config_new_item
      (file, descriptor, default_value, ROADMAP_CONFIG_DISTANCE);
}


static int roadmap_config_compare (const void *e1, const void *e2) {

   RoadMapConfigItem **item1 = (RoadMapConfigItem **)e1;
   RoadMapConfigItem **item2 = (RoadMapConfigItem **)e2;

   return strcmp ((*item2)->category, (*item1)->category);
}

int roadmap_config_first (const char *config,
                          RoadMapConfigDescriptor *descriptor) {

    int i;
    RoadMapConfig *file = roadmap_config_find_file(config);
    RoadMapConfigItem *item;
    RoadMapConfigItem **sorted;


    if (file == NULL || file->first_item == NULL) {

        descriptor->category = NULL;
        descriptor->name = NULL;
        descriptor->reference = NULL;
        return 0;
    }

    /* Sort all the items before we show them. */

    sorted = (RoadMapConfigItem **)
       malloc (sizeof(RoadMapConfigItem *) * file->count);

    for (i = 0, item = file->first_item; item != NULL; ++i, item = item->next) {
       if (i >= file->count) {
          roadmap_log (ROADMAP_FATAL, "invalid config item count");
       }
       sorted[i] = item;
    }

    qsort (sorted, file->count, sizeof(RoadMapConfigItem *),
           roadmap_config_compare);

    for (item = NULL, i = file->count -1; i >= 0; --i) {
       sorted[i]->next = item;
       item = sorted[i];
    }
    file->first_item = sorted[0];

    free (sorted);


    /* Now we can start scanning the list. */

    descriptor->category = file->first_item->category;
    descriptor->name = file->first_item->name;
    descriptor->reference = file->first_item;
    
    return 1;
}


int roadmap_config_next (RoadMapConfigDescriptor *descriptor) {

    if (descriptor == NULL || descriptor->reference == NULL) {
        return 0;
    }

    descriptor->reference = descriptor->reference->next;
   
    if (descriptor->reference == NULL) {

       descriptor->category  = NULL;
       descriptor->name      = NULL;
       return 0;
    }
   
    descriptor->category  = descriptor->reference->category;
    descriptor->name      = descriptor->reference->name;

    return 1;
}


void *roadmap_config_get_enumeration (RoadMapConfigDescriptor *descriptor) {

   RoadMapConfigItem *item = roadmap_config_retrieve (descriptor);

   if (item == NULL) {
      return NULL;
   }

   if (item->type != ROADMAP_CONFIG_ENUM) {
      return NULL;
   }

   return item->detail.enumeration_values;
}


char *roadmap_config_get_enumeration_value (void *enumeration) {

   RoadMapConfigEnum *item = (RoadMapConfigEnum *)enumeration;

   if (item == NULL) { 
      return NULL;
   }

   return item->value;
}


void *roadmap_config_get_enumeration_next (void *enumeration) {

   RoadMapConfigEnum *item = (RoadMapConfigEnum *)enumeration;

   if (item == NULL) { 
      return NULL;
   }

   return item->next;
}


static void roadmap_config_set_item
                (RoadMapConfigItem *item, const char *value) {

    if (item->state == ROADMAP_CONFIG_READONLY) return;
    if (item->file->state == ROADMAP_CONFIG_READONLY) return;

    /* Check that this new value actually changes something. */
    
    if (item->value != NULL) {
        if (value != NULL) {
            if (strcmp (item->value, value) == 0) {

                return; /* Nothing was changed. */
            }
        }
        free(item->value);
    } else {
        if (value != NULL) {
            if (strcmp (item->default_value, value) == 0) {
                return; /* Still is the default. */
            }
        }
    }
   
    if (value != NULL) {
        item->value = strdup(value);
    } else {
        item->value = NULL;
    }

    item->state = ROADMAP_CONFIG_DIRTY;
    item->file->state = ROADMAP_CONFIG_DIRTY;
    item->cached_valid = 0;
}


static char *roadmap_config_skip_until (char *p, char c) {

   while (*p != '\n' && *p != c && *p > 0) p++;
   return p;
}

static char *roadmap_config_skip_spaces (char *p) {

   while (*p == ' ' || *p == '\t') p++;
   return p;
}


static int roadmap_config_load_file
               (const char *path, RoadMapConfig *config, char intended_state) {

   char *p;
   FILE *file;
   char  line[1024];

   char *category;
   char *name;
   char *value;

   RoadMapConfigItem *item;
   RoadMapConfigDescriptor descriptor;


   file = roadmap_file_fopen (path, config->file_name, "sr");
   if (file == NULL) return 0;


   while (!feof(file)) {

        /* Read the next line, skip empty lines and comments. */

        if (fgets (line, sizeof(line), file) == NULL) break;

        category =
            roadmap_config_extract_data (line, sizeof(line));

        if (category == NULL) continue;


        /* Decode the line (category.name: value). */
        
        p = roadmap_config_skip_until (category, '.');
        if (*p != '.') continue;
        *(p++) = 0;

        name = p;

        p = roadmap_config_skip_until (p, ':');
        if (*p != ':') continue;
        *(p++) = 0;

        p = roadmap_config_skip_spaces (p);
        value = p;

        p = roadmap_config_skip_until (p, 0);
        *p = 0;


        /* Detach the strings from the line buffer. */

        value = strdup (value);
        descriptor.name = strdup (name);
        descriptor.category = strdup (category);
        descriptor.reference = NULL;


        /* Retrieve or create this configuration item. */
        
        item = roadmap_config_new_item
                    (config, &descriptor, "", ROADMAP_CONFIG_STRING);
        if (item->value != NULL) {
            free(item->value);
        }
        item->value = value;
        item->state = intended_state;
      
        item->cached_valid = 0;
    }
    fclose (file);

    config->state = ROADMAP_CONFIG_CLEAN;
    return 1;
}


static RoadMapConfig *roadmap_config_new_file (const char *name,
                                               const char *file, int writable) {

   RoadMapConfig *new_config;

   roadmap_check_allocated (new_config = malloc (sizeof(RoadMapConfig)));

   roadmap_check_allocated (new_config->name = strdup(name));
   if (file == NULL) {
      new_config->file_name = new_config->name;
   } else {
      roadmap_check_allocated (new_config->file_name = strdup(file));
   }

   new_config->state =
      writable ? ROADMAP_CONFIG_CLEAN : ROADMAP_CONFIG_READONLY;

   new_config->required = 1;
   new_config->count = 0;
   new_config->first_item = NULL;

   new_config->next = RoadMapConfigFiles;
   RoadMapConfigFiles = new_config;

   return new_config;
}


static void roadmap_config_update (RoadMapConfig *config, int force) {

   FILE *file;
   const char *value;
   RoadMapConfigItem *item;


   if (force || (config->state == ROADMAP_CONFIG_DIRTY)) {

      if (config->state == ROADMAP_CONFIG_READONLY) return;

      file = roadmap_file_fopen (roadmap_path_user(), config->name, "w");

      if (file) {

         for (item = config->first_item; item != NULL; item = item->next) {

            if ((! force) && (item->state == ROADMAP_CONFIG_SHARED)) continue;

            if (item->value != NULL) {
               value = item->value;
            } else {
               value = item->default_value;
            }
            fprintf (file, "%s.%s: %s\n", item->category, item->name, value);
         }

         fclose (file);
         config->state = ROADMAP_CONFIG_CLEAN;
      }
   }
}


char *roadmap_config_extract_data (char *line, int size) {
    
    char *p;
    
    line[size-1] = 0;
    
    line = roadmap_config_skip_spaces (line);

    if (*line == '\n' || *line == '#') return NULL;
        
    p = strchr (line, '\n');
    if (p != NULL) *p = 0;
        
    return line;
}


void  roadmap_config_initialize (void) {

   RoadMapConfig *file;
   struct RoadMapConfigPredefined *known;

   if (RoadMapConfigFiles != NULL) return;

   for (known = RoadMapConfigPredefinedFiles; known->name != NULL; ++known) {

      file = roadmap_config_new_file (known->name, NULL, 1);
      file->required = known->required;
   }
}


void  roadmap_config_load (void) {

    const char *p;
    RoadMapConfig *file;


    for (file = RoadMapConfigFiles; file != NULL; file = file->next) {

      int loaded = 0;

      for (p = roadmap_path_last("config");
           p != NULL;
           p = roadmap_path_previous("config", p)) {

         loaded |= roadmap_config_load_file (p, file, ROADMAP_CONFIG_SHARED);
      }

      loaded |= roadmap_config_load_file (roadmap_path_user(),
                                          file, ROADMAP_CONFIG_CLEAN);

      if (file->required && (!loaded)) {
         roadmap_log
            (ROADMAP_ERROR,
             "found no '%s' config file, check RoadMap installation",
             file->name);
      }
   }
}


const char *roadmap_config_new (const char *file, int writable) {

   RoadMapConfig *new_config;
   const char *name = roadmap_path_skip_directories (file);


   if (roadmap_config_search_file (name) != NULL) {
      roadmap_log (ROADMAP_FATAL, "config %s redefined as %s", name, file);
   }

   new_config = roadmap_config_new_file (name, file, writable);

   roadmap_config_load_file (NULL, new_config, ROADMAP_CONFIG_CLEAN);
   return name;
}


void roadmap_config_save (int force) {

   RoadMapConfig *file;

   for (file = RoadMapConfigFiles; file != NULL; file = file->next) {

      if (file->state != ROADMAP_CONFIG_READONLY) {
         roadmap_config_update (file, force);
      }
   }
}


const char *roadmap_config_file (const char *name) {

   RoadMapConfig *file = roadmap_config_find_file (name);

   if (file == NULL) return NULL;

   return file->file_name;
}


int roadmap_config_get_type (RoadMapConfigDescriptor *descriptor) {

    RoadMapConfigItem *item = roadmap_config_retrieve (descriptor);

    if (item == NULL) {
        return ROADMAP_CONFIG_STRING;
    }

    return item->type;
}


const char *roadmap_config_get (RoadMapConfigDescriptor *descriptor) {

    RoadMapConfigItem *item = roadmap_config_retrieve (descriptor);

    if (item != NULL) {

        if (item->value != NULL) {
            return item->value;
        }
        return item->default_value;
    }

   return "";
}


int roadmap_config_get_integer(RoadMapConfigDescriptor *descriptor) {

    int had_units;
    const char *actual;
    RoadMapConfigItem *item = roadmap_config_retrieve (descriptor);

    if (item != NULL) {
        
        if (! item->cached_valid) {
            
            if (item->value != NULL) {
                actual = item->value;
            } else {
                actual = item->default_value;
            }

            if (item->type == ROADMAP_CONFIG_DISTANCE) {

                item->cached_value =
                  roadmap_math_distance_convert(actual, &had_units);
               if (! had_units) {
                  /* Force the current unit to appear in the preferences. */
                  roadmap_config_set_integer
                     (descriptor, item->cached_value);
               }
            } else {
               item->cached_value = atoi (actual);
            }
            item->cached_valid = 1;
        }
        return item->cached_value;
    }
    return 0;
}


const char *roadmap_config_get_from (const char *config,
                                     const char *category,
                                     const char *name) {

   RoadMapConfig *file;
   RoadMapConfigItem *item;

   for (file = RoadMapConfigFiles; file != NULL; file = file->next) {

      if (strcmp (file->name, config) == 0) {

         for (item = file->first_item; item != NULL; item = item->next) {

            if ((strcmp (item->name, name) == 0) &&
                (strcmp (item->category, category) == 0)) {

               if (item->value == NULL) {
                  return item->default_value;
               }
               return item->value;
            }
         }
      }
   }

   return NULL;
}


void  roadmap_config_set (RoadMapConfigDescriptor *descriptor,
                          const char *value) {

    RoadMapConfigItem *item = roadmap_config_retrieve (descriptor);

    if (item != NULL) {
        roadmap_config_set_item (item, value);
    }
}


void  roadmap_config_set_integer (RoadMapConfigDescriptor *descriptor, int x) {

    char image[16];
    RoadMapConfigItem *item = roadmap_config_retrieve (descriptor);

    if (item->type == ROADMAP_CONFIG_DISTANCE) { 
       snprintf (image, sizeof(image),
                 "%d %s", x, roadmap_math_distance_unit());
    } else {
       snprintf (image, sizeof(image), "%d", x);
    }
    roadmap_config_set (descriptor, image);
}


/* This function compares the current item's value with the given text.
 * This is very useful for enumerations.
 */
int   roadmap_config_match
        (RoadMapConfigDescriptor *descriptor, const char *text) {

   return (strcasecmp (roadmap_config_get(descriptor), text) == 0);
}


/* The following two functions are special because positions are always
 * session items. The reason is only that I am anal: I don't see any reason
 * for having a position in the schema or preferences.
 */
void roadmap_config_get_position
        (RoadMapConfigDescriptor *descriptor, RoadMapPosition *position) {

   const char *center;
   RoadMapConfig *file;
   RoadMapConfigItem *item;

   file = roadmap_config_search_file ("session");
   if (file == NULL) {
       roadmap_log (ROADMAP_FATAL, "cannot retrieve session file context");
   }
   item = roadmap_config_search_item (file, descriptor);

   if (item->value != NULL) {
      center = item->value;
   } else {
      center = item->default_value;
   }

   if (center != NULL && center[0] != 0) {

      char *latitude_ascii;
      char buffer[128];


      if (strlen(center) >= sizeof(buffer)) {
         roadmap_log (ROADMAP_FATAL,
                      "position string '%s' is too long", center);
      }
      strcpy (buffer, center);
      latitude_ascii = strchr (buffer, ',');
      if (latitude_ascii != NULL) {
         *(latitude_ascii++) = 0;
      }

      position->longitude = atoi(buffer);
      position->latitude  = atoi(latitude_ascii);

   } else {

      position->longitude = 0;
      position->latitude  = 0;
   }
}

void  roadmap_config_set_position
        (RoadMapConfigDescriptor *descriptor, const RoadMapPosition *position) {

   char buffer[128];
   RoadMapConfig *file;
   RoadMapConfigItem *item;

   file = roadmap_config_search_file ("session");
   if (file == NULL) {
       roadmap_log (ROADMAP_FATAL, "cannot retrieve session file context");
   }
   item = roadmap_config_search_item (file, descriptor);

   if (item != NULL) {

      sprintf (buffer, "%d,%d", position->longitude, position->latitude);
      roadmap_config_set_item (item, buffer);
   }
}

