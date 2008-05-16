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
#include "roadmap_messagebox.h"

#include "roadmap_net.h"
#include "roadmap_file.h"
#include "roadmap_serial.h"
#include "roadmap_state.h"
#include "roadmap_nmea.h"
#include "roadmap_gpsd2.h"
#include "roadmap_vii.h"
#include "roadmap_driver.h"

#include "roadmap_gps.h"


static RoadMapConfigDescriptor RoadMapConfigGPSAccuracy =
                        ROADMAP_CONFIG_ITEM("Accuracy", "GPS Position");

static RoadMapConfigDescriptor RoadMapConfigGPSSpeedAccuracy =
                        ROADMAP_CONFIG_ITEM("Accuracy", "GPS Speed");

RoadMapConfigDescriptor RoadMapConfigGPSSource =
                        ROADMAP_CONFIG_ITEM("GPS", "Source");

#ifdef _WIN32
static RoadMapConfigDescriptor RoadMapConfigGPSBaudRate =
                        ROADMAP_CONFIG_ITEM("GPS", "Baud Rate");
#endif

static RoadMapConfigDescriptor RoadMapConfigGPSTimeout =
                        ROADMAP_CONFIG_ITEM("GPS", "Timeout");

static RoadMapConfigDescriptor RoadMapConfigGPSLostFixTimeout =
                        ROADMAP_CONFIG_ITEM("GPS", "LostFixWarningTimeout");


static char RoadMapGpsTitle[] = "GPS receiver";

static RoadMapIO RoadMapGpsLink;

#define ROADMAP_GPS_CLIENTS 16
static roadmap_gps_listener RoadMapGpsListeners[ROADMAP_GPS_CLIENTS] = {NULL};
static roadmap_gps_monitor  RoadMapGpsMonitors[ROADMAP_GPS_CLIENTS] = {NULL};
static roadmap_gps_logger   RoadMapGpsLoggers[ROADMAP_GPS_CLIENTS] = {NULL};

#define ROADMAP_GPS_NONE     0
#define ROADMAP_GPS_NMEA     1
#define ROADMAP_GPS_GPSD2    2
#define ROADMAP_GPS_VII      3
#define ROADMAP_GPS_OBJECT   4
static int RoadMapGpsProtocol = ROADMAP_GPS_NONE;

static void *RoadMapGpsLostFixMessage;
static time_t RoadMapGPSLostFixTime;

/* Listeners information (navigation data) ----------------------------- */

static char   RoadMapLastKnownStatus = 'A';
static time_t RoadMapGpsLatestData = 0;
static int    RoadMapGpsEstimatedError = 0;
int    RoadMapGpsRetryPending = 0;
static time_t RoadMapGpsReceivedTime = 0;
int    RoadMapGpsReception = 0;

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

static void roadmap_gps_call_all_listeners (void);

static void roadmap_gps_got_data(void);

roadmap_gps_periodic_control RoadMapGpsPeriodicAdd =
                                    &roadmap_gps_no_periodic_control;

roadmap_gps_periodic_control RoadMapGpsPeriodicRemove =
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

   if (RoadMapGpsLink.subsystem == ROADMAP_IO_INVALID ||
            RoadMapGpsLatestData == 0) {
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

      RoadMapGpsReception = new_state;

      if (new_state <= GPS_RECEPTION_NONE) {
         /* we want listeners to get at least one notification that
          * gps has gone away.
          */
         roadmap_gps_call_all_listeners ();
      }

      roadmap_state_refresh ();
   }
}

