/* ssd_container.c - Container widget
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

#include "roadmap_canvas.h"

#include "ssd_widget.h"
#include "ssd_text.h"
#include "ssd_button.h"
#include "ssd_dialog.h"

#include "ssd_container.h"

static int initialized;
static RoadMapPen bg;
static RoadMapPen border;
static const char *default_fg = "#000000";
static const char *default_bg = "#ffffff";

static void init_containers (void) {
   bg = roadmap_canvas_create_pen ("container_bg");
   roadmap_canvas_set_foreground (default_bg);

   border = roadmap_canvas_create_pen ("container_border");
   roadmap_canvas_set_foreground (default_fg);
   roadmap_canvas_set_thickness (2);

   initialized = 1;
}


static void draw (SsdWidget widget, RoadMapGuiRect *rect, int flags) {

   RoadMapGuiPoint points[5];
   int count = 5;

   if (!(flags & SSD_GET_SIZE)) {

      if (widget->bg_color) {
         roadmap_canvas_select_pen (bg);
         roadmap_canvas_set_foreground (widget->bg_color);
         roadmap_canvas_erase_area (rect);
      }
   }

   if (widget->flags & SSD_CONTAINER_BORDER) {

      points[0].x = rect->minx + 1;
      points[0].y = rect->miny + 1;
      points[1].x = rect->maxx - 0;
      points[1].y = rect->miny + 1;
      points[2].x = rect->maxx - 0;
      points[2].y = rect->maxy - 0;
      points[3].x = rect->minx + 1;
      points[3].y = rect->maxy - 0;
      points[4].x = rect->minx + 1;
      points[4].y = rect->miny + 1;

      if (!(flags & SSD_GET_SIZE)) {
         const char *color = default_fg;

         roadmap_canvas_select_pen (border);
         if (widget->fg_color) color = widget->fg_color;
         roadmap_canvas_set_foreground (color);

         roadmap_canvas_draw_multiple_lines (1, &count, points, 0);
      }

      rect->minx += 2;
      rect->miny += 2;
      rect->maxx -= 2;
      rect->maxy -= 2;
   }

   if ((flags & SSD_GET_SIZE) && (widget->flags & SSD_CONTAINER_TITLE)) {
      SsdWidget title;

      title = ssd_widget_get (widget, "title_bar");

      if (title) {
         SsdSize max_size = {rect->maxx - rect->minx + 1,
                             rect->maxy - rect->miny + 1};
         SsdSize title_size;
         ssd_widget_get_size (title, &title_size, &max_size);
         rect->miny += title_size.height;
      }
   }
}


static int button_callback (SsdWidget widget, const char *new_value) {
   ssd_dialog_hide_current ();

   return 1;
}


static int short_click (SsdWidget widget, const RoadMapGuiPoint *point) {

   if (widget->callback) {
      (*widget->callback) (widget, SSD_BUTTON_SHORT_CLICK);
      return 1;
   }

   return 0;
}


static void add_title (SsdWidget w, int flags) {
   SsdWidget title;
   const char *close_icon[] = {"rm_quit"};

   title = ssd_container_new ("title_bar", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE,
                              SSD_END_ROW);
   ssd_widget_set_color (title, "#000000", "#efefef");

   ssd_widget_add (title,
         ssd_text_new ("title_text", "" , 15, SSD_WIDGET_SPACE|SSD_ALIGN_VCENTER));

   ssd_widget_add (title,
            ssd_button_new ("close", "", close_icon, 1,
                            SSD_ALIGN_RIGHT, button_callback));

   ssd_widget_add (w, title);
}


static int set_value (SsdWidget widget, const char *value) {

   if (!(widget->flags & SSD_CONTAINER_TITLE)) return -1;

   return ssd_widget_set_value (widget, "title_text", value);
}


SsdWidget ssd_container_new (const char *name, const char *title,
                             int width, int height, int flags) {

   SsdWidget w;

   if (!initialized) {
      init_containers();
   }

   w = ssd_widget_new (name, flags);

   w->typeid = "Container";

   w->flags = flags;

   w->draw = draw;

   w->size.width = width;
   w->size.height = height;

   w->set_value = set_value;
   w->short_click = short_click;

   w->fg_color = default_fg;
   w->bg_color = default_bg;

   if (flags & SSD_CONTAINER_TITLE) {
      add_title (w, flags);
   }

   set_value (w, title);

   return w;
}


