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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_main.h"
#include "roadmap_preferences.h"
#include "roadmap_help.h"
#include "roadmap_path.h"
#include "roadmap_input.h"
#include "roadmap_math.h"

#include "roadmap_factory.h"


static RoadMapConfigDescriptor RoadMapConfigGeneralToolbar =
                        ROADMAP_CONFIG_ITEM("General", "Toolbar");

static RoadMapConfigDescriptor RoadMapConfigGeneralToolbarOrientation =
                        ROADMAP_CONFIG_ITEM("General", "Toolbar Orientation");

static RoadMapConfigDescriptor RoadMapConfigGeneralIcons =
                        ROADMAP_CONFIG_ITEM("General", "Icons");

static RoadMapConfigDescriptor RoadMapConfigGeneralTooltips =
                        ROADMAP_CONFIG_ITEM("General", "Tooltips");


const char RoadMapFactorySeparator[] = "--separator--";
const char RoadMapFactoryHelpTopics[] = "--help-topics--";

typedef struct {

   RoadMapCallback action;
   char *title;

} RoadMapFactoryMenuPopup;

void roadmap_factory_menu_popup_n(int n);

void roadmap_factory_menu_popup_00(void) { roadmap_factory_menu_popup_n(0); };
void roadmap_factory_menu_popup_01(void) { roadmap_factory_menu_popup_n(1); };
void roadmap_factory_menu_popup_02(void) { roadmap_factory_menu_popup_n(2); };
void roadmap_factory_menu_popup_03(void) { roadmap_factory_menu_popup_n(3); };
void roadmap_factory_menu_popup_04(void) { roadmap_factory_menu_popup_n(4); };
void roadmap_factory_menu_popup_05(void) { roadmap_factory_menu_popup_n(5); };
void roadmap_factory_menu_popup_06(void) { roadmap_factory_menu_popup_n(6); };
void roadmap_factory_menu_popup_07(void) { roadmap_factory_menu_popup_n(7); };
void roadmap_factory_menu_popup_08(void) { roadmap_factory_menu_popup_n(8); };
void roadmap_factory_menu_popup_09(void) { roadmap_factory_menu_popup_n(9); };
void roadmap_factory_menu_popup_10(void) { roadmap_factory_menu_popup_n(10); };
void roadmap_factory_menu_popup_11(void) { roadmap_factory_menu_popup_n(11); };
void roadmap_factory_menu_popup_12(void) { roadmap_factory_menu_popup_n(12); };
void roadmap_factory_menu_popup_13(void) { roadmap_factory_menu_popup_n(13); };
void roadmap_factory_menu_popup_14(void) { roadmap_factory_menu_popup_n(14); };
void roadmap_factory_menu_popup_15(void) { roadmap_factory_menu_popup_n(15); };
void roadmap_factory_menu_popup_16(void) { roadmap_factory_menu_popup_n(16); };
void roadmap_factory_menu_popup_17(void) { roadmap_factory_menu_popup_n(17); };
void roadmap_factory_menu_popup_18(void) { roadmap_factory_menu_popup_n(18); };
void roadmap_factory_menu_popup_19(void) { roadmap_factory_menu_popup_n(19); };
void roadmap_factory_menu_popup_20(void) { roadmap_factory_menu_popup_n(20); };
void roadmap_factory_menu_popup_21(void) { roadmap_factory_menu_popup_n(21); };
void roadmap_factory_menu_popup_22(void) { roadmap_factory_menu_popup_n(22); };
void roadmap_factory_menu_popup_23(void) { roadmap_factory_menu_popup_n(23); };
void roadmap_factory_menu_popup_24(void) { roadmap_factory_menu_popup_n(24); };
void roadmap_factory_menu_popup_25(void) { roadmap_factory_menu_popup_n(25); };
void roadmap_factory_menu_popup_26(void) { roadmap_factory_menu_popup_n(26); };
void roadmap_factory_menu_popup_27(void) { roadmap_factory_menu_popup_n(27); };
void roadmap_factory_menu_popup_28(void) { roadmap_factory_menu_popup_n(28); };
void roadmap_factory_menu_popup_29(void) { roadmap_factory_menu_popup_n(29); };

