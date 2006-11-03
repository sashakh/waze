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


static void draw_button_text (SsdWidget widget, const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   char value[255];
   char *key2 = NULL;
   RoadMapGuiPoint p = *point;
   int text_size;

   strncpy(value, widget->value, sizeof(value));
   value[sizeof(value)-1] = '\0';



   if (widget->flags & SSD_BUTTON_KEY) {
      p.x += 14;
      p.y += 18;
      text_size = 20;

      key2 = strchr(value, '|');
      if (key2) {
         *key2 = '\0';
         key2++;
      }

   } else {
      int text_width;
      int text_ascent;
      int text_descent;
      int text_height;

      text_size = 14;

      roadmap_canvas_get_text_extents 
         (value, text_size, &text_width, &text_ascent, &text_descent, NULL);

      text_height = text_ascent + text_descent;

      p.x += (widget->size.width - text_width) / 2;
      p.y += (widget->size.height - text_height) / 2 + text_height / 2;
   }

   if (data->state == BUTTON_STATE_SELECTED) {
      p.x += 2;
      p.y += 2;
   }
   roadmap_canvas_draw_string_angle (&p, NULL, 0, text_size, value);

   if (widget->flags & SSD_BUTTON_KEY) {
      if (key2 && strcmp(value, key2)) {
         p.x += 13;
         p.y += 13;

         if (data->state == BUTTON_STATE_SELECTED) {
            p.x += 2;
            p.y += 2;
         }
         roadmap_canvas_draw_string_angle (&p, NULL, 0, text_size - 5, key2);
      }
   }
}


static void format_below_text (char *text, char **lines, int max_lines) {
   *lines = text;
   lines++;
   *lines = NULL;
}


static void draw_button_text_below (SsdWidget widget,
                                    const RoadMapGuiPoint *point) {

   char text[255];
   char *lines[2];
   char **line;

   RoadMapGuiPoint p = *point;
   int text_size;

   strncpy(text, widget->value, sizeof(text));
   text[sizeof(text)-1] = '\0';

   format_below_text (text, lines, 2);

   p.y += widget->size.height + 2;

   line = lines;

   while (*line) {

      int text_width;
      int text_ascent;
      int text_descent;
      int text_height;
      RoadMapGuiPoint text_p = p;

      text_size = 14;

      roadmap_canvas_get_text_extents 
         (*line, text_size, &text_width, &text_ascent, &text_descent, NULL);

      text_height = text_ascent + text_descent;

      text_p.x += (widget->size.width - text_width) / 2;
      text_p.y += text_height;

      roadmap_canvas_draw_string_angle (&text_p, NULL, 0, text_size, *line);

      p.y += text_height + 2;
      line++;
   }
}


static void draw (SsdWidget widget, const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   RoadMapImage image;
   int i;

   for (i=data->state; i>=0; i--) {
      if (data->bitmaps[i] &&
         (image = roadmap_res_get (RES_BITMAP, 0, data->bitmaps[i]))) {
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
      roadmap_canvas_draw_image (image, point, 0, IMAGE_NORMAL);
      break;
   case BUTTON_STATE_SELECTED:
      if (i == data->state) {
         roadmap_canvas_draw_image (image, point, 0, IMAGE_NORMAL);
      } else {
         roadmap_canvas_draw_image (image, point, 0, IMAGE_SELECTED);
      }
      break;
   }

   if ((widget->flags & SSD_BUTTON_KEY) ||
       (widget->flags & SSD_BUTTON_TEXT)) {

      draw_button_text (widget, point);
   }
       
   if (widget->flags & SSD_BUTTON_TEXT_BELOW) {

      draw_button_text_below (widget, point);
   }
       

}


int ssd_button_pointer_down (SsdWidget widget, const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   data->state = BUTTON_STATE_SELECTED;

   return 0;
}


int ssd_button_short_click (SsdWidget widget, const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   if (widget->callback) {
      (*widget->callback)
         (widget->name, SSD_BUTTON_SHORT_CLICK, widget->context);
   }

   data->state = BUTTON_STATE_NORMAL;

   return 0;
}


int ssd_button_long_click (SsdWidget widget, const RoadMapGuiPoint *point) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;

   if (widget->callback) {
      (*widget->callback)
         (widget->name, SSD_BUTTON_LONG_CLICK, widget->context);
   }

   data->state = BUTTON_STATE_NORMAL;

   return 0;
}


static int set_value (SsdWidget widget, const char *value) {
   struct ssd_button_data *data = (struct ssd_button_data *) widget->data;
   RoadMapImage bmp;

   if (widget->value && *widget->value) free (widget->value);

   if (*value) widget->value = strdup(value);
   else widget->value = "";

   bmp = roadmap_res_get (RES_BITMAP, 0, data->bitmaps[0]);
   if (!bmp) return -1;

   widget->size.height = roadmap_canvas_image_height(bmp);
   widget->size.width  = roadmap_canvas_image_width(bmp);

   return 0;
}


SsdWidget ssd_button_new (const char *name, const char *value,
                          const char **bitmaps, int num_bitmaps,
                          int flags) {

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

   set_value (w, value);

   w->pointer_down = ssd_button_pointer_down;
   w->short_click  = ssd_button_short_click;
   w->long_click   = ssd_button_long_click;
   w->set_value    = set_value;

   return w;
}


