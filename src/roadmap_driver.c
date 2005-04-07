/* roadmap_driver.c - a driver manager for the RoadMap application.
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
 *   See roadmap_driver.h
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
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

#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_spawn.h"

#include "roadmap_nmea.h"
#include "roadmap_gps.h"
#include "roadmap_object.h"

#include "roadmap_driver.h"

/* What the driver may subscribe to (a bit mask): */
#define ROADMAP_DRIVER_RMC    1  /* Subscribed to GPRMC information. */

struct roadmap_driver_descriptor {

   char *name;

   RoadMapFeedback process;
   int             pipes[2];

   int             subscription;

   struct roadmap_driver_descriptor *next;
};

static struct roadmap_driver_descriptor *RoadMapDriverList = NULL;

static int RoadMapDriverSubscription = 0; /* OR of all drivers subscriptions. */


static void roadmap_driver_no_link_control (int fd) {}

static roadmap_gps_link_control RoadMapDriverLinkAdd =
                                    &roadmap_driver_no_link_control;

static roadmap_gps_link_control RoadMapDriverLinkRemove =
                                    &roadmap_driver_no_link_control;

static RoadMapNmeaListener RoadMapDriverNextPxrmadd = NULL;
static RoadMapNmeaListener RoadMapDriverNextPxrmmov = NULL;
static RoadMapNmeaListener RoadMapDriverNextPxrmdel = NULL;
static RoadMapNmeaListener RoadMapDriverNextPxrmsub = NULL;


static void roadmap_driver_pxrmadd (void *context,
                                    const RoadMapNmeaFields *fields) {

   roadmap_object_add (fields->pxrmadd.id,
                       fields->pxrmadd.name,
                       fields->pxrmadd.sprite);

   (*RoadMapDriverNextPxrmadd) (context, fields);
}


static void roadmap_driver_pxrmmov (void *context,
                                    const RoadMapNmeaFields *fields) {

   RoadMapGpsPosition position;

   position.latitude = fields->pxrmmov.latitude;
   position.longitude = fields->pxrmmov.longitude;

   position.speed = fields->pxrmmov.speed;

   if (position.speed > roadmap_gps_speed_accuracy()) {

      /* Update the steering only if the speed is significant:
       * when the speed is too low, the steering indicated by
       * the GPS device is not reliable; in that case the best
       * guess is that we did not turn.
       */
      position.steering  = fields->rmc.steering;
   }
   roadmap_object_move (fields->pxrmadd.id, &position);

   (*RoadMapDriverNextPxrmmov) (context, fields);
}


static void roadmap_driver_pxrmdel (void *context,
                                    const RoadMapNmeaFields *fields) {

   roadmap_object_remove (fields->pxrmdel.id);

   (*RoadMapDriverNextPxrmdel) (context, fields);
}


static void roadmap_driver_pxrmsub (void *context,
                                    const RoadMapNmeaFields *fields) {

   int i;
   struct roadmap_driver_descriptor *driver;

   for (driver = RoadMapDriverList; driver != NULL; driver = driver->next) {
      if (strcasecmp(driver->name, fields->pxrmsub.name) == 0) break;
   }

   if (driver != NULL) {
      for (i = 0; i < fields->pxrmsub.count; ++i) {

         if (strcasecmp(fields->pxrmsub.subscribed[i].item, "RMC") == 0) {

            driver->subscription |= ROADMAP_DRIVER_RMC;

         } else {

            roadmap_log (ROADMAP_ERROR,
                         "invalid subscription %s by driver %s",
                         fields->pxrmsub.subscribed[i].item,
                         driver->name);
         }
      }
   }

   (*RoadMapDriverNextPxrmdel) (context, fields);
}


static void roadmap_driver_to_nmea (int value, int *ddmm, int *mmmm) {

   int degrees;
   int minutes;

   degrees = value / 1000000;
   value = 60 * (value % 1000000);
   minutes = value / 1000000;

   *ddmm = degrees * 10 + minutes;
   *mmmm = (value % 1000000) / 100;
}

