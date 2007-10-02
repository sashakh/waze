/* roadmap_sprite.c - manage the roadmap sprites that represent moving objects.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 * DESCRIPTION:
 *
 *   This module draws predefined graphical objects on the RoadMap canvas.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_file.h"
#include "roadmap_scan.h"
#include "roadmap_canvas.h"
#include "roadmap_sprite.h"


typedef struct {

   int  object_count;
   int *objects;

   int  point_count;
   RoadMapGuiPoint *points;

} RoadmapSpriteDrawingSequence;


typedef struct roadmap_plane_record {

   struct roadmap_plane_record *next;

   RoadMapPen pen;

   RoadmapSpriteDrawingSequence lines;
   RoadmapSpriteDrawingSequence disks;
   RoadmapSpriteDrawingSequence circles;
   RoadmapSpriteDrawingSequence polygons;

} RoadMapSpritePlane;

typedef struct roadmap_sprite_record {

   char *name;
   char *alias_name;

   union {
      struct {
         RoadMapSpritePlane  first;
         RoadMapSpritePlane *last;
      } planes;
      struct roadmap_sprite_record *alias;
   } drawing;

   RoadMapGuiRect bbox;

   struct roadmap_sprite_record *next;

} *RoadMapSprite;

static RoadMapSprite RoadMapSpriteList = NULL;

/* The default sprite used when the sprite was not found: */

static struct roadmap_sprite_record *RoadMapSpriteDefault = NULL;


int RoadMapSpritePointCount = 0;
RoadMapGuiPoint *RoadMapSpritePoints = NULL;



static RoadMapSprite roadmap_sprite_search (const char *name) {

   RoadMapSprite cursor;

   for (cursor = RoadMapSpriteList; cursor != NULL; cursor = cursor->next) {

      if (strcasecmp(name, cursor->name) == 0) {
         if (cursor->alias_name != NULL) {
            cursor = cursor->drawing.alias;
         }
         break;
      }
   }

   if (cursor == NULL) {
      return RoadMapSpriteDefault;
   }
   return cursor;
}



static void roadmap_sprite_decode_plane
               (RoadMapSprite sprite,
                const char *color, const char *thickness) {

   int   t;
   char  pen[256];

   if (sprite->drawing.planes.last == NULL) {

      sprite->drawing.planes.last = &(sprite->drawing.planes.first);
      sprite->drawing.planes.first.next = NULL;

   } else {

      sprite->drawing.planes.last->next =
         calloc (1, sizeof(sprite->drawing.planes.first));
      roadmap_check_allocated(sprite->drawing.planes.last->next);

      sprite->drawing.planes.last = sprite->drawing.planes.last->next;
   }

   t = atoi(thickness);

   snprintf (pen, sizeof(pen), "%s.%d.%s", sprite->name, t, color);
   sprite->drawing.planes.last->pen = roadmap_canvas_create_pen (pen);

   roadmap_canvas_set_foreground (color);
   roadmap_canvas_set_thickness  (t);
}


static void roadmap_sprite_decode_point
               (RoadMapGuiPoint *point, const char *data) {

   char *p;

   point->x = atoi(data);
   p = strchr (data, ',');
   if (p != NULL) {
      point->y = atoi(p+1);
   } else {
      point->y = 0;
   }
}

static void roadmap_sprite_adjust_bbox
                (RoadMapGuiRect *bbox, short x, short y) {
    if (x < bbox->minx) bbox->minx = x;
    if (x > bbox->maxx) bbox->maxx = x;
    if (y < bbox->miny) bbox->miny = y;
    if (y > bbox->maxy) bbox->maxy = y;
}


static void roadmap_sprite_decode_sequence
               (RoadMapSprite sprite,
                RoadmapSpriteDrawingSequence *sequence,
                int argc, const char **argv) {

   int i;
   RoadMapGuiPoint *points;

   argc -= 1;

   sequence->object_count += 1;
   sequence->objects =
       realloc (sequence->objects,
                sequence->object_count * sizeof(*(sequence->objects)));

   sequence->objects[sequence->object_count-1] = argc;

   sequence->points =
       realloc (sequence->points,
                (sequence->point_count + argc) * sizeof(*(sequence->points)));

   points = sequence->points + sequence->point_count - 1;

   for (i = 1; i <= argc; ++i) {
      roadmap_sprite_decode_point (++points, argv[i]);
      roadmap_sprite_adjust_bbox(&sprite->bbox, points->x, points->y);
   }
   sequence->point_count += argc;
}


