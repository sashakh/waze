/*
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *   Copyright 2008, 2009, Danny Backx
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

#ifndef INCLUDE__NAVIGATE_VISUAL_H
#define INCLUDE__NAVIGATE_VISUAL_H

#include "roadmap_canvas.h"

void navigate_visual_initialize (void);
void navigate_visual_display (int status);
int navigate_visual_override_pen (int line, int cfcc, int fips, int pen_type,
                                   RoadMapPen *override_pen);

void navigate_visual_refresh (void);

void navigate_visual_route_clear(void);
void navigate_visual_route_add(int line, int layer, int fips);
#endif // INCLUDE__NAVIGATE_VISUAL_H