static char roadmap_gps_update_status (char status) {

   time_t fixtimeout;

   /* Reading from a file is usually for debugging.  gpsbabel
    * doesn't recreate status correctly, so force good status here.
    */
   if (RoadMapGpsLink.subsystem == ROADMAP_IO_FILE) {
       status = 'A';
   }

   fixtimeout = (time_t) roadmap_config_get_integer
                            (&RoadMapConfigGPSLostFixTimeout);
   if (RoadMapGpsLostFixMessage) {
      if (time(NULL) - RoadMapGPSLostFixTime > fixtimeout ||
                    status != RoadMapLastKnownStatus) {
         roadmap_messagebox_hide(RoadMapGpsLostFixMessage);
         RoadMapGpsLostFixMessage = 0;
      }
      roadmap_messagebox_hide(RoadMapGpsLostFixMessage);
      RoadMapGpsLostFixMessage = 0;
   }

   if (status != RoadMapLastKnownStatus) {
       if (RoadMapLastKnownStatus == 'A') {
          if (fixtimeout != 0) { /* zero timeout implies no popup at all */
             roadmap_log (ROADMAP_WARNING,
                       "GPS receiver lost satellite fix (status: %c)", status);
             RoadMapGpsLostFixMessage = roadmap_messagebox
                        ("GPS Error", "GPS lost satellite fix");
             RoadMapGPSLostFixTime = time(NULL);
             RoadMapGpsActiveSatelliteCount = 0;
          }
       }
       RoadMapLastKnownStatus = status;
   }

   return status;
}

static void roadmap_gps_got_data(void) {

   RoadMapGpsLatestData = time(NULL);
}


static void roadmap_gps_call_all_listeners (void) {

   int i;

   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {

      if (RoadMapGpsListeners[i] == NULL) break;

      (RoadMapGpsListeners[i]) (RoadMapGpsReception,
                                RoadMapGpsReceivedTime,
                                &RoadMapGpsQuality,
                                &RoadMapGpsReceivedPosition);
   }

}


static void roadmap_gps_call_all_monitors (void) {

   int i;

   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {

      if (RoadMapGpsMonitors[i] == NULL) break;

      (RoadMapGpsMonitors[i]) (RoadMapGpsReception,
                               &RoadMapGpsQuality,
                               RoadMapGpsDetected,
                               RoadMapGpsActiveSatelliteCount,
                               RoadMapGpsSatelliteCount);
   }

}


static void roadmap_gps_call_loggers (const char *data) {

   int i;

   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {
      if (RoadMapGpsLoggers[i] == NULL) break;
      (RoadMapGpsLoggers[i]) (data);
   }
}


static void roadmap_gps_keep_alive (void) {

   time_t timeout, now;

   if (RoadMapGpsLink.subsystem != ROADMAP_IO_INVALID) {
      switch (RoadMapGpsProtocol) {
      case ROADMAP_GPS_GPSD2:
          roadmap_gpsd2_periodic();
          break;
      }
   }

   roadmap_gps_update_reception ();

   now = time(NULL);

   /* should the "lost satellite" message go away */
   if (RoadMapGpsLostFixMessage) {
      timeout = (time_t) roadmap_config_get_integer
                                (&RoadMapConfigGPSLostFixTimeout);
      if (now - RoadMapGPSLostFixTime > timeout) {
         roadmap_messagebox_hide(RoadMapGpsLostFixMessage);
         RoadMapGpsLostFixMessage = 0;
      }
   }

   /* have we lost communication with the GPS? */
   timeout = (time_t) roadmap_config_get_integer (&RoadMapConfigGPSTimeout);
   if (now - RoadMapGpsLatestData > timeout) {
      roadmap_gps_update_status ('V');
   }

}


/* NMEA protocol support ----------------------------------------------- */

static RoadMapNmeaAccount RoadMapGpsNmeaAccount;
static RoadMapNmeaAccount RoadMapGpsExtendedAccount;


static void roadmap_gps_pgrmm (void *context, const RoadMapNmeaFields *fields) {

    if ((strcasecmp (fields->pgrmm.datum, "NAD83") != 0) &&
        (strcasecmp (fields->pgrmm.datum, "WGS 84") != 0)) {
        roadmap_log (ROADMAP_FATAL,
                     "bad datum '%s': 'NAD83' or 'WGS 84' is required",
                     fields->pgrmm.datum);
    }
}


static void roadmap_gps_pgrme (void *context, const RoadMapNmeaFields *fields) {

    RoadMapGpsEstimatedError =
        roadmap_math_to_current_unit (fields->pgrme.horizontal,
                                      fields->pgrme.horizontal_unit);
}