RoadMapFactoryMenuPopup RoadMapFactoryMenuPopups[] = {
   { roadmap_factory_menu_popup_00, NULL },
   { roadmap_factory_menu_popup_01, NULL },
   { roadmap_factory_menu_popup_02, NULL },
   { roadmap_factory_menu_popup_03, NULL },
   { roadmap_factory_menu_popup_04, NULL },
   { roadmap_factory_menu_popup_05, NULL },
   { roadmap_factory_menu_popup_06, NULL },
   { roadmap_factory_menu_popup_07, NULL },
   { roadmap_factory_menu_popup_08, NULL },
   { roadmap_factory_menu_popup_09, NULL },
   { roadmap_factory_menu_popup_10, NULL },
   { roadmap_factory_menu_popup_11, NULL },
   { roadmap_factory_menu_popup_12, NULL },
   { roadmap_factory_menu_popup_13, NULL },
   { roadmap_factory_menu_popup_14, NULL },
   { roadmap_factory_menu_popup_15, NULL },
   { roadmap_factory_menu_popup_16, NULL },
   { roadmap_factory_menu_popup_17, NULL },
   { roadmap_factory_menu_popup_18, NULL },
   { roadmap_factory_menu_popup_19, NULL },
   { roadmap_factory_menu_popup_20, NULL },
   { roadmap_factory_menu_popup_21, NULL },
   { roadmap_factory_menu_popup_22, NULL },
   { roadmap_factory_menu_popup_23, NULL },
   { roadmap_factory_menu_popup_24, NULL },
   { roadmap_factory_menu_popup_25, NULL },
   { roadmap_factory_menu_popup_26, NULL },
   { roadmap_factory_menu_popup_27, NULL },
   { roadmap_factory_menu_popup_28, NULL },
   { roadmap_factory_menu_popup_29, NULL },
};

#define MAX_MENUPOPUPS \
    ( sizeof(RoadMapFactoryMenuPopups) / sizeof(*RoadMapFactoryMenuPopups) )

int RoadMapFactoryMenuPopupCount;


void roadmap_factory_menu_popup_n(int n) {

    RoadMapGuiPoint point;

    /* We don't really know where the mouse is right now, so
     * pretend it's in the center of the screen.  The position
     * will probably be unused in any case.
     */
    roadmap_math_get_context (NULL, NULL, &point);
    point.x /= 2;
    point.y /= 2;

    roadmap_factory_popup ( RoadMapFactoryMenuPopups[n].title, &point );
}

struct RoadMapFactoryKeyMap {

   const char          *key;
   const RoadMapAction *action;
};

static struct RoadMapFactoryKeyMap *RoadMapFactoryBindings = NULL;

static int RoadMapFactoryKeyLength = 0;


struct RoadMapFactoryPopup {

   const char *title;
   RoadMapMenu menu;

   struct RoadMapFactoryPopup *next;
};

static struct RoadMapFactoryPopup *RoadMapFactoryPopupList = NULL;


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

static void roadmap_factory_add_help (RoadMapMenu menu) {

   int ok;
   const char *label;
   RoadMapCallback callback;

   if (menu == NULL) return;

   for (ok = roadmap_help_first_topic(&label, &callback);
        ok;
        ok = roadmap_help_next_topic(&label, &callback)) {

      roadmap_main_add_menu_item (menu, label, label, callback);
   }
}