static void roadmap_sprite_decode_circle
               (RoadMapSprite sprite,
                RoadmapSpriteDrawingSequence *sequence, const char **argv) {

   RoadMapGuiPoint *center;
   int diam;

   sequence->object_count += 1;
   sequence->point_count += 1;
   sequence->objects =
       realloc (sequence->objects,
                sequence->object_count * sizeof(*(sequence->objects)));

   diam = atoi(argv[2]);
   sequence->objects[sequence->object_count-1] = diam;

   sequence->points =
       realloc (sequence->points,
                sequence->object_count * sizeof(*(sequence->points)));

   roadmap_sprite_decode_point
      (sequence->points+sequence->object_count-1, argv[1]);

   center = sequence->points+sequence->object_count-1;

   roadmap_sprite_adjust_bbox(&sprite->bbox, center->x + diam, center->y);
   roadmap_sprite_adjust_bbox(&sprite->bbox, center->x - diam, center->y);
   roadmap_sprite_adjust_bbox(&sprite->bbox, center->x, center->y + diam);
   roadmap_sprite_adjust_bbox(&sprite->bbox, center->x, center->y - diam);
}

static void roadmap_sprite_decode_bbox
               (RoadMapGuiRect        *bbox, int argc, const char **argv) {

   RoadMapGuiPoint pt;

   argc -= 1;

   if (argc >= 1) {
      roadmap_sprite_decode_point (&pt, argv[1]);
      bbox->minx = pt.x;
      bbox->miny = pt.y;
   }
   if (argc >= 2) {
      roadmap_sprite_decode_point (&pt, argv[2]);
      bbox->maxx = pt.x;
      bbox->maxy = pt.y;
   }
}


static RoadMapSprite roadmap_sprite_new
          (int argc, const char **argv) {

   RoadMapSprite sprite = calloc(1, sizeof(*sprite));
   roadmap_check_allocated(sprite);

   sprite->name = strdup (argv[1]);

   sprite->drawing.planes.first.pen = roadmap_canvas_create_pen ("Black");

   sprite->next = RoadMapSpriteList;
   RoadMapSpriteList = sprite;

   return sprite;
}


static int roadmap_sprite_maxpoint (RoadMapSprite sprite) {

   int max_point_count = 0;

   RoadMapSpritePlane *plane;

   if (sprite->alias_name != NULL) return 0;

   for (plane = &(sprite->drawing.planes.first);
        plane != NULL;
        plane = plane->next) {

      if (max_point_count < plane->lines.point_count) {
         max_point_count = plane->lines.point_count;
      }
      if (max_point_count < plane->disks.point_count) {
         max_point_count = plane->disks.point_count;
      }
      if (max_point_count < plane->circles.point_count) {
         max_point_count = plane->circles.point_count;
      }
      if (max_point_count < plane->polygons.point_count) {
         max_point_count = plane->polygons.point_count;
      }
   }
   return max_point_count;
}


static void roadmap_sprite_alias (RoadMapSprite sprite, const char *arg) {

   sprite->alias_name = strdup(arg);
   roadmap_check_allocated(sprite->alias_name);
}


static int roadmap_sprite_drawing_is_valid (RoadMapSprite sprite) {

   if (sprite == NULL) {
      roadmap_log (ROADMAP_ERROR, "sprite name is missing");
      return 0;
   }

   if (sprite->alias_name != NULL) {
      roadmap_log (ROADMAP_ERROR,
            "sprite alias %s has drawing directives",
            sprite->name);
      return 0; /* Failed. */
   }

   if (sprite->drawing.planes.last == NULL) {
       roadmap_log (ROADMAP_ERROR,
                    "first plane of sprite %s is missing", sprite->name);
       return 0;
   }

   return 1;
}


