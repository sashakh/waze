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

static int initialized;
static RoadMapPen pen;

static void init_containers (void) {
   pen = roadmap_canvas_create_pen ("ssd_text_pen");
   roadmap_canvas_set_foreground ("#ffffff");

   initialized = 1;
}


static void draw (SsdWidget widget, const RoadMapGuiPoint *point) {
   RoadMapGuiPoint p = *point;
   p.y += widget->size.height / 2 + 2;
   roadmap_canvas_draw_string_angle (&p, NULL, 0, -1, widget->value);
}


static int set_value (SsdWidget widget, const char *value) {

   int text_width;
   int text_ascent;
   int text_descent;
   int text_height;

   if (widget->value && *widget->value) free (widget->value);

   if (*value) widget->value = strdup(value);
   else widget->value = "";

   roadmap_canvas_get_text_extents 
         (value, -1, &text_width, &text_ascent, &text_descent, NULL);

   text_height = text_ascent + text_descent;

   widget->size.width = text_width;
   widget->size.height = text_height;

   return 0;
}


SsdWidget ssd_text_new (const char *name, const char *value, int flags) {

   SsdWidget w;

   if (!initialized) {
      init_containers();
   }

   w = ssd_widget_new (name, flags);

   w->typeid = "Text";

   w->draw = draw;
   w->set_value = set_value;
   w->flags = flags;

   set_value (w, value);

   return w;
}


