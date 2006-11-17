/* ssd_text.c - Static text widget
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
 *   See ssd_text.h
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_canvas.h"

#include "ssd_widget.h"
#include "ssd_text.h"

struct ssd_text_data {
   int size;
};

static int initialized;
static RoadMapPen pen;
static const char *def_color = "#000000";

static void init_containers (void) {
   pen = roadmap_canvas_create_pen ("ssd_text_pen");
   roadmap_canvas_set_foreground (def_color);

   initialized = 1;
}


static int format_text (SsdWidget widget, int draw,
                        RoadMapGuiRect *rect) {

   struct ssd_text_data *data = (struct ssd_text_data *) widget->data;
   char line[255] = {0};
   const char *text;
   int text_width;
   int text_ascent;
   int text_descent;
   int text_height = 0;
   int width = rect->maxx - rect->minx + 1;
   int max_width = 0;
   RoadMapGuiPoint p;
   
   if (draw) {
      p.x = rect->minx;
      p.y = rect->miny;
   }

   text = widget->value;

   while (*text || *line) {
      const char *space = strchr(text, ' ');
      const char *new_line = strchr(text, '\n');
      unsigned int len;
      unsigned int new_len;

      if (!*text) {
         new_len = strlen(line);

      } else {

         if (new_line && (!space || (new_line < space))) {
            space = new_line;
         } else {
            new_line = NULL;
         }

         if (space) {
            len = space - text;
         } else {
            len = strlen(text);
         }

         if (len > (sizeof(line) - strlen(line) - 1)) {
            len = sizeof(line) - strlen(line) - 1;
         }

         if (*line) {
            strcat (line ," ");
         }

         new_len = strlen(line) + len;
         strncpy (line + strlen(line), text, len);
         line[new_len] = '\0';

         text += len;

         if (space) text++;
      }

      roadmap_canvas_get_text_extents 
         (line, data->size, &text_width, &text_ascent,
          &text_descent, NULL);

      if (!*text || new_line || (text_width > width)) {
         int h;

         if ((text_width > width) && (new_len > len)) {
            line[new_len - len - 1] = '\0';
            text -= len;
            if (space) text--;

            roadmap_canvas_get_text_extents 
               (line, data->size, &text_width, &text_ascent,
                &text_descent, NULL);
         }

         h = text_ascent + text_descent;

         if (draw) {
            p.y += h;
            p.x = rect->minx;

            if (widget->flags & SSD_ALIGN_CENTER) {
               if (ssd_widget_rtl ()) {
                  p.x += (width - text_width) / 2;
               } else {
                  p.x -= (width - text_width) / 2;
               }
            } else if (ssd_widget_rtl ()) {
               p.x += width - text_width;
            }

            roadmap_canvas_draw_string_angle (&p, NULL, 0, data->size, line);
         }

         line[0] = '\0';

         if (text_width > max_width) max_width = text_width;

         text_height += h;
      }
   }

   rect->maxx = rect->minx + max_width - 1;
   rect->maxy = rect->miny + text_height - 1;

   return 0;
}


static void draw (SsdWidget widget, RoadMapGuiRect *rect, int flags) {

   const char *color = def_color;

   if (flags & SSD_GET_SIZE) {
      format_text (widget, 0, rect);
      return;
   }

   // compensate for descending characters
   // Doing it dynamically is not good as the text will move vertically.
   rect->miny -= 2;

   roadmap_canvas_select_pen (pen);
   if (widget->fg_color) color = widget->fg_color;
   roadmap_canvas_set_foreground (color);

   format_text (widget, 1, rect);

   rect->miny += 2;
}


static int set_value (SsdWidget widget, const char *value) {

   if (widget->callback && !widget->callback (widget, value)) return -1;

   if (widget->value && *widget->value) free (widget->value);

   if (*value) {
      int len = strlen(value);

      if (widget->flags & SSD_TEXT_LABEL) {
         len += 1;
      }

      widget->value = (char *) malloc (len + 1);
      strcpy (widget->value, value);

      if (widget->flags & SSD_TEXT_LABEL) strcat (widget->value, ":");

   } else {
      widget->value = "";
   }

   ssd_widget_reset_cache (widget);

   return 0;
}


SsdWidget ssd_text_new (const char *name, const char *value,
                        int size, int flags) {

   SsdWidget w;
   struct ssd_text_data *data =
      (struct ssd_text_data *)calloc (1, sizeof(*data));

   if (!initialized) {
      init_containers();
   }

   w = ssd_widget_new (name, flags);

   w->typeid = "Text";

   w->draw = draw;
   w->set_value = set_value;
   w->flags = flags;

   data->size = size;
   w->data = data;

   set_value (w, value);

   return w;
}


