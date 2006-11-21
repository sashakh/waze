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
#include "roadmap_canvas.h"
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


static void calc_pack_size (SsdWidget w, RoadMapGuiRect *rect,
                            SsdSize *size) {

   int width  = rect->maxx - rect->minx + 1;
   int height = rect->maxy - rect->miny + 1;
   int cur_width = 0;
   int max_height = 0;
   int max_width = 0;
   int count = 0;
   SsdWidget last_drawn_widget = NULL;
   SsdWidget prev_w = NULL;

   while (w != NULL) {
      SsdSize max_size;
      SsdSize size;

      if (!count) {
         width  = rect->maxx - rect->minx + 1;

         if (w->flags & SSD_WIDGET_SPACE) {
            rect->miny += 4;
            width -= 4;
            cur_width += 4;
         }
      }

      max_size.width = width;
      max_size.height = rect->maxy - rect->miny + 1;
      ssd_widget_get_size (w, &size, &max_size);

      if ((count > 0) && (((cur_width + size.width) > width) ||
                         w->flags & SSD_START_NEW_ROW)) {
         if (cur_width > max_width) max_width = cur_width;

         count = 0;
         cur_width = 0;
         rect->miny += max_height;
         if (last_drawn_widget && 
            (last_drawn_widget->flags & SSD_WIDGET_SPACE)) {

            rect->miny += SSD_WIDGET_SEP;
         }
         max_height = 0;
         last_drawn_widget = w;
         prev_w = NULL;
      }

      count++;
      cur_width += size.width;
      if (prev_w && prev_w->flags & SSD_WIDGET_SPACE) {
         cur_width += SSD_WIDGET_SEP;
      }

      if (size.height > max_height) max_height = size.height;

      if (w->flags & SSD_END_ROW) {
         if (cur_width > max_width) max_width = cur_width;

         count = 0;
         cur_width = 0;
         rect->miny += max_height;
         if (last_drawn_widget && 
            (last_drawn_widget->flags & SSD_WIDGET_SPACE)) {

            rect->miny += SSD_WIDGET_SEP;
         }
         max_height = 0;
         last_drawn_widget = w;
         prev_w = NULL;
      }

      prev_w = w;
      w = w->next;
   }

   if (count) {
      if (cur_width > max_width) max_width = cur_width;

      count = 0;
      cur_width = 0;
      rect->miny += max_height;
      if (last_drawn_widget && 
            (last_drawn_widget->flags & SSD_WIDGET_SPACE)) {

         rect->miny += SSD_WIDGET_SEP;
      }
   }

   size->width = max_width;
   size->height = height - (rect->maxy - rect->miny);
}


static void ssd_widget_draw_one (SsdWidget w, int x, int y, int height) {

   RoadMapGuiRect rect;
   SsdSize size;

   ssd_widget_get_size (w, &size, NULL);

   if (w->flags & SSD_ALIGN_VCENTER) {
      y += (height - size.height) / 2;
   }

   w->position.x = x;
   w->position.y = y;

   if (size.width && size.height) {
      rect.minx = x;
      rect.miny = y;
      rect.maxx = x + size.width - 1;
      rect.maxy = y + size.height - 1;

      w->draw(w, &rect, 0);

      if (w->children) ssd_widget_draw (w->children, &rect, w->flags);
   }
}


