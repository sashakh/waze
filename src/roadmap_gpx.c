/* roadmap_gpx.c - stubs, shims, and other interfaces to the GPX library
 *
 * LICENSE:
 *
 *   Copyright 2002 Paul G. Fox
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


#include <expat.h>
#include "roadmap.h"
#include "roadmap_trip.h"
#include "roadmap_path.h"
#define ROADMAP 1
#include "gpx/defs.h"


void fatal (const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buf, sizeof (buf), fmt, ap);
    va_end (ap);
    roadmap_log (ROADMAP_ERROR, "%s", buf);

    exit (1);
}

void warning (const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (buf, sizeof (buf), fmt, ap);
    va_end (ap);
    roadmap_log (ROADMAP_WARNING, "%s", buf);
}

void gpx_rd_cleanup(void);
void gpx_rd_prepare(void);

int
roadmap_gpx_read_file(const char *path,
        const char *name, queue *w, queue *r, queue *t)
{
    queue *elem, *tmp;
    FILE *ifile;
    int ret;

    ifile = roadmap_file_fopen (path, name, "r");
    if (!ifile) return 0;

    ret = gpx_read(ifile, w, r, t);

    if (t) {
        QUEUE_FOR_EACH (t, elem, tmp) {
            route_head *th = (route_head *) elem;
            th->rte_is_track = 1;
        }
    }

    fclose(ifile);

    return ret;
}

int
roadmap_gpx_read_waypoints
        (const char *path, const char *name, queue *waypoints)
{
    int ret;

    ret = roadmap_gpx_read_file(path, name, waypoints, NULL, NULL);

    return ret;

}

int
roadmap_gpx_read_one_track(
    const char *path, const char *name, route_head **track)
{
    int ret;
    queue tracklist;
    route_head *trk;
    QUEUE_INIT(&tracklist);

    ret = roadmap_gpx_read_file(path, name, NULL, NULL, &tracklist);

    if (!ret || QUEUE_EMPTY(&tracklist)) {
        route_flush_queue(&tracklist);
        return 0;
    }

    trk = (route_head *)QUEUE_FIRST(&tracklist);
    dequeue(&trk->Q);
    QUEUE_INIT(&trk->Q);

    /* In case we read more than one track from the file.  */
    route_flush_queue(&tracklist);

    if (*track) route_free(*track);
    *track = trk;

    return ret;

}

int
roadmap_gpx_read_one_route(
    const char *path, const char *name, route_head **route)
{
    int ret;
    queue routelist;
    route_head *rte;

    QUEUE_INIT(&routelist);

    ret = roadmap_gpx_read_file(path, name, NULL, &routelist, NULL);

    if (!ret || QUEUE_EMPTY(&routelist))
        return 0;

    rte = (route_head *)QUEUE_FIRST(&routelist);
    dequeue(&rte->Q);
    QUEUE_INIT(&rte->Q);

    /* In case we read more than one route from the file.  */
    route_flush_queue(&routelist);

    if (*route) route_free(*route);
    *route = rte;

    return ret;

}


int
roadmap_gpx_write_file(const char *path, const char *name,
        queue *w, queue *r, queue *t)
{
    FILE *fp;
    int ret;

    fp = roadmap_file_fopen (path, name, "w");
    if (!fp) return 0;

    ret = gpx_write(fp, w, r, t);

    if (fclose(fp) != 0)
        ret = 0;

    return ret;
}

int roadmap_gpx_write_waypoints(const char *path, const char *name,
        queue *waypoints)
{
    FILE *fp;
    int ret;

    fp = roadmap_file_fopen (path, name, "w");
    if (!fp) return 0;

    ret = gpx_write(fp, waypoints, NULL, NULL);

    if (fclose(fp) != 0)
        ret = 0;

    return ret;
}

int roadmap_gpx_write_route(const char *path, const char *name,
        route_head *route)
{
    FILE *fp;
    int ret;
    queue route_head;

    QUEUE_INIT(&route_head);
    ENQUEUE_TAIL(&route_head, &route->Q);

    fp = roadmap_file_fopen (path, name, "w");
    if (!fp) return 0;

    ret = gpx_write(fp, NULL, &route_head, NULL);

    if (fclose(fp) != 0)
        ret = 0;

    return ret;
}

int roadmap_gpx_write_track(const char *path, const char *name,
        route_head *track)
{
    FILE *fp;
    int ret;
    queue track_head;

    QUEUE_INIT(&track_head);
    ENQUEUE_TAIL(&track_head, &track->Q);

    fp = roadmap_file_fopen (path, name, "w");
    if (!fp) return 0;

    ret = gpx_write(fp, NULL, NULL, &track_head);

    if (fclose(fp) != 0)
        ret = 0;

    return ret;
}
