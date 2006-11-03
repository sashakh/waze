/* ssd_keyboard.c - Full screen keyboard
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
 *   See ssd_keyboard.h.
 */

#include <wchar.h>
#ifdef UNDER_CE
size_t __cdecl mbsrtowcs(wchar_t *, const char **, size_t, mbstate_t *);
size_t __cdecl wcrtomb(char *, wchar_t, mbstate_t *);
#endif

#include <string.h>
#include <stdlib.h>

#include "roadmap_lang.h"
#include "roadmap_path.h"
#include "ssd_dialog.h"
#include "ssd_button.h"

#define SSD_KEY_CHAR 1
#define SSD_KEY_WORD 2

#define SSD_KEY_CONFIRM   0x1
#define SSD_KEY_SPACE     0x2
#define SSD_KEY_BACKSPACE 0x4

typedef struct ssd_keyboard {
   const char *name;
   const char *keys;
   int special_keys;
} SsdKeyboard;

SsdKeyboard def_keyboards[] = {
   {"letters_kbd",
    "א1ב2ג3ד4ה5ו6ז7ח8ט9י0כךללמםנןססעעפףצץקקר:ש,ת.",
    SSD_KEY_CONFIRM | SSD_KEY_SPACE | SSD_KEY_BACKSPACE}
};

static int button_callback (const char *name, const char *new_value,
                            void *context) {
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

      const char *value = ssd_dialog_get_value (name);
      count = mbsrtowcs(wkeys, &value, sizeof(wkeys) / sizeof(wchar_t), NULL);
      if (count <= 0) return 1;

      if (!strcmp(new_value, SSD_BUTTON_SHORT_CLICK)) {

         if ((len = wcrtomb(key, wkeys[0], NULL)) <= 0) return 1;
      } else {

         if ((len = wcrtomb(key, wkeys[2], NULL)) <= 0) return 1;
      }

      key[len] = '\0';
   }

   strncpy(text, ssd_dialog_get_value ("input"), sizeof(text));
   text[sizeof(text)-1] = '\0';

   if ((strlen(text) + len) >= sizeof(text)) return 1;

   strcat(text, key);

   ssd_dialog_set_value ("input", text);

   return 0;
}

static void add_key (const char *key1, const char *key2, int type) {

   char name[255];
   char value[255];
   const char *icons[] = {
      "key_button1.bmp",
      "key_button2.bmp"
   };

   strcpy(name, key1);
   strcpy(value, key1);

   if (type == SSD_BUTTON_KEY) {
      strcat(value, "|");
      strcat(value, key2);
   }

   ssd_dialog_new_button (name, value, icons, 2, type, button_callback);
}

static void ssd_keyboard_new (SsdKeyboard *keyboard) {

   wchar_t wkeys[100];
   const char *keys = keyboard->keys;
   int count;
   int i;

   if (!ssd_dialog_activate (keyboard->name, NULL)) return;

   count = mbsrtowcs(wkeys, &keys,
                         sizeof(wkeys) / sizeof(wchar_t), NULL);

   ssd_dialog_new (keyboard->name, "", 0);
   ssd_dialog_new_entry ("input", "", 0, NULL);

   for (i=0; i<count/2; i++) {
      char key1[20];
      char key2[20];
      int len;

      if ((len = wcrtomb(key1, wkeys[i*2], NULL)) <= 0) continue;
      key1[len] = '\0';
      if ((len = wcrtomb(key2, wkeys[i*2+1], NULL)) <= 0) continue;
      key2[len] = '\0';

      add_key (key1, key2, SSD_BUTTON_KEY);

   }

   ssd_dialog_new_line ();

   if (keyboard->special_keys & SSD_KEY_CONFIRM) {
      add_key (roadmap_lang_get ("Ok"), NULL, SSD_BUTTON_TEXT);
   }

   if (keyboard->special_keys & SSD_KEY_BACKSPACE) {
      add_key (roadmap_lang_get ("Del"), NULL, SSD_BUTTON_TEXT);
   }

   if (keyboard->special_keys & SSD_KEY_SPACE) {
      add_key (roadmap_lang_get ("SPC"), NULL, SSD_BUTTON_TEXT);
   }

   ssd_dialog_activate (keyboard->name, NULL);
   ssd_dialog_draw ();
}

void ssd_keyboard_show (void) {
   ssd_keyboard_new (&def_keyboards[0]);
}