static int ssd_widget_draw_row (SsdWidget *w, int count,
                                int width, int height,
                                int x, int y) {
   int row_height = 0;
   int cur_x;
   int total_width;
   int space;
   int vcenter = 0;
   int bottom = 0;
   int i;
   SsdWidget prev_widget;

   if (ssd_widget_rtl ()) cur_x = x;
   else cur_x = x + width;

   for (i=0; i<count; i++) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      if (size.height > row_height) row_height = size.height;
      if (w[i]->flags & SSD_ALIGN_VCENTER) vcenter = 1;
      if (w[i]->flags & SSD_ALIGN_BOTTOM) bottom = 1;
   }

   if (bottom) {
      y += (height - row_height);

   } else if (vcenter) {
      y += (height - row_height) / 2 - 1;
   }

   for (i=count-1; i>=0; i--) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      if (w[i]->flags & SSD_ALIGN_RIGHT) {
         cur_x += w[i]->offset_x;
         if (ssd_widget_rtl ()) {
            if ((i < (count-1)) && (w[i+1]->flags & SSD_WIDGET_SPACE)) {
               cur_x += SSD_WIDGET_SEP;
            }
            ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
            cur_x += size.width;
         } else {
            cur_x -= size.width;
            if ((i < (count-1)) && (w[i+1]->flags & SSD_WIDGET_SPACE)) {
               cur_x -= SSD_WIDGET_SEP;
            }
            ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
         }
         //width -= size.width;
         if ((i < (count-1)) && (w[i+1]->flags & SSD_WIDGET_SPACE)) {
            //width -= SSD_WIDGET_SEP;
         }
         count--;
         if (i != count) {
            memcpy (&w[i], &w[i+1], sizeof(w[0]) * count - i);
         }
      }
   }

   if (ssd_widget_rtl ()) cur_x = x + width;
   else cur_x = x;

   prev_widget = NULL;

   while (count > 0) {
      SsdSize size;

      if (w[0]->flags & SSD_ALIGN_CENTER) {
         break;
      }

      ssd_widget_get_size (w[0], &size, NULL);

      if (ssd_widget_rtl ()) {
         cur_x -= size.width;
         cur_x -= w[0]->offset_x;
         if (prev_widget && (prev_widget->flags & SSD_WIDGET_SPACE)) {
            cur_x -= SSD_WIDGET_SEP;
         }
         ssd_widget_draw_one (w[0], cur_x, y + w[0]->offset_y, row_height);
      } else {
         cur_x += w[0]->offset_x;
         if (prev_widget && (prev_widget->flags & SSD_WIDGET_SPACE)) {
            cur_x += SSD_WIDGET_SEP;
         }
         ssd_widget_draw_one (w[0], cur_x, y + w[0]->offset_y, row_height);
         cur_x += size.width;
      }

      width -= size.width;
      if (prev_widget && (prev_widget->flags & SSD_WIDGET_SPACE)) {
         width -= SSD_WIDGET_SEP;
      }
      prev_widget = *w;
      w++;
      count--;
   }

   if (count == 0) return row_height;

   /* align center */

   total_width = 0;
   for (i=0; i<count; i++) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      total_width += size.width;
   }

   space = (width - total_width) / (count + 1);

   for (i=0; i<count; i++) {
      SsdSize size;
      ssd_widget_get_size (w[i], &size, NULL);

      if (ssd_widget_rtl ()) {
         cur_x -= space;
         cur_x -= size.width;
         cur_x -= w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
      } else {
         cur_x += space;
         cur_x += w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, row_height);
         cur_x += size.width;
      }
   }

   return row_height;
}


static int ssd_widget_draw_grid_row (SsdWidget *w, int count,
                                     int width,
                                     int avg_width,
                                     int height,
                                     int x, int y) {
   int cur_x;
   int space;
   int i;

   /* align center */

   space = (width - avg_width*count) / (count + 1);

   if (ssd_widget_rtl ()) cur_x = x + width;
   else cur_x = x;

   for (i=0; i<count; i++) {

      if (ssd_widget_rtl ()) {
         cur_x -= space;
         cur_x -= avg_width;
         cur_x -= w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, height);
      } else {
         cur_x += space;
         cur_x += w[i]->offset_x;
         ssd_widget_draw_one (w[i], cur_x, y + w[i]->offset_y, height);
         cur_x += avg_width;
      }
   }

   return height;
}


