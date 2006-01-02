/* buildmap_index.h - Build the index table for RoadMap.
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
 */

#ifndef INCLUDED__BUILDMAP_INDEX__H
#define INCLUDED__BUILDMAP_INDEX__H

#include "roadmap_types.h"

void buildmap_index_initialize (const char *path);

void buildmap_index_add_map (int wtid,
                             const char *class,
                             const char *authority,
                             const char *filename);

void buildmap_index_set_territory_name (const char *name);
void buildmap_index_add_authority_name (const char *name);
void buildmap_index_set_map_edges      (const RoadMapArea *edges);
void buildmap_index_add_city           (const char *city);
void buildmap_index_add_postal_code    (unsigned int code);

void buildmap_index_sort (void);
void buildmap_index_save (void);
void buildmap_index_summary (void);

#endif // INCLUDED__BUILDMAP_INDEX__H

