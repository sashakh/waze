/* roadmap_preferences.c - handle the roadmap dialogs managing user preferences.
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
 *   void roadmap_preferences_edit (void);
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_config.h"
#include "roadmap_dialog.h"
#include "roadmap_preferences.h"


typedef struct EditableItemRecord EditableItem;

struct EditableItemRecord {

   EditableItem *next;

   char *category;
   char *name;
};


typedef struct category_list CategoryList;

struct category_list {

   CategoryList *next;
   EditableItem *children;

   char *name;
};


typedef struct configuration_context ConfigurationContext;

struct configuration_context {

   ConfigurationContext *next;
   CategoryList         *children;

   char *name;
};

static ConfigurationContext *RoadMapConfigurationDialogs = NULL;


static void roadmap_preferences_cancel (const char *name, void *data) {

   roadmap_dialog_hide (name);
}

static void roadmap_preferences_ok (const char *name, void *data) {

   ConfigurationContext *context = (ConfigurationContext *) data;
   CategoryList *list;
   EditableItem *item;

   if (context == NULL) return;

   for (list = context->children; list != NULL; list = list->next) {

      for (item = list->children; item != NULL; item = item->next) {

         roadmap_config_set (item->category,
                             item->name,
                             (char *) roadmap_dialog_get_data (item->category,
                                                               item->name));
      }
   }

   return roadmap_preferences_cancel (name, data);
}

static void roadmap_preferences_force (const char *name, void *data) {

   roadmap_preferences_ok (name, data);
   roadmap_config_save (1);
}


static EditableItem *roadmap_preferences_new_item
                        (ConfigurationContext *context,
                         char *category,
                         char *name) {

   CategoryList *list;
   EditableItem *item;

   for (list = context->children; list != NULL; list = list->next) {

      if (strcmp (category, list->name) == 0) {

         for (item = list->children; item != NULL; item = item->next) {

            if (strcmp (name, item->name) == 0) {
               return item;
            }
         }
      }
   }

   if (list == NULL) {

      list = malloc (sizeof(CategoryList));
      if (list == NULL) {
         roadmap_log (ROADMAP_FATAL, "no more memory");
      }
      list->name     = name;
      list->children = NULL;

      list->next = context->children;
      context->children = list;
   }

   item = malloc (sizeof(EditableItem));
   if (item == NULL) {
      roadmap_log (ROADMAP_FATAL, "no more memory");
   }

   item->category = category;
   item->name     = name;

   item->next = list->children;
   list->children = item;

   return item;
}


static void roadmap_preferences_new_dialog
               (ConfigurationContext *context, void *cursor) {

   void *next;
   void *enumeration;

   int   type;
   char *category;
   char *name;
   char *value;

   int   count;
   char *values[256];

   EditableItem *item;

   int use_keyboard;


   while (cursor != NULL) {

      type = roadmap_config_get_type (cursor);
      next = roadmap_config_scan (cursor, &category, &name, &value);

      item = roadmap_preferences_new_item (context, category, name);

      switch (type) {

      case ROADMAP_CONFIG_ENUM:

         count = 1;
         values[0] = value; /* Always make the original value appear first. */

         for (enumeration = roadmp_config_get_enumeration (cursor);
              enumeration != NULL;
              enumeration = roadmp_config_get_enumeration_next (enumeration)) {

            values[count] = roadmp_config_get_enumeration_value (enumeration);

            if (strcmp (values[count], value) != 0) {

               if (count >= 256) {
                  roadmap_log (ROADMAP_FATAL,
                               "too many values for item %s.%s",
                               category, name);
               }
               count += 1;
            }
         }
         roadmap_dialog_new_choice
            (category, name, count, values, (void **)values, NULL);
         break;

      case ROADMAP_CONFIG_COLOR:
      case ROADMAP_CONFIG_STRING:

         roadmap_dialog_new_entry (category, name);
         break;

      default:
         roadmap_log (ROADMAP_FATAL, "invalid preference item type %d", type);
      }

      cursor = next;
   }

   roadmap_dialog_add_button ("Ok", roadmap_preferences_ok);
   roadmap_dialog_add_button ("Force", roadmap_preferences_force);
   roadmap_dialog_add_button ("Cancel", roadmap_preferences_cancel);


   use_keyboard = (strcasecmp (roadmap_config_get ("General", "Keyboard"),
                               "yes") == 0);

   roadmap_dialog_complete (use_keyboard);
}


static void roadmap_preferences_reset_dialog
               (ConfigurationContext *context, void *cursor) {

   CategoryList *list;
   EditableItem *item;

   if (context == NULL) return;

   for (list = context->children; list != NULL; list = list->next) {

      for (item = list->children; item != NULL; item = item->next) {

         roadmap_dialog_set_data (item->category,
                                  item->name,
                                  roadmap_config_get (item->category,
                                                      item->name));
      }
   }
}


static void roadmap_preferences_show (char *title, void *cursor) {

   char full_title[256];
   ConfigurationContext *context;

   if (cursor == NULL) {
      roadmap_log (ROADMAP_ERROR, "no preference item found");
      return;
   }

   strcpy (full_title, "RoadMap ");
   strcat (full_title, title);

   for (context = RoadMapConfigurationDialogs;
        context != NULL;
        context = context->next) {

      if (strcmp (context->name, full_title) == 0) break;
   }

   if (context == NULL) {

      context = (ConfigurationContext *) malloc (sizeof(ConfigurationContext));
      if (context == NULL) {
         roadmap_log (ROADMAP_FATAL, "no more memory");
      }

      context->name = strdup(full_title);
      context->children = NULL;

      context->next = RoadMapConfigurationDialogs;
      RoadMapConfigurationDialogs = context;
   }

   if (roadmap_dialog_activate (full_title, context)) {

      roadmap_preferences_new_dialog (context, cursor);
   }
   roadmap_preferences_reset_dialog (context, cursor);
}


void roadmap_preferences_edit (void) {
   roadmap_preferences_show
      ("Preferences", roadmap_config_first ("preferences"));
}

