/* roadmap_vii.c - a module to interact with the VII GPS daemon.
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
 *
 * DESCRIPTION:
 *
 *   This module hides the gpsd library API (version 2).
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_math.h"
#include "roadmap_nmea.h"
#include "roadmap_vii.h"


static RoadMapViiNavigation RoadmapViiNavigationListener = NULL;


RoadMapSocket roadmap_vii_connect (const char *name) {

   RoadMapSocket socket;

   int port = 2947;
   char *copy = strdup(name);
   char *p = strchr (copy, ':');

   if (p != NULL) {
      *p = 0;
      port = atoi(p+1);
   }
   socket = roadmap_net_connect ("tcp", copy, port);
   free(copy);

   roadmap_log (ROADMAP_DEBUG, "Connected to glty on %s", name);

   return socket;
}


void roadmap_vii_subscribe_to_navigation (RoadMapViiNavigation navigation) {

   RoadmapViiNavigationListener = navigation;
}


int roadmap_vii_decode (void *user_context,
                        void *decoder_context, char *sentence) {

   int status;
   int latitude;
   int longitude;
   int altitude;
   int speed;
   int steering;

   char *reply[20];
   int   reply_count;

   int  gps_time = 0;


   /* We skip any leftover from previous transmission problems,
    * check that the '$' is really here, compute the checksum
    * and then decode the "csv" format.
    */
   while ((*sentence != '$') && (*sentence >= ' ')) ++sentence;

   if (*sentence != '$') return 0; /* Ignore this ill-formed sentence. */


   /* We do not use the glty responses and notifications here, so ignore
    * them all.
    */
   if (strncmp (sentence, "$PGLTY,", 7) == 0) return 0;


   /* The VII protocol is built as a proprietary extension to the
    * NMEA 0183 protocol. If the sentence is not a VII one, or a glty
    * sentence, default to the NMEA decoder.
    */
   if (strncmp (sentence, "$PVII", 5) != 0) {
     return roadmap_nmea_decode (user_context, decoder_context, sentence);
   }

   /* This is a VII sentence, which is not known to the NMEA decoder.
    * So we must decode it here. This sentence is very simple, so it
    * makes sense to just keep it separate. Plus, it can only happen
    * with glty, so it does not make sense to weight NMEA down with it.
    */
   reply_count = roadmap_input_split (sentence, ',', reply, 20);

   if ((reply_count <= 10) || strcmp (reply[0], "$PVII")) {
      return 0;
   }

   /* default value (invalid value): */
   if (*(reply[3]) == '0') {
      status = 'V';
   } else {
      status = 'A';
   }
   gps_time  = atoi (reply[2]);
   latitude  = roadmap_math_from_floatstring (reply[4], MILLIONTHS);
   longitude = roadmap_math_from_floatstring (reply[5], MILLIONTHS);
   altitude  = atoi (reply[6]);
   speed     = atoi (reply[7]);
   steering  = atoi (reply[8]);

   RoadmapViiNavigationListener
      (status, gps_time, latitude, longitude, altitude, speed, steering);

   return 1;
}

