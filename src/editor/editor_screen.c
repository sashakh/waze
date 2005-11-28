/* editor_screen.c - screen drawing
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *   See editor_screen.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_screen.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "roadmap_pointer.h"
#include "roadmap_line.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"
#include "roadmap_layer.h"
#include "roadmap_locator.h"
#include "roadmap_hash.h"
#include "roadmap_main.h"

#include "db/editor_db.h"
#include "db/editor_point.h"
#include "db/editor_shape.h"
#include "db/editor_line.h"
#include "db/editor_square.h"
#include "db/editor_street.h"
#include "db/editor_route.h"
#include "db/editor_override.h"
#include "db/editor_trkseg.h"
#include "editor_main.h"

#include "static/editor_dialog.h"
#include "track/editor_track.h"

#include "editor_screen.h"

#define MIN_THICKNESS 3
#define MAX_LAYERS (ROADMAP_ROAD_LAST + 1)
#define MAX_LINE_SELECTIONS 100

#define MAX_PEN_LAYERS 2

#define MAX_ROAD_STATES 4
#define SELECTED_STATE 0
#define NO_DATA_STATE 1
#define MISSING_ROUTE_STATE 2
#define MISSING_NAME_STATE 3
#define NO_ROAD_STATE 4

typedef struct editor_pen_s {
   int in_use;
   int thickness;
   RoadMapPen pen;
} editor_pen;

static editor_pen EditorPens[MAX_LAYERS][MAX_PEN_LAYERS][MAX_ROAD_STATES];
static editor_pen EditorTrackPens[MAX_PEN_LAYERS];

SelectedLine SelectedLines[MAX_LINE_SELECTIONS];

static int select_count;

static RoadMapPointerHandler SaveShortClickHandler;
static RoadMapPointerHandler SaveLongClickHandler;

/* track which lines were drawn */
#define DEFAULT_LINES_DRAWN_SIZE 100
static int *LinesDrawn;
static int LinesDrawnCount;
static int LinesDrawnSize;
static RoadMapHash *LinesDrawnHash;


static void init_lines_drawn (void) {
   
   if (LinesDrawn == NULL) {

      LinesDrawnSize = DEFAULT_LINES_DRAWN_SIZE;
      LinesDrawn = malloc (LinesDrawnSize * sizeof(int));
      LinesDrawnHash = roadmap_hash_new ("LinesDrawn", LinesDrawnSize);
      LinesDrawnCount = 0;
      
   } else {

      LinesDrawnCount = 0;
      roadmap_hash_free (LinesDrawnHash);
      LinesDrawnHash = roadmap_hash_new ("LinesDrawn", LinesDrawnSize);

   }
}
 
 
static int should_draw_line (int line) {

   int i;
   
   for (i = roadmap_hash_get_first (LinesDrawnHash, line);
        i >= 0;
        i = roadmap_hash_get_next (LinesDrawnHash, i)) {

      if (line == LinesDrawn[i]) return 0;
   }

   if (LinesDrawnSize == LinesDrawnCount) {
      
      LinesDrawnSize *= 2;
      LinesDrawn = realloc (LinesDrawn, LinesDrawnSize * sizeof(int));
      roadmap_hash_resize (LinesDrawnHash, LinesDrawnSize);
   }

   LinesDrawn[LinesDrawnCount] = line;
   roadmap_hash_add (LinesDrawnHash, line, LinesDrawnCount);

   LinesDrawnCount++;

   return 1;
}


static void editor_screen_update_segments (void) {

   editor_segments_properties (SelectedLines, select_count);
}


static void editor_screen_delete_segments (void) {

   int i;
   
   for (i=0; i<select_count; i++) {

      SelectedLine *line = &SelectedLines[i];

      if (editor_db_activate (roadmap_plugin_get_fips(&line->line)) == -1) {

         editor_db_create (roadmap_plugin_get_fips(&line->line));

         if (editor_db_activate (roadmap_plugin_get_fips(&line->line)) == -1) {
            continue;
         }
      }

      if (roadmap_plugin_get_id (&line->line) == ROADMAP_PLUGIN_ID) {

         int line_id = roadmap_plugin_get_line_id (&line->line);

         editor_override_line_set_flags (line_id,
            editor_override_line_get_flags (line_id) |
            ED_LINE_DELETED);

      } else if (roadmap_plugin_get_id (&line->line) == EditorPluginID) {

        //TODO: The flag should be added and nor reset other flags
         editor_line_modify_properties
            (roadmap_plugin_get_line_id (&line->line),
             roadmap_plugin_get_line_cfcc (&line->line),
        ED_LINE_DELETED);
      }
   }

   select_count = 0;
   roadmap_screen_redraw ();

   return;

}