static void ssd_widget_draw_grid (SsdWidget w, const RoadMapGuiRect *rect) {
   SsdWidget widgets[100];
   int width  = rect->maxx - rect->minx + 1;
   int height = rect->maxy - rect->miny + 1;
   SsdSize max_size = {width, height};
   int cur_y = rect->miny;
   int max_height = 0;
   int avg_width = 0;
   int count = 0;
   int width_per_row;
   int cur_width = 0;
   int rows;
   int num_widgets;
   int space;
   int i;

   while (w != NULL) {
      SsdSize size;
      ssd_widget_get_size (w, &size, &max_size);

      if (size.height > max_height) max_height = size.height;
      avg_width += size.width;

      widgets[count] = w;
      count++;

      if (count == sizeof(widgets) / sizeof(SsdWidget)) {
         roadmap_log (ROADMAP_FATAL, "Too many widgets in grid!");
      }

      w = w->next;
   }

   max_height += SSD_WIDGET_SEP;
   avg_width = avg_width / count + 1;

   rows = height / max_height;

   while ((rows > 1) && ((count * avg_width / rows) < (width * 3 / 5))) rows--;

   if ((rows == 1) && (count > 2)) rows++;

   width_per_row = count * avg_width / rows;
   num_widgets = 0;

   space = (height - max_height*rows) / (rows + 1);

   for (i=0; i < count; i++) {
      SsdSize size;
      ssd_widget_get_size (widgets[i], &size, NULL);

      cur_width += avg_width;

      if (size.width > avg_width*1.5) {
         cur_width += avg_width;
      }

      num_widgets++;

      if ((cur_width >= width_per_row) || (i == (count-1))) {
         cur_y += space;

          ssd_widget_draw_grid_row
               (&widgets[i-num_widgets+1], num_widgets,
                width, avg_width, max_height, rect->minx, cur_y);
         cur_y += max_height;
         cur_width = 0;
         num_widgets = 0;
      }
   }
}


static void ssd_widget_draw_pack (SsdWidget w, const RoadMapGuiRect *rect) {
   SsdWidget row[100];
   int width  = rect->maxx - rect->minx + 1;
   int height = rect->maxy - rect->miny + 1;
   int minx   = rect->minx;
   int cur_y  = rect->miny;
   int cur_width = 0;
   int count = 0;
   SsdWidget last_drawn_widget = NULL;
   SsdWidget prev_w = NULL;

   while (w != NULL) {

      SsdSize size;
      SsdSize max_size;

      if (!count) {
         width  = rect->maxx - rect->minx + 1;
         height = rect->maxy - cur_y + 1;
         minx   = rect->minx;

         if (w->flags & SSD_WIDGET_SPACE) {
            width  -= 4;
            height -= 2;
            cur_y  += 2;
            minx   += 2;
         }
      }

      max_size.width = width - cur_width;
      max_size.height = height;
      ssd_widget_get_size (w, &size, &max_size);

      if ((count == sizeof(row)/sizeof(SsdWidget)) ||
         ((count > 0) &&
                     (((cur_width + size.width) > width) ||
                     (w->flags & SSD_START_NEW_ROW)))) {

         if (last_drawn_widget && 
            (last_drawn_widget->flags & SSD_WIDGET_SPACE)) {

            cur_y += SSD_WIDGET_SEP;
         }

         cur_y += ssd_widget_draw_row
                     (row, count, width, height, minx, cur_y);
         count = 0;
         cur_width = 0;
         last_drawn_widget = prev_w;
         prev_w = NULL;
      }

      row[count++] = w;
      cur_width += size.width;
      if (prev_w && prev_w->flags & SSD_WIDGET_SPACE) {
         cur_width += SSD_WIDGET_SEP;
      }

      if (w->flags & SSD_END_ROW) {

         if (last_drawn_widget && 
            (last_drawn_widget->flags & SSD_WIDGET_SPACE)) {

            cur_y += SSD_WIDGET_SEP;
         }

         cur_y += ssd_widget_draw_row
                     (row, count, width, height, minx, cur_y);
         count = 0;
         cur_width = 0;
         last_drawn_widget = w;
         prev_w = NULL;
      }

      prev_w = w;
      w = w->next;
   }

   if (count) {
      if (last_drawn_widget && 
            (last_drawn_widget->flags & SSD_WIDGET_SPACE)) {

         cur_y += SSD_WIDGET_SEP;
      }
      ssd_widget_draw_row (row, count, width, height, minx, cur_y);
   }
}


