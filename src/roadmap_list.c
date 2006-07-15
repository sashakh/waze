/* roadmap_list.c - Manage a list.
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
 *   See roadmap_list.h.
 */

#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_list.h"


void roadmap_list_insert (RoadMapList *list, RoadMapListItem *item) {
    
    item->next = list->first;
    
    if (list->first == NULL) {
        list->last = item;
        list->first  = item;
    } else {
        list->first->previous = item;
    }
    item->previous = NULL;
    list->first = item;
}


void roadmap_list_append (RoadMapList *list, RoadMapListItem *item) {
    
    item->previous = list->last;
    
    if (list->last == NULL) {
        list->first = item;
        list->last  = item;
    } else {
        list->last->next = item;
    }
    item->next = NULL;
    list->last = item;
}


RoadMapListItem *roadmap_list_remove (RoadMapList *list, RoadMapListItem *item) {
    
    if (item->previous == NULL) {
        list->first = item->next;
    } else {
        item->previous->next = item->next;
    }
    if (item->next == NULL) {
        list->last = item->previous;
    } else {
        item->next->previous = item->previous;
    }
    return item;
}

int  roadmap_list_count  (const RoadMapList *list) {
    
    int count = 0;
    RoadMapListItem *item;
    
    for (item = list->first; item != NULL; item = item->next) {
        count +=1;
    }
    return count;
}
