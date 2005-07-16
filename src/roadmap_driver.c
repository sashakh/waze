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

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_config.h"

#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_spawn.h"

#include "roadmap_input.h"
#include "roadmap_nmea.h"
#include "roadmap_gps.h"
#include "roadmap_object.h"

#include "roadmap_driver.h"


static RoadMapIO RoadMapDriverServer;


static RoadMapConfigDescriptor RoadMapDriverTemplate =
                         ROADMAP_CONFIG_ITEM("Drivers", "xxx");

static RoadMapConfigDescriptor RoadMapDriverConfigPort =
                         ROADMAP_CONFIG_ITEM("Drivers", "Port");


/* What the driver may subscribe to (a bit mask): */
#define ROADMAP_DRIVER_RMC    1  /* Subscribed to GPRMC information. */
#define ROADMAP_DRIVER_NMEA   2  /* Subscribed to all NMEA information. */

typedef struct roadmap_driver_descriptor {

   RoadMapDynamicString name;

   char *command;
   char *arguments;

   RoadMapFeedback process;
   RoadMapIO       input;
   RoadMapIO       output;

   RoadMapConfigDescriptor enable;

   RoadMapInputContext nmea;

   int             subscription;

   struct roadmap_driver_descriptor *next;

} RoadMapDriver;

static RoadMapDriver *RoadMapDriverList = NULL;

static int RoadMapDriverSubscription = 0; /* OR of all drivers subscriptions. */


static void roadmap_driver_no_link_control (RoadMapIO *io) {}

static roadmap_gps_link_control RoadMapDriverLinkAdd =
                                    &roadmap_driver_no_link_control;

static roadmap_gps_link_control RoadMapDriverLinkRemove =
                                    &roadmap_driver_no_link_control;

static roadmap_gps_link_control RoadMapDriverServerAdd = NULL;


static RoadMapNmeaListener RoadMapDriverNextPxrmadd = NULL;
static RoadMapNmeaListener RoadMapDriverNextPxrmmov = NULL;
static RoadMapNmeaListener RoadMapDriverNextPxrmdel = NULL;
static RoadMapNmeaListener RoadMapDriverNextPxrmsub = NULL;
static RoadMapNmeaListener RoadMapDriverNextPxrmcfg = NULL;



/* Publishing functions. ------------------------------------------------ */

static void roadmap_driver_send (const char *data, int mask);

static void roadmap_driver_to_nmea (int value, int *ddmm, int *mmmm) {

   int degrees;
   int minutes;

   value = abs(value);
   degrees = value / 1000000;
   value = 60 * (value % 1000000);
   minutes = value / 1000000;

   *ddmm = degrees * 100 + minutes;
   *mmmm = (value % 1000000) / 100;
}


