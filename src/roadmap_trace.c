/* roadmap_trace.c - RoadMap NMEA trace driver for testing purposes.
 *
 * LICENSE:
 *
 *   Copyright 2003 tz1
 *   Copyright 2005 Pascal F. Martin
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
 * DESCRIPTION:
 *
 *   This program is a very simple (and useless) driver that request
 *   the NMEA information and prints it.
 *
 *   The purpose of that driver is to excercise and test the driver
 *   management code.
 *
 *   This program is normally launched by RoadMap.
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "roadmap.h"


struct delay_buffer {
    char data[256];
};


int main(int argc, char *argv[]) {

   int   received;
   char  buffer[256];

   printf ("$PXRMSUB,NMEA\n");
   fflush(stdout);

   for(;;) {

      /* Retrieve the data from RoadMap. ------------------------------- */

      received = read (0, buffer, sizeof(buffer));

      if (received <= 0) {
         exit(0); /* RoadMap cut the pipe. */
      }

      buffer[received] = 0;
      fputs (buffer, stderr);
   }

   return 0; /* Some compilers might not detect the forever loop. */
}

