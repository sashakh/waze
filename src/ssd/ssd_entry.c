/* ssd_entry.c - Text entry widget
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
 *   See ssd_entry.h
 */

#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_canvas.h"

#include "ssd_widget.h"
#include "ssd_text.h"
#include "ssd_entry.h"

static int initialized;
static RoadMapPen bg;
static RoadMapPen text_pen;

static void init (void) {
   bg = roadmap_canvas_create_pen ("ssd_entry_bg");
   roadmap_canvas_set_foreground ("#ffffff");

   text_pen = roadmap_canvas_create_pen ("ssd_entry_txt");
   roadmap_canvas_set_foreground ("#000000");

   initialized = 1;
}


static void draw (SsdWidget widget, RoadMapGuiRect *rect, int flags) {

   RoadMapGuiPoint points[5];
   int count = 5;

   if (!(flags & SSD_GET_SIZE)) {
      roadmap_canvas_select_pen (bg);
      roadmap_canvas_erase_area (rect);
   }

   points[0].x = rect->minx;
   points[0].y = rect->miny;
   points[1].x = rect->maxx;
   points[1].y = rect->miny;
   points[2].x = rect->maxx;
   points[2].y = rect->maxy;
   points[3].x = rect->minx;
   points[3].y = rect->maxy;
   points[4].x = rect->minx;
   points[4].y = rect->miny;

   if (!(flags & SSD_GET_SIZE)) {
      roadmap_canvas_select_pen (text_pen);
      roadmap_canvas_draw_multiple_lines (1, &count, points, 0);
   }

   rect->minx += 1;
   rect->miny += 1;
   rect->maxx -= 1;
   rect->maxy -= 1;
}


static int set_value (SsdWidget widget, const char *value) {

   SsdWidget w = ssd_widget_get (widget, "text");
   w->set_value (w, value);

   widget->value = w->value;

   return 0;
}


SsdWidget ssd_entry_new (const char *name, const char *value, int flags) {

   SsdWidget w;

   if (!initialized) {
      init ();
   }

   w = ssd_widget_new (name, flags);

   w->typeid = "Entry";

   w->draw = draw;
   w->set_value = set_value;
   w->flags = flags;
//   w->size.height = 24;
//   w->size.width = roadmap_canvas_width() - 6;

   w->children = ssd_text_new ("text", value, 20, 0);

   set_value (w, value);

   return w;
}