static void roadmap_driver_publish_position
                (int gps_time, const RoadMapGpsPosition *position, int mask) {

   /* Format the GPRMC sentence that is used to publish our position. */

   int latitude_ddmm;
   int latitude_mmmm;
   int longitude_ddmm;
   int longitude_mmmm;

   struct tm *gmt;
   time_t now = (time_t)gps_time;

   char buffer[128];  /* large enough and more. */


   roadmap_driver_to_nmea (position->latitude,
                           &latitude_ddmm, &latitude_mmmm);

   roadmap_driver_to_nmea (position->longitude,
                           &longitude_ddmm, &longitude_mmmm);

   if (now == 0) now = time(NULL); /* Fallback. */
   gmt = gmtime (&now);
   if (gmt == NULL) return;

   snprintf (buffer, sizeof(buffer),
             "$GPRMC,"
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

   /* Send that data to all concerned drivers. */
   roadmap_driver_send (buffer, mask);
}


static void roadmap_driver_publish_satellites
                (const RoadMapGpsPrecision *precision,
                 const RoadMapGpsSatellite *satellites, int count, int mask) {

   int i;
   int j;
   int active;
   int gsv_count;

   char buffer[1024];  /* large enough and more. */
   char buffer2[128];  /* large enough and more. */


   snprintf (buffer, sizeof(buffer),
             "$GPGSA,"
                     "A,"    /* Force automatic mode. */
                     "%d",   /* Dimension of fix. */
             precision->dimension);

   active = 0;

   for (i = 0; i < count; ++i) {

      if (satellites[i].status == 'A') {

         char id[4];

         snprintf (id, sizeof(id), ",%02d", satellites[i].id);
         strcat (buffer, id);

         if (++active >= 12) break;
      }
   }

   /* The other satellites are not actively used for the fix. */
   if (active < 12) {
      strcat (buffer, ",,,,,,,,,,,," + active);
   }

   if (precision->dimension >= 2) {
      snprintf (buffer2, sizeof(buffer2), ",%.1f,%.1f,%.1f\r\n",
                precision->dilution_position,
                precision->dilution_horizontal,
                precision->dilution_vertical);
      strcat (buffer, buffer2);
   } else {
      strcat (buffer, ",,,\r\n");
   }

   roadmap_driver_send (buffer, mask);


   gsv_count = count / 4;
   if (count % 4) gsv_count += 1;

   for (i = 1; i <= gsv_count; ++i) {

      snprintf (buffer, sizeof(buffer),
                "$GPGSV,"
                     "%d,"   /* Number of messages. */
                     "%d,"   /* Sequence number for this message. */
                     "%d",   /* Number of satellites in view. */
                gsv_count, i, count);

      for (j = (i-1) * 4; j < count && j < i*4; ++j) {

         snprintf (buffer2, sizeof(buffer2), ",%02d,%d,%d,%03d",
                   satellites[j].id,
                   satellites[j].elevation,
                   satellites[j].azimuth,
                   satellites[j].strength);

         strcat (buffer, buffer2);
      }

      if (j < i*4) {
         strcat (buffer, ",,,,,,,,,,,,,,,," + (j*4));
      }
      strcat (buffer, "\r\n");

      roadmap_driver_send (buffer, mask);
   }
}


static void roadmap_driver_listener
               (int gps_time, const RoadMapGpsPosition *position) {

   static RoadMapGpsPosition RoadMapDriverLastPosition;


   if (RoadMapDriverSubscription & ROADMAP_DRIVER_NMEA) {

      if (! roadmap_gps_is_nmea()) {
         roadmap_driver_publish_position
            (gps_time, position, ROADMAP_DRIVER_NMEA);
      }
   }
   
   if (RoadMapDriverSubscription & ROADMAP_DRIVER_RMC) {

      if ((RoadMapDriverLastPosition.latitude  != position->latitude) ||
          (RoadMapDriverLastPosition.longitude != position->longitude) ||
          (RoadMapDriverLastPosition.speed     != position->speed) ||
          (RoadMapDriverLastPosition.steering  != position->steering)) {

         RoadMapDriverLastPosition = *position;

         roadmap_driver_publish_position
            (gps_time, position, ROADMAP_DRIVER_RMC);
      }
   }
}


static void roadmap_driver_monitor (const RoadMapGpsPrecision *precision,
                                    const RoadMapGpsSatellite *satellites,
                                    int count) {

   if (RoadMapDriverSubscription & ROADMAP_DRIVER_NMEA) {

      if (! roadmap_gps_is_nmea()) {

         roadmap_driver_publish_satellites
            (precision, satellites, count, ROADMAP_DRIVER_NMEA);
      }
   }
}

static void roadmap_driver_nmea_logger (const char *line) {

   if (! (RoadMapDriverSubscription & ROADMAP_DRIVER_NMEA)) return;

   if (roadmap_gps_is_nmea()) {

      char buffer[1024];

      snprintf(buffer, sizeof(buffer), "%s\r\n", line);
      roadmap_driver_send (buffer, ROADMAP_DRIVER_NMEA);
   }
}



/* NMEA processing functions. ------------------------------------------- */

static void roadmap_driver_pxrmadd (void *context,
                                    const RoadMapNmeaFields *fields) {

   RoadMapDriver *driver = (RoadMapDriver *)context;

   roadmap_object_add (driver->name,
                       fields->pxrmadd.id,
                       fields->pxrmadd.name,
                       fields->pxrmadd.sprite);

   (*RoadMapDriverNextPxrmadd) (context, fields);
}


static void roadmap_driver_pxrmmov (void *context,
                                    const RoadMapNmeaFields *fields) {

   RoadMapGpsPosition position;

   position.latitude  = fields->pxrmmov.latitude;
   position.longitude = fields->pxrmmov.longitude;

   position.speed    = fields->pxrmmov.speed;
   position.steering = fields->pxrmmov.steering;
   position.altitude = 0;

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
   RoadMapDriver *driver = (RoadMapDriver *)context;

   if (driver != NULL) {
      for (i = 0; i < fields->pxrmsub.count; ++i) {

         if (roadmap_string_match(fields->pxrmsub.subscribed[i].item, "RMC")) {

            driver->subscription |= ROADMAP_DRIVER_RMC;
            RoadMapDriverSubscription |= ROADMAP_DRIVER_RMC;

         } else if (roadmap_string_match
                       (fields->pxrmsub.subscribed[i].item, "NMEA")) {

            if (! (RoadMapDriverSubscription & ROADMAP_DRIVER_NMEA)) {

               roadmap_gps_register_monitor  (roadmap_driver_monitor);
               roadmap_gps_register_logger   (roadmap_driver_nmea_logger);
            }

            driver->subscription |= ROADMAP_DRIVER_NMEA;
            RoadMapDriverSubscription |= ROADMAP_DRIVER_NMEA;

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


static void roadmap_driver_pxrmcfg (void *context,
                                    const RoadMapNmeaFields *fields) {

   RoadMapDriver *driver = (RoadMapDriver *)context;

   const char *value;
   RoadMapConfigDescriptor descriptor;
   char  buffer[1024];

   roadmap_string_lock (fields->pxrmcfg.category);
   roadmap_string_lock (fields->pxrmcfg.name);
   roadmap_string_lock (fields->pxrmcfg.value);

   descriptor.category  = roadmap_string_get(fields->pxrmcfg.category);
   descriptor.name      = roadmap_string_get(fields->pxrmcfg.name);
   descriptor.reference = NULL;

   roadmap_config_declare ("preferences",
                           &descriptor,
                           roadmap_string_get(fields->pxrmcfg.value));

   value = roadmap_config_get (&descriptor);

   snprintf (buffer, sizeof(buffer), "$PXRMCFG,%s,%s,%s\n",
             descriptor.category,
             descriptor.name,
             value);
   roadmap_io_write (&driver->output, buffer, strlen(buffer));

   /* We do not release the category, name and default value strings,
    * because these are still referenced in the configuration data.
    */

   (*RoadMapDriverNextPxrmcfg) (context, fields);
}

/* End of NMEA processing. ---------------------------------------------- */


static char *roadmap_driver_strdup (const char *text) {

   if (text == NULL) return NULL;

   return strdup(text);
}


static int roadmap_driver_exists (const char *name) {

   RoadMapDriver *driver;

   for (driver = RoadMapDriverList; driver != NULL; driver = driver->next) {
      if (roadmap_string_match(driver->name, name)) return 1;
   }
   return 0;
}


static void roadmap_driver_onexit (void *context) {

   RoadMapDriver *driver = (RoadMapDriver *) context;

   if (driver->input.subsystem != ROADMAP_IO_INVALID) {

      roadmap_object_cleanup (driver->name);

      (*RoadMapDriverLinkRemove) (&driver->input);

      roadmap_io_close (&driver->input);
      roadmap_io_close (&driver->output);

      driver->input.subsystem = ROADMAP_IO_INVALID;
      driver->output.subsystem = ROADMAP_IO_INVALID;
   }
}


static int roadmap_driver_receive (void *context, char *data, int size) {

   RoadMapDriver *driver = (RoadMapDriver *)context;

   return roadmap_io_read (&driver->input, data, size);
}


static void roadmap_driver_send (const char *data, int mask) {

   RoadMapDriver *driver;
   int length = strlen(data);

   for (driver = RoadMapDriverList; driver != NULL; driver = driver->next) {
      if ((driver->output.subsystem != ROADMAP_IO_INVALID) &&
          (driver->subscription & mask)) {
         roadmap_io_write (&driver->output, data, length);
      }
   }
}


static void roadmap_driver_configure (const char *path) {

   char *p;
   char *name;
   char *command;
   char *arguments;
   FILE *file;
   char  buffer[1024];

   RoadMapDriver *driver;

   file = roadmap_file_fopen (path, "drivers", "sr");

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

         /* Eliminate trailing blanks (all spaces before the separator). */
         while (isspace(*(command-1))) --command;

         *command = 0; /* Separate the name from the value. */

         /* Eliminate all spaces after the separator. */
         for (++command; isspace(*command); ++command) ;

         /* retrieve the arguments part of the driver command. */
         arguments = strchr(command, ' ');
         if (arguments == NULL) {
            arguments = strchr(command, '\t');
         }

         if (arguments != NULL) {
            *arguments = 0; /* Separate the name from the arguments. */
            for (++arguments; isspace(*arguments); ++arguments) ;
         }

         /* Ignore this new entry if that driver was already configured. */
         if (roadmap_driver_exists(name)) continue;

         driver = calloc (1, sizeof(RoadMapDriver));
         if (driver == NULL) break;

         driver->name      = roadmap_string_new(name);
         driver->command   = roadmap_driver_strdup(command);
         driver->arguments = roadmap_driver_strdup(arguments);

         driver->input.subsystem  = ROADMAP_IO_INVALID;
         driver->output.subsystem = ROADMAP_IO_INVALID;
         driver->process.handler = roadmap_driver_onexit;
         driver->process.data    = (void *)driver;

         driver->subscription = 0;

         /* Prepare the NMEA context. */
         driver->nmea.title        = roadmap_string_get(driver->name);
         driver->nmea.user_context = driver;
         driver->nmea.cursor       = 0;
         driver->nmea.logger       = NULL;
         driver->nmea.receiver     = roadmap_driver_receive;
         driver->nmea.decoder      = roadmap_nmea_decode;

         /* Configuration item (enable/disable). */
         driver->enable = RoadMapDriverTemplate;
         driver->enable.name = driver->nmea.title;

         roadmap_config_declare_enumeration
            ("preferences", &driver->enable, "Disabled", "Enabled", NULL);

         driver->next = RoadMapDriverList;
         RoadMapDriverList = driver;
      }

      fclose(file);
   }
}


void roadmap_driver_accept (RoadMapIO *io) {

   RoadMapDriver *driver;

   RoadMapIO client;
   
   if (io->subsystem != ROADMAP_IO_NET) {
      roadmap_log (ROADMAP_FATAL, "Driver server has invalid socket");
      return;
   }
         
   client.os.socket = roadmap_net_accept(io->os.socket);
         
   if (!ROADMAP_NET_IS_VALID(client.os.socket)) {
      roadmap_log (ROADMAP_ERROR,
                   "Can't accept a connection for driver server");
      return;
   }
               
   client.subsystem = ROADMAP_IO_NET;
                  
   driver = calloc (1, sizeof(RoadMapDriver));
   if (driver == NULL) {
      roadmap_io_close (&client);
      return;
   }
                     
   driver->name      = roadmap_string_new("network driver");
   driver->command   = roadmap_driver_strdup("");
   driver->arguments = roadmap_driver_strdup("");
                     
   driver->input = client;
   driver->output = client;
   driver->process.handler = roadmap_driver_onexit;
   driver->process.data    = (void *)driver;
                     
   driver->subscription = 0;
                     
   /* Prepare the NMEA context. */
   driver->nmea.title        = roadmap_string_get(driver->name);
   driver->nmea.user_context = driver;
   driver->nmea.cursor       = 0;
   driver->nmea.logger       = NULL;
   driver->nmea.receiver     = roadmap_driver_receive;
   driver->nmea.decoder      = roadmap_nmea_decode;
                     
   /* Configuration item (enable/disable). */
   driver->enable = RoadMapDriverTemplate;
   driver->enable.name = driver->nmea.title;
                        
   driver->next = RoadMapDriverList;
   RoadMapDriverList = driver;
                     
   (*RoadMapDriverLinkAdd) (&driver->input);
}


void roadmap_driver_input (RoadMapIO *io) {

   RoadMapDriver *driver;

   if (io->subsystem == ROADMAP_IO_INVALID) return;

   for (driver = RoadMapDriverList; driver != NULL; driver = driver->next) {

      if (roadmap_io_same (&driver->input, io)) {
         if (roadmap_input (&driver->nmea) < 0) {
            roadmap_driver_onexit (driver);
         }
         break;
      }
   }
}


void roadmap_driver_initialize (void) {

   const char *path;


   roadmap_driver_configure (roadmap_path_user());

   for (path = roadmap_path_first("config");
        path != NULL;
        path = roadmap_path_next("config", path)) {

      roadmap_driver_configure (path);
   }

   RoadMapDriverNextPxrmadd =
      roadmap_nmea_subscribe ("XRM", "ADD", roadmap_driver_pxrmadd);

   RoadMapDriverNextPxrmmov =
      roadmap_nmea_subscribe ("XRM", "MOV", roadmap_driver_pxrmmov);

   RoadMapDriverNextPxrmdel =
      roadmap_nmea_subscribe ("XRM", "DEL", roadmap_driver_pxrmdel);

   RoadMapDriverNextPxrmsub =
      roadmap_nmea_subscribe ("XRM", "SUB", roadmap_driver_pxrmsub);

   RoadMapDriverNextPxrmcfg =
      roadmap_nmea_subscribe ("XRM", "CFG", roadmap_driver_pxrmcfg);

   roadmap_config_declare
      ("preferences", &RoadMapDriverConfigPort, "2007");

   RoadMapDriverServer.subsystem = ROADMAP_IO_INVALID;
}


void roadmap_driver_activate (void) {

   int port;
   RoadMapDriver *driver;
   RoadMapPipe    pipes[2];


   roadmap_gps_register_listener (roadmap_driver_listener);

   port = roadmap_config_get_integer (&RoadMapDriverConfigPort);

   if (port > 0) {

      RoadMapDriverServer.os.socket = roadmap_net_listen(port);

      if (ROADMAP_NET_IS_VALID(RoadMapDriverServer.os.socket)) {

         RoadMapDriverServer.subsystem = ROADMAP_IO_NET;

         if (RoadMapDriverServerAdd != NULL) {
            (*RoadMapDriverServerAdd) (&RoadMapDriverServer);
         }
      }
   }


   for (driver = RoadMapDriverList; driver != NULL; driver = driver->next) {

      if (strcasecmp(roadmap_config_get (&driver->enable), "Enabled")) continue;

      if (roadmap_spawn_with_pipe
            (driver->command,
             driver->arguments,
             pipes,
             &driver->process) > 0) {

         /* Declare this I/O to the GUI so that we can wake up
          * when the driver talks to us.
          */
         driver->input.subsystem = driver->output.subsystem = ROADMAP_IO_PIPE;
         driver->input.os.pipe = pipes[0];
         driver->output.os.pipe = pipes[1];

         (*RoadMapDriverLinkAdd) (&driver->input);

         // TBD: any initialization message needed??
      }
   }
}


void roadmap_driver_register_link_control
        (roadmap_gps_link_control add, roadmap_gps_link_control remove) {

   RoadMapDriverLinkAdd    = add;
   RoadMapDriverLinkRemove = remove;
}


void roadmap_driver_register_server_control
        (roadmap_gps_link_control add, roadmap_gps_link_control remove) {

   if (RoadMapDriverServer.subsystem != ROADMAP_IO_INVALID) {
      (*add)(&RoadMapDriverServer);
   } else {
      RoadMapDriverServerAdd = add;
   }
}


void roadmap_driver_shutdown (void) {

   RoadMapDriver *driver;

   for (driver = RoadMapDriverList; driver != NULL; driver = driver->next) {
      roadmap_driver_onexit (driver);
   }

   if (RoadMapDriverServer.subsystem != ROADMAP_IO_INVALID) {
      RoadMapDriverLinkRemove (&RoadMapDriverServer);
   }
}

