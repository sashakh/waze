/* roadmap_gps_detect.c - GPS auto detection - win32 only
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

#include "roadmap_dialog.h"
#include "roadmap_main.h"
#include "roadmap_lang.h"

#include "roadmap_gps.h"


static RoadMapConfigDescriptor RoadMapConfigGPSVirtual =
			ROADMAP_CONFIG_ITEM("GPS", "Virtual");

static RoadMapConfigDescriptor RoadMapConfigGPSBaudRate =
                        ROADMAP_CONFIG_ITEM("GPS", "Baud Rate");

static void roadmap_gps_detect_periodic(void);

static void roadmap_gps_detect_finalize(void) {
   roadmap_main_remove_periodic (roadmap_gps_detect_periodic);
   roadmap_dialog_set_data ("GPS Receiver Auto Detect", "Port", "");
   roadmap_dialog_set_data ("GPS Receiver Auto Detect", "Speed", "");
   roadmap_dialog_hide ("Detect GPS receiver");
}


static void roadmap_gps_detect_periodic(void) {

   static const int *serial_ports;
   static const char **speeds;
   static time_t OpenTime;

   static int SpeedIndex = -1;
   static int CurrentPort = -1;
   static int VirtualPort;

   static char *SavedSpeed;
   static char *SavedPort;
   static int SavedRetryPending = 0;

   static char Prompt[100];

   if (CurrentPort == -1) { /* new run */
      const char *virtual_port = roadmap_config_get (&RoadMapConfigGPSVirtual);
      serial_ports = roadmap_serial_enumerate ();
      speeds = roadmap_serial_get_speeds ();

      /* check for virtual port configuration */
      if ((strlen(virtual_port) >= 5) && !strncmp(virtual_port, "COM", 3)) {
         VirtualPort = atoi (virtual_port + 3);
      } else {
         VirtualPort = -1;
      }

      SavedSpeed = (char *)roadmap_config_get (&RoadMapConfigGPSBaudRate);
      SavedPort = (char *)roadmap_config_get (&RoadMapConfigGPSSource);
      SavedRetryPending = RoadMapGpsRetryPending;

      OpenTime = time(NULL) - (time_t)3; /* make sure first time will be processed */
      CurrentPort++;
      /* skip undefined ports */
      while ((CurrentPort < MAX_SERIAL_ENUMS) &&
            (!serial_ports[CurrentPort] ||
            (CurrentPort == VirtualPort))) {

         CurrentPort++;
      }
   }

   if (time(NULL) - OpenTime > (time_t)2) { /* passed 2 seconds since trying to open gps */
      SpeedIndex++;
      if (speeds[SpeedIndex] == NULL) {
         SpeedIndex = 0;
         CurrentPort++;

         /* skip undefined ports */
         while ((CurrentPort < MAX_SERIAL_ENUMS) &&
               (!serial_ports[CurrentPort] ||
               (CurrentPort == VirtualPort))) {

            CurrentPort++;
         }
      }

      if (CurrentPort == MAX_SERIAL_ENUMS) { /* not found */
         roadmap_gps_detect_finalize();
         roadmap_config_set (&RoadMapConfigGPSSource, SavedPort);
         roadmap_config_set (&RoadMapConfigGPSBaudRate, SavedSpeed);
         if ((SavedRetryPending) && (! RoadMapGpsRetryPending)) {
            (*RoadMapGpsPeriodicAdd) (roadmap_gps_open);
            RoadMapGpsRetryPending = 1;
         }
         SpeedIndex = -1;
         CurrentPort = -1;
         roadmap_messagebox(roadmap_lang_get ("Error"),
            roadmap_lang_get ("GPS Receiver not found. Make sure your receiver is connected and turned on."));
         return;
      }

      /* prepare to test the new configuration */
      if (RoadMapGpsRetryPending) {
         (*RoadMapGpsPeriodicRemove) (roadmap_gps_open);
         RoadMapGpsRetryPending = 0;
      }

      roadmap_gps_shutdown();

      sprintf (Prompt, "COM%d:", CurrentPort);

      roadmap_dialog_set_data ("GPS Receiver Auto Detect", "Speed",
                               speeds[SpeedIndex]);
      roadmap_dialog_set_data ("GPS Receiver Auto Detect", "Port", Prompt);

      roadmap_config_set (&RoadMapConfigGPSSource, Prompt);
      roadmap_config_set (&RoadMapConfigGPSBaudRate, speeds[SpeedIndex]);

      OpenTime = time(NULL);
      roadmap_gps_open();
      return;
   }

   if (RoadMapGpsReception != 0) { /* found */
      roadmap_gps_detect_finalize();
      snprintf (Prompt, sizeof(Prompt),
             "%s\nPort: COM%d:\nSpeed: %s",
             roadmap_lang_get ("Found GPS Receiver."),
             CurrentPort, speeds[SpeedIndex]);
      SpeedIndex = -1;
      CurrentPort = -1;
      roadmap_messagebox(roadmap_lang_get ("Info"), Prompt);
   }
}


void roadmap_gps_detect_receiver (void) {

   if (RoadMapGpsReception != 0) {
      roadmap_messagebox(roadmap_lang_get ("Info"),
                         roadmap_lang_get ("GPS already connected!"));
   } else {
      if (roadmap_dialog_activate ("Detect GPS receiver", NULL)) {

         roadmap_dialog_new_label  ("GPS Receiver Auto Detect", "Port");
         roadmap_dialog_new_label  ("GPS Receiver Auto Detect", "Speed");
         roadmap_dialog_new_label  ("GPS Receiver Auto Detect", "Status");

         roadmap_dialog_complete (0);
      }

      roadmap_dialog_set_data ("GPS Receiver Auto Detect", "Status",
                               roadmap_lang_get ("Running, please wait..."));
      roadmap_main_set_periodic (200,roadmap_gps_detect_periodic);
   }
}
