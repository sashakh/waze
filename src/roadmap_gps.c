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
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_config.h"

#include "roadmap_net.h"
#include "roadmap_nmea.h"

#include "roadmap_gps.h"


static RoadMapConfigDescriptor RoadMapConfigGPSAccuracy =
                        ROADMAP_CONFIG_ITEM("Accuracy", "GPS Position");

static RoadMapConfigDescriptor RoadMapConfigGPSSpeedAccuracy =
                        ROADMAP_CONFIG_ITEM("Accuracy", "GPS Speed");

static RoadMapConfigDescriptor RoadMapConfigGPSSource =
                        ROADMAP_CONFIG_ITEM("GPS", "Source");

static RoadMapConfigDescriptor RoadMapConfigGPSTimeout =
                        ROADMAP_CONFIG_ITEM("GPS", "Timeout");


static int RoadMapGpsLink = -1;

static time_t RoadMapGpsConnectedSince = -1;

#define ROADMAP_GPS_CLIENTS 16
static roadmap_gps_listener RoadMapGpsListeners[ROADMAP_GPS_CLIENTS] = {NULL};
static roadmap_gps_logger   RoadMapGpsLoggers[ROADMAP_GPS_CLIENTS] = {NULL};

static RoadMapNmeaListener RoadMapGpsNextGprmc = NULL;
static RoadMapNmeaListener RoadMapGpsNextPgrme = NULL;
static RoadMapNmeaListener RoadMapGpsNextPgrmm = NULL;


static char   RoadMapLastKnownStatus = 'A';
static time_t RoadMapGpsLatestData = 0;
static int    RoadMapGpsEstimatedError = 0;
static int    RoadMapGpsRetryPending = 0;


static void roadmap_gps_no_link_control (int fd) {}
static void roadmap_gps_no_periodic_control (RoadMapCallback callback) {}


static roadmap_gps_periodic_control RoadMapGpsPeriodicAdd =
                                    &roadmap_gps_no_periodic_control;

static roadmap_gps_periodic_control RoadMapGpsPeriodicRemove =
                                    &roadmap_gps_no_periodic_control;

static roadmap_gps_link_control RoadMapGpsLinkAdd =
                                    &roadmap_gps_no_link_control;

static roadmap_gps_link_control RoadMapGpsLinkRemove =
                                    &roadmap_gps_no_link_control;


static void roadmap_gps_pgrmm (void *context, const RoadMapNmeaFields *fields) {

    if ((strcasecmp (fields->pgrmm.datum, "NAD83") != 0) &&
        (strcasecmp (fields->pgrmm.datum, "WGS 84") != 0)) {
        roadmap_log (ROADMAP_FATAL,
                     "bad datum '%s': 'NAD83' or 'WGS 84' is required",
                     fields->pgrmm.datum);
    }

    (*RoadMapGpsNextPgrmm) (context, fields);
}


static void roadmap_gps_pgrme (void *context, const RoadMapNmeaFields *fields) {

    RoadMapGpsEstimatedError =
        roadmap_math_to_current_unit (fields->pgrme.horizontal,
                                      fields->pgrme.horizontal_unit);

    (*RoadMapGpsNextPgrme) (context, fields);
}


static void roadmap_gps_gprmc (void *context, const RoadMapNmeaFields *fields) {

   int i;

   if (fields->gprmc.status != RoadMapLastKnownStatus) {
       if (RoadMapLastKnownStatus == 'A') {
          roadmap_log (ROADMAP_ERROR,
                       "GPS receiver lost satellite fix (status: %c)",
                       fields->gprmc.status);
       }
   }
   RoadMapLastKnownStatus = fields->gprmc.status;

   if (fields->gprmc.status == 'A') {

      static RoadMapGpsPosition position;

      position.latitude  = fields->gprmc.latitude;
      position.longitude = fields->gprmc.longitude;
      position.speed     = fields->gprmc.speed;
      position.altitude  = 0; /* Not implemented yet (in gpgga). */

      if (position.speed > roadmap_gps_speed_accuracy()) {

         /* Update the steering only if the speed is significant:
          * when the speed is too low, the steering indicated by
          * the GPS device is not reliable; in that case the best
          * guess is that we did not turn.
          */
         position.steering  = fields->gprmc.steering;
      }

      for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {

         if (RoadMapGpsListeners[i] == NULL) break;

         (RoadMapGpsListeners[i]) (&position);
      }
   }

   (*RoadMapGpsNextGprmc) (context, fields);
}


