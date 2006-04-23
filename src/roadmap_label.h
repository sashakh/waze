/* roadmap_label.h - Manage map labels.
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *
 *   This code was mostly taken from UMN Mapserver
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

#ifndef __ROADMAP_LABEL__H
#define __ROADMAP_LABEL__H

#include "roadmap.h"
#include "roadmap_gui.h"

#define MAX_LABELS 150

typedef struct {

  int featuresize;

  PluginLine line;
  PluginStreet street;

  int angle; /* degrees */
  RoadMapGuiPoint point; /* label point */
  RoadMapGuiRect bbox; /* label bounding box */

  int status; /* has this label been drawn or not */

} labelCacheMemberObj;

typedef struct {
  labelCacheMemberObj labels[MAX_LABELS];
  int numlabels;
} labelCacheObj;


int roadmap_label_add (const RoadMapGuiPoint *point, int angle,
                       int featuresize, const PluginLine *line);

int roadmap_label_initialize (void);

int roadmap_label_draw_cache (void);

#endif // __ROADMAP_LABEL__H