static void roadmap_gps_gga (void *context, const RoadMapNmeaFields *fields) {

   char status;

   RoadMapGpsQuality.dilution_horizontal = fields->gga.dilution/100.0;
   RoadMapGpsActiveSatelliteCount = fields->gga.count;

   if (fields->gga.quality == ROADMAP_NMEA_QUALITY_INVALID) {

      status = roadmap_gps_update_status ('V');

   } else {

      status = roadmap_gps_update_status ('A');

      RoadMapGpsReceivedTime = fields->gga.fixtime;

      RoadMapGpsReceivedPosition.latitude  = fields->gga.latitude;
      RoadMapGpsReceivedPosition.longitude = fields->gga.longitude;
      /* speed not available: keep previous value. */
      RoadMapGpsReceivedPosition.altitude  =
         roadmap_math_to_current_unit (fields->gga.altitude,
                                       fields->gga.altitude_unit);

      roadmap_gps_call_all_listeners();
   }

   roadmap_gps_got_data();
}


static void roadmap_gps_gll (void *context, const RoadMapNmeaFields *fields) {

   char status = roadmap_gps_update_status (fields->gll.status);

   if (status == 'A') {

      RoadMapGpsReceivedPosition.latitude  = fields->gll.latitude;
      RoadMapGpsReceivedPosition.longitude = fields->gll.longitude;
      /* speed not available: keep previous value. */
      /* altitude not available: keep previous value. */
      /* steering not available: keep previous value. */

      roadmap_gps_call_all_listeners();
   }

   roadmap_gps_got_data();
}


static void roadmap_gps_rmc (void *context, const RoadMapNmeaFields *fields) {

   char status = roadmap_gps_update_status (fields->rmc.status);

   if (status == 'A') {

      RoadMapGpsReceivedTime = fields->rmc.fixtime;

      RoadMapGpsReceivedPosition.latitude  = fields->rmc.latitude;
      RoadMapGpsReceivedPosition.longitude = fields->rmc.longitude;
      RoadMapGpsReceivedPosition.speed     = fields->rmc.speed; // knots
      /* altitude not available: keep previous value. */

      if (fields->rmc.speed > roadmap_gps_speed_accuracy()) {

         /* Update the steering only if the speed is significant:
          * when the speed is too low, the steering indicated by
          * the GPS device is not reliable; in that case the best
          * guess is that we did not turn.
          */
         RoadMapGpsReceivedPosition.steering  = fields->rmc.steering;
      }

      roadmap_gps_call_all_listeners();
   }

   roadmap_gps_got_data();
}


static void roadmap_gps_gsa
               (void *context, const RoadMapNmeaFields *fields) {

   int i;

   RoadMapGpsActiveSatelliteHash = 0;

   for (i = 0; i < ROADMAP_NMEA_MAX_SATELLITE; i += 1) {

      RoadMapGpsActiveSatellite[i] = fields->gsa.satellite[i];
      RoadMapGpsActiveSatelliteHash |=
         (1 << (RoadMapGpsActiveSatellite[i] & 0x1f));
   }

   RoadMapGpsQuality.dimension = fields->gsa.dimension;
   RoadMapGpsQuality.dilution_position   = fields->gsa.dilution_position;
   RoadMapGpsQuality.dilution_horizontal = fields->gsa.dilution_horizontal;
   RoadMapGpsQuality.dilution_vertical   = fields->gsa.dilution_vertical;

   roadmap_gps_got_data();

}


static void roadmap_gps_gsv
               (void *context, const RoadMapNmeaFields *fields) {

   int i;
   int id;
   int index;

   for (i = 0, index = (fields->gsv.index - 1) * 4;
        i < 4 && index < fields->gsv.count;
        i += 1, index += 1) {

      RoadMapGpsDetected[index].id        = fields->gsv.satellite[i];
      RoadMapGpsDetected[index].elevation = fields->gsv.elevation[i];
      RoadMapGpsDetected[index].azimuth   = fields->gsv.azimuth[i];
      RoadMapGpsDetected[index].strength  = fields->gsv.strength[i];

      RoadMapGpsDetected[index].status  = 'F';
   }

   if (fields->gsv.index == fields->gsv.total) {
      int active_count = 0;

      RoadMapGpsSatelliteCount = fields->gsv.count;

      if (RoadMapGpsSatelliteCount > ROADMAP_NMEA_MAX_SATELLITE) {
         RoadMapGpsSatelliteCount = ROADMAP_NMEA_MAX_SATELLITE;
      }

      for (index = 0; index < RoadMapGpsSatelliteCount; index += 1) {

         id = RoadMapGpsDetected[index].id;

         if (RoadMapGpsActiveSatelliteHash & (1 << id)) {

            for (i = 0; i < ROADMAP_NMEA_MAX_SATELLITE; i += 1) {

               if (RoadMapGpsActiveSatellite[i] == id) {
                  RoadMapGpsDetected[index].status = 'A';
                  active_count++;
                  break;
               }
            }
         }
      }

      RoadMapGpsActiveSatelliteCount = active_count;
      roadmap_gps_call_all_monitors ();
   }

   roadmap_gps_got_data();
}


