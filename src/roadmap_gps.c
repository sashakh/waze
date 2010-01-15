/* roadmap_gps.c - GPS interface for the RoadMap application.
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
 *   See roadmap_gps.h
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_string.h"
#include "roadmap_object.h"
#include "roadmap_config.h"

#include "roadmap_net.h"
#include "roadmap_file.h"
#include "roadmap_serial.h"
#include "roadmap_state.h"
#include "roadmap_nmea.h"
#include "roadmap_gpsd2.h"
#include "roadmap_message.h"

#include "roadmap_dialog.h"
#include "roadmap_main.h"
#include "roadmap_messagebox.h"

#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>

#include "roadmap_gps.h"

static LocationGPSDControl *control;
static RoadMapConfigDescriptor RoadMapConfigGPSAccuracy =
                        ROADMAP_CONFIG_ITEM("Accuracy", "GPS Position");

static RoadMapConfigDescriptor RoadMapConfigGPSSpeedAccuracy =
                        ROADMAP_CONFIG_ITEM("Accuracy", "GPS Speed");

static RoadMapConfigDescriptor RoadMapConfigGPSSource =
                        ROADMAP_CONFIG_ITEM("GPS", "Source");

static RoadMapConfigDescriptor RoadMapConfigGPSTimeout =
                        ROADMAP_CONFIG_ITEM("GPS", "Timeout");


static char RoadMapGpsTitle[] = "GPS receiver";

static RoadMapIO RoadMapGpsLink;

static time_t RoadMapGpsConnectedSince = -1;

#define ROADMAP_GPS_CLIENTS 16
static roadmap_gps_listener RoadMapGpsListeners[ROADMAP_GPS_CLIENTS] = {NULL};
static roadmap_gps_monitor  RoadMapGpsMonitors[ROADMAP_GPS_CLIENTS] = {NULL};
static roadmap_gps_logger   RoadMapGpsLoggers[ROADMAP_GPS_CLIENTS] = {NULL};

static int RoadMapGpsProtocol = 0;


/* Listeners information (navigation data) ----------------------------- */

static char   RoadMapLastKnownStatus = 'A';
static time_t RoadMapGpsLatestData = 0;
static int    RoadMapGpsEstimatedError = 0;
static int    RoadMapGpsRetryPending = 0;
static time_t RoadMapGpsReceivedTime = 0;
static int    RoadMapGpsReception = 0;

static RoadMapGpsPosition RoadMapGpsReceivedPosition;


/* Monitors information (GPS system status) ---------------------------- */

static int RoadMapGpsActiveSatelliteHash;
static int RoadMapGpsSatelliteCount;
static int RoadMapGpsActiveSatelliteCount;

static char RoadMapGpsActiveSatellite[ROADMAP_NMEA_MAX_SATELLITE];
static RoadMapGpsSatellite RoadMapGpsDetected[ROADMAP_NMEA_MAX_SATELLITE];

static RoadMapGpsPrecision RoadMapGpsQuality;


static void roadmap_gps_no_link_control (RoadMapIO *io) {}
static void roadmap_gps_no_periodic_control (RoadMapCallback callback) {}


static roadmap_gps_periodic_control RoadMapGpsPeriodicAdd =
                                    &roadmap_gps_no_periodic_control;

static roadmap_gps_periodic_control RoadMapGpsPeriodicRemove =
                                    &roadmap_gps_no_periodic_control;

static roadmap_gps_link_control RoadMapGpsLinkAdd =
                                    &roadmap_gps_no_link_control;

static roadmap_gps_link_control RoadMapGpsLinkRemove =
                                    &roadmap_gps_no_link_control;


/* Basic support functions -------------------------------------------- */

static int roadmap_gps_reception_state (void) {

   return RoadMapGpsReception;
}


