/* roadmap_gpsd2.c - a module to interact with gpsd using its library.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_gpsd2.h"


static RoadMapGpsdNavigation RoadmapGpsd2NavigationListener = NULL;


static int roadmap_gpsd2_decode_numeric (const char *input) {

   if (input[0] == '?') return ROADMAP_NO_VALID_DATA;

   return atoi(input);
}


static int roadmap_gpsd2_decode_coordinate (const char *input) {

   char *point = strchr (input, '.');

   if (point != NULL) {

      /* This is a floating point value: patch the input to multiply
       * it by 1000000 and then make it an integer (TIGER format).
       */
      const char *from;

      int   i;
      char *to;
      char  modified[16];

      to = modified;

      /* Copy the integer part. */
      for (from = input; from < point; ++from) {
         *(to++) = *from;
      }

      /* Now copy the decimal part. */
      for (from = point + 1, i = 0; *from > 0 && i < 6; ++from, ++i) {
         *(to++) = *from;
      }
      while (i++ < 6) *(to++) = '0';
      *to = 0;

      return atoi(modified);
   }

   return roadmap_gpsd2_decode_numeric (input);
}


RoadMapSocket roadmap_gpsd2_connect (const char *name) {

   RoadMapSocket socket = roadmap_net_connect ("tcp", name, 2947);

   if (ROADMAP_NET_IS_VALID(socket)) {

      /* Start watching what happens. */

      static const char request[] = "w+\n";

      if (roadmap_net_send
            (socket, request, sizeof(request)-1) != sizeof(request)-1) {

         roadmap_log (ROADMAP_WARNING, "Lost gpsd server session");
         roadmap_net_close (socket);

         return ROADMAP_INVALID_SOCKET;
      }
   }

   return socket;
}


void roadmap_gpsd2_subscribe_to_navigation (RoadMapGpsdNavigation navigation) {

   RoadmapGpsd2NavigationListener = navigation;
}


int roadmap_gpsd2_query_navigation (RoadMapSocket socket) {

   static const char request[] = "w+\n"; // "mpavt\n";

   if (roadmap_net_send
         (socket, request, sizeof(request)-1) != sizeof(request)-1) {

      roadmap_log (ROADMAP_WARNING, "Lost gpsd server session");
      return 0;
   }
   return 1;
}


int roadmap_gpsd2_decode (void *context, char *sentence) {

   int got_navigation_data;

   int status;
   int latitude;
   int longitude;
   int altitude;
   int speed;
   int steering;

   char *reply[256];
   int   reply_count;
   int   i;

   reply_count = roadmap_input_split (sentence, ',', reply, 256);

   if ((reply_count <= 1) || strcmp (reply[0], "GPSD")) {
      return 0;
   }

   /* default value (invalid value): */
   status = 'V';
   latitude  = ROADMAP_NO_VALID_DATA;
   longitude = ROADMAP_NO_VALID_DATA;
   altitude  = ROADMAP_NO_VALID_DATA;
   speed     = ROADMAP_NO_VALID_DATA;
   steering  = ROADMAP_NO_VALID_DATA;


   got_navigation_data = 0;

   for(i = 1; i < reply_count; ++i) {

      char *item = reply[i];
      char *value = item + 2;
      char *argument[2];


      if (item[1] != '=') {
         continue;
      }

      switch (item[0]) {

         case 'M':

            if (atoi(value) > 0) {
               status = 'A';
            } else {
               status = 'V';
               got_navigation_data = 1;
               goto end_of_decoding;
            }
            break;

         case 'P':

            if (roadmap_input_split (value, ' ', argument, 2) != 2) {
               continue;
            }

            latitude = roadmap_gpsd2_decode_coordinate (argument[0]);
            longitude = roadmap_gpsd2_decode_coordinate (argument[1]);

            if ((longitude != ROADMAP_NO_VALID_DATA) &&
                  (latitude != ROADMAP_NO_VALID_DATA)) {
               got_navigation_data = 1;
               status = 'A';
            }
            break;

         case 'A':

            altitude = roadmap_gpsd2_decode_numeric (value);
            break;

         case 'V':

            speed = roadmap_gpsd2_decode_numeric (value);
            break;

         case 'T':

            steering = roadmap_gpsd2_decode_numeric (value);
            break;
      }
   }

end_of_decoding:

   if (got_navigation_data) {

      RoadmapGpsd2NavigationListener
         (status, latitude, longitude, altitude, speed, steering);

      return 1;
   }
   return 0;
}


void roadmap_gpsd2_close (RoadMapSocket socket) {

   roadmap_net_close (socket);
}

