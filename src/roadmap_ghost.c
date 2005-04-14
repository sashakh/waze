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
 *   beside being entertaining, the purpose of that driver is to
 *   excercise and test the driver management code.
 *
 *   This program is normally launched by RoadMap.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/select.h>

#include "roadmap.h"


struct delay_buffer {
    char data[256];
};

static struct delay_buffer *delay_line = NULL;
static int delay_cursor = 0;
static int delay_length = 0;



int main(int argc, char *argv[]) {

   int previous = -1;

   if (argc > 1 && strcmp(argv[1], "--help") == 0) {
      printf ("usage: %s [--help] [--delay=N]\n"
              "  --delay=N:    set the delay length (default is 10).\n");
      exit(0);
   }

   if (argc > 1 && strncmp(argv[1], "--delay=", 8) == 0) {
      delay_length = atoi(argv[1] + 8);
      argc -= 1;
      argv += 1;
   }

   if (delay_length <= 1) delay_length = 10;

   delay_line = calloc (delay_length, sizeof(struct delay_buffer));
   if (delay_line == NULL) {
      roadmap_log (ROADMAP_FATAL, "no memory");
   }

   printf ("$PXRMADD,ghost,ghost,Ghost\n");
   printf ("$PXRMSUB,ghost,RMC\n");
   fflush(stdout);

   for(;;) {

      fd_set reads;

      FD_ZERO(&reads);
      FD_SET(0, &reads);

      if (select (1, &reads, NULL, NULL, NULL) == 0) continue;

      if (FD_ISSET(0, &reads)) {

         int received =
            read (0, delay_line[delay_cursor].data, sizeof(delay_line[0].data));

         if (received <= 0) {
            exit(0); /* RoadMap cut the pipe. */
         }

         delay_line[delay_cursor].data[received] = 0;

         if (previous >= 0) {
            if (strcmp (delay_line[delay_cursor].data,
                        delay_line[previous].data) == 0) {

               continue; /* ignore this repetition. */
            }
         }

         previous = delay_cursor;
         if (++delay_cursor >= delay_length) delay_cursor = 0;


         if (delay_line[delay_cursor].data[0] != 0) {

            /* retrieve the latitude from the RMC sentence (xx field). */
            char *p = strchr (delay_line[delay_cursor].data, ',');

            if (p == NULL) continue;
            p = strchr (p+1, ',');    /* Skip the time. */
            if (p == NULL) continue;
            p = strchr (p+1, ',');    /* Skip the status. */
            if (p == NULL) continue;

            printf ("$PXRMMOV,ghost%s", p);
            fflush (stdout);
         }

         if (ferror(stdout)) {
            exit(0); /* RoadMap closed the pipe. */
         }
      }
   }
   return 0; /* Some compilers might not detect the forever loop. */
}