static void roadmap_gps_update_reception (void) {

   int new_state;

   if (!roadmap_gps_active ()) {
      new_state = GPS_RECEPTION_NA;

   } else if (RoadMapLastKnownStatus != 'A') {
      new_state = GPS_RECEPTION_NONE;

   } else if ((RoadMapGpsActiveSatelliteCount <= 3) ||
         (RoadMapGpsQuality.dilution_horizontal > 2.3)) {

      new_state = GPS_RECEPTION_POOR;
   } else {
      new_state = GPS_RECEPTION_GOOD;
   }

   if (RoadMapGpsReception != new_state) {

      int old_state = RoadMapGpsReception;
      RoadMapGpsReception = new_state;

      if ((old_state <= GPS_RECEPTION_NONE) ||
            (new_state <= GPS_RECEPTION_NONE)) {

         roadmap_state_refresh ();
      }
   }
}


static void roadmap_gps_update_status (char status) {

   if (status != RoadMapLastKnownStatus) {
       if (RoadMapLastKnownStatus == 'A') {
          roadmap_log (ROADMAP_ERROR,
                       "GPS receiver lost satellite fix (status: %c)", status);
       }
       RoadMapLastKnownStatus = status;
   }
}


static void roadmap_gps_process_position (void) {

   int i;

   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {

      if (RoadMapGpsListeners[i] == NULL) break;

      (RoadMapGpsListeners[i])
           (RoadMapGpsReceivedTime,
            &RoadMapGpsQuality,
            &RoadMapGpsReceivedPosition);
   }

   roadmap_gps_update_reception ();
}


static void roadmap_gps_call_monitors (void) {

   int i;

   roadmap_message_set ('c', "%d", RoadMapGpsActiveSatelliteCount);

   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {

      if (RoadMapGpsMonitors[i] == NULL) break;

      (RoadMapGpsMonitors[i])
         (&RoadMapGpsQuality, RoadMapGpsDetected, RoadMapGpsSatelliteCount);
   }

   roadmap_gps_update_reception ();
}


static void roadmap_gps_call_loggers (const char *data) {

   int i;

   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {
      if (RoadMapGpsLoggers[i] == NULL) break;
      (RoadMapGpsLoggers[i]) (data);
   }
}


static void roadmap_gps_keep_alive (void) {

   if (roadmap_gps_active ()) return;

   roadmap_log (ROADMAP_ERROR, "GPS timeout detected.");

   roadmap_gps_shutdown ();

   /* Try to establish a new IO channel: */
   roadmap_gps_open();
}

static void on_changed(LocationGPSDevice *device, gpointer data)
{
        if (!device)
                return;

        if (device->fix) {
				int i;
				RoadMapGpsReceivedPosition.longitude=device->fix->longitude*1000000;
				RoadMapGpsReceivedPosition.latitude=device->fix->latitude*1000000;
				RoadMapGpsReceivedPosition.altitude=device->fix->altitude;
				RoadMapGpsReceivedPosition.speed=device->fix->speed;
				RoadMapGpsReceivedPosition.steering=device->fix->track;
				RoadMapGpsQuality.dimension = device->fix->mode;
				RoadMapGpsQuality.dilution_position = 2; //device->fix->eph;
				RoadMapGpsQuality.dilution_horizontal = 2; //device->fix->eph;
				RoadMapGpsQuality.dilution_vertical = 2; //device->fix->epv;
				RoadMapGpsReceivedTime = time(NULL);
#if 1
				RoadMapGpsSatelliteCount = device->satellites_in_view;
				RoadMapGpsActiveSatelliteCount = device->satellites_in_use;
				for(i=0;i<RoadMapGpsSatelliteCount;i++) {
					LocationGPSDeviceSatellite *sat =
						        g_ptr_array_index(device->satellites, i);
					RoadMapGpsDetected[i].id = sat->prn;
					RoadMapGpsDetected[i].elevation = sat->elevation;
					RoadMapGpsDetected[i].azimuth = sat->azimuth;
					RoadMapGpsDetected[i].strength = sat->signal_strength;
					RoadMapGpsDetected[i].status = sat->in_use ? 'A':'F';
				}
				roadmap_gps_call_monitors();
#endif
				roadmap_gps_process_position();
			}
}
                                              

