/* ssd_button.c - Bitmap button widget
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
 *   See ssd_button.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_res.h"
#include "roadmap_canvas.h"

#include "ssd_widget.h"
#include "ssd_text.h"
#include "ssd_button.h"

/* Buttons states */
#define BUTTON_STATE_NORMAL   0
#define BUTTON_STATE_SELECTED 1
#define BUTTON_STATE_DISABLED 2
#define MAX_STATES            3

struct ssd_button_data {
   int state;
   const char *bitmaps[MAX_STATES];
};


static void draw (SsdWidget widget, RoadMapGuiRect *rect, int flags) {

   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   RoadMapGuiPoint point = {rect->minx, rect->miny};
   RoadMapImage image;
   int i;

   if ((flags & SSD_GET_SIZE)) return;

   for (i=data->state; i>=0; i--) {
      if (data->bitmaps[i] &&
         (image = roadmap_res_get (RES_BITMAP, RES_SKIN, data->bitmaps[i]))) {
         break;
      }
   }

   if (!image) {
      roadmap_log (ROADMAP_ERROR, "SSD - Can't get image for button widget: %s",
      widget->name);
      return;
   }

   switch (data->state) {
   case BUTTON_STATE_NORMAL:
      roadmap_canvas_draw_image (image, &point, 0, IMAGE_NORMAL);
      break;
   case BUTTON_STATE_SELECTED:
      if (i == data->state) {
         roadmap_canvas_draw_image (image, &point, 0, IMAGE_NORMAL);
      } else {
         roadmap_canvas_draw_image (image, &point, 0, IMAGE_SELECTED);
      }
      break;
   }

#if 0
   if ((widget->flags & SSD_BUTTON_KEY) ||
       (widget->flags & SSD_BUTTON_TEXT)) {

      draw_button_text (widget, point);
   }
       
   if (widget->flags & SSD_BUTTON_TEXT_BELOW) {

      draw_button_text_below (widget, point);
   }
#endif

}


static int ssd_button_pointer_down (SsdWidget widget,
                                    const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   data->state = BUTTON_STATE_SELECTED;

   return 1;
}


static int ssd_button_short_click (SsdWidget widget,
                                   const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   if (widget->callback) {
      (*widget->callback) (widget, SSD_BUTTON_SHORT_CLICK);
   }

   data->state = BUTTON_STATE_NORMAL;

   return 1;
}


static int ssd_button_long_click (SsdWidget widget,
                                  const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   if (widget->callback) {
      (*widget->callback) (widget, SSD_BUTTON_LONG_CLICK);
   }

   data->state = BUTTON_STATE_NORMAL;

   return 1;
}


static int set_value (SsdWidget widget, const char *value) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   RoadMapImage bmp;

   if (widget->value && *widget->value) free (widget->value);

   if (*value) widget->value = strdup(value);
   else widget->value = "";

   bmp = roadmap_res_get (RES_BITMAP, RES_SKIN, data->bitmaps[0]);
   if (!bmp) {
      widget->size.height = widget->size.width = 0;

      return -1;
   }

   widget->size.height = roadmap_canvas_image_height(bmp);
   widget->size.width  = roadmap_canvas_image_width(bmp);

   return 0;
}


SsdWidget ssd_button_new (const char *name, const char *value,
                          const char **bitmaps, int num_bitmaps,
                          int flags, SsdCallback callback) {

   SsdWidget w;
   struct ssd_button_data *data =
      (struct ssd_button_data *)calloc (1, sizeof(*data));
   int i;

   w = ssd_widget_new (name, flags);

   w->typeid = "Button";

   w->draw  = draw;
   w->flags = flags;

   data->state  = BUTTON_STATE_NORMAL;
   for (i=0; i<num_bitmaps; i++) data->bitmaps[i] = bitmaps[i];

   w->data = data;
   w->callback = callback;

   set_value (w, value);

   w->pointer_down = ssd_button_pointer_down;
   w->short_click  = ssd_button_short_click;
   w->long_click   = ssd_button_long_click;
   w->set_value    = set_value;

   return w;
}


SsdWidget ssd_button_label (const char *name, const char *label,
                            int flags, SsdCallback callback) {

   const char *button_icon[]   = {"button"};
   SsdWidget button = ssd_button_new (name, "", button_icon, 1,
                                      flags, callback);
   ssd_widget_add (button,
      ssd_text_new ("label", label, -1, SSD_ALIGN_VCENTER| SSD_ALIGN_CENTER));

   return button;
}

