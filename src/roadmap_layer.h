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
 *
 * DESCRIPTION:
 *
 *   This module gives access to the layers provided by the current maps.
 *
 *   The layers are grouped in "classes". One class represents the list of
 *   layers implemented in one map file, i.e. the object from one map file
 *   are all described within a class.
 *
 *   Any layer index is relative to the class the layer belongs to. In
 *   other words, one MUST first select a class before accessing layers.
 *   The class selection should normally go pair to pair with a map
 *   selection.
 *
 *   This module support several sets of classes. A set is a way to
 *   describe a specific style for the layers, and one can switch from
 *   one set to the other at any time, except during the drawing of
 *   the map. In other word, a screen must be drawn with one single set
 *   from beginning to end.
 *
 *   The module supports a predefined list of sets: "day", "night" and
 *   "default". The intend is for RoadMap to switch between "day" and
 *   "night" depending on the sunrise and sunset times. Either the two
 *   sets "day" and "night" must be defined, or else "default" is used.
 *   Which one is used is determined by the configuration data (see below).
 *
 *   Last, but not least, the user can specify in the preferences which
 *   configuration to use, i.e. which map skin. A skin defines all the
 *   classes for all the required sets. A skin may contains only one
 *   unamed set of class (i.e. "default"), or it may contains the two
 *   sets "day" and "night". In the second case, "default" will not be
 *   used, even if defined.
 */

#ifndef INCLUDED__ROADMAP_LAYER__H
#define INCLUDED__ROADMAP_LAYER__H

#include "roadmap_canvas.h"

/* This is the maximum number of layers PER CLASS. As there is no limit on
 * the number of classes, the total number of layers is actually unlimited.
 * We could have implemented support for an unlimited number of layers per
 * class, but who wants a thousand layers per map, anyway?
 * Having this limit makes life easier for everyone, and the downside is nil.
 */
#define ROADMAP_MAX_LAYERS  1024

unsigned int roadmap_layer_max_pen(void);

void roadmap_layer_adjust (void);


void roadmap_layer_select_set (const char *set); /* Either "day" or "night". */

int  roadmap_layer_select_class (const char *name);

/* DEPRECATED, use: roadmap_layer_select_class ("Roads") */
int  roadmap_layer_visible_roads (int *layers, int size);

int  roadmap_layer_visible_lines
        (int *layers, int size, unsigned int pen_index);

int  roadmap_layer_line_is_visible (int layer);

RoadMapPen roadmap_layer_get_pen (int layer, unsigned int pen_index);

void roadmap_layer_class_first (void);
void roadmap_layer_class_next (void);

const char *roadmap_layer_class_name (void);

void roadmap_layer_load (void);

void roadmap_layer_initialize (void);

#endif // INCLUDED__ROADMAP_LAYER__H

