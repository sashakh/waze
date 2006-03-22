/* buildmap_dglib.h - Build dglib graph.
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

#ifndef _BUILDMAP_DGLIB__H_
#define _BUILDMAP_DGLIB__H_

void buildmap_dglib_initialize (time_t creation_time);
int  buildmap_dglib_add
       (unsigned char from_flags,
        unsigned char to_flags,
        unsigned char from_max_speed,
        unsigned char to_max_speed,
        unsigned short from_cross_time,
        unsigned short to_cross_time,
        int line);

void buildmap_dglib_sort (void);

int  buildmap_dglib_count (void);

void  buildmap_dglib_save (const char *path, const char *name);
void buildmap_dglib_summary (void);
void buildmap_dglib_reset   (time_t creation_time);

#endif // _BUILDMAP_DGLIB__H_