void roadmap_gps_initialize (roadmap_gps_listener listener) {

   int i;


   for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {
      if (RoadMapGpsListeners[i] == NULL) {
         RoadMapGpsListeners[i] = listener;
         break;
      }
   }

   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSSpeedAccuracy, "4");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSAccuracy, "30");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSSource, "gpsd://localhost");
   roadmap_config_declare
       ("preferences", &RoadMapConfigGPSTimeout, "10");
}


void roadmap_gps_open (void) {

   const char *url;


   /* Check if we have a gps interface defined: */

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

   if (strncasecmp (url, "gpsd://", 7) == 0) {

      RoadMapGpsLink = roadmap_net_connect (url+7, 2947);

      if (RoadMapGpsLink > 0) {

         if (write (RoadMapGpsLink, "r\n", 2) != 2) {
            roadmap_log (ROADMAP_WARNING,
                         "cannot subscribe to gpsd, errno = %d", errno);
            close(RoadMapGpsLink);
            RoadMapGpsLink = -1;
         }
      }

   } else if (strncasecmp (url, "tty://", 6) == 0) {

      roadmap_log (ROADMAP_ERROR, "tty GPS source not yet implemented");
      return;

   } else if (strncasecmp (url, "file://", 7) == 0) {

      RoadMapGpsLink = open (url+7, O_RDONLY);

   } else if (url[0] == '/') {

      RoadMapGpsLink = open (url, O_RDONLY);

   } else {
      roadmap_log (ROADMAP_ERROR, "invalid protocol in url %s", url);
      return;
   }

   if (RoadMapGpsLink < 0) {
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

   RoadMapGpsConnectedSince = time(NULL);

   /* Declare this IO to the GUI toolkit so that we wake up on GPS data. */

   (*RoadMapGpsLinkAdd) (RoadMapGpsLink);

   if (RoadMapGpsNextGprmc == NULL) {

      RoadMapGpsNextGprmc =
         roadmap_nmea_subscribe ("GPRMC", roadmap_gps_gprmc);
   }

   if (RoadMapGpsNextPgrme == NULL) {

      RoadMapGpsNextPgrme =
         roadmap_nmea_subscribe ("PGRME", roadmap_gps_pgrme);
   }

   if (RoadMapGpsNextPgrmm == NULL) {

      RoadMapGpsNextPgrmm =
         roadmap_nmea_subscribe ("PGRMM", roadmap_gps_pgrmm);
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


void roadmap_gps_input (int fd) {

   static char input_buffer[1024];
   static int  input_cursor = 0;

   int received;


   if (input_cursor < (int)sizeof(input_buffer) - 1) {

      received =
         roadmap_net_receive
            (fd, input_buffer + input_cursor,
                 sizeof(input_buffer) - input_cursor - 1);

      if (received <= 0) {

         if (errno != 0) {
            roadmap_log (ROADMAP_ERROR,
                         "lost GPS connection after %d seconds, errno = %d",
                         time(NULL) - RoadMapGpsConnectedSince, errno);
         } else {
            roadmap_log (ROADMAP_INFO,
                         "lost GPS connection after %d seconds",
                         time(NULL) - RoadMapGpsConnectedSince);
         }

         /* We lost that connection. */

         (*RoadMapGpsLinkRemove) (fd);
         close (fd);

         /* Try to establish a new connection: */

         roadmap_gps_open();

      } else {
         input_cursor += received;
      }
   }

   while (input_cursor > 0) {

      int i;
      int next;
      int line_is_complete;

      char *new_line;
      char *carriage_return;


      input_buffer[input_cursor] = 0;

      line_is_complete = 0;
      new_line = strchr (input_buffer, '\n');
      carriage_return = strchr (input_buffer, '\r');

      if (new_line != NULL) {
         *new_line = 0;
         line_is_complete = 1;
      }
      if (carriage_return != NULL) {
         *carriage_return = 0;
         line_is_complete = 1;
      }

      if (! line_is_complete) break;

      next = strlen (input_buffer) + 1;


      for (i = 0; i < ROADMAP_GPS_CLIENTS; ++i) {

         if (RoadMapGpsLoggers[i] == NULL) break;

         (RoadMapGpsLoggers[i]) (input_buffer);
      }

      if (roadmap_nmea_decode (NULL, input_buffer)) {
         RoadMapGpsLatestData = time(NULL);
      }

      while ((input_buffer[next] < ' ') && (next < input_cursor)) next++;

      input_cursor -= next;

      for (i = 0; i < input_cursor; ++i) {
         input_buffer[i] = input_buffer[next+i];
      }
   }
}


int roadmap_gps_active (void) {

   int timeout;

   if (RoadMapGpsLink < 0) {
      return 0;
   }

   timeout = roadmap_config_get_integer (&RoadMapConfigGPSTimeout);

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

