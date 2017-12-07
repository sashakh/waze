/* roadmap_libgps.c - a module to interact with gpsd using its library.
 *
 * LICENSE:
 *
 *   Copyright 2017 Sasha Khapyosky
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
 *
 * DESCRIPTION:
 *
 *   This module hides the gpsd library API (version 3).
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <gps.h>

#include "roadmap.h"
#include "roadmap_libgps.h"

static RoadMapGpsdNavigation libgps_navigation;
static RoadMapGpsdSatellite libgps_satellite;
static RoadMapGpsdDilution libgps_dilution;

void roadmap_libgps_subscribe_to_navigation(RoadMapGpsdNavigation navigation)
{
	libgps_navigation = navigation;
}

void roadmap_libgps_subscribe_to_satellites(RoadMapGpsdSatellite satellite)
{
	libgps_satellite = satellite;
}

void roadmap_libgps_subscribe_to_dilution(RoadMapGpsdDilution dilution)
{
	libgps_dilution = dilution;
}

static struct gps_data_t gpsdata;

int roadmap_libgps_connect(RoadMapIO * io, const char *hostname,
			   const char *port)
{
	int ret;

	memset(&gpsdata, 0, sizeof(gpsdata));

	ret = gps_open(hostname, port, &gpsdata);
	if (ret) {
		roadmap_log(ROADMAP_ERROR, "cannot gps_open(): %s",
			    gps_errstr(ret));
		return -1;
	}

	ret = gps_stream(&gpsdata, WATCH_ENABLE, NULL);
	if (ret) {
		roadmap_log(ROADMAP_ERROR, "cannot gps_stream(): %s",
			    gps_errstr(ret));
		return -1;
	}

	gpsdata.set = 0;

	unsigned n;
	for (n = 0; n < 5; n++) {
		gpsdata.set = 0;
		gps_waiting(&gpsdata, 1000 * 1000);
		gps_read(&gpsdata);
		//printf("%s: gps_read(): %08lx: %s\n", __func__, gpsdata.set, gps_data(&gpsdata));
		if (gpsdata.set & TIME_SET)
			break;
	}

	io->os.file = gpsdata.gps_fd;
	io->subsystem = ROADMAP_IO_FILE;

	return 0;
}

int roadmap_libgps_input(void *context)
{
	struct gps_data_t *gd = &gpsdata;

	gd->set = 0;
	gps_read(gd);

	//printf("%s: gps_read(): %08lx: %s\n", __func__, gd->set, gps_data(gd));

	if (!(gd->set & TIME_SET))
		return 0;

	//int status;
	int latitude = ROADMAP_NO_VALID_DATA;
	int longitude = ROADMAP_NO_VALID_DATA;
	int altitude = ROADMAP_NO_VALID_DATA;
	int speed = ROADMAP_NO_VALID_DATA;
	int steering = ROADMAP_NO_VALID_DATA;

	if (gd->set & LATLON_SET) {
		latitude = gd->fix.latitude * 1000000;
		longitude = gd->fix.longitude * 1000000;
	}
	if (gd->set & ALTITUDE_SET)
		altitude = gd->fix.altitude;
	if (gd->set & SPEED_SET)
		speed = gd->fix.speed * 3600 / 1000;	/* m/s -> km/h */
	if (gd->set & TRACK_SET)
		steering = gd->fix.track;

	libgps_navigation('A', gd->fix.time, latitude, longitude, altitude,
			  speed, steering, ROADMAP_NO_VALID_DATA);

	return 0;
}