void editor_screen_long_click (RoadMapGuiPoint *point) {
   
   static RoadMapMenu menu;

   if (select_count == 0) return;

   if (menu == NULL) {
      menu = roadmap_main_new_menu ();
      roadmap_main_add_menu_item (menu, "Properties",
            "Update road properties", editor_screen_update_segments);
      roadmap_main_add_separator (menu);
      roadmap_main_add_menu_item (menu, "Delete", "Delete selected roads", editor_screen_delete_segments);
   }

   roadmap_main_popup_menu (menu, point->x, point->y);
}
 

void editor_screen_short_click (RoadMapGuiPoint *point) {
    
   RoadMapPosition position;
   PluginLine line;
   int distance;

   int i;
    
   roadmap_math_to_position (point, &position);
   
   if (roadmap_navigate_retrieve_line
         (&position, 20, &line, &distance) == -1) {
       
      select_count = 0;
      roadmap_screen_redraw ();
      return;
   }
   for (i=0; i<select_count; i++) {
      if (roadmap_plugin_same_line(&SelectedLines[i].line, &line)) break;
   }

   if (i<select_count) {
      /* line was already selected, remove it */
      int j;
      for (j=i+1; j<select_count; j++) {
         SelectedLines[j-1] = SelectedLines[j];
      }

      select_count--;
      roadmap_screen_redraw ();
      return;
   }

   if (select_count == MAX_LINE_SELECTIONS) return;

   SelectedLines[select_count].line = line;
   select_count++;

   roadmap_screen_redraw ();
}


/* TODO: this is a bad callback which is called from roadmap_layer_adjust().
 * This should be changed. Currently when the editor is enabled, an explicit
 * call to roadmap_layer_adjust() is called. When this is fixed, that call
 * should be removed.
 */
void editor_screen_adjust_layer (int layer, int thickness, int pen_count) {
    
   int i;
   int j;

   if (layer > ROADMAP_ROAD_LAST) return;
   if (!editor_is_enabled()) return;

   if ((pen_count > 1) && (thickness < 3)) {
      pen_count = 1;
   }

   for (i=0; i<MAX_PEN_LAYERS; i++) 
      for (j=0; j<MAX_ROAD_STATES; j++) {

         editor_pen *pen = &EditorPens[layer][i][j];

         pen->in_use = i<pen_count;

         if (!pen->in_use) continue;

         roadmap_canvas_select_pen (pen->pen);

         if (i == 1) {
            pen->thickness = thickness-2;
         } else {
            pen->thickness = thickness;
         }

         roadmap_canvas_set_thickness (pen->thickness);
      }

   if (layer == ROADMAP_ROAD_STREET) {
      roadmap_canvas_select_pen (EditorTrackPens[0].pen);
      EditorTrackPens[0].thickness = thickness;
      EditorTrackPens[0].in_use = 1;
      roadmap_canvas_set_thickness (EditorTrackPens[0].thickness);
      if (pen_count == 1) {
         EditorTrackPens[1].in_use = 0;
      } else {
         roadmap_canvas_select_pen (EditorTrackPens[1].pen);
         EditorTrackPens[1].thickness = thickness - 2;
         EditorTrackPens[1].in_use = 1;
         roadmap_canvas_set_thickness (EditorTrackPens[1].thickness);
      }


   }
}


static int editor_screen_get_road_state (int line, int plugin_id, int fips) {

   int route;
   int has_street = 0;
   int has_route = 0;

   //TODO get route from roadmap
   
   if (editor_db_activate (fips) != -1) {
      
      if (!plugin_id) {
         route = editor_override_line_get_route (line);
      } else {
         route = editor_line_get_route (line);
      }

      if (route != -1) {
         has_route = 1;
      }
   }

   if (plugin_id == 0) {
      
      RoadMapStreetProperties properties;

      roadmap_locator_activate (fips);
      roadmap_street_get_properties (line, &properties);

      if (properties.street != -1) {
         has_street = 1;
      }

   } else {

      EditorStreetProperties properties;

      editor_db_activate (fips);
      editor_street_get_properties (line, &properties);

      if (properties.street != -1) {
         has_street = 1;
      }
   }

   if (has_street && has_route) return NO_ROAD_STATE;
   else if (!has_street && !has_route) return NO_DATA_STATE;
   else if (has_street) return MISSING_ROUTE_STATE;
   else return MISSING_NAME_STATE;
}


