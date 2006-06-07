/* roadmap_county.c - retrieve the county to which a specific place belongs.
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
 */

#ifndef INCLUDED__ROADMAP_INDEX__H
#define INCLUDED__ROADMAP_INDEX__H

#include "roadmap_types.h"
#include "roadmap_dbread.h"

int  roadmap_index_by_position
        (const RoadMapPosition *position, int *wtid, int count);

int  roadmap_index_by_city (const char *authority, const char *city);

int  roadmap_index_by_authority (const char *authority, int *wtid, int count);

int  roadmap_index_by_territory
        (int wtid, char **class, char **files, int count);

const char *roadmap_index_get_authority_name (int wtid);

const char *roadmap_index_get_territory_name (int wtid);
const char *roadmap_index_get_territory_path (int wtid);

const RoadMapArea *roadmap_index_get_territory_edges (int wtid);

int  roadmap_index_get_territory_count (void);

int  roadmap_index_get_map_count (void);

int  roadmap_index_list_authorities (char ***symbols, char ***names);

extern roadmap_db_handler RoadMapIndexHandler;

#endif // INCLUDED__ROADMAP_INDEX__H