static RoadMapNmeaAccount roadmap_gps_subscribe (const char *title) {

      RoadMapNmeaAccount account = roadmap_nmea_create (title);

      roadmap_nmea_subscribe (NULL, "RMC", roadmap_gps_rmc, account);

      roadmap_nmea_subscribe (NULL, "GGA", roadmap_gps_gga, account);

      roadmap_nmea_subscribe (NULL, "GLL", roadmap_gps_gll, account);

      roadmap_nmea_subscribe ("GRM", "E", roadmap_gps_pgrme, account);

      roadmap_nmea_subscribe ("GRM", "M", roadmap_gps_pgrmm, account);

      return account;
}

static void roadmap_gps_nmea (void) {

   if (RoadMapGpsNmeaAccount == NULL) {
      RoadMapGpsNmeaAccount = roadmap_gps_subscribe (RoadMapGpsTitle);
   }

   if (RoadMapGpsExtendedAccount == NULL) {
      RoadMapGpsExtendedAccount = roadmap_gps_subscribe (RoadMapGpsTitle);
      roadmap_driver_subscribe (RoadMapGpsExtendedAccount);
   }

   if (RoadMapGpsMonitors[0] != NULL) {

      roadmap_nmea_subscribe
         (NULL, "GSA", roadmap_gps_gsa, RoadMapGpsNmeaAccount);

      roadmap_nmea_subscribe
         (NULL, "GSV", roadmap_gps_gsv, RoadMapGpsNmeaAccount);
   }
}

/* End of NMEA protocol support ---------------------------------------- */


/* GPSD (or other) protocol support ------------------------------------ */

static void roadmap_gps_navigation (char status,
                                    int gmt_time,
                                    int latitude,
                                    int longitude,
                                    int altitude,   // "preferred" units
                                    int speed,      // knots
                                    int steering) {

   status = roadmap_gps_update_status (status);

   if (status == 'A') {

      RoadMapGpsReceivedTime = gmt_time;

      if (latitude != ROADMAP_NO_VALID_DATA) {
         RoadMapGpsReceivedPosition.latitude  = latitude;
      }

      if (longitude != ROADMAP_NO_VALID_DATA) {
         RoadMapGpsReceivedPosition.longitude  = longitude;
      }

      if (altitude != ROADMAP_NO_VALID_DATA) {
         RoadMapGpsReceivedPosition.altitude  = altitude;
      }

      if (speed != ROADMAP_NO_VALID_DATA) {
         RoadMapGpsReceivedPosition.speed  =  speed;
      }

      if ((RoadMapGpsReceivedPosition.speed >=
                        roadmap_gps_speed_accuracy()) &&
                (steering != ROADMAP_NO_VALID_DATA)) {
         RoadMapGpsReceivedPosition.steering  = steering;
      }

      roadmap_gps_call_all_listeners();

   }

   roadmap_gps_got_data();
}


static void roadmap_gps_satellites  (int sequence,
                                     int id,
                                     int elevation,
                                     int azimuth,
                                     int strength,
                                     int active) {

   static int active_count;
   if (sequence == 0) {

      /* End of list: propagate the information. */

      RoadMapGpsActiveSatelliteCount = active_count;
      roadmap_gps_call_all_monitors ();

   } else {

      int index = sequence - 1;

      if (index == 0) {
         active_count = 0;
      }

      RoadMapGpsDetected[index].id        = (char)id;
      RoadMapGpsDetected[index].elevation = (char)elevation;
      RoadMapGpsDetected[index].azimuth   = (short)azimuth;
      RoadMapGpsDetected[index].strength  = (short)strength;

      if (active) {

         if (RoadMapGpsQuality.dimension < 2) {
            RoadMapGpsQuality.dimension = 2;
         }
         RoadMapGpsDetected[index].status  = 'A';
         active_count++;

      } else {
         RoadMapGpsDetected[index].status  = 'F';
      }
   }

   RoadMapGpsSatelliteCount = sequence;
}