void roadmap_gps_initialize (void) {

   static int RoadMapGpsInitialized = 0;
   if (! RoadMapGpsInitialized) {

      roadmap_state_add ("GPS_reception", &roadmap_gps_reception_state);
	  LocationGPSDevice *device;
	  GMainLoop *loop;

	  g_type_init();

	  loop = g_main_loop_new(NULL, FALSE);

	  control = location_gpsd_control_get_default();
	  device = g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);

	  g_object_set(G_OBJECT(control), "preferred-method", LOCATION_METHOD_USER_SELECTED,
				  "preferred-interval", LOCATION_INTERVAL_DEFAULT, NULL);

//	  g_signal_connect(control, "error-verbose", G_CALLBACK(on_error), loop);
	  g_signal_connect(device, "changed", G_CALLBACK(on_changed), control);
//	  g_signal_connect(control, "gpsd-stopped", G_CALLBACK(on_stop), loop);
	  location_gpsd_control_start(control);
   }
}


void roadmap_gps_shutdown (void) {
      location_gpsd_control_stop(control);
}


void roadmap_gps_register_listener (roadmap_gps_listener listener) {

   int i;

   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {
      if (RoadMapGpsListeners[i] == NULL) {
         RoadMapGpsListeners[i] = listener;
         break;
      }
   }
}


void roadmap_gps_register_monitor (roadmap_gps_monitor monitor) {

   int i;

   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {
      if (RoadMapGpsMonitors[i] == NULL) {
         RoadMapGpsMonitors[i] = monitor;
         break;
      }
   }
}


void roadmap_gps_open (void) {
}


void roadmap_gps_register_logger (roadmap_gps_logger logger) {

   int i;

   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {

      if (RoadMapGpsLoggers[i] == logger) {
         break;
      }
      if (RoadMapGpsLoggers[i] == NULL) {
         RoadMapGpsLoggers[i] = logger;
         break;
      }
   }
}


void roadmap_gps_register_link_control
        (roadmap_gps_link_control add, roadmap_gps_link_control remove) {

   RoadMapGpsLinkAdd    = add;
   RoadMapGpsLinkRemove = remove;
}


void roadmap_gps_register_periodic_control
                 (roadmap_gps_periodic_control add,
                  roadmap_gps_periodic_control remove) {

   RoadMapGpsPeriodicAdd      = add;
   RoadMapGpsPeriodicRemove   = remove;
}


void roadmap_gps_input (RoadMapIO *io) {

   static RoadMapInputContext decode;
   int res;


   res = roadmap_input (&decode);

   if (res < 0) {

      (*RoadMapGpsLinkRemove) (io);

      roadmap_io_close (io);

      /* Try to establish a new IO channel: */

      (*RoadMapGpsPeriodicRemove) (roadmap_gps_keep_alive);
      roadmap_gps_open();
   }

   RoadMapGpsLatestData = time (NULL);
}


int roadmap_gps_active (void) {

   time_t timeout;
	return 1;

   if (RoadMapGpsLink.subsystem == ROADMAP_IO_INVALID) {
      return 0;
   }

   timeout = (time_t) roadmap_config_get_integer (&RoadMapConfigGPSTimeout);

   if (time(NULL) - RoadMapGpsLatestData >= timeout) {
      return 0;
   }

   return 1;
}


int roadmap_gps_estimated_error (void) {

    if (RoadMapGpsEstimatedError == 0) {
        return roadmap_config_get_integer (&RoadMapConfigGPSAccuracy);
    }

    return RoadMapGpsEstimatedError;
}


int  roadmap_gps_speed_accuracy (void) {
    return roadmap_config_get_integer (&RoadMapConfigGPSSpeedAccuracy);
}


int  roadmap_gps_is_nmea (void) {

   return 0; /* safe bet in case of something wrong. */
}


void roadmap_gps_detect_receiver (void) {}