void ssd_widget_draw (SsdWidget w, const RoadMapGuiRect *rect,
                      int parent_flags) {

   if (parent_flags & SSD_ALIGN_GRID) ssd_widget_draw_grid (w, rect);
   else ssd_widget_draw_pack (w, rect);
}


SsdWidget ssd_widget_new (const char *name, int flags) {

   SsdWidget w;

   w = (SsdWidget) calloc (1, sizeof (*w));

   roadmap_check_allocated(w);

   w->name          = strdup(name);
   w->size.height = SSD_MIN_SIZE;
   w->size.width  = SSD_MIN_SIZE;

   w->cached_size.height = w->cached_size.width = -1;

   return w;
}


SsdWidget ssd_widget_find_by_pos (SsdWidget widget,
                                  const RoadMapGuiPoint *point) {

   while (widget != NULL) {
      SsdSize size;
      ssd_widget_get_size (widget, &size, NULL);

      if ((widget->position.x <= point->x) &&
          ((widget->position.x + size.width) >= point->x) &&
          (widget->position.y <= point->y) &&
          ((widget->position.y + size.height) >= point->y)) {

         return widget;
      }

      widget = widget->next;
   }

   return NULL;
}


void ssd_widget_set_callback (SsdWidget widget, SsdCallback callback) {
   widget->callback = callback;
}


int ssd_widget_rtl (void) {
   return roadmap_lang_rtl ();
}


int ssd_widget_pointer_down (SsdWidget widget, const RoadMapGuiPoint *point) {

   widget = ssd_widget_find_by_pos (widget, point);

   if (!widget) return 0;

   if (widget->pointer_down && widget->pointer_down(widget, point)) {
      return 1;

   } else if (widget->children != NULL) {

      return ssd_widget_pointer_down (widget->children, point);
   }

   return 0;
}


int ssd_widget_short_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   widget = ssd_widget_find_by_pos (widget, point);

   if (!widget) return 0;

   if (widget->short_click && widget->short_click(widget, point)) {
      return 1;

   } else if (widget->children != NULL) {

      return ssd_widget_short_click (widget->children, point);
   }

   return 0;
}


int ssd_widget_long_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   widget = ssd_widget_find_by_pos (widget, point);

   if (!widget) return 0;

   if (widget->long_click && widget->long_click(widget, point)) {
      return 1;

   } else if (widget->children != NULL) {

      return ssd_widget_long_click (widget->children, point);
   }

   return 0;
}


const char *ssd_widget_get_value (const SsdWidget widget, const char *name) {
   SsdWidget w = ssd_widget_get (widget, name);
   if (!w) return "";

   if (w->get_value) return w->get_value (w);

   return w->value;
}


const void *ssd_widget_get_data (const SsdWidget widget, const char *name) {
   SsdWidget w = ssd_widget_get (widget, name);
   if (!w) return "";

   if (w->get_data) return w->get_data (w);
   else return NULL;
}


int ssd_widget_set_value (const SsdWidget widget, const char *name,
                          const char *value) {

   SsdWidget w = ssd_widget_get (widget, name);
   if (!w || !w->set_value) return -1;

   return w->set_value(w, value);
}


int ssd_widget_set_data (const SsdWidget widget, const char *name,
                          const void *value) {

   SsdWidget w = ssd_widget_get (widget, name);
   if (!w || !w->set_data) return -1;

   return w->set_data(w, value);
}


void ssd_widget_add (SsdWidget parent, SsdWidget child) {

   SsdWidget last = parent->children;

   child->parent = parent;

   if (!last) {
      parent->children = child;
      return;
   }

   while (last->next) last=last->next;
   last->next = child;
}


void ssd_widget_set_size (SsdWidget widget, int width, int height) {

   widget->size.width  = width;
   widget->size.height = height;
}


void ssd_widget_set_offset (SsdWidget widget, int x, int y) {

   widget->offset_x = x;
   widget->offset_y = y;
}


