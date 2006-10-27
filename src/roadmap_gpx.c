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


#ifdef ROADMAP_USES_EXPAT
#include <expat.h>
#endif

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

#ifdef ROADMAP_USES_EXPAT

int
roadmap_gpx_read_file(const char *path,
        const char *name, queue_head *w, int wee, queue_head *r, queue_head *t)
{

    queue *elem, *tmp;
    FILE *ifile;
    int ret;

    ifile = roadmap_file_fopen (path, name, "r");
    if (!ifile) return 0;

    ret = gpx_read(ifile, w, wee, r, t);

    if (t) {
        QUEUE_FOR_EACH (t, elem, tmp) {
            route_head *th = (route_head *) elem;
            th->rte_is_track = 1;
        }
    }

    fclose(ifile);

    return ret;
}

/* read a list of waypoints (or optionally, weepoints) from a file */
int
roadmap_gpx_read_waypoints
        (const char *path, const char *name, queue_head *waypoints, int wee)
{
    int ret;

    ret = roadmap_gpx_read_file(path, name, waypoints, wee, NULL, NULL);

    return ret;

}

int
roadmap_gpx_read_one_track(
    const char *path, const char *name, route_head **track)
{
    int ret;
    queue_head tracklist;
    route_head *trk;

    QUEUE_INIT(&tracklist);

    ret = roadmap_gpx_read_file(path, name, NULL, 0, NULL, &tracklist);

    if (!ret || QUEUE_EMPTY(&tracklist)) {
        route_flush_queue(&tracklist);
        return 0;
    }

    trk = (route_head *)QUEUE_FIRST(&tracklist);
    roadmap_list_remove(&trk->Q);
    QUEUE_INIT((queue_head *)&trk->Q);  /* safety -- no dangling links */

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
    queue_head routelist;
    route_head *rte;

    QUEUE_INIT(&routelist);

    ret = roadmap_gpx_read_file(path, name, NULL, 0, &routelist, NULL);

    if (!ret || QUEUE_EMPTY(&routelist))
        return 0;

    rte = (route_head *)QUEUE_FIRST(&routelist);
    roadmap_list_remove(&rte->Q);
    QUEUE_INIT((queue_head *)&rte->Q);  /* safety -- no dangling links */

    /* In case we read more than one route from the file.  */
    route_flush_queue(&routelist);

    if (*route) route_free(*route);
    *route = rte;

    return ret;
}

int
roadmap_gpx_write_file(const char *path, const char *name,
        queue_head *w, queue_head *r, queue_head *t)
{
    FILE *fp;
    int ret;

    fp = roadmap_file_fopen (path, name, "w");
    if (!fp) return 0;

    ret = gpx_write(fp, w, r, t);

    if (fclose(fp) != 0)
        ret = 0;

    if (!ret)
	roadmap_log (ROADMAP_ERROR, "GPX file save of %s / %s failed", path, name);

    return ret;
}

/* write a file of waypoints.  weepoints cannot be written */
int roadmap_gpx_write_waypoints(const char *path, const char *name,
        queue_head *waypoints)
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
    queue_head route_head;

    QUEUE_INIT(&route_head);
    ENQUEUE_TAIL(&route_head, &route->Q);

    fp = roadmap_file_fopen (path, name, "w");
    if (!fp) return 0;

    ret = gpx_write(fp, NULL, &route_head, NULL);
    if (ferror(fp))
	ret = 0;

    if (fclose(fp) != 0)
        ret = 0;

    return ret;
}

int roadmap_gpx_write_track(const char *path, const char *name,
        route_head *track)
{
    FILE *fp;
    int ret;
    queue_head track_head;

    QUEUE_INIT(&track_head);
    ENQUEUE_TAIL(&track_head, &track->Q);

    fp = roadmap_file_fopen (path, name, "w");
    if (!fp) return 0;

    ret = gpx_write(fp, NULL, NULL, &track_head);

    if (fclose(fp) != 0)
        ret = 0;

    return ret;
}

#else // ROADMAP_USES_EXPAT


static void roadmap_gpx_tell_no_expat (void) {

    static int roadmap_gpx_told_no_expat = 0;

    if (! roadmap_gpx_told_no_expat) {
       roadmap_log (ROADMAP_ERROR, "No GPX file import/export (no expat library)");
       roadmap_gpx_told_no_expat = 1;
    }
}

int
roadmap_gpx_read_file(const char *path,
        const char *name, queue *w, int wee, queue *r, queue *t)
{
    roadmap_gpx_tell_no_expat ();
    return 0;
}

int
roadmap_gpx_read_waypoints
        (const char *path, const char *name, queue *waypoints)
{
    roadmap_gpx_tell_no_expat ();
    return 0;

}

int
roadmap_gpx_read_one_track(
    const char *path, const char *name, route_head **track)
{
    roadmap_gpx_tell_no_expat ();
    return 0;

}

int
roadmap_gpx_read_one_route(
    const char *path, const char *name, route_head **route)
{
    roadmap_gpx_tell_no_expat ();
    return 0;
}

int
roadmap_gpx_write_file(const char *path, const char *name,
        queue *w, queue *r, queue *t)
{
    roadmap_gpx_tell_no_expat ();
    return 0;
}

int roadmap_gpx_write_waypoints(const char *path, const char *name,
        queue *waypoints)
{
    roadmap_gpx_tell_no_expat ();
    return 0;
}

int roadmap_gpx_write_route(const char *path, const char *name,
        route_head *route)
{
    roadmap_gpx_tell_no_expat ();
    return 0;
}

int roadmap_gpx_write_track(const char *path, const char *name,
        route_head *track)
{
    roadmap_gpx_tell_no_expat ();
    return 0;
}

#endif // ROADMAP_USES_EXPAT