static void roadmap_gps_dilution (int dimension,
                                  double position,
                                  double horizontal,
                                  double vertical) {

   RoadMapGpsQuality.dimension = dimension;
   RoadMapGpsQuality.dilution_position   = position;
   RoadMapGpsQuality.dilution_horizontal = horizontal;
   RoadMapGpsQuality.dilution_vertical   = vertical;
}

/* End of GPSD protocol support ---------------------------------------- */


/* OBJECTS pseudo protocol support ------------------------------------- */

static RoadMapObjectListener RoadMapGpsNextObjectListener;
static RoadMapDynamicString  RoadmapGpsObjectId;

static void roadmap_gps_object_listener (RoadMapDynamicString id,
                                         const RoadMapGpsPosition *position) {

   RoadMapGpsReceivedPosition = *position;

   (void)roadmap_gps_update_status ('A');
   roadmap_gps_call_all_listeners();

   (*RoadMapGpsNextObjectListener) (id, position);

   roadmap_gps_got_data();
}


static void roadmap_gps_object_monitor (RoadMapDynamicString id) {

   if (id == RoadmapGpsObjectId) {
      roadmap_gps_open();
   }
}

/* End of OBJECT protocol support -------------------------------------- */


void roadmap_gps_initialize (void) {

   static int RoadMapGpsInitialized = 0;

   if (! RoadMapGpsInitialized) {

      roadmap_config_declare
         ("preferences", &RoadMapConfigGPSSpeedAccuracy, "4");
      roadmap_config_declare_distance
         ("preferences", &RoadMapConfigGPSAccuracy, "30m");
#ifndef _WIN32
      roadmap_config_declare
         ("preferences", &RoadMapConfigGPSSource, "gpsd://localhost");
#else
      roadmap_config_declare
         ("preferences", &RoadMapConfigGPSSource, "COM1:");
      roadmap_config_declare
         ("preferences", &RoadMapConfigGPSBaudRate, "4800");
#endif
      roadmap_config_declare
         ("preferences", &RoadMapConfigGPSTimeout, "10");

      roadmap_config_declare
         ("preferences", &RoadMapConfigGPSLostFixTimeout, "10000");

      RoadMapGpsInitialized = 1;

      roadmap_state_add ("get_GPS_reception", &roadmap_gps_reception_state);
   }
}


