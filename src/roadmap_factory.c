/* roadmap_factory.c - The menu/toolbar/shortcut factory for RoadMap.
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
 *   See roadmap_factory.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_main.h"
#include "roadmap_preferences.h"
#include "roadmap_help.h"

#include "roadmap_factory.h"


static RoadMapConfigDescriptor RoadMapConfigGeneralToolbar =
                        ROADMAP_CONFIG_ITEM("General", "Toolbar");

static RoadMapConfigDescriptor RoadMapConfigGeneralIcons =
                        ROADMAP_CONFIG_ITEM("General", "Icons");


const char RoadMapFactorySeparator[] = "--separator--";
const char RoadMapFactoryHelpTopics[] = "--help-topics--";


struct RoadMapFactoryKeyMap {

   const char          *key;
   const RoadMapAction *action;
};

static struct RoadMapFactoryKeyMap *RoadMapFactoryBindings = NULL;

static int RoadMapFactoryKeyLength = 0;


static void roadmap_factory_keyboard (char *key) {

   const struct RoadMapFactoryKeyMap *binding;

   if (RoadMapFactoryBindings == NULL) return;

   for (binding = RoadMapFactoryBindings; binding->key != NULL; ++binding) {

      if (strcasecmp (binding->key, key) == 0) {
         if (binding->action != NULL) {
            RoadMapCallback callback = binding->action->callback;
            if (callback != NULL) {
               (*callback) ();
               break;
            }
         }
      }
   }
}

static void roadmap_factory_add_help (void) {

   int ok;
   const char *label;
   RoadMapCallback callback;

   for (ok = roadmap_help_first_topic(&label, &callback);
        ok;
        ok = roadmap_help_next_topic(&label, &callback)) {

      roadmap_main_add_menu_item (label, label, callback);
   }
}


static const RoadMapAction *roadmap_factory_find_action
                              (const RoadMapAction *actions, const char *item) {

   while (actions->name != NULL) {
      if (strcmp (actions->name, item) == 0) return actions;
      ++actions;
   }

   return NULL;
}


static const char *roadmap_factory_terse (const RoadMapAction *action) {

   if (action->label_terse != NULL) {
      return action->label_terse;
   }
   if (action->label_short != NULL) {
      return action->label_short;
   }
   return action->label_long;
}


void roadmap_factory (const RoadMapAction  *actions,
                      const char           *menu[],
                      const char           *toolbar[]) {

   int i;
   int prefix = strlen(ROADMAP_MENU);

   int use_toolbar =
            (strcasecmp (roadmap_config_get (&RoadMapConfigGeneralToolbar),
                         "yes") == 0);

   int use_icons =
            (strcasecmp (roadmap_config_get (&RoadMapConfigGeneralIcons),
                         "yes") == 0);


   for (i = 0; menu[i] != NULL; ++i) {

      const char *item = menu[i];

      if (item == RoadMapFactorySeparator) {

         roadmap_main_add_separator ();

      } else if (item == RoadMapFactoryHelpTopics) {

         roadmap_factory_add_help ();

      } else if (strncmp (item, ROADMAP_MENU, prefix) == 0) {

         roadmap_main_add_menu (item + prefix);

      } else {
         const RoadMapAction *this_action;

         this_action = roadmap_factory_find_action (actions, item);
         if (this_action != NULL) {
            roadmap_main_add_menu_item (this_action->label_long,
                                        this_action->tip,
                                        this_action->callback);
         }
      }
   }

   if (use_toolbar) {

      for (i = 0; toolbar[i] != NULL; ++i) {

         const char *item = toolbar[i];

         if (item == RoadMapFactorySeparator) {

            roadmap_main_add_tool_space ();

         } else {

            const RoadMapAction *this_action;

            this_action = roadmap_factory_find_action (actions, item);

            if (this_action != NULL) {
               roadmap_main_add_tool (roadmap_factory_terse(this_action),
                                      (use_icons) ? this_action->name : NULL,
                                      this_action->tip,
                                      this_action->callback);
            }
         }
      }
   }

   if (RoadMapFactoryBindings != NULL) {
      roadmap_main_set_keyboard (roadmap_factory_keyboard);
   }
}


void roadmap_factory_keymap (const RoadMapAction  *actions,
                             const char           *shortcuts[]) {

   int i;

   if (RoadMapFactoryBindings != NULL) {
      roadmap_log (ROADMAP_FATAL, "RoadMap factory was called twice");
   }

   /* Count how many shortcuts we have to process. */
   for (i = 0; shortcuts[i] != NULL; ++i) ;

   /* Create the keyboard mapping table. */

   if (i > 0) {

      int j = 0;

      RoadMapFactoryBindings = 
         (struct RoadMapFactoryKeyMap *)
             calloc (i+1, sizeof(struct RoadMapFactoryKeyMap));
      roadmap_check_allocated(RoadMapFactoryBindings);

      for (i = 0; shortcuts[i] != NULL; ++i) {

         char *text;
         char *separator;
         const RoadMapAction *this_action;

         text = strdup (shortcuts[i]);
         roadmap_check_allocated(text);

         separator = strstr (text, ROADMAP_MAPPED_TO);
         if (separator != NULL) {

            char *p;

            /* Separate the name of the key from the name of the action. */

            for (p = separator; *p <= ' '; --p) *p = 0;

            p = separator + strlen(ROADMAP_MAPPED_TO);
            while (*p <= ' ') ++p;
            this_action = roadmap_factory_find_action (actions, p);

            if (this_action != NULL) {

               int length = strlen(text);

               if (length > RoadMapFactoryKeyLength) {
                  RoadMapFactoryKeyLength = length;
               }
               RoadMapFactoryBindings[j].key = text;
               RoadMapFactoryBindings[j].action = this_action;
               ++j;
            }
         }
      }
      RoadMapFactoryBindings[j].key = NULL;

      roadmap_main_set_keyboard (roadmap_factory_keyboard);
   }
}


void roadmap_factory_show_keymap (void) {

   const struct RoadMapFactoryKeyMap *binding;

   printf ("RoadMap button & key bindings:\n\n");

   for (binding = RoadMapFactoryBindings; binding->key != NULL; ++binding) {

      const RoadMapAction *action = binding->action;

      if (action != NULL) {
         printf ("  %-*.*s  %s.\n",
                 RoadMapFactoryKeyLength,
                 RoadMapFactoryKeyLength,
                 binding->key,
                 action->tip);
      }
   }
}

