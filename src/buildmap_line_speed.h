/* buildmap_line_speed.h - Build a line speed table
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
 */

#ifndef _BUILDMAP_LINE_SPEED__H_
#define _BUILDMAP_LINE_SPEED__H_

#include "roadmap_db_line_speed.h"
struct buildmap_line_speeds_struct {

   RoadMapLineSpeedRef *speeds;
   int count;
};

typedef struct buildmap_line_speeds_struct BuildMapSpeed;

BuildMapSpeed *buildmap_line_speed_new (void);
void buildmap_line_speed_free (BuildMapSpeed *speed);
void buildmap_line_speed_add_slot (BuildMapSpeed *speeds, int time_slot,
                                   int speed);

void buildmap_line_speed_initialize (void);
int  buildmap_line_speed_add (BuildMapSpeed *speeds, int line, int opposite);
int  buildmap_line_speed_add_avg (unsigned char from_avg_speed,
                                  unsigned char to_avg_speed,
                                  int line);

void buildmap_line_speed_sort (void);

int  buildmap_line_speed_count (void);

void buildmap_line_speed_save    (void);
void buildmap_line_speed_summary (void);
void buildmap_line_speed_reset   (void);

#endif // _BUILDMAP_LINE_SPEED__H_

