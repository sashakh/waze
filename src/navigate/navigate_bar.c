/* navigate_bar.c - implement navigation bar
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
 *   See navigate_bar.h
 */

#include <stdlib.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_screen_obj.h"
#include "roadmap_file.h"
#include "roadmap_res.h"
#include "roadmap_math.h"

#include "navigate_main.h"
#include "navigate_bar.h"

typedef struct {
   const char     *image_file;
   int             min_screen_width;
   RoadMapGuiPoint instruction_pos;
   RoadMapGuiRect  distance_rect;
   RoadMapGuiPoint distance_value_pos;
   RoadMapGuiPoint distance_unit_pos;
   int             street_start;
   int             street_width;
} NavigateBarPanel;

static NavigateBarPanel NavigateBarDefaultPanels[] = {
   {"nav_panel_wide", 320, {0, 0}, {0, 65, 48, 110}, {3, 70}, {3, 95}, 75, 230},
   {"nav_panel", 240, {0, 0}, {55, 0, 95, 50}, {55, 5}, {58, 30}, 98, 133}
};


static NavigateBarPanel *NavigatePanel = NULL;
static RoadMapScreenSubscriber navigate_prev_after_refresh = NULL;


const char NAVIGATE_DIR_IMG[][40] = {
   "nav_turn_left",
   "nav_turn_right",
   "nav_keep_left",
   "nav_keep_right",
   "nav_continue",
   "nav_approaching"
};

static RoadMapImage NavigateBarImage;
static RoadMapImage NavigateBarBG;
static RoadMapImage NavigateDirections[LAST_DIRECTION];
static int NavigateBarInitialized = 0;
static RoadMapGuiPoint NavigateBarLocation;

static enum NavigateInstr NavigateBarCurrentInstr = LAST_DIRECTION;
static int  NavigateBarCurrentDistance = -1;
static int  NavigateBarEnabled = 0;

static int navigate_bar_align_text (char *text, char **line1, char **line2,
                                    int size) {

   int width, ascent, descent;

   roadmap_canvas_get_text_extents
      (text, size, &width, &ascent, &descent, NULL);

   if (width >= 2 * NavigatePanel->street_width) return -1;

   if (width < NavigatePanel->street_width) {

      *line1 = text;
      return 1;

   } else {

      /* Check if we can place the text in two lines */

      char *text_line = text;
      char *text_end = text_line + strlen(text_line);
      char *p1 = text_line + (strlen(text_line) / 2);
      char *p2 = p1;

      while (p1 > text_line) {
         if (*p1 == ' ') {
            break;
         }
         p1 -= 1;
      }
      while (p2 < text_end) {
         if (*p2 == ' ') {
            break;
         }
         p2 += 1;
      }
      if (text_end - p1 > p2 - text_line) {
         p1 = p2;
      }
      if (p1 > text_line) {

         char saved = *p1;
         *p1 = 0;

         roadmap_canvas_get_text_extents
            (text_line, size, &width, &ascent, &descent, NULL);

         if (width < NavigatePanel->street_width) {

            roadmap_canvas_get_text_extents
               (text_line, size, &width, &ascent, &descent, NULL);

            if (width < NavigatePanel->street_width) {

               *line1 = text_line;
               *line2 = p1 + 1;
               return 2;
            }
         }

         *p1 = saved;
      }
   }

   return -1;
}


static void navigate_bar_after_refresh (void) {

   if (NavigateBarEnabled) {
      navigate_bar_draw ();
   }

   if (navigate_prev_after_refresh) {
      (*navigate_prev_after_refresh) ();
   }
}


void navigate_bar_initialize (void) {

   int i;
   int width;

   if (NavigateBarInitialized) return;
      
   width = roadmap_canvas_width ();

   for (i=0;
       (unsigned)i<
        sizeof(NavigateBarDefaultPanels)/sizeof(NavigateBarDefaultPanels[0]);
       i++) {

      if (width >= NavigateBarDefaultPanels[i].min_screen_width) {
         NavigatePanel = NavigateBarDefaultPanels + i;
         break;
      }
   }

   if (!NavigatePanel) {
      roadmap_log (ROADMAP_ERROR, "Can't find nav panel for screen width: %d",
            width);
      NavigateBarInitialized = -1;
   }

   NavigateBarBG =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN|RES_NOCACHE, NavigatePanel->image_file);

   NavigateBarImage =
      (RoadMapImage) roadmap_res_get
         (RES_BITMAP, RES_SKIN, NavigatePanel->image_file);

   if (!NavigateBarBG || !NavigateBarImage) goto error;

   for (i=0; i<LAST_DIRECTION; i++) {
      NavigateDirections[i] =
         (RoadMapImage) roadmap_res_get
               (RES_BITMAP, RES_SKIN, NAVIGATE_DIR_IMG[i]);

      if (!NavigateDirections[i]) goto error;
   }
      

   NavigateBarLocation.x = 0;
   NavigateBarLocation.y = 0;

   NavigateBarInitialized = 1;
   navigate_prev_after_refresh = 
      roadmap_screen_subscribe_after_refresh (navigate_bar_after_refresh);

   return;