int editor_screen_override_pen (int line,
                                int cfcc,
                                int fips,
                                int pen_type,
                                RoadMapPen *override_pen) {

   int i;
   int ActiveDB;
   int road_state;
   int is_selected = 0;
   editor_pen *pen = NULL;
   int route = -1;

   if (cfcc > ROADMAP_ROAD_LAST) return 0;

   ActiveDB = editor_db_activate (fips);

   if (!editor_is_enabled()) return 0;

   for (i=0; i<select_count; i++) {
      if ((line == SelectedLines[i].line.line_id) &&
          (SelectedLines[i].line.plugin_id != EditorPluginID) &&
          (fips == SelectedLines[i].line.fips)) {
         is_selected = 1;
         break;
      }
   }

   if (is_selected) {

      road_state = SELECTED_STATE;
   } else {
      
      road_state = editor_screen_get_road_state (line, 0, fips);
   }

   if (road_state != NO_ROAD_STATE) {

      if (pen_type > 1) return 0;
      
      pen = &EditorPens[cfcc][pen_type][road_state];
      if (!pen->in_use) {
         *override_pen = NULL;
         return 1;
      }
      *override_pen = pen->pen;
   }
   
   if (ActiveDB != -1) {
      //TODO check roadmap route data
      route = editor_override_line_get_route (line);
   }

   return 1;

#if 0   
   if ((pen->thickness > 20) &&
         (roadmap_layer_get_pen (cfcc, pen_type+1) == NULL) &&
         (route != -1)) {

      RoadMapPosition from;
      RoadMapPosition to;

      direction = editor_route_get_direction (route, ED_ROUTE_CAR);

      if (direction <= 0) {
         return 0;
      }

      roadmap_line_from (line, &from);
      roadmap_line_to (line, &to);

      if (*override_pen == NULL) {
         *override_pen = roadmap_layer_get_pen (cfcc, pen_type);
      }

      roadmap_screen_draw_one_line
            (&from, &to, 0, &from, first_shape, last_shape,
             roadmap_shape_get_position, *override_pen);

      roadmap_screen_draw_line_direction
            (&from, &to, &from, first_shape, last_shape,
             roadmap_shape_get_position,
             pen->thickness, direction);

      return 1;
   }

   return 0;
#endif
}


static char *editor_screen_get_pen_color (int pen_type, int road_state) {

   if (pen_type == 0) {
      switch (road_state) {
      case SELECTED_STATE: return "yellow";
      default: return "black";
      }
   }

   switch (road_state) {
      
   case SELECTED_STATE:       return "black";
   case NO_DATA_STATE:        return "dark red";
   case MISSING_NAME_STATE:   return "red";
   case MISSING_ROUTE_STATE:  return "orange";

   default:
                              assert (0);
                              return "black";
   }
}


void editor_screen_initialize (void) {
    
   int i;
   int j;
   int k;
   char name[80];

   /* FIXME should only create pens for road class */

   for (i=1; i<MAX_LAYERS; ++i) 
      for (j=0; j<MAX_PEN_LAYERS; j++) 
         for (k=0; k<MAX_ROAD_STATES; k++) {

            editor_pen *pen = &EditorPens[i][j][k];

            pen->in_use = 0;

            snprintf (name, sizeof(name), "EditorPen%d", i*100+j*10+k);
            pen->pen = roadmap_canvas_create_pen (name);
            roadmap_canvas_set_foreground (editor_screen_get_pen_color(j,k));
            roadmap_canvas_set_thickness (1);
         }

   EditorTrackPens[0].pen = roadmap_canvas_create_pen ("EditorTrack0");
   roadmap_canvas_set_foreground ("black");
   roadmap_canvas_set_thickness (1);
   EditorTrackPens[1].pen = roadmap_canvas_create_pen ("EditorTrack1");
   roadmap_canvas_set_foreground ("blue");
   roadmap_canvas_set_thickness (1);
}


