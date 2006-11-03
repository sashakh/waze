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
#include "ssd_container.h"

static int initialized;
static RoadMapPen bg;
static RoadMapPen border;

static void init_containers (void) {
   bg = roadmap_canvas_create_pen ("container_bg");
   roadmap_canvas_set_foreground ("#000000");

   border = roadmap_canvas_create_pen ("container_border");
   roadmap_canvas_set_foreground ("#ffffff");
   roadmap_canvas_set_thickness (2);

   initialized = 1;
}


static void draw (SsdWidget widget, const RoadMapGuiPoint *point) {
   RoadMapGuiRect rect;
   RoadMapGuiPoint points[5];
   int count = 5;

   rect.minx = point->x;
   rect.miny = point->y;
   rect.maxx = rect.minx + widget->size.width - 1;
   rect.maxy = rect.miny + widget->size.height - 1;

   roadmap_canvas_select_pen (bg);
   roadmap_canvas_erase_area (&rect);
   roadmap_canvas_select_pen (border);

   points[0].x = rect.minx;
   points[0].y = rect.miny;
   points[1].x = rect.maxx;
   points[1].y = rect.miny;
   points[2].x = rect.maxx;
   points[2].y = rect.maxy;
   points[3].x = rect.minx;
   points[3].y = rect.maxy;
   points[4].x = rect.minx;
   points[4].y = rect.miny;

   roadmap_canvas_draw_multiple_lines (1, &count, points, 0);

   rect.minx += 3;
   rect.miny += 3;
   rect.maxx -= 3;
   rect.maxy -= 3;

   ssd_widget_draw (widget->children, &rect);
}


static int set_value (SsdWidget widget, const char *value) {

   widget = ssd_widget_get (widget->children, "title");

   if (widget) {
      return widget->set_value (widget, value);
   } else {
      return -1;
   }
}


SsdWidget ssd_container_new (const char *name, const char *title, int flags) {

   SsdWidget w;

   if (!initialized) {
      init_containers();
   }

   w = ssd_widget_new (name, flags);

   w->typeid = "Container";

   w->draw = draw;
   w->set_value = set_value;

   w->size.width = roadmap_canvas_width ();
   w->size.height = roadmap_canvas_height ();

   w->children = ssd_text_new ("title", title, SSD_ALIGN_CENTER|SSD_END_ROW);

   return w;
}