static int roadmap_sprite_load_text (char *line) {

   int argc;
   const char *argv[256];

   static RoadMapSprite sprite;

   argv[0] = strtok(line, " \t\n\r");
   if (argv[0] == NULL || argv[0][0] == '#') return 1;
   argc = 1;

   while((argv[argc] = strtok(NULL, " \t\n\r")) != NULL) {
      argc++;
   }

   if (sprite == NULL) {

      if (argv[0][0] != 'S') {
         roadmap_log (ROADMAP_ERROR, "sprite name is missing");
         return 0;
      }

   }

   switch (argv[0][0]) {

   case 'A':

      if (sprite->alias_name != NULL) {
         roadmap_log (ROADMAP_ERROR,
                      "repeated alias for sprite %s", sprite->name);
         break;
      }
      if (sprite->drawing.planes.last != NULL) {
         roadmap_log (ROADMAP_ERROR,
                      "sprite alias %s has drawing directives",
                      sprite->name);
         break;
      }
      roadmap_sprite_alias (sprite, argv[1]);
      break;

   case 'F':

      if (sprite->alias_name != NULL) {
         roadmap_log (ROADMAP_ERROR,
                      "sprite alias %s has drawing directives",
                      sprite->name);
         break;
      }
      roadmap_sprite_decode_plane (sprite, argv[1], argv[2]);
      break;

   case 'L':

      if (roadmap_sprite_drawing_is_valid (sprite)) {
         roadmap_sprite_decode_sequence
            (sprite, &(sprite->drawing.planes.last->lines), argc, argv);
      }
      break;

   case 'P':

      if (roadmap_sprite_drawing_is_valid (sprite)) {
         roadmap_sprite_decode_sequence
            (sprite, &(sprite->drawing.planes.last->polygons), argc, argv);
      }
      break;

   case 'D':

      if (roadmap_sprite_drawing_is_valid (sprite)) {
         roadmap_sprite_decode_circle
            (sprite, &(sprite->drawing.planes.last->disks), argv);
      }
      break;

   case 'C':

      if (roadmap_sprite_drawing_is_valid (sprite)) {
         roadmap_sprite_decode_circle
            (sprite, &(sprite->drawing.planes.last->circles), argv);
      }
      break;

   case 'B':

      if (roadmap_sprite_drawing_is_valid (sprite)) {
         roadmap_sprite_decode_bbox(&sprite->bbox, argc, argv);
      }
      break;

   case 'S':

      sprite = roadmap_sprite_new (argc, argv);
      break;
      
   }

   return 1;
}


static void roadmap_sprite_load_file (const char *path) {

   FILE *file;

   if ((file = roadmap_file_fopen (path, "sprites", "r")) == NULL) {

      roadmap_log (ROADMAP_ERROR, "cannot open file 'sprites' in %s", path);

   } else {
      char  line[1024];

      while (!feof(file)) {

         if (fgets (line, sizeof(line), file) == NULL) break;

         if (roadmap_sprite_load_text (line) == 0) break;

      }

      fclose (file);
   }
}


static void roadmap_sprite_resolve_aliases (void) {

   RoadMapSprite sprite;
   RoadMapSprite cursor;

   /* First retrieve the sprite referenced in each alias. */

   for (sprite = RoadMapSpriteList; sprite != NULL; sprite = sprite->next) {

      if (sprite->alias_name == NULL) continue;

      /* We don't use roadmap_sprite_search because we don't want
       * to follow the alias link (not fully initialized yet).
       */
      for (cursor = RoadMapSpriteList; cursor != NULL; cursor = cursor->next) {
         if (strcasecmp(sprite->alias_name, cursor->name) == 0) break;
      }

      if (cursor == sprite) {
         roadmap_log (ROADMAP_ERROR,
               "self alias reference for sprite %s", sprite->name);
         cursor = RoadMapSpriteDefault; /* Break the death spiral. */
      }
      else if (cursor == NULL) {
         roadmap_log (ROADMAP_ERROR,
                      "cannot resolve alias %s for sprite %s",
                      sprite->alias_name,
                      sprite->name);
         cursor = RoadMapSpriteDefault;
      }
      sprite->drawing.alias = cursor;
   }

   /* Now resolve the multiple levels of aliases. */

   for (sprite = RoadMapSpriteList; sprite != NULL; sprite = sprite->next) {

      if (sprite->alias_name == NULL) continue;

      while (sprite->drawing.alias->alias_name != NULL) {

         if (sprite->drawing.alias == sprite) {
            roadmap_log (ROADMAP_ERROR,
                         "circular alias link for sprite %s", sprite->name);
            sprite->drawing.alias = RoadMapSpriteDefault;
            break;
         }

         sprite->drawing.alias = sprite->drawing.alias->drawing.alias;
      }
   }
}