void ssd_widget_set_context (SsdWidget widget, void *context) {

   widget->context = context;
}


void ssd_widget_get_size (SsdWidget w, SsdSize *size, const SsdSize *max) {

   SsdSize pack_size = {0, 0};
                        
   RoadMapGuiRect max_size = {0, 0, 0, 0};

   *size = w->size;

   if ((w->size.height >= 0) && (w->size.width >= 0)) {
      return;
   }

   if (!max && (w->cached_size.width < 0)) {
      roadmap_log (ROADMAP_FATAL,
             "ssd_widget_get_size called without max, and no size cache!");
   }

   if ((w->cached_size.width >= 0) && (w->cached_size.height >= 0)) {
      *size = w->cached_size;
      return;
   }

   if (w->flags & SSD_DIALOG_FLOAT) {
      if (size->width == SSD_MAX_SIZE) size->width = max->width - 70;
      if (size->height == SSD_MAX_SIZE) size->height = max->height;

   } else {

      if (size->width == SSD_MAX_SIZE) size->width = max->width;
      if (size->height == SSD_MAX_SIZE) size->height = max->height;
   }

   if ((size->height >= 0) && (size->width >= 0)) {
      w->cached_size = *size;
      return;
   }

   if (size->width >= 0)  {
      max_size.maxx = size->width - 1;
   } else {
      max_size.maxx = max->width - 1;
   }

   if (size->height >= 0) {
      max_size.maxy = size->height - 1;
   } else {
      max_size.maxy = max->height - 1;
   }

   if (w->children) {
      RoadMapGuiRect container_rect = max_size;
      int container_width;
      int container_height;

      w->draw (w, &max_size, SSD_GET_SIZE);

      container_width  = max_size.minx - container_rect.minx +
                         container_rect.maxx - max_size.maxx;
      container_height = max_size.miny - container_rect.miny +
                         container_rect.maxy - max_size.maxy;

      calc_pack_size (w->children, &max_size, &pack_size);

      pack_size.width  += container_width;
      pack_size.height += container_height;

   } else {
      w->draw (w, &max_size, SSD_GET_SIZE);
      pack_size.width  = max_size.maxx - max_size.minx + 1;
      pack_size.height = max_size.maxy - max_size.miny + 1;
   }

   if (size->height < 0) size->height = pack_size.height;
   if (size->width < 0) size->width = pack_size.width;

   w->cached_size = *size;

}


void ssd_widget_set_color (SsdWidget w, const char *fg_color,
                           const char *bg_color) {
   w->fg_color = fg_color;
   w->bg_color = bg_color;
}


void ssd_widget_container_size (SsdWidget dialog, SsdSize *size) {

   SsdSize max_size;

   /* Calculate size recurisvely */
   if (dialog->parent) {
      ssd_widget_container_size (dialog->parent, size);
      max_size.width = size->width;
      max_size.height = size->height;

   } else {
      max_size.width = roadmap_canvas_width ();
      max_size.height = roadmap_canvas_height ();
   }

   ssd_widget_get_size (dialog, size, &max_size);

   if (dialog->draw) {
      RoadMapGuiRect rect = {0, 0, size->width - 1, size->height - 1};

      dialog->draw (dialog, &rect, SSD_GET_SIZE);

      size->width = rect.maxx - rect.minx + 1;
      size->height = rect.maxy - rect.miny + 1;
   }
}


void *ssd_widget_get_context (SsdWidget w) {

   return w->context;
}


void ssd_widget_reset_cache (SsdWidget w) {

   SsdWidget child = w->children;

   w->cached_size.width = w->cached_size.height = -1;

   while (child != NULL) {

      ssd_widget_reset_cache (child);
      child = child->next;
   }
}


void ssd_widget_hide (SsdWidget w) {
   w->flags |= SSD_WIDGET_HIDE;
}


void ssd_widget_show (SsdWidget w) {
   w->flags &= ~SSD_WIDGET_HIDE;
}


void ssd_widget_set_flags (SsdWidget widget, int flags) {
   widget->flags = flags;
}

