/* roadmap_polygon.h - the format of the polygon table used by RoadMap.
 *
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
 * SYNOPSYS:
 *
 * The RoadMap polygons are described by the following tables:
 *
 * polygon/head     for each polygon, the category and list of lines.
 * polygon/points   the list of points defining the polygons border.
 */

#ifndef INCLUDED__ROADMAP_DB_POLYGON__H
#define INCLUDED__ROADMAP_DB_POLYGON__H

#include "roadmap_types.h"

typedef struct {  /* table polygon/head */

   unsigned short first;  /* low 16 bits */
   unsigned short count;  /* low 16 bits */

   RoadMapString name;
   unsigned char  cfcc;
   unsigned char hi_f_c;  /* high 4 bits of "first" and "count" values */

   /* TBD: replace these with a RoadMapArea (not compatible!). */
   int   north;
   int   west;
   int   east;
   int   south;

} RoadMapPolygon;

typedef struct {  /* table polygon/points */

   int point;

} RoadMapPolygonPoint;

/* these accessors for the "count" and "first" values, which
 * are 20 bits each -- 16 in an unsigned short, and 4 in each
 * nibble of the "hi_f_c" element.
 */

#define hifirst(t) (((t)->hi_f_c >> 4) & 0xf)
#define hicount(t) (((t)->hi_f_c >> 0) & 0xf)

#define sethifirst(t,i) { (t)->hi_f_c = \
        ((t)->hi_f_c & 0x0f) + (((i) & 0xf) << 4); }
#define sethicount(t,i) { (t)->hi_f_c = \
        ((t)->hi_f_c & 0xf0) + (((i) & 0xf) << 0); }

#define roadmap_polygon_get_count(this) \
        ((this)->count + (hicount(this) * 65536))
#define roadmap_polygon_get_first(this) \
        ((this)->first + (hifirst(this) * 65536))

#define buildmap_polygon_set_count(this, c) do { \
                (this)->count = (c) % 65536; \
                sethicount(this, (c) / 65536); \
        } while(0)
#define buildmap_polygon_set_first(this, f) do { \
                (this)->first = (f) % 65536; \
                sethifirst(this, (f) / 65536); \
        } while(0)

#endif // INCLUDED__ROADMAP_DB_POLYGON__H