void roadmap_gps_shutdown (void) {

   if (RoadMapGpsLink.subsystem == ROADMAP_IO_INVALID) return;

   (*RoadMapGpsPeriodicRemove) (roadmap_gps_keep_alive);

   (*RoadMapGpsLinkRemove) (&RoadMapGpsLink);

   roadmap_io_close (&RoadMapGpsLink);
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

   const char *url;

   /* Check if we have a gps interface defined: */

   RoadMapGpsActiveSatelliteCount = 0;
   RoadMapLastKnownStatus = 0;
   RoadMapGpsLatestData = 0;
   roadmap_gps_update_reception ();

   url = roadmap_gps_source ();

   if (url == NULL) {

      url = roadmap_config_get (&RoadMapConfigGPSSource);

      if (url == NULL) {
         return;
      }
      if (*url == 0) {
         return;
      }
   }

   /* We do have a gps interface: */

   RoadMapGpsLink.subsystem = ROADMAP_IO_INVALID;
   RoadMapGpsProtocol = ROADMAP_GPS_NMEA; /* This is the default. */

   if (strncasecmp (url, "gpsd://", 7) == 0) {

      RoadMapGpsLink.os.socket = roadmap_net_connect ("tcp", url+7, 2947);

      if (ROADMAP_NET_IS_VALID(RoadMapGpsLink.os.socket)) {

         if (roadmap_net_send (RoadMapGpsLink.os.socket, "r\n", 2) == 2) {

            RoadMapGpsLink.subsystem = ROADMAP_IO_NET;

         } else {

            roadmap_log (ROADMAP_WARNING, "cannot subscribe to gpsd");
            roadmap_net_close(RoadMapGpsLink.os.socket);
         }
      }

   } else if (strncasecmp (url, "gpsd2://", 8) == 0) {

      RoadMapGpsLink.os.socket = roadmap_gpsd2_connect (url+8);

      if (ROADMAP_NET_IS_VALID(RoadMapGpsLink.os.socket)) {

            RoadMapGpsLink.subsystem = ROADMAP_IO_NET;
            RoadMapGpsProtocol = ROADMAP_GPS_GPSD2;
      }

   } else if (strncasecmp (url, "vii://", 6) == 0) {

      RoadMapGpsLink.os.socket = roadmap_vii_connect (url+6);

      if (ROADMAP_NET_IS_VALID(RoadMapGpsLink.os.socket)) {

         static char subscribe[] = "$PGLTY,SUBSCRIBE,PXRM\n";

         if (roadmap_net_send (RoadMapGpsLink.os.socket,
                               subscribe,
                               sizeof(subscribe)-1) == sizeof(subscribe)-1) {

            roadmap_log (ROADMAP_DEBUG, "Subscribed to PXRM");

            RoadMapGpsLink.subsystem = ROADMAP_IO_NET;
            RoadMapGpsProtocol = ROADMAP_GPS_VII;

         } else {

            roadmap_log (ROADMAP_WARNING,
                         "cannot subscribe to PXRM through glty");
            roadmap_net_close(RoadMapGpsLink.os.socket);
         }
      }

#ifndef _WIN32
   } else if (strncasecmp (url, "tty://", 6) == 0) {

      /* The syntax of the url is: tty://dev/ttyXXX[:baud] */

      char *device = strdup (url + 5); /* url is a const (config data). */
      char *baud  = strchr (device, ':');

      if (baud == NULL) {
         baud = "4800"; /* Hardcoded default matches NMEA standard. */
      } else {
         *(baud++) = 0;
      }

#else
   } else if ((strncasecmp (url, "com", 3) == 0) && (url[4] == ':')) {

      char *device = strdup(url); /* I do know this is not smart.. */
      const char *baud = roadmap_config_get (&RoadMapConfigGPSBaudRate);

#endif

      RoadMapGpsLink.os.serial =
         roadmap_serial_open (device, "r", atoi(baud));

      if (ROADMAP_SERIAL_IS_VALID(RoadMapGpsLink.os.serial)) {
         RoadMapGpsLink.subsystem = ROADMAP_IO_SERIAL;
      }

      free(device);

   } else if (strncasecmp (url, "file://", 7) == 0) {

      RoadMapGpsLink.os.file = roadmap_file_open (url+7, "r");

      if (ROADMAP_FILE_IS_VALID(RoadMapGpsLink.os.file)) {
         RoadMapGpsLink.subsystem = ROADMAP_IO_FILE;
      }

#if LATER
   // someday, maybe allow for --fildes=N in roadgps, so roadmap can
   // feed results to roadgps when it's spawned.  this would be to
   // allow sharing the serial port without gpsd.
   // alternative is moving roadgps guts into roadmap -- perhaps not
   // that hard -- just take over the canvas.
   } else if (strncasecmp (url, "filedes://", 7) == 0) {

      char *end;
      int fd = strtol(url+10, &end, 10);
      if (*end) fd = -1;
      RoadMapGpsLink.os.file = (RoadMapFile)fd;

      if (ROADMAP_FILE_IS_VALID(RoadMapGpsLink.os.file)) {
         RoadMapGpsLink.subsystem = ROADMAP_IO_PIPE;
      }
#endif

   } else if (strncasecmp (url, "object:", 7) == 0) {

      if (strcmp (url+7, "GPS") == 0) {

         roadmap_log (ROADMAP_ERROR, "cannot resolve self-reference to GPS");

      } else {
         RoadMapGpsLink.subsystem = ROADMAP_IO_NULL;
         RoadMapGpsProtocol = ROADMAP_GPS_OBJECT;

         RoadmapGpsObjectId = roadmap_string_new(url+7);

         RoadMapGpsNextObjectListener =
            roadmap_object_register_listener (RoadmapGpsObjectId,
                                              roadmap_gps_object_listener);

         if (RoadMapGpsNextObjectListener == NULL) {
            RoadMapGpsLink.subsystem = ROADMAP_IO_INVALID;
            roadmap_object_register_monitor (roadmap_gps_object_monitor);
         }
      }

   } else if (url[0] == '/') {

      RoadMapGpsLink.os.file = roadmap_file_open (url, "r");

      if (ROADMAP_FILE_IS_VALID(RoadMapGpsLink.os.file)) {
         RoadMapGpsLink.subsystem = ROADMAP_IO_FILE;
      }

   } else {
      roadmap_log (ROADMAP_ERROR, "invalid protocol in url %s", url);
      return;
   }

   if (RoadMapGpsLink.subsystem == ROADMAP_IO_INVALID) {
      if (! RoadMapGpsRetryPending) {
         roadmap_log (ROADMAP_WARNING, "cannot access GPS source %s", url);
         (*RoadMapGpsPeriodicAdd) (roadmap_gps_open);
         RoadMapGpsRetryPending = 1;
      }
      return;
   }

   if (RoadMapGpsRetryPending) {
      (*RoadMapGpsPeriodicRemove) (roadmap_gps_open);
      RoadMapGpsRetryPending = 0;
   }

    /* Give time for the whole system to initialize itself.  */
   roadmap_gps_got_data();

   (*RoadMapGpsPeriodicAdd) (roadmap_gps_keep_alive);

   /* Declare this IO to the GUI toolkit so that we wake up on GPS data. */

   if (RoadMapGpsLink.subsystem != ROADMAP_IO_NULL) {
      (*RoadMapGpsLinkAdd) (&RoadMapGpsLink);
   }

   switch (RoadMapGpsProtocol) {

      case ROADMAP_GPS_NMEA:

         roadmap_gps_nmea();
         break;

      case ROADMAP_GPS_GPSD2:

         roadmap_gpsd2_subscribe_to_navigation (roadmap_gps_navigation);
         roadmap_gpsd2_subscribe_to_satellites (roadmap_gps_satellites);
         roadmap_gpsd2_subscribe_to_dilution   (roadmap_gps_dilution);
         break;

      case ROADMAP_GPS_VII:

         roadmap_vii_subscribe_to_navigation (roadmap_gps_navigation);
         break;

      case ROADMAP_GPS_OBJECT:
         break;

      default:

         roadmap_log (ROADMAP_FATAL, "internal error (unsupported protocol)");
   }
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


   if (decode.title == NULL) {

      decode.title    = RoadMapGpsTitle;
      decode.logger   = roadmap_gps_call_loggers;
   }

   decode.io = io;

   switch (RoadMapGpsProtocol) {

      case ROADMAP_GPS_NMEA:

         decode.decoder = roadmap_nmea_decode;
         decode.decoder_context = (void *)RoadMapGpsNmeaAccount;

         break;

      case ROADMAP_GPS_GPSD2:

         decode.decoder = roadmap_gpsd2_decode;
         decode.decoder_context = NULL;
         break;

      case ROADMAP_GPS_VII:

         decode.decoder = roadmap_vii_decode;
         decode.decoder_context = (void *)RoadMapGpsExtendedAccount;
         break;

      case ROADMAP_GPS_OBJECT:

         return;

      default:

         roadmap_log (ROADMAP_FATAL, "internal error (unsupported protocol)");
   }

   if (roadmap_input (&decode) < 0) {

      /* This io context is "owned" by the GUI module that manage
       * the background inputs. It will be destroyed when removed,
       * so we must keep a copy before that.
       */
      RoadMapIO context = *io;

      (*RoadMapGpsLinkRemove) (io);

      roadmap_io_close (&context);

      /* Try to establish a new IO channel, but don't reread a file: */
      (*RoadMapGpsPeriodicRemove) (roadmap_gps_keep_alive);
      if (RoadMapGpsLink.subsystem != ROADMAP_IO_FILE) {
         roadmap_gps_open();
      }
   }

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

   switch (RoadMapGpsProtocol) {

      case ROADMAP_GPS_NMEA:              return 1;
      case ROADMAP_GPS_GPSD2:             return 0;
      case ROADMAP_GPS_VII:               return 0;
      case ROADMAP_GPS_OBJECT:            return 0;
   }

   return 0; /* safe bet in case of something wrong. */
}
