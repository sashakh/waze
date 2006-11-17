/* ssd_messagebox.c - ssd messagebox.
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai.
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
 *   See roadmap_messagebox.h
 */

#include <stdlib.h>
#include "ssd_dialog.h"
#include "ssd_text.h"
#include "ssd_container.h"
#include "ssd_button.h"

#include "roadmap_lang.h"

#include "roadmap_messagebox.h"


static int button_callback (SsdWidget widget, const char *new_value) {
   ssd_dialog_hide ("message_box");
   return 0;
}

static void create_messagebox (void) {

   SsdWidget dialog;
   SsdWidget button;
   const char *ok_button[]   = {"button.bmp"};

   dialog = ssd_dialog_new ("message_box", "",
         SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|SSD_DIALOG_FLOAT|
         SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);

   ssd_widget_add (dialog,
      ssd_text_new ("text", "", 13, SSD_END_ROW|SSD_WIDGET_SPACE));

   /* Spacer */
   ssd_widget_add (dialog,
      ssd_container_new ("spacer1", NULL, 0, 20, SSD_END_ROW));

   button = ssd_button_new ("confirm", "", ok_button, 1,
                       SSD_ALIGN_CENTER|SSD_START_NEW_ROW, button_callback);

   ssd_widget_add (button,
      ssd_text_new ("ok", roadmap_lang_get ("Ok"), 16,
                    SSD_ALIGN_VCENTER| SSD_ALIGN_CENTER));

   ssd_widget_add (dialog, button);
}


void roadmap_messagebox (const char *title, const char *text) {

   SsdWidget dialog = ssd_dialog_activate ("message_box", NULL);
   title = roadmap_lang_get (title);
   text  = roadmap_lang_get (text);

   if (!dialog) {
      create_messagebox ();
      dialog = ssd_dialog_activate ("message_box", NULL);
   }

   dialog->set_value (dialog, title);
   ssd_widget_set_value (dialog, "text", text);

   ssd_dialog_draw ();
}

