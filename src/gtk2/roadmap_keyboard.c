/* roadmap_keyboard.c - Provide a keyboard widget for RoadMap dialogs.
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
 *   See roadmap_keyboard.h.
 */

#include <math.h>
#include <stdlib.h>

#include "roadmap_keyboard.h"

#include "roadmap.h"


#define ROADMAP_KEYBOARD_ROWS      4
#define ROADMAP_KEYBOARD_COLUMNS  10
#define ROADMAP_KEYBOARD_KEYS (ROADMAP_KEYBOARD_ROWS*ROADMAP_KEYBOARD_COLUMNS)

typedef struct {

   char character;

   GtkWidget       *button;
   RoadMapKeyboard  keyboard;

} RoadMapKey;


struct roadmap_keyboard_context {

   GtkWidget *frame;
   GtkWidget *focus;

   RoadMapKey keys[ROADMAP_KEYBOARD_KEYS];
};


static gint roadmap_keyboard_pressed (GtkWidget *w, gpointer data) {

   gint cursor;

   RoadMapKey *key = (RoadMapKey *) data;
   RoadMapKeyboard keyboard = (RoadMapKeyboard) key->keyboard;


   if (keyboard->focus != NULL) {

      cursor = gtk_editable_get_position (GTK_EDITABLE(keyboard->focus));

      if (key->character >= ' ') {

         char text[2];

         text[0] = key->character;
         text[1] = 0;
         gtk_editable_insert_text (GTK_EDITABLE(keyboard->focus),
                                   text, 1, &cursor);
         gtk_editable_set_position (GTK_EDITABLE(keyboard->focus), cursor);

      } else if (key->character == '\b') {

         cursor = gtk_editable_get_position (GTK_EDITABLE(keyboard->focus));

         gtk_editable_delete_text
            (GTK_EDITABLE(keyboard->focus), cursor-1, cursor);

      } else if (key->character == '\r') {

         gtk_editable_delete_text (GTK_EDITABLE(keyboard->focus), 0, -1);
      }
   }

   return FALSE;
}



static GtkWidget *roadmap_keyboard_add_key (RoadMapKey *key, char character) {

   char label[3]; /* Provision for UTF8 2-bytes sequences. */

   key->character = character;

   label[0] = character;
   label[1] = 0;

   if (character < ' ') {
      if (character == '\r') {
         /* Note: we use the '<<' character, which is not a standard ASCII
          * 7 bits character (it is part of the PC 8 bits extension set).
          * Therefore we have to use the proper multibyte UTF8 sequence.
          */
         label[0] = 0xc2; /* The 2 high order bits. */
         label[1] = 0xab; /* The 6 low order bits. */
         label[2] = 0;
      }
      if (character == '\b') {
         label[0] = '<';
      }
   }

   key->button = gtk_button_new_with_label (label);

   GTK_WIDGET_UNSET_FLAGS (key->button, GTK_CAN_FOCUS);

   g_signal_connect (key->button,
                     "clicked",
                     (GCallback) roadmap_keyboard_pressed, key);

   return key->button;
}


RoadMapKeyboard roadmap_keyboard_new (void) {

   int i;
   int j;
   int k;
   char keymap[] = "1234567890qwertyuiopasdfghjkl\b-zxcvbnm \r";

   RoadMapKeyboard keyboard;
   GtkWidget *button;

   keyboard = (RoadMapKeyboard) malloc (sizeof(*keyboard));
   roadmap_check_allocated(keyboard);

   keyboard->frame =
      gtk_table_new (ROADMAP_KEYBOARD_ROWS, ROADMAP_KEYBOARD_COLUMNS, TRUE);

   keyboard->focus = NULL;

   for (i = 0; i < ROADMAP_KEYBOARD_ROWS; i++) {

      for (j = 0; j < ROADMAP_KEYBOARD_COLUMNS; j++) {

         k = (ROADMAP_KEYBOARD_COLUMNS * i) + j;

         if (keymap[k] == 0) return keyboard;

         button = roadmap_keyboard_add_key (&keyboard->keys[k], keymap[k]);
         gtk_table_attach_defaults
            (GTK_TABLE(keyboard->frame), button, j, j+1, i, i+1);

         keyboard->keys[k].keyboard = keyboard;
      }
   }

   return keyboard;
}


GtkWidget *roadmap_keyboard_widget (RoadMapKeyboard keyboard) {

   return keyboard->frame;
}


void roadmap_keyboard_set_focus (RoadMapKeyboard keyboard, GtkWidget *w) {

   keyboard->focus = w;
}

