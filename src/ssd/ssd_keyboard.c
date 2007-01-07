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
#include "ssd_text.h"
#include "ssd_container.h"
#include "ssd_button.h"

#include "ssd_keyboard.h"

#define SSD_KEY_CHAR 1
#define SSD_KEY_WORD 2

#define SSD_KEY_CONFIRM   0x1
#define SSD_KEY_SPACE     0x2
#define SSD_KEY_BACKSPACE 0x4
#define SSD_KEY_SWITCH    0x8

#define SSD_KEYBOARD_MULTI 0x1
#define SSD_KEYBOARD_LTR   0x2

typedef struct key_set_t {
   const char   *key_images[2];
   const char   *action_images[2];
   int size;
} SsdKeySet;
   
static SsdKeySet key_sets[] = {
   {
      {"key1_large_up.png", "key1_large_down.png"},
      {"key2_large_up.png", "key2_large_down.png"},
      46
   },
   {
      {"key1_medium_up.png", "key1_medium_down.png"},
      {"key2_medium_up.png", "key2_medium_down.png"},
      40
   }
};

typedef struct ssd_keyboard {
   const char   *name;
   int           type;
   int           flags;
   const char   *keys;
   int           special_keys;
   const char   *extra_key;
   SsdWidget     widget;
   SsdDialogCB   callback;
   void *context;
   int key_set;
} SsdKeyboard;

static SsdKeyboard *CurrentKeyboard = NULL;

static void show_keyboard (SsdKeyboard *keyboard, const char *title,
                           const char *value, const char *extra_key,
                           SsdDialogCB callback, void *context);

SsdKeyboard keyboards[] = {
   {"letters",
    SSD_KEYBOARD_LETTERS,
    SSD_KEYBOARD_MULTI, 
    "א1ב2ג3ד4ה5ו6ז7ח8ט9י0כךללמםנןס-ע\"פףצץקקר:ש,ת.||",
    SSD_KEY_CONFIRM | SSD_KEY_SPACE | SSD_KEY_BACKSPACE | SSD_KEY_SWITCH,
    NULL,
    NULL,
    NULL,
    NULL,
    0},

   {"english",
    SSD_KEYBOARD_LETTERS,
    SSD_KEYBOARD_MULTI|SSD_KEYBOARD_LTR, 
    "a1b2c3d4e5f6g7h8i9j0kkllmmnnooppqqrrssttuuvvwwx:y,z.||",
    SSD_KEY_CONFIRM | SSD_KEY_SPACE | SSD_KEY_BACKSPACE | SSD_KEY_SWITCH,
    NULL,
    NULL,
    NULL,
    NULL,
    0},

   {"digits",
    SSD_KEYBOARD_DIGITS,
    SSD_KEYBOARD_LTR,
    "112233||445566||778899||00||",
    SSD_KEY_CONFIRM | SSD_KEY_BACKSPACE | SSD_KEY_SWITCH,
    NULL,
    NULL,
    NULL,
    NULL,
    0}
};

static void switch_type(SsdKeyboard *keyboard) {
   SsdKeyboard *new_keyboard;
   unsigned int i;

   for (i=0; i < sizeof(keyboards) / sizeof(keyboards[0]); i++) {
      if (&keyboards[i] == keyboard) break;
   }

   i = (i + 1) % (sizeof(keyboards) / sizeof(keyboards[0]));

   new_keyboard = &keyboards[i];

   show_keyboard (new_keyboard,
                  ssd_widget_get_value (keyboard->widget, NULL),
                  ssd_widget_get_value (keyboard->widget, "input"),
                  keyboard->extra_key,
                  keyboard->callback,
                  keyboard->context);

   ssd_dialog_hide (keyboard->name);
}


static void calc_key_size (SsdKeyboard *keyboard, wchar_t *wkeys, int count,
                           SsdSize *size) {

   unsigned int set;
   int i;

   for (set=0; set<sizeof(key_sets) / sizeof(SsdKeySet); set++) {
      int key_size = key_sets[set].size;
      int cur_x = 0;
      int cur_y = 0;

      keyboard->key_set = set;

      if (keyboard->special_keys) {
         cur_y += key_size;
      }

      for (i=0; i<count/2; i++) {
         char key1[20];
         int len;

         if ((len = wcrtomb(key1, wkeys[i*2], NULL)) <= 0) continue;
         key1[len] = '\0';

         if (key1[0] == '|') {
            cur_x = 0;
            cur_y += key_size;
            if (cur_y > size->height) break;
            continue;
         }

         cur_x += key_size;
         if (cur_x > size->width) {
            cur_x = key_size;
            cur_y += key_size;
            if (cur_y > size->height) break;
         }
      }

      if (i == count/2) return;
   }
}


