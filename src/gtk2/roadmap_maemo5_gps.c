/* roadmap_maemo5_gps.c - RoadMap interface to Maemo5 location service
 *
 * LICENSE:
 *
 *   Copyright 2011 Sasha Khapyorsky <sashakh@gmail.com>
 *
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

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include <string.h>

#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>

static RoadMapGpsdNavigation gps_navigation;
static RoadMapGpsdSatellite gps_satellite;
static RoadMapGpsdDilution gps_dilution;

static LocationGPSDControl *control;
static LocationGPSDevice *device;

#define printf(fmt, ...) { printf(fmt, ##__VA_ARGS__ ); fflush(stdout); }

static void on_changed(LocationGPSDevice * device, gpointer data)
{
	guint32 mask;
	int gmt_time, latitude, longitude, altitude, speed, track;

	//printf("%s: ...\n", __func__);

	if (!device->fix)
		return;

	latitude = longitude = altitude = speed = track = ROADMAP_NO_VALID_DATA;

	mask = device->fix->fields;

	if (mask & LOCATION_GPS_DEVICE_TIME_SET)
		gmt_time = (int)device->fix->time;
	else {
		gps_navigation('N', time(NULL), latitude, longitude, altitude,
			       speed, track);
		return;
	}

	if (mask & LOCATION_GPS_DEVICE_LATLONG_SET) {
		latitude = (int)(device->fix->latitude * 1000000);
		longitude = (int)(device->fix->longitude * 1000000);
	}
	if (mask & LOCATION_GPS_DEVICE_ALTITUDE_SET)
		altitude = (int)device->fix->altitude;
	if (mask & LOCATION_GPS_DEVICE_SPEED_SET)
		speed = (int)device->fix->speed;
	if (mask & LOCATION_GPS_DEVICE_TRACK_SET)
		track = (int)device->fix->track;

	printf("%s(%d) mode %d, %x: time %f, lat %d, lon %d, alt %d, speed %d, track %d; eph %f, epv %f\n", __func__,
			time(NULL), device->fix->mode, mask, device->fix->time,
			latitude, longitude, altitude, speed, track,
			device->fix->eph, device->fix->epv);

	gps_navigation('A', time(NULL),
		       latitude, longitude, altitude, speed, track);

#if 0
	gps_dulition(device->fix->mode, );

	int i;
static void roadmap_gps_dilution (int dimension,
                                  double position,
                                  double horizontal,
                                  double vertical) {

   RoadMapGpsQuality.dimension = dimension;
   RoadMapGpsQuality.dilution_position   = position;
   RoadMapGpsQuality.dilution_horizontal = horizontal;
   RoadMapGpsQuality.dilution_vertical   = vertical;

   roadmap_message_set ('p', "%.2f", RoadMapGpsQuality.dilution_position);
   roadmap_message_set ('h', "%.2f", RoadMapGpsQuality.dilution_horizontal);
   roadmap_message_set ('v', "%.2f", RoadMapGpsQuality.dilution_vertical);
}

	RoadMapGpsReceivedPosition.longitude = device->fix->longitude * 1000000;
	RoadMapGpsReceivedPosition.latitude = device->fix->latitude * 1000000;
	RoadMapGpsReceivedPosition.altitude = device->fix->altitude;
	RoadMapGpsReceivedPosition.speed = device->fix->speed;
	RoadMapGpsReceivedPosition.steering = device->fix->track;

	RoadMapGpsQuality.dimension = device->fix->mode;
	RoadMapGpsQuality.dilution_position = 2;	//device->fix->eph;
	RoadMapGpsQuality.dilution_horizontal = 2;	//device->fix->eph;
	RoadMapGpsQuality.dilution_vertical = 2;	//device->fix->epv;

	RoadMapGpsReceivedTime = time(NULL);

	RoadMapGpsSatelliteCount = device->satellites_in_view;
	RoadMapGpsActiveSatelliteCount = device->satellites_in_use;
	for (i = 0; i < RoadMapGpsSatelliteCount; i++) {
		LocationGPSDeviceSatellite *sat =
		    g_ptr_array_index(device->satellites, i);
		RoadMapGpsDetected[i].id = sat->prn;
		RoadMapGpsDetected[i].elevation = sat->elevation;
		RoadMapGpsDetected[i].azimuth = sat->azimuth;
		RoadMapGpsDetected[i].strength = sat->signal_strength;
		RoadMapGpsDetected[i].status = sat->in_use ? 'A' : 'F';
	}
	roadmap_gps_call_monitors();
	roadmap_gps_process_position();
#endif
}

static int maemo5_location_start()
{
	GMainLoop *loop;

	g_type_init();

	loop = g_main_loop_new(NULL, FALSE);

	control = location_gpsd_control_get_default();
	device = g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);

	g_object_set(G_OBJECT(control), "preferred-method",
		     LOCATION_METHOD_USER_SELECTED, "preferred-interval",
		     LOCATION_INTERVAL_DEFAULT, NULL);

//        g_signal_connect(control, "error-verbose", G_CALLBACK(on_error), loop);
	g_signal_connect(device, "changed", G_CALLBACK(on_changed), control);
//        g_signal_connect(control, "gpsd-stopped", G_CALLBACK(on_stop), loop);
	location_gpsd_control_start(control);

	/* location_gpsd_control_stop(control); */
}

void roadmap_maemo5_gps_subscribe_to_navigation(RoadMapGpsdNavigation navigation)
{
	printf("%s: ...\n", __func__);
	gps_navigation = navigation;
	maemo5_location_start();
}

void roadmap_maemo5_gps_subscribe_to_satellites(RoadMapGpsdSatellite satellite)
{
	printf("%s: ...\n", __func__);
	gps_satellite = satellite;
}

void roadmap_maemo5_gps_subscribe_to_dilution(RoadMapGpsdDilution dilution)
{
	printf("%s: ...\n", __func__);
	gps_dilution = dilution;
}

#if 0
void roadmap_gpsandroid_set_position(AndroidPositionMode mode, char status,
				     int gps_time, int latitude, int longitude,
				     int altitude, int speed, int steering)
{
	if (mode == _andr_pos_gps) {
		if (GpsdNavigationCallback) {
			(*GpsdNavigationCallback) (status, gps_time, latitude,
						   longitude, altitude, speed,
						   steering);
		} else {
			roadmap_log(ROADMAP_WARNING,
				    "The navigation callback is not initialized");
		}

	}
	if (mode == _andr_pos_cell) {
		roadmap_gps_coarse_fix(latitude, longitude);
	}
}
#endif
