/* roadmap_ghost.c - RoadMap "ghost" car driver for fun.
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
 *   This program is a very simple (and useless) driver that shows
 *   the RoadMap vehicle with a few cycle delay, so that it appears
 *   to be pursuing the RoadMap vehicle.
 *
 *   Beside being entertaining, the purpose of that driver is to
 *   excercise and test the driver management code.
 *
 *   This program is normally launched by RoadMap.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "roadmap.h"


struct delay_buffer {
    char data[256];
};


int main(int argc, char *argv[]) {

   const char *driver = "Ghost";

   char  config[256];
   int   config_length;

   char  buffer[256];
   int   received;

   int previous = -1;
   int delay_cursor = 0;
   int delay_length = 0;

   struct delay_buffer *delay_line = NULL;


   if (argc > 1 && strncmp (argv[1], "--driver=", 9) == 0) {
      driver = argv[1] + 9;
   }
   snprintf (config, sizeof(config), "$PXRMCFG,%s,Delay,", driver);
   config_length = strlen(config);

   printf ("$PXRMADD,%s,%s,Friend\n", driver, driver);
   printf ("$PXRMSUB,RMC\n");
   printf ("%s10\n", config);
   fflush(stdout);

   for(;;) {

      /* Retrieve the data from RoadMap. ------------------------------- */

      received = read (0, buffer, sizeof(buffer));

      if (received <= 0) {
         exit(0); /* RoadMap cut the pipe. */
      }

      buffer[received] = 0;


      /* Proces the configuration information. ------------------------- */

      if (strncmp (buffer, config, config_length) == 0) {

         int configured = atoi (buffer + config_length);

         if (configured <= 0) configured = 10;

         if (configured != delay_length) {

            /* Reallocate and reset the delay line. */

            delay_length = configured;

            if (delay_line != NULL) free (delay_line);

            delay_line = calloc (delay_length, sizeof(struct delay_buffer));
            if (delay_line == NULL) {
               roadmap_log (ROADMAP_FATAL, "no more memory");
            }
            delay_cursor = 0;
            previous = -1;
         }
         continue; /* Don't echo configuration information. */
      }

      if (delay_line == NULL) continue; /* Not configured yet. */


      /* Store the latest position information. ------------------------ */

      if (previous >= 0) {
         if (strcmp (buffer, delay_line[previous].data) == 0) {

            continue; /* ignore this repetition. */
         }
      }
      strncpy (delay_line[delay_cursor].data,
               buffer,
               sizeof(delay_line[delay_cursor].data));

      previous = delay_cursor;
      if (++delay_cursor >= delay_length) delay_cursor = 0;


      /* Echoe the oldest position, if any. ---------------------------- */

      if (delay_line[delay_cursor].data[0] != 0) {

         /* retrieve the latitude from the RMC sentence (xx field). */
         char *p = strchr (delay_line[delay_cursor].data, ',');

         if (p == NULL) continue;
         p = strchr (p+1, ',');    /* Skip the time. */
         if (p == NULL) continue;
         p = strchr (p+1, ',');    /* Skip the status. */
         if (p == NULL) continue;

         printf ("$PXRMMOV,%s%s", driver, p);
         fflush (stdout);
      }

      if (ferror(stdout)) {
         exit(0); /* RoadMap closed the pipe. */
      }
   }

   return 0; /* Some compilers might not detect the forever loop. */
}