static int button_callback (SsdWidget widget, const char *new_value) {

   wchar_t wkeys[3];
   char key[20];
   char text[255];
   int count;
   int len;

   SsdKeyboard *keyboard;
   SsdWidget k = widget->parent;

   keyboard = (SsdKeyboard *)k->context;

   if (keyboard->extra_key && !strcmp (keyboard->extra_key, widget->name)) {
      return (*keyboard->callback)
               (SSD_KEYBOARD_EXTRA, ssd_dialog_get_value ("input"),
               keyboard->context);

   } else if (!strcmp (roadmap_lang_get ("Ok"), widget->name)) {
      return (*keyboard->callback)
               (SSD_KEYBOARD_OK, ssd_dialog_get_value ("input"),
               keyboard->context);

   } else if (!strcmp (roadmap_lang_get ("Del"), widget->name)) {
      wchar_t wtext[255];
      const char *t = ssd_dialog_get_value ("input");
      int value_len = strlen(t);

      if (!value_len) return 1;

      count = mbsrtowcs(wtext, &t, sizeof(wtext) / sizeof(wchar_t), NULL);

      len = wcrtomb(key, wtext[count-1], NULL);

      if ((len <= 0) || (len > value_len)) return 1;

      if (len == value_len) {
         ssd_dialog_set_value ("input", "");
         return 1;
      }

      strncpy(text, ssd_dialog_get_value ("input"), sizeof(text));
      text[sizeof(text)-1] = '\0';

      text[strlen(text) - len] = '\0';

      ssd_dialog_set_value ("input", text);

      return 1;

   } else if (!strcmp (roadmap_lang_get ("SPC"), widget->name)) {
      len = 1;
      strcpy (key, " ");

   } else if (!strcmp (roadmap_lang_get ("SWCH"), widget->name)) {
      switch_type(keyboard);

   } else {

      const char *value = widget->value;

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

   return 1;
}


static int input_callback (SsdWidget widget, const char *new_value) {
   SsdKeyboard *keyboard;
   SsdWidget k = widget->parent->parent;

   keyboard = (SsdKeyboard *)k->context;

   if (!keyboard) return 1;

   return (*keyboard->callback) (0, new_value, keyboard->context);
}


static SsdWidget add_key (SsdWidget container,
                          const char *key1, const char *key2, int type,
                          int flags, int key_set) {

   char name[255];
   char value[255];
   SsdWidget button;
   const int KEY_TEXT_SIZE = 20;
   int txt_offset = (key_sets[key_set].size - KEY_TEXT_SIZE) / 2 - 0;

   strcpy(name, key1);
   strcpy(value, key1);

   if (type == SSD_BUTTON_KEY) {
      strcat(value, "|");
      strcat(value, key2);
      button =
         ssd_button_new (name, value, key_sets[key_set].key_images, 2,
               SSD_ALIGN_CENTER|flags,
               button_callback);
   } else {
      button =
         ssd_button_new (name, value, key_sets[key_set].action_images, 2,
               SSD_ALIGN_CENTER|flags,
               button_callback);
   }

   if (type == SSD_BUTTON_KEY) {
      SsdWidget w = ssd_text_new ("key1", key1, KEY_TEXT_SIZE, SSD_END_ROW);

      ssd_widget_set_offset (w, txt_offset, 3);
      ssd_widget_add (button, w);
   } else {
      ssd_widget_add (button, ssd_text_new ("key1", key1, 14, SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_END_ROW));
   }

   if ((type == SSD_BUTTON_KEY) && strcmp (key1, key2)) {

      SsdWidget w = ssd_text_new ("key2", key2, 13, SSD_ALIGN_RIGHT|SSD_END_ROW);

      if (flags & SSD_ALIGN_LTR) txt_offset = -txt_offset;
      ssd_widget_set_color (w, "#ee0000", 0);
      ssd_widget_set_offset (w, txt_offset, 3);
      ssd_widget_add (button, w);
   }

   ssd_widget_add (container, button);
   return button;
}

static void ssd_keyboard_new (SsdKeyboard *keyboard) {

   SsdWidget dialog;
   SsdWidget entry;
   SsdWidget input;
   SsdSize   size;
   SsdSize   entry_size;

   wchar_t wkeys[100];
   const char *keys = keyboard->keys;
   int flags;
   int count;
   int i;

   dialog = ssd_dialog_new (keyboard->name, "", SSD_CONTAINER_TITLE);
   keyboard->widget = dialog;

   ssd_widget_set_color (dialog, "#000000", "#000000");

   entry = ssd_container_new ("entry", NULL, SSD_MAX_SIZE, 19,
                             SSD_CONTAINER_BORDER|SSD_END_ROW);

   ssd_widget_set_color (entry, "#000000", "#ffffff");

   input = ssd_text_new ("input", "", 15, 0);
   ssd_widget_set_callback (input, input_callback);

   ssd_widget_add (entry, input);

   ssd_widget_add (dialog, entry);

   ssd_widget_get_size (dialog, &size, NULL);
   ssd_widget_get_size (entry, &entry_size, NULL);

   size.height -= entry_size.height;

   count = mbsrtowcs(wkeys, &keys,
                     sizeof(wkeys) / sizeof(wchar_t), NULL);
   
   calc_key_size (keyboard, wkeys, count, &size);

   flags = 0;
   for (i=0; i<count/2; i++) {
      char key1[20];
      char key2[20];
      int len;

      if ((len = wcrtomb(key1, wkeys[i*2], NULL)) <= 0) continue;
      key1[len] = '\0';
      if ((len = wcrtomb(key2, wkeys[i*2+1], NULL)) <= 0) continue;
      key2[len] = '\0';

      if (key1[0] == '|') {
         flags = SSD_START_NEW_ROW;
         continue;
      }

      if (keyboard->flags & SSD_KEYBOARD_LTR) {
         flags |= SSD_ALIGN_LTR;
      }

      if (keyboard->flags & SSD_KEYBOARD_MULTI) {
         add_key (keyboard->widget, key1, key2, SSD_BUTTON_KEY, flags,
                  keyboard->key_set);
      } else {
         add_key (keyboard->widget, key1, key2, SSD_BUTTON_TEXT, flags,
                  keyboard->key_set);
      }
      flags = 0;
   }

   if (keyboard->special_keys & SSD_KEY_CONFIRM) {
      add_key (keyboard->widget, roadmap_lang_get ("Ok"),
               NULL, SSD_BUTTON_TEXT, flags, keyboard->key_set);
      flags = 0;
   }

   if (keyboard->special_keys & SSD_KEY_BACKSPACE) {
      add_key (keyboard->widget, roadmap_lang_get ("Del"),
               NULL, SSD_BUTTON_TEXT, flags, keyboard->key_set);
      flags = 0;
   }

   if (keyboard->special_keys & SSD_KEY_SPACE) {
      add_key (keyboard->widget, roadmap_lang_get ("SPC"),
               NULL, SSD_BUTTON_TEXT, flags, keyboard->key_set);
      flags = 0;
   }

   if (keyboard->special_keys & SSD_KEY_SWITCH) {
      add_key (keyboard->widget, roadmap_lang_get ("SWCH"),
               NULL, SSD_BUTTON_TEXT, flags, keyboard->key_set);
      flags = 0;
   }
}


static void set_title (SsdWidget keyboard, const char *title) {
   keyboard->set_value (keyboard, title);
}


static void set_value (SsdWidget keyboard, const char *title) {
   ssd_widget_set_value (keyboard, "input", title);
}


static void hide_extra_key (SsdKeyboard *keyboard) {

   SsdWidget key;

   if (!keyboard->extra_key) return;

   key = ssd_widget_get (keyboard->widget, keyboard->extra_key);

   key->flags |= SSD_WIDGET_HIDE;
}


static void setup_extra_key (SsdKeyboard *keyboard) {

   SsdWidget key;

   if (!keyboard->extra_key) return;

   key = ssd_widget_get (keyboard->widget, keyboard->extra_key);

   if (!key) key = add_key (keyboard->widget,
                            keyboard->extra_key, keyboard->extra_key,
                            SSD_BUTTON_TEXT, 0, keyboard->key_set);

   key->flags &= ~SSD_WIDGET_HIDE;
}


static void show_keyboard (SsdKeyboard *keyboard, const char *title,
                           const char *value, const char *extra_key,
                           SsdDialogCB callback, void *context) {

   if (!keyboard->widget) {
      ssd_keyboard_new (keyboard);
   }

   keyboard->callback = callback;
   keyboard->context = context;
   keyboard->extra_key = extra_key;

   ssd_widget_set_context (keyboard->widget, keyboard);

   set_title (keyboard->widget, title);
   set_value (keyboard->widget, value);

   setup_extra_key (keyboard);

   CurrentKeyboard = keyboard;
   ssd_dialog_activate (keyboard->name, NULL);

   ssd_dialog_draw ();
}


void ssd_keyboard_show (int type, const char *title, const char *value,
                        const char *extra_key, SsdDialogCB callback,
                        void *context) {

   SsdKeyboard *keyboard;
   unsigned int i;

   for (i=0; i < sizeof(keyboards) / sizeof(keyboards[0]); i++) {
      if (keyboards[i].type == type) break;
   }

   i = i % (sizeof(keyboards) / sizeof(keyboards[0]));

   keyboard = &keyboards[i];

   show_keyboard (keyboard, title, value, extra_key, callback, context);
}


void ssd_keyboard_hide (void) {

   if (CurrentKeyboard) {
      hide_extra_key (CurrentKeyboard);
      ssd_dialog_hide (CurrentKeyboard->name);
      CurrentKeyboard = NULL;
   }
}

