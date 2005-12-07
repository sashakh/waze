/*
    Perform various operations on waypoints.

    Copyright (C) 2002-2005 Robert Lipe, robertlipe@usa.net

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


waypoint *
waypt_dupe(const waypoint *wpt) 
{
        /*
         * This and waypt_free should be closely synced.
         */
        waypoint * tmp;
        tmp = waypt_new();
        memcpy(tmp, wpt, sizeof(waypoint));

        if (wpt->shortname) {
                tmp->shortname = xstrdup(wpt->shortname);
                tmp->wpt_flags.shortname_is_synthetic = 
                    wpt->wpt_flags.shortname_is_synthetic;
        }
        if (wpt->description)
                tmp->description = xstrdup(wpt->description);
        if (wpt->notes)
                tmp->notes = xstrdup(wpt->notes);
        if (wpt->url)
                tmp->url = xstrdup(wpt->url);
        if (wpt->url_link_text)
                tmp->url_link_text = xstrdup(wpt->url_link_text);
        if (wpt->icon_descr && wpt->wpt_flags.icon_descr_is_dynamic)
                tmp->icon_descr = xstrdup(wpt->icon_descr);
        if (wpt->gc_data.desc_short.utfstring) {
                tmp->gc_data.desc_short.utfstring = 
                        xstrdup(wpt->gc_data.desc_short.utfstring);
        }
        if (wpt->gc_data.desc_long.utfstring) {
                tmp->gc_data.desc_long.utfstring = 
                        xstrdup(wpt->gc_data.desc_long.utfstring);
        }
        if (wpt->gc_data.placer) {
                tmp->gc_data.placer = xstrdup(wpt->gc_data.placer);
        }
        if (wpt->gc_data.hint) {
                tmp->gc_data.hint = xstrdup(wpt->gc_data.hint);
        }

        /*
         * It's important that this duplicated waypoint not appear
         * on the master Q.
         */
        tmp->Q.next = tmp->Q.prev = NULL;
        tmp->fs = fs_chain_copy( wpt->fs );

        return tmp;
}

void
waypt_add(queue *q, waypoint *wpt)
{
        static int waypt_index;
        ENQUEUE_TAIL(q, &wpt->Q);
        waypt_index++;

        /*
         * Some input may not have one or more of these types so we
         * try to be sure that we have these fields even if just by
         * copying them from elsewhere.
         */
        if (wpt->shortname == NULL) {
                if (wpt->description) {
                        wpt->shortname = xstrdup(wpt->description);
                } else if (wpt->notes) {
                        wpt->shortname = xstrdup(wpt->notes);
                } else {
                /* Last ditch:  make up a name */
                        char cbuf[10];
                        snprintf(cbuf, sizeof(cbuf), "WPT%03d", waypt_index);
                        wpt->shortname = xstrdup(cbuf);
                }
        }

        if (wpt->description == NULL || strlen(wpt->description) == 0) {
                if (wpt->description)
                        xfree(wpt->description);
                if (wpt->notes != NULL) {
                        wpt->description = xstrdup(wpt->notes);
                }
        }
}

void
waypt_del(waypoint *wpt)
{
        dequeue(&wpt->Q);
}

/*
 * A constructor for a single waypoint.
 */
waypoint *
waypt_new(void)
{
        waypoint *wpt;

        wpt = (waypoint *) xcalloc(sizeof (*wpt), 1);
        wpt->altitude = unknown_alt;
        wpt->course = -999.0;
        wpt->speed = -999.0;
        wpt->fix = fix_unknown;
        wpt->sat = -1;

        return wpt;
}

void
waypt_iterator(queue *q, waypt_cb cb)
{
        queue *elem, *tmp;
        waypoint *waypointp;

        QUEUE_FOR_EACH(q, elem, tmp) {
                waypointp = (waypoint *) elem;
                (*cb) (waypointp);
        }
}