static void editor_screen_draw_square
              (int square, int fips, int min_cfcc, int pen_type) {

   int line;
   int count;
   int i;
   int j;
   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition trk_from_pos;
   int first_shape;
   int last_shape;
   int cfcc;
   int flag;
   int square_cfccs;
   int fully_visible = 0;
   int road_state;
   RoadMapPen pen = NULL;
   int trkseg;
   int route;

   roadmap_log_push ("editor_screen_draw_square");

   square_cfccs = editor_square_get_cfccs (square);

   if (! (square_cfccs && (-1 << min_cfcc))) {
      roadmap_log_pop ();
      return;
   }

   count = editor_square_get_num_lines (square);

   for (i=0; i<count; i++) {
      
      line = editor_square_get_line (square, i);

      editor_line_get (line, &from, &to, &trkseg, &cfcc, &flag);
      if (cfcc < min_cfcc) continue;

      if (!should_draw_line (line)) continue; 

      if (flag & ED_LINE_DELETED) continue;

      if (editor_is_enabled () ) {
         
         int is_selected = 0;

         for (j=0; j<select_count; j++) {

            if ((line == SelectedLines[j].line.line_id) &&
                (SelectedLines[j].line.plugin_id == EditorPluginID)  &&
                (fips == SelectedLines[j].line.fips)) {
            
               is_selected = 1;
               break;
            }
         }
      
         if (is_selected) {

            road_state = SELECTED_STATE;
         } else {
      
            road_state = editor_screen_get_road_state (line, 1, fips);
         }

      } else {
         road_state = NO_ROAD_STATE;
      }

      if (road_state == NO_ROAD_STATE) {

         pen = roadmap_layer_get_pen (cfcc, pen_type);
      } else {

         if (pen_type > 1) continue;
         if (!EditorPens[cfcc][pen_type][road_state].in_use) continue;

         pen = EditorPens[cfcc][pen_type][road_state].pen;
      }
         
      if (pen == NULL) continue;

      editor_trkseg_get (trkseg, &j, &first_shape, &last_shape, NULL);
      editor_point_position (j, &trk_from_pos);

      roadmap_screen_draw_one_line
               (&from, &to, fully_visible,
                &trk_from_pos, first_shape, last_shape,
                editor_shape_position, pen);

      if ((EditorPens[cfcc][pen_type][0].thickness > 20) &&
         (pen_type == 1)) {
      
         int direction;

         route = editor_line_get_route (line);
      
         if (route != -1) {

            direction = editor_route_get_direction (route, ED_ROUTE_CAR);
            if (direction > 0) { 
               roadmap_screen_draw_line_direction
                  (&from, &to, &trk_from_pos, first_shape, last_shape,
                   editor_shape_position,
                   EditorPens[cfcc][pen_type][0].thickness,
                   direction);
            }
         }
      }
   }

   roadmap_log_pop ();
}


static void editor_screen_repaint_square (int square, int fips, int pen_type) {

   int i;
   int count;
   int layers[256];
   int min_category = 256;

   roadmap_log_push ("editor_screen_repaint_square");

   count = roadmap_layer_visible_lines (layers, 256, pen_type);
   
   for (i = 0; i < count; ++i) {
        if (min_category > layers[i]) min_category = layers[i];
   }

   editor_screen_draw_square (square, fips, min_category, pen_type);

   roadmap_log_pop ();
}

   
void editor_screen_repaint (int max_pen) {

   int x0, y0, x1, y1;
   int x,y;
   int k;

   int fips = roadmap_locator_active ();

   if (editor_db_activate(fips) != -1) {

      editor_square_view (&x0, &y0, &x1, &y1);

      for (k = 0; k < max_pen; ++k) {

         init_lines_drawn ();

         for (y=y0; y<=y1; y++)

            for (x=x0; x<=x1; x++) {
               int square;

               square = editor_square_find (x, y);

               if (square < 0) continue;
         
               editor_screen_repaint_square (square, fips, k);
            }

      }
   }

   for (k = 0; k < max_pen; ++k) {

      if (k < MAX_PEN_LAYERS) {
         if (EditorTrackPens[k].in_use) {
            editor_track_draw_current (EditorTrackPens[k].pen);
         }
      }
   }
}


void editor_screen_set (int status) {

   if (status) {

      SaveShortClickHandler = roadmap_pointer_register_short_click
         (editor_screen_short_click);
      SaveLongClickHandler = roadmap_pointer_register_long_click
         (editor_screen_long_click);

      /* TODO: remove this call */
      roadmap_layer_adjust();
   } else {

      roadmap_pointer_register_short_click (SaveShortClickHandler);
      roadmap_pointer_register_long_click (SaveLongClickHandler);
      select_count = 0;
   }
}


void editor_screen_reset_selected (void) {

   select_count = 0;
   roadmap_screen_redraw ();
}

