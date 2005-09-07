/* roadgps_logger.c - Log NMEA messages.
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
 *   See roadgps_logger.h
 */

#include <stdio.h>

#include "roadmap.h"
#include "roadmap_gps.h"
#include "roadmap_path.h"
#include "roadmap_main.h"

#include "roadgps_logger.h"


static FILE *RoadGpsOutput = NULL;


static void roadgps_logger (const char *sentence) {

   if (RoadGpsOutput != NULL) {
      fprintf (RoadGpsOutput, "%s\n", sentence);
   }

   roadmap_main_set_status (sentence);
}


void roadgps_logger_stop (void) {

   if (RoadGpsOutput != NULL) {
      fclose (RoadGpsOutput);
      RoadGpsOutput = NULL;
   }
}


void roadgps_logger_start (void) {

   static unsigned char RoadGpsCounter = 1;
   static char *RoadGpsName = "roadgps-%d.log";

   char name[80];
   char *fullname = NULL; /* Avoid spurious warning. */


   roadgps_logger_stop ();

   for (;;) {

      if (RoadGpsCounter <= 0) {
         RoadGpsCounter = 1;
         break;
      }
      snprintf (name, sizeof(name), RoadGpsName, RoadGpsCounter);
      fullname = roadmap_path_join (roadmap_path_temporary(), name);

      RoadGpsOutput = fopen (fullname, "r");

      if (RoadGpsOutput == NULL) break;

      fclose (RoadGpsOutput);
      RoadGpsCounter += 1;
      roadmap_path_free (fullname);
   }

   snprintf (name, sizeof(fullname), RoadGpsName, RoadGpsCounter);
   RoadGpsOutput = fopen (fullname, "w");

   if (RoadGpsOutput == NULL) {
      roadmap_log (ROADMAP_ERROR, "cannot open log file %s", fullname);
   }
   roadmap_path_free (fullname);
}


void roadgps_logger_initialize (void) {
   roadmap_gps_register_logger (roadgps_logger);
}

