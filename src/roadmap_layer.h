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

#include "roadmap_canvas.h"

#define LAYER_VISIBLE_ROADS   1
#define LAYER_ALL_ROADS       2

int  roadmap_layer_max_pen(void);

int roadmap_layer_all_roads (int *layers, int size);
int  roadmap_layer_visible_roads (int *layers, int size);
int  roadmap_layer_visible_lines (int *layers, int size, int pen_type);

int  roadmap_layer_is_visible (int layer);

RoadMapPen roadmap_layer_get_pen (int layer, int pen_type);

void roadmap_layer_adjust (void);

void roadmap_layer_initialize (void);

void roadmap_layer_get_categories_names (char **names[], int *count);

#endif // INCLUDE__ROADMAP_LAYER__H
