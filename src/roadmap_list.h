/* roadmap_list.h - Manage a list.
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
 */

#ifndef INCLUDE__ROADMAP_LIST__H
#define INCLUDE__ROADMAP_LIST__H

struct roadmap_list_link {
    struct roadmap_list_link *next;
    struct roadmap_list_link *previous;
};

typedef struct roadmap_list_link RoadMapListItem;
    
struct roadmap_list_head {
    struct roadmap_list_link *first;
    struct roadmap_list_link *last;
};

typedef struct roadmap_list_head RoadMapList;
    
#define ROADMAP_LIST_EMPTY {NULL, NULL}

#define ROADMAP_LIST_IS_EMPTY(l) ((l)->first == NULL)


void roadmap_list_insert (RoadMapList *list, RoadMapListItem *item);
void roadmap_list_append (RoadMapList *list, RoadMapListItem *item);
void roadmap_list_remove (RoadMapList *list, RoadMapListItem *item);

int  roadmap_list_count  (RoadMapList *list);

#endif // INCLUDE__ROADMAP_LIST__H