static void roadmap_sprite_place (RoadmapSpriteDrawingSequence *sequence,
                                  RoadMapGuiPoint *location,
                                  int orientation) {

   int i;
   int x = location->x;
   int y = location->y;
   RoadMapGuiPoint *from = sequence->points;
   RoadMapGuiPoint *to   = RoadMapSpritePoints;

   for (i = sequence->point_count - 1; i >= 0; --i) {

      to->x = x + from->x;
      to->y = y + from->y;

      to += 1;
      from += 1;
   }
   roadmap_math_rotate_object
      (sequence->point_count, RoadMapSpritePoints, location, orientation);
}

void roadmap_sprite_bbox (const char *name, RoadMapGuiRect *bbox) {

   RoadMapSprite sprite = roadmap_sprite_search (name);

   if (sprite == NULL || sprite->alias_name != NULL) return;

   *bbox = sprite->bbox;

}

void roadmap_sprite_draw
        (const char *name, RoadMapGuiPoint *location, int orientation) {

   RoadMapSprite sprite = roadmap_sprite_search (name);
   RoadMapSpritePlane *plane;


   if (sprite == NULL || sprite->alias_name != NULL) return;

   for (plane = &(sprite->drawing.planes.first);
        plane != NULL;
        plane = plane->next) {

      roadmap_canvas_select_pen (plane->pen);

      if (plane->polygons.object_count > 0) {

         roadmap_sprite_place (&(plane->polygons), location, orientation);

         roadmap_canvas_draw_multiple_polygons
            (plane->polygons.object_count,
             plane->polygons.objects, RoadMapSpritePoints, 1, 0);
      }

      if (plane->disks.object_count > 0) {

         roadmap_sprite_place (&(plane->disks), location, orientation);

         roadmap_canvas_draw_multiple_circles
            (plane->disks.object_count,
             RoadMapSpritePoints, plane->disks.objects, 1, 0);
      }

      if (plane->lines.object_count > 0) {

         roadmap_sprite_place (&(plane->lines), location, orientation);

         roadmap_canvas_draw_multiple_lines
            (plane->lines.object_count,
             plane->lines.objects, RoadMapSpritePoints, 0);
      }

      if (plane->circles.object_count > 0) {

         roadmap_sprite_place (&(plane->circles), location, orientation);

         roadmap_canvas_draw_multiple_circles
            (plane->circles.object_count,
             RoadMapSpritePoints, plane->circles.objects, 0, 0);
      }
   }
}


void roadmap_sprite_load (void) {

   int max_point_count;
   const char *cursor;

   RoadMapSprite sprite;


   for (cursor = roadmap_scan ("config", "sprites");
        cursor != NULL;
        cursor = roadmap_scan_next ("config", "sprites", cursor)) {

      roadmap_sprite_load_file (cursor);
   }

   for (cursor = roadmap_scan ("user", "sprites");
        cursor != NULL;
        cursor = roadmap_scan_next ("user", "sprites", cursor)) {

      roadmap_sprite_load_file (cursor);
   }

   RoadMapSpriteDefault = roadmap_sprite_search ("Default");

   if (RoadMapSpriteDefault == NULL) {

      /* initialize character arrays to keep these strings writeable --
       * strtok() will process them
       */
      char hardcoded1[] = "S Default";
      char hardcoded2[] = "F black 1";
      char hardcoded3[] = "P -4,-4 4,-4 4,4 -4,4";

      roadmap_sprite_load_text (hardcoded1);
      roadmap_sprite_load_text (hardcoded2);
      roadmap_sprite_load_text (hardcoded3);

      RoadMapSpriteDefault = roadmap_sprite_search ("Default");
   }

   roadmap_sprite_resolve_aliases();


   /* Allocate the space required to draw any configured sprite: */

   RoadMapSpritePointCount = roadmap_sprite_maxpoint (RoadMapSpriteDefault);

   for (sprite = RoadMapSpriteList; sprite != NULL; sprite = sprite->next) {

      if (sprite->alias_name != NULL) continue;

      max_point_count = roadmap_sprite_maxpoint (sprite);

      if (RoadMapSpritePointCount < max_point_count) {
         RoadMapSpritePointCount = max_point_count;
      }
   }
   RoadMapSpritePoints =
      calloc (RoadMapSpritePointCount, sizeof(*RoadMapSpritePoints));
}