error:
   NavigateBarInitialized = -1;
}


void navigate_bar_set_instruction (enum NavigateInstr instr) {

   RoadMapGuiPoint pos = NavigatePanel->instruction_pos;

   if (NavigateBarInitialized != 1) return;
   
   roadmap_canvas_copy_image (NavigateBarImage, &pos, NULL, NavigateBarBG,
                              CANVAS_COPY_NORMAL);

   roadmap_canvas_copy_image (NavigateBarImage, &pos, NULL,
                              NavigateDirections[(int)instr],
                              CANVAS_COPY_BLEND);

   NavigateBarCurrentInstr = instr;

}


void navigate_bar_set_distance (int distance) {

   char str[100];
   char unit_str[20];
   int  distance_far;
   RoadMapGuiPoint position = {0, 0};

   if (NavigateBarInitialized != 1) return;
   if (NavigateBarCurrentDistance == distance) return;

   /* erase the old distance */
   roadmap_canvas_copy_image (NavigateBarImage, &position,
                              &NavigatePanel->distance_rect,
                              NavigateBarBG,
                              CANVAS_COPY_NORMAL);

   distance_far =
      roadmap_math_to_trip_distance(distance);

   if (distance_far > 0) {

      int tenths = roadmap_math_to_trip_distance_tenths(distance);
      snprintf (str, sizeof(str), "%d.%d", distance_far, tenths % 10);
      snprintf (unit_str, sizeof(unit_str), "%s", roadmap_math_trip_unit());
   } else {

      snprintf (str, sizeof(str), "%d", distance);
      snprintf (unit_str, sizeof(unit_str), "%s",
                roadmap_math_distance_unit());
   };

   position = NavigatePanel->distance_value_pos;
   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->distance_value_pos, 22, str);


   position = NavigatePanel->distance_unit_pos;
   roadmap_canvas_draw_image_text
      (NavigateBarImage, &NavigatePanel->distance_unit_pos, 18, unit_str);
}


void navigate_bar_set_street (const char *street) {

#define NORMAL_SIZE 20
#define SMALL_SIZE  16

   int width, ascent, descent;
   int size;
   char *line1;
   char *line2;
   char *text;
   int num_lines;
   int i;

   //street = "רחוב המלך הגדולי מלקט פרחים 18";
   if (NavigateBarInitialized != 1) return;

   text = strdup(street);

   size = NORMAL_SIZE;

   num_lines = navigate_bar_align_text (text, &line1, &line2, size);

   if (num_lines < 0) {
      /* Try again with a smaller font size */

      size = SMALL_SIZE;
      num_lines = navigate_bar_align_text (text, &line1, &line2, size);
   }

   /* Cut some text until it fits */
   while (num_lines < 0) {

      char *end = text + strlen(text) - 1;

      while ((end > text) && (*end != ' ')) end--;

      if (end == text) {

         roadmap_log (ROADMAP_ERROR, "Can't align street in nav bar: %s",
                      street);

         free(text);
         return;
      }

      *end = '\0';
      num_lines = navigate_bar_align_text (text, &line1, &line2, size);
   }

   for (i=0; i < num_lines; i++) {

      char *line;
      RoadMapGuiPoint position = {NavigatePanel->street_start, 5};

      if (i ==0 ) {
         line = line1;
      } else {
         line = line2;
         position.y = 20;
      }

      roadmap_canvas_get_text_extents
         (line, size, &width, &ascent, &descent, NULL);
      
      position.x = NavigatePanel->street_width - width + NavigatePanel->street_start;

      roadmap_canvas_draw_image_text (NavigateBarImage, &position, size, line);
   }
}


void navigate_bar_set_mode (int mode) {
   int x_offset;
   int y_offset;

   if (NavigateBarEnabled == mode) return;

   x_offset = 0;
   y_offset = roadmap_canvas_image_height (NavigateBarBG);

   if (mode) {
      roadmap_screen_obj_offset (x_offset, y_offset);
      roadmap_screen_move_center (-y_offset / 2);
   } else {
      roadmap_screen_obj_offset (-x_offset, -y_offset);
      roadmap_screen_move_center (y_offset / 2);
   }
   
   NavigateBarEnabled = mode;
}


void navigate_bar_draw (void) {

   if (NavigateBarInitialized != 1) return;

   roadmap_canvas_draw_image (NavigateBarImage, &NavigateBarLocation, 0,
         IMAGE_NORMAL);
}