void roadmap_driver_publish (const RoadMapGpsPosition *position) {

   /* Format the GPRMC sentence that is used to publish our position. */

   int latitude_ddmm;
   int latitude_mmmm;
   int longitude_ddmm;
   int longitude_mmmm;

   time_t now;
   struct tm *gmt;

   int length;
   char buffer[128];  /* large enough and more. */

   struct roadmap_driver_descriptor *driver;


   if (! (RoadMapDriverSubscription & ROADMAP_DRIVER_RMC)) return;


   roadmap_driver_to_nmea (position->latitude,
                           &latitude_ddmm, &latitude_mmmm);

   roadmap_driver_to_nmea (position->longitude,
                           &longitude_ddmm, &longitude_mmmm);

   time(&now);
   gmt = gmtime (&now);
   if (gmt == NULL) return;

   sprintf (buffer, "$GPRMC,"
                    "%02d%02d%02d.000,A," /* Time (hhmmss) and status. */
                    "%d.%04d,%c,"         /* Latitude. */
                    "%d.%04d,%c,"         /* Longitude. */
                    "%d,"                 /* Speed (knots). */
                    "%d,"                 /* Course over ground. */
                    "%02d%02d%02d,"       /* UTC date (DDMMYY). */
                    "0,E\n",              /* Magnetic variation (none). */
            gmt->tm_hour, gmt->tm_min, gmt->tm_sec,
            latitude_ddmm, latitude_mmmm, position->latitude > 0 ? 'N' : 'S',
            longitude_ddmm, longitude_mmmm, position->longitude > 0 ? 'E' : 'W',
            position->speed,
            position->steering,
            gmt->tm_mday, gmt->tm_mon+1, gmt->tm_year%100);

   /* Send that data to all drivers. */

   length = strlen(buffer);

   for (driver = RoadMapDriverList; driver != NULL; driver = driver->next) {
      if (driver->subscription & ROADMAP_DRIVER_RMC) {
         write (driver->pipes[1], buffer, length);
      }
   }
}


void roadmap_driver_initialize (void) {

   char *p;
   char *name;
   char *command;
   FILE *file;
   char  buffer[1024];

   struct roadmap_driver_descriptor *driver;


   file = roadmap_file_open (roadmap_path_user(), "drivers", "sr");

   if (file != NULL) {

      while (! feof(file)) {

         fgets (buffer, sizeof(buffer), file);

         if (feof(file) || ferror(file)) break;

         buffer[sizeof(buffer)-1] = 0;

         /* remove the end-of-line character. */
         p = strchr (buffer, '\n');
         if (p != NULL) *p = 0;


         /* Retrieve the name part of the driver definition line:
          * skip any leading space.
          */
         for (name = buffer; isspace(*name); ++name) ;

         if ((*name == 0) || (*name == '#')) continue; /* Empty line. */


         /* retrieve the command (i.e. value) part of the driver
          * definition line: search for the ':' separator and skip
          * all leading space characters.
          */
         command = strchr(name, ':');
         if (command == NULL) {
            roadmap_log (ROADMAP_ERROR, "%s: invalid syntax", buffer);
            continue;
         }
         *command = 0; /* Separate the name from the value. */
         for (++command; isspace(command); ++command) ;

         driver = calloc (1, sizeof(struct roadmap_driver_descriptor));
         if (driver == NULL) break;

         if (roadmap_spawn_with_pipe
               (command, p, driver->pipes, &driver->process) > 0) {

            /* Declare this I/O to the GUI so that we can wake up
             * when the driver talks to us.
             */
            (*RoadMapDriverLinkAdd) (driver->pipes[0]);

            // TBD: any initialization message needed??

            driver->name = strdup(name);
            driver->subscription = 0;

         } else {
            free (driver);
         }
      }

      fclose(file);
   }

   RoadMapDriverNextPxrmadd =
      roadmap_nmea_subscribe ("XRM", "ADD", roadmap_driver_pxrmadd);

   RoadMapDriverNextPxrmmov =
      roadmap_nmea_subscribe ("XRM", "MOV", roadmap_driver_pxrmmov);

   RoadMapDriverNextPxrmdel =
      roadmap_nmea_subscribe ("XRM", "DEL", roadmap_driver_pxrmdel);

   RoadMapDriverNextPxrmsub =
      roadmap_nmea_subscribe ("XRM", "SUB", roadmap_driver_pxrmsub);
}


void roadmap_driver_register_link_control
        (roadmap_gps_link_control add, roadmap_gps_link_control remove) {

   RoadMapDriverLinkAdd    = add;
   RoadMapDriverLinkRemove = remove;
}

