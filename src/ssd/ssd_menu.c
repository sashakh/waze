/* ssd_menu.c - Icons menu
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
 *   See ssd_menu.h.
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "roadmap_factory.h"
#include "ssd_dialog.h"
#include "ssd_button.h"

#include "ssd_menu.h"


static int button_callback (const char *name, const char *new_value,
                            void *context) {
#if 0
   wchar_t wkeys[3];
   char key[20];
   char text[255];
   int count;
   int len;

   if (!strcmp (roadmap_lang_get ("Ok"), name)) {
      return 0;

   } else if (!strcmp (roadmap_lang_get ("Del"), name)) {
      wchar_t wtext[255];
      const char *t = ssd_dialog_get_value ("input");
      int value_len = strlen(t);

      if (!value_len) return 1;

      count = mbsrtowcs(wtext, &t, sizeof(wtext) / sizeof(wchar_t), NULL);

      len = wcrtomb(key, wtext[count-1], NULL);

      if ((len <= 0) || (len > value_len)) return 1;

      if (len == value_len) {
         ssd_dialog_set_value ("input", "");
         return 0;
      }

      strncpy(text, ssd_dialog_get_value ("input"), sizeof(text));
      text[sizeof(text)-1] = '\0';

      text[strlen(text) - len] = '\0';

      ssd_dialog_set_value ("input", text);

      return 0;

   } else if (!strcmp (roadmap_lang_get ("SPC"), name)) {
      len = 1;
      strcpy (key, " ");

   } else {

      count = mbsrtowcs(wkeys, &name, sizeof(wkeys) / sizeof(wchar_t), NULL);
      if (count <= 0) return 1;

      if (!strcmp(new_value, SSD_BUTTON_SHORT_CLICK)) {

         if ((len = wcrtomb(key, wkeys[0], NULL)) <= 0) return 1;
      } else {

         if ((len = wcrtomb(key, wkeys[1], NULL)) <= 0) return 1;
      }

      key[len] = '\0';
   }

   strncpy(text, ssd_dialog_get_value ("input"), sizeof(text));
   text[sizeof(text)-1] = '\0';

   if ((strlen(text) + len) >= sizeof(text)) return 1;

   strcat(text, key);

   ssd_dialog_set_value ("input", text);

#endif
   return 0;
}


static void ssd_menu_new (const char           *name,
                          const char           *items[],
                          const RoadMapAction  *actions) {


   int i;
   const char **menu_items;
   const char *icons[] = {
      "sync.bmp"
   };

   ssd_dialog_new (name, "", 0);
   menu_items = items;

   for (i = 0; menu_items[i] != NULL; ++i) {

      const char *item = menu_items[i];

      if (item == RoadMapFactorySeparator) {

      } else {

         ssd_dialog_new_button (item, "test", icons, 1,
                                SSD_ALIGN_CENTER | SSD_BUTTON_TEXT_BELOW,
                                button_callback);
      }
   }

   ssd_dialog_activate (name, NULL);
}


void ssd_menu_activate (const char           *name,
                        const char           *items[],
                        const RoadMapAction  *actions) {

   if (!ssd_dialog_activate (name, NULL)) return;

   ssd_menu_new (name, items, actions);
}

