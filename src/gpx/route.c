/* 
    Copyright (C) 2002 Robert Lipe, robertlipe@usa.net

    Modified for use with RoadMap, Paul Fox, 2005

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA

 */

#include <stdio.h>
#include "defs.h"


route_head *
route_head_alloc(void)
{
        route_head *rte_head;
        rte_head = (route_head *) xcalloc(sizeof (*rte_head), 1);
        QUEUE_INIT((queue_head *)&rte_head->Q);         /* safety */
        QUEUE_INIT(&rte_head->waypoint_list);
        return rte_head;
}


void
route_add(queue_head *rq, route_head *rte)
{
        ENQUEUE_TAIL(rq, &rte->Q);
}

void
route_del(route_head *rte)
{
        roadmap_list_remove( &rte->Q );
        route_free( rte );
}

route_head *
route_find_route_by_name(queue_head *routes, const char *name)
{
        queue *elem, *tmp;
        route_head *rte;

        QUEUE_FOR_EACH(routes, elem, tmp) {
                rte = (route_head *) elem;
                if (0 == strcmp(rte->rte_name, name)) {
                        return rte;
                }
        }

        return NULL;
}

void
route_add_wpt(route_head *rte, waypoint *next, waypoint *wpt, int after)
{
        static int rte_waypt_index;
        if (after)
            ENQUEUE_AFTER(&next->Q, &wpt->Q);
        else
            ENQUEUE_BEFORE(&next->Q, &wpt->Q);
        rte->rte_waypt_ct++;    /* waypoints in this route */
        rte_waypt_index++;              /* total waypoints in all routes */
        if (wpt->shortname == NULL) {
                char tmpnam[10];
                snprintf(tmpnam, sizeof(tmpnam), "RPT%03d",rte_waypt_index);
                wpt->shortname = xstrdup(tmpnam);
                wpt->wpt_flags.shortname_is_synthetic = 1;
        }
}

void
route_add_wpt_tail(route_head *rte, waypoint *wpt)
{       /* add at end of list */
        route_add_wpt(rte, (waypoint *)&rte->waypoint_list, wpt, 0);
}

waypoint *
route_find_waypt_by_name( route_head *rh, const char *name )
{
        queue *elem, *tmp;

        QUEUE_FOR_EACH(&rh->waypoint_list, elem, tmp) {
                waypoint *waypointp = (waypoint *) elem;
                if (0 == strcmp(waypointp->shortname, name)) {
                        return waypointp;
                }
        }
        return NULL;
}

void 
route_del_wpt( route_head *rte, waypoint *wpt)
{
        roadmap_list_remove( &wpt->Q );
        waypt_free( wpt );
        rte->rte_waypt_ct--;
}

void
route_free(route_head *rte)
{
        if ( rte->rte_name ) {
                xfree(rte->rte_name);
        }
        if ( rte->rte_desc ) {
                xfree(rte->rte_desc);
        }
        waypt_flush_queue(&rte->waypoint_list);
        if ( rte->fs ) {
                fs_chain_destroy( rte->fs );
        }
        xfree(rte);
}

void
route_waypt_iterator (const route_head *rh, waypt_cb cb )
{
        queue *elem, *tmp;
        if (!cb)  {
                return;
        }
        QUEUE_FOR_EACH(&rh->waypoint_list, elem, tmp) {
                waypoint *waypointp;
                waypointp = (waypoint *) elem;
                        (*cb)(waypointp);
        }
                
}

void 
route_reverse(const route_head *rte_hd)
{
        /* Cast away const-ness */
        route_head *rh = (route_head *) rte_hd;
        queue *elem, *tmp;
        QUEUE_FOR_EACH(&rh->waypoint_list, elem, tmp) {
                ENQUEUE_HEAD(&rh->waypoint_list, roadmap_list_remove(elem));
        }
}

void
route_iterator(queue_head *qh, route_hdr rh, route_trl rt, waypt_cb wc)
{
        queue *elem, *tmp;
        QUEUE_FOR_EACH(qh, elem, tmp) {
                const route_head *rhp;
                rhp = (route_head *) elem;
                if (rh) (*rh)(rhp);
                route_waypt_iterator(rhp, wc);
                if (rt) (*rt)(rhp);
        }
}


void
route_flush_queue(queue_head *head)
{
        queue *elem, *tmp;
        queue *q;

        QUEUE_FOR_EACH(head, elem, tmp) {
                q = roadmap_list_remove(elem);
                route_free((route_head *) q);
        }
}