void
waypt_init_bounds(bounds *bounds)
{
        /* Set data out of bounds so that even one waypoint will reset */
        bounds->max_lat = -500000000;
        bounds->max_lon = -500000000;
        bounds->min_lat = 500000000;
        bounds->min_lon = 500000000;
}

int
waypt_bounds_valid(bounds *bounds)
{
        /* Returns true if bb has any 'real' data in it */
        return bounds->max_lat  > -360;
}

/*
 * Recompund bounding box based on new position point.
 */
void 
waypt_add_to_bounds(bounds *bounds, const waypoint *waypointp)
{
        if (waypointp->pos.latitude > bounds->max_lat)
                bounds->max_lat = waypointp->pos.latitude;
        if (waypointp->pos.longitude > bounds->max_lon)
                bounds->max_lon = waypointp->pos.longitude;
        if (waypointp->pos.latitude < bounds->min_lat)
                bounds->min_lat = waypointp->pos.latitude;
        if (waypointp->pos.longitude < bounds->min_lon)
                bounds->min_lon = waypointp->pos.longitude;
}



/*
 *  Makes another pass over the data to compute bounding
 *  box data and populates bounding box information.
 */

void
waypt_compute_bounds(queue *q, bounds *bounds)
{
        queue *elem, *tmp;
        waypoint *waypointp;

        /* Set data out of bounds so that even one waypoint will reset */
        bounds->max_lat = -9999;
        bounds->max_lon = -9999;
        bounds->min_lat = 9999;
        bounds->min_lon = 9999;

        QUEUE_FOR_EACH(q, elem, tmp) {
                waypointp = (waypoint *) elem;
                if (waypointp->pos.latitude > bounds->max_lat)
                        bounds->max_lat = waypointp->pos.latitude;
                if (waypointp->pos.longitude > bounds->max_lon)
                        bounds->max_lon = waypointp->pos.longitude;
                if (waypointp->pos.latitude < bounds->min_lat)
                        bounds->min_lat = waypointp->pos.latitude;
                if (waypointp->pos.longitude < bounds->min_lon)
                        bounds->min_lon = waypointp->pos.longitude;
        }
}

waypoint *
waypt_find_waypt_by_name(queue *q, const char *name)
{
        queue *elem, *tmp;
        waypoint *waypointp;

        QUEUE_FOR_EACH(q, elem, tmp) {
                waypointp = (waypoint *) elem;
                if (0 == strcmp(waypointp->shortname, name)) {
                        return waypointp;
                }
        }

        return NULL;
}

void 
waypt_free( waypoint *wpt )
{
        /*
         * This and waypt_dupe should be closely synced.
         */
        if (wpt->shortname) {
                xfree(wpt->shortname);
        }
        if (wpt->description) {
                xfree(wpt->description);
        }
        if (wpt->notes) {
                xfree(wpt->notes);
        }
        if (wpt->url) {
                xfree(wpt->url);
        }
        if (wpt->url_link_text) {
                xfree(wpt->url_link_text);
        }
        if (wpt->icon_descr && wpt->wpt_flags.icon_descr_is_dynamic) {
                xfree((char *)(void *)wpt->icon_descr);
        }
        if (wpt->gc_data.desc_short.utfstring) {
                xfree(wpt->gc_data.desc_short.utfstring);
        }
        if (wpt->gc_data.desc_long.utfstring) {
                xfree(wpt->gc_data.desc_long.utfstring);
        }
        if (wpt->gc_data.placer) {
                xfree(wpt->gc_data.placer);
        }
        if (wpt->gc_data.hint) {
                xfree (wpt->gc_data.hint);
        } 
        fs_chain_destroy( wpt->fs );
        xfree(wpt);     
}

void 
waypt_flush_queue( queue *head )
{
        queue *elem, *tmp;

        QUEUE_FOR_EACH(head, elem, tmp) {
                waypoint *q = (waypoint *) dequeue(elem);
                waypt_free(q);
        }
}


