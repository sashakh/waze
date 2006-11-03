/* ssd_widget.c - small screen devices Widgets (designed for touchscreens)
 * (requires agg)
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
 *   See ssd_widget.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_lang.h"
#include "ssd_widget.h"

#define SSD_WIDGET_SEP 2

SsdWidget ssd_widget_get (SsdWidget child, const char *name) {

   while (child != NULL) {
      if (strcmp (child->name, name) == 0) {
         return child;
      }

      if (child->children != NULL) {
         SsdWidget w = ssd_widget_get (child->children, name);
         if (w) return w;
      }

      child = child->next;
   }

   return NULL;
}


static void ssd_widget_draw_one (SsdWidget w, int x, int y, int height) {

   w->position.x = x;
   w->position.y = y;
   w->position.y += (height - w->size.height) / 2;
   w->draw(w, &w->position);
}


static int ssd_widget_draw_row (SsdWidget *w, int count, int width,
                                int x, int y) {
   int height = 0;
   int cur_x;
   int total_width;
   int space;
   int i;

   if (ssd_widget_rtl ()) cur_x = x;
   else cur_x = x + width;

   for (i=0; i<count; i++) {
      if (w[i]->size.height > height) height = w[i]->size.height;
   }

   for (i=count-1; i>0; i--) {
      if (w[i]->flags & SSD_ALIGN_RIGHT) {
         if (ssd_widget_rtl ()) {
            ssd_widget_draw_one (w[i], cur_x, y, height);
            cur_x += w[i]->size.width + SSD_WIDGET_SEP;
         } else {
            cur_x -= w[i]->size.width;
            ssd_widget_draw_one (w[i], cur_x, y, height);
            cur_x -= SSD_WIDGET_SEP;
         }
         width -= w[i]->size.width + SSD_WIDGET_SEP;
         count--;
      } else {
         break;
      }
   }

   if (ssd_widget_rtl ()) cur_x = x + width;
   else cur_x = x;

   while (count > 0) {
      if (w[0]->flags & SSD_ALIGN_CENTER) {
         break;
      }

      if (ssd_widget_rtl ()) {
         cur_x -= w[0]->size.width;
         ssd_widget_draw_one (w[0], cur_x, y, height);
         cur_x -= SSD_WIDGET_SEP;
      } else {
         ssd_widget_draw_one (w[0], cur_x, y, height);
         cur_x += w[0]->size.width + SSD_WIDGET_SEP;
      }

      width -= w[0]->size.width + SSD_WIDGET_SEP;
      w++;
      count--;
   }

   if (count == 0) return height + SSD_WIDGET_SEP;

   /* align center */

   total_width = 0;
   for (i=0; i<count; i++) {
      total_width += w[i]->size.width;
   }

   space = (width - total_width) / (count + 1);

   for (i=0; i<count; i++) {
      if (ssd_widget_rtl ()) {
         cur_x -= space;
         cur_x -= w[i]->size.width;
         ssd_widget_draw_one (w[i], cur_x, y, height);
      } else {
         cur_x += space;
         ssd_widget_draw_one (w[i], cur_x, y, height);
         cur_x += w[i]->size.width;
      }
   }

   return height + SSD_WIDGET_SEP;
}


static SsdWidget ssd_widget_find_by_pos (SsdWidget widget,
                                         const RoadMapGuiPoint *point) {

   while (widget != NULL) {

      if ((widget->position.x <= point->x) &&
          ((widget->position.x + widget->size.width) >= point->x) &&
          (widget->position.y <= point->y) &&
          ((widget->position.y + widget->size.height) >= point->y)) {

         return widget;
      }

      widget = widget->next;
   }

   return NULL;
}


void ssd_widget_draw (SsdWidget w, const RoadMapGuiRect *rect) {
   SsdWidget row[100];
   int width  = rect->maxx - rect->minx + 1;
   //int height = rect->maxy - rect->miny + 1;
   int cur_y = rect->miny;
   int cur_width = 0;
   int count = 0;

   while (w != NULL) {

      if ((count == sizeof(row)/sizeof(SsdWidget)) ||
         ((count > 0) && ((cur_width + w->size.width) >= width))) {
         cur_y += ssd_widget_draw_row (row, count, width, rect->minx, cur_y);
         count = 0;
         cur_width = 0;

      } else if (count && (w->flags & SSD_START_NEW_ROW)) {
         cur_y += ssd_widget_draw_row (row, count, width, rect->minx, cur_y);
         count = 0;
         cur_width = 0;
      }

      row[count++] = w;
      cur_width += w->size.width + SSD_WIDGET_SEP;

      if (w->flags & SSD_END_ROW) {
         cur_y += ssd_widget_draw_row (row, count, width, rect->minx, cur_y);
         count = 0;
         cur_width = 0;
      }

      w = w->next;
   }

   if (count) ssd_widget_draw_row (row, count, width, rect->minx, cur_y);
}


SsdWidget ssd_widget_new (const char *name, int flags) {

   SsdWidget w;

   w = (SsdWidget) calloc (1, sizeof (*w));

   roadmap_check_allocated(w);

   w->name          = strdup(name);

   return w;
}


void ssd_widget_set_callback (SsdWidget widget, SsdCallback callback) {
   widget->callback = callback;
}


int ssd_widget_rtl (void) {
   return roadmap_lang_rtl ();
}


void ssd_widget_pointer_down (SsdWidget widget, RoadMapGuiPoint *point) {

   widget = ssd_widget_find_by_pos (widget, point);

   if (!widget) return;

   if (widget->pointer_down && !widget->pointer_down(widget, point)) {
      return;

   } else if (widget->children != NULL) {

      ssd_widget_pointer_down (widget->children, point);
   }
}


void ssd_widget_short_click (SsdWidget widget, RoadMapGuiPoint *point) {

   widget = ssd_widget_find_by_pos (widget, point);

   if (!widget) return;

   if (widget->short_click && !widget->short_click(widget, point)) {
      return;

   } else if (widget->children != NULL) {

      ssd_widget_short_click (widget->children, point);
   }
}


void ssd_widget_long_click (SsdWidget widget, RoadMapGuiPoint *point) {

   widget = ssd_widget_find_by_pos (widget, point);

   if (!widget) return;

   if (widget->long_click && !widget->long_click(widget, point)) {
      return;

   } else if (widget->children != NULL) {

      ssd_widget_long_click (widget->children, point);
   }
}


const char *ssd_widget_get_value (const SsdWidget widget, const char *name) {
   SsdWidget w = ssd_widget_get (widget, name);
   if (!w) return "";

   return w->value;
}


int ssd_widget_set_value (const SsdWidget widget, const char *name,
                          const char *value) {

   SsdWidget w = ssd_widget_get (widget, name);
   if (!w || !w->set_value) return -1;

   return w->set_value(w, value);
}

