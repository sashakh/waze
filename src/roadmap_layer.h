/* roadmap_layer.h - layer management: declutter, filter, etc..
 *
 * LICENSE:
 *
 *   Copyright 2003 Pascal F. Martin
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
 */

#ifndef INCLUDE__ROADMAP_LAYER__H
#define INCLUDE__ROADMAP_LAYER__H

int  roadmap_layer_max_pen(void);

void roadmap_layer_adjust (void);

/* DEPRECATED, use: roadmap_layer_select_class ("Roads") */
int  roadmap_layer_visible_roads (int *layers, int size);

int  roadmap_layer_visible_lines (int *layers, int size, int pen_type);

int  roadmap_layer_line_is_visible (int layer);

void roadmap_layer_polygon_select_pen (int layer, int pen_type);
void roadmap_layer_line_select_pen    (int layer, int pen_type);

void roadmap_layer_class_first (void);
void roadmap_layer_class_next (void);

const char *roadmap_layer_class_name (void);

int  roadmap_layer_select_class (const char *name);

void roadmap_layer_select_set (const char *set); /* Either "day" or "night". */

void roadmap_layer_load (void);

void roadmap_layer_initialize (void);

#endif // INCLUDE__ROADMAP_LAYER__H