static RoadMapAction *roadmap_factory_find_action
                          (RoadMapAction *actions, const char *item) {

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


static const char **roadmap_factory_load_config (const char *file_name,
                                                 RoadMapAction *actions,
                                                 const char *path) {

   static const char *loaded[256];

   int   count = 0;

   char *p;
   char buffer[256];
   FILE *file = roadmap_file_fopen (path, file_name, "sr");

   if (file == NULL) return NULL;

   while (! feof(file)) {

      fgets (buffer, sizeof(buffer), file);

      if (feof(file) || ferror(file)) break;

      buffer[sizeof(buffer)-1] = 0;

      /* remove the end-of-line character, and trailing whitespace */
      p = buffer + strlen(buffer);
      while (p-- > buffer && isspace(*p))
          *p = '\0';

      /* Remove any leading space. */
      for (p = buffer; isspace(*p); ++p) ;

      if ((*p == 0) || (*p == '#')) continue; /* Empty line. */

      if (strncmp (p, ROADMAP_MENU, sizeof(ROADMAP_MENU)-1) == 0 ||
          strncmp (p, ROADMAP_SUBMENU, sizeof(ROADMAP_SUBMENU)-1) == 0 ||
          strncmp (p, ROADMAP_INVOKE_SUBMENU, sizeof(ROADMAP_INVOKE_SUBMENU)-1) == 0) {

         p = strdup(p);
         roadmap_check_allocated(p);
         loaded[count++] = p;

      } else if (*p == '|' || *p == '-') {

         loaded[count++] = RoadMapFactorySeparator;

      } else {

         const RoadMapAction *this_action;

         this_action = roadmap_factory_find_action (actions, p);
         if (this_action == NULL) {
            roadmap_log (ROADMAP_ERROR, "invalid action name '%s' in %s%s%s",
                p, path ? path : "", path ? "/":"",  file_name);
         } else {
            loaded[count++] = this_action->name;
         }
      }
   }
   fclose(file);

   /* okay to return with count == 0 -- this allows user to override
    * builtin configs with empty files
    */

   loaded[count] = NULL;
   return loaded;
}

static const char **roadmap_factory_user_config
                                    (const char *name,
                                     const char *category,
                                     RoadMapAction *actions) {

   const char **loaded;

   char file_name[256];


   snprintf (file_name, sizeof(file_name), "%s.%s", name, category);

   loaded = roadmap_factory_load_config
               (file_name, actions, roadmap_path_user());

   if (loaded == NULL) {

      const char *path;

      for (path = roadmap_path_first("config");
           path != NULL;
           path = roadmap_path_next("config", path)) {

         loaded = roadmap_factory_load_config (file_name, actions, path);
         if (loaded != NULL) break;
      }
   }
   return loaded;
}


static int roadmap_factory_load_action_labels (const char *file_name,
                                               RoadMapAction *actions,
                                               const char *path) {


   /* let the user define new long and terse labels for actions.  lines
    * in the file will usually look like:
    *    starttrip,,TripBeg
    * or
    *    gps,Focus on GPS location...,ToGPS
    * but any punctuation character can be used as the separator,
    * as long as it doesn't appear in the labels.
    */

   int n;
   int i;
   char *p;
   RoadMapAction *this_action;

   char  separator;
   char *fields[4];
   char  buffer[1024];
   FILE *file = roadmap_file_fopen (path, file_name, "sr");

   if (file == NULL) return 0;

   while (! feof(file)) {

      fgets (buffer, sizeof(buffer), file);

      if (feof(file) || ferror(file)) break;

      buffer[sizeof(buffer)-1] = 0;

      /* remove the end-of-line character, and trailing whitespace */
      p = buffer + strlen(buffer);
      while (p-- > buffer && isspace(*p))
          *p = '\0';

      /* Remove any leading space. */
      for (p = buffer; isspace(*p); ++p) ;

      if ((*p == 0) || (*p == '#')) continue; /* Comment or empty line. */

      /* Retrieve what separator character is used in this line. */
      n = strspn (p, "qwertyuiopasdfghjklzxcvbnm"
                     "QWERTYUIOPASDFGHJKLZXCVBNM"
                     "1234567890");
      if (n == 0) continue; /* Ignore invalid lines. */
      separator = p[n];


      n = roadmap_input_split (p, separator, fields, 4);
      if (n <= 1) continue; /* No label was provided. */

      this_action = roadmap_factory_find_action (actions, fields[0]);
      if (this_action == NULL) {
         roadmap_log (ROADMAP_ERROR, "invalid action name '%s' in %s%s%s",
                p, path ? path : "", path ? "/":"",  file_name);
         continue;
      }

      /* Reallocate all strings to disconnect them from the input buffer. */
      for (i = 1; i < n; ++i) {
         if (fields[i][0] == 0) {
            fields[i] = NULL; /* No value. */
         } else {
            fields[i] = strdup(fields[i]);
            roadmap_check_allocated(fields[i]);
         }
      }
      for (i = n; i < 4; ++i) fields[i] = NULL;

      if (fields[1] != NULL) {
         roadmap_log (ROADMAP_DEBUG, "assigning long label '%s' to %s",
                     fields[1], this_action->name);
         this_action->label_long = fields[1];
      }

      if (fields[2] != NULL) {
         roadmap_log (ROADMAP_DEBUG, "assigning terse label '%s' to %s",
                     fields[2], this_action->name);
         this_action->label_terse = fields[2];
      }

      if (fields[3] != NULL) {
         roadmap_log (ROADMAP_DEBUG, "assigning tip '%s' to %s",
                     fields[3], this_action->name);
         this_action->tip = fields[3];
      }
   }
   fclose(file);

   return 1;
}

static int roadmap_factory_user_action_labels
                                    (const char *name,
                                     const char *category,
                                     RoadMapAction *actions) {

   int loaded;

   char file_name[256];


   snprintf (file_name, sizeof(file_name), "%s.%s", name, category);

   loaded = roadmap_factory_load_action_labels
               (file_name, actions, roadmap_path_user());

   if (!loaded) {

      const char *path;

      for (path = roadmap_path_first("config");
           path != NULL;
           path = roadmap_path_next("config", path)) {

         loaded = roadmap_factory_load_action_labels
                              (file_name, actions, path);
         if (loaded) break;
      }
   }
   return loaded;
}

static void roadmap_factory_add_popup (RoadMapMenu menu, const char *title) {

   struct RoadMapFactoryPopup *popup;

   popup = (struct RoadMapFactoryPopup *)
      malloc (sizeof(struct RoadMapFactoryPopup));

   roadmap_check_allocated(popup);

   popup->title = title;
   popup->menu  = menu;

   popup->next = RoadMapFactoryPopupList;
   RoadMapFactoryPopupList = popup;
}

static RoadMapAction * roadmap_factory_menu_dummy_action
        (const char *title) {
   int i;
   static char tip[128];
   static RoadMapAction this_action;

   /* See if we've already allocated an action routine for
    * this popup.
    */
   for (i = 0; i < RoadMapFactoryMenuPopupCount; i++) {
      if ( strcmp (RoadMapFactoryMenuPopups[i].title, title) == 0 )
          break;
   }

   if (i == MAX_MENUPOPUPS) { /* Not found, and no room for more */

      roadmap_log (ROADMAP_ERROR,
         "No more room for menu '%s' (%d submenus max)",
         title, MAX_MENUPOPUPS );
      return NULL;

   }

   if (i == RoadMapFactoryMenuPopupCount) {
      RoadMapFactoryMenuPopups
          [RoadMapFactoryMenuPopupCount++].title = strdup(title);
   }

   snprintf(tip, sizeof(tip),
      "Submenu for %s", RoadMapFactoryMenuPopups[i].title);

   this_action.label_long = RoadMapFactoryMenuPopups[i].title;
   this_action.tip = (const char *)tip;
   this_action.callback = RoadMapFactoryMenuPopups[i].action;

   return &this_action;
}

static RoadMapAction *roadmap_factory_find_action_or_menu
                          (RoadMapAction *actions, const char *item) {

   if (strncmp (item, ROADMAP_INVOKE_SUBMENU,
                sizeof(ROADMAP_INVOKE_SUBMENU)-1) == 0) {
      return roadmap_factory_menu_dummy_action
                          (item + sizeof(ROADMAP_INVOKE_SUBMENU)-1);
   }

   return roadmap_factory_find_action (actions, item);
}

void roadmap_factory_config_menu
      (const char **item, RoadMapAction *actions,
       int doing_menus, int use_tips) {

   RoadMapMenu gui_menu = NULL;
   int menuprefix = sizeof(ROADMAP_MENU)-1;
   int submenuprefix = sizeof(ROADMAP_SUBMENU)-1;
   const char *title;

   for (; *item != NULL; item++) {

      if (strncmp (*item, ROADMAP_MENU, menuprefix) == 0) {

         int is_menu = 0;

         if (strncmp (*item, ROADMAP_SUBMENU, submenuprefix) == 0) {
            title = *item + submenuprefix;
         } else {
            title = *item + menuprefix;
            is_menu = 1;
         }

         gui_menu = roadmap_main_new_menu (title);

         /* all menus are available as popups */
         roadmap_factory_add_popup (gui_menu, title);

         /* but only non-popups go into the menubar */
         if (doing_menus && is_menu) 
            roadmap_main_add_menu (gui_menu, title);


      } else if ( gui_menu == NULL ) {

         continue;

      } else if (*item == RoadMapFactorySeparator) {

         roadmap_main_add_separator (gui_menu);

      } else if (*item == RoadMapFactoryHelpTopics) {

         roadmap_factory_add_help (gui_menu);

      } else {

         const RoadMapAction *this_action;

         this_action = roadmap_factory_find_action_or_menu (actions, *item);

         if (this_action != NULL) {
            roadmap_main_add_menu_item (gui_menu,
                  this_action->label_long,
                  use_tips ? this_action->tip : NULL,
                  this_action->callback);
         } else {
            roadmap_log
               (ROADMAP_ERROR, "invalid action name '%s' in menu", *item);
         }
      }
   }
}

void roadmap_factory_config_toolbar
                (const char **item, RoadMapAction  *actions,
                 int use_icons, int use_tips) {

   roadmap_main_add_toolbar
      (roadmap_config_get (&RoadMapConfigGeneralToolbarOrientation));

   for (; *item != NULL; item++) {

      if (*item == RoadMapFactorySeparator) {

         roadmap_main_add_tool_space ();

      } else if (strncmp (*item, ROADMAP_MENU, sizeof(ROADMAP_MENU)-1) == 0) {

         continue;

      } else {

         const RoadMapAction *this_action;

         this_action = roadmap_factory_find_action_or_menu (actions, *item);

         if (this_action != NULL) {
            roadmap_main_add_tool (roadmap_factory_terse(this_action),
                                   use_icons ? this_action->name : NULL,
                                   use_tips ? this_action->tip : NULL,
                                   this_action->callback);
         } else {
            roadmap_log 
               (ROADMAP_ERROR, "invalid action name '%s' in toolbar", *item);
         }
      }
   }
}

void roadmap_factory (const char           *name,
                            RoadMapAction  *actions,
                      const char           *menu[],
                      const char           *toolbar[]) {

   const char **userconfig;

   int use_toolbar;
   int use_tips;
   int use_icons;

   use_toolbar = roadmap_config_match (&RoadMapConfigGeneralToolbar, "yes");
   use_tips = roadmap_config_match (&RoadMapConfigGeneralTooltips, "yes");
   use_icons = roadmap_config_match (&RoadMapConfigGeneralIcons, "yes");

   roadmap_config_declare_enumeration ("preferences",
                                       &RoadMapConfigGeneralTooltips,
                                       "yes", "no", NULL);

   roadmap_config_declare_enumeration ("preferences",
                                       &RoadMapConfigGeneralToolbarOrientation,
                                       "Top",
                                       "Bottom",
                                       "Left",
                                       "Right",
                                       NULL);


   roadmap_factory_user_action_labels (name, "actionlabels", actions);

   userconfig = roadmap_factory_user_config (name, "menus", actions);
   if (userconfig != NULL) {
      /* The user supplied config.  Load the internal config
       * anyway, but only as popups, to make those menus available
       * to the user's config.  */
      roadmap_factory_config_menu(menu, actions, 0, use_tips);
      roadmap_factory_config_menu(userconfig, actions, 1, use_tips);
   } else {
      roadmap_factory_config_menu(menu, actions, 1, use_tips);
   }

   userconfig = roadmap_factory_user_config (name, "popup", actions);
   if (userconfig != NULL) {
      roadmap_factory_config_menu(userconfig, actions, 0, use_tips);
   }


   if (use_toolbar) {

      userconfig = roadmap_factory_user_config (name, "toolbar", actions);

      if (userconfig != NULL) {
         roadmap_factory_config_toolbar
            (userconfig, actions, use_icons, use_tips);
      } else {
         roadmap_factory_config_toolbar
            (toolbar, actions, use_icons, use_tips);
      }
   }

   if (RoadMapFactoryBindings != NULL) {
      roadmap_main_set_keyboard (roadmap_factory_keyboard);
   }
}


void roadmap_factory_keymap (RoadMapAction  *actions,
                             const char     *shortcuts[]) {

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


static void roadmap_factory_show_keymap (void) {

   const struct RoadMapFactoryKeyMap *binding;

   printf ("\nKEYMAP:\n");

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


void roadmap_factory_popup (const char *title,
                            const RoadMapGuiPoint *position) {

   struct RoadMapFactoryPopup *popup;

   if (title == NULL || title[0] == 0) return; /* No menu attached. */

   if (RoadMapFactoryPopupList == NULL) return; /* No menus configured */

   for (popup = RoadMapFactoryPopupList; popup != NULL; popup = popup->next) {

      if (strcmp (popup->title, title) == 0) {
         roadmap_main_popup_menu (popup->menu, position);
         return;
      }
   }
   roadmap_log (ROADMAP_ERROR, "Couldn't find configured popup menu '%s'", title);
}


void roadmap_factory_usage (const char *section, const RoadMapAction *action) {

   if ((section == NULL) || (strcasecmp(section, "KEYMAP") == 0)) {
      roadmap_factory_show_keymap();
   }

   if ((section == NULL) || (strcasecmp(section, "ACTIONS") == 0)) {

       printf ("\nACTIONS:\n");

       while (action->name != NULL) {

          printf ("%-20.20s %s\n", action->name, action->tip);
          action += 1;
       }
   }
}

