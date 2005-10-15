/* roadmap_kismet.c - RoadMap driver for kismet.
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
 *   This program is a kismet client that receive updates from kismet
 *   and forwards them to RoadMap as "moving objects".
 *
 *   This program is normally launched by RoadMap.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <sys/select.h>

#include "roadmap_net.h"


#define MAX_WIRELESS_STATIONS   9999

struct knetlist {
    int btop, bbot;
};

static struct knetlist knl[MAX_WIRELESS_STATIONS];
static int knlmax = 0;

static char kismet_data[256];


static int connect_to_kismet (const char *host) {

   int socket = -1;
   
   while (socket < 0) {

      socket = roadmap_net_connect ("tcp", host, 2501);
      sleep (1);
   }

   /* Consume out any introductory data from kismet. */
   read(socket, kismet_data, sizeof(kismet_data));
   sleep(1);

   {
      static char xx[] =
         "!2 ENABLE NETWORK bssid,channel,bestsignal,bestlat,bestlon,wep\n";
      write(socket, xx, strlen(xx));
   }

   /* Get rid of the answer? */
   read(socket, kismet_data, sizeof(kismet_data));
   sleep(1);

   return socket;
}


/* Convert the kismet "decimal degree" format into the NMEA ddmm.mmmmm
 * format.
 */
static void convert_to_nmea (double kismet, int *nmea_ddmm, int *nmea_mmmm) {

   double converting;
   int nmea_dd;
   int nmea_mm;

   converting = fabs(kismet) + 0.0000005;
   nmea_dd = (int) converting;
   converting -= nmea_dd;

   converting *= 60;
   nmea_mm = (int) converting;
   converting -= nmea_mm;

   if (converting > 1.0) {
      fprintf (stderr,
               "invalid value %f (dd -> %d, mm -> %d, remainder %f)\n"
                  "kismet data was: %s\n",
               kismet,
               nmea_dd,
               nmea_mm,
               converting,
               kismet_data);
      exit(1);
   }
   *nmea_mmmm = (int) (kismet * 10000.0);
   *nmea_ddmm = (nmea_dd * 100) + nmea_mm;
}


int main(int argc, char *argv[]) {

   char *driver = "Kismet";

   char config_server[256];
   int  config_server_length;

   int gps_mode = 0;

   char *kismet_host;

   FILE *sfp;
   int knet = -1;
   int i, kfd;
   int fdcount = 1;

   unsigned int chan, sig, wep, maxsig = 0, minsig = 255;
   unsigned char bssid[6];
   unsigned int bsstop, bssbot;

   double la, lo;
   int la_ddmm, lo_ddmm; /* The "ddmm" part. */
   int la_mmmm, lo_mmmm; /* The fractional ".mmmmm" part. */
   char la_hemi, lo_hemi;


   if (argc > 1 && strcmp(argv[1], "--help") == 0) {
      fprintf (stderr,
               "usage: %s [--help] [--gps]\n"
               "  --gps:         simulate GPS position information.\n"
               "  --driver=name: use the specified driver name.\n");
      exit(0);
   }

   if (argc > 1 && strcmp(argv[1], "--gps") == 0) {
      gps_mode = 1;
      argc -= 1;
      argv += 1;
   }

   if (argc > 1 && strncmp (argv[1], "--driver=", 9) == 0) {
      driver = argv[1] + 9;
   }
   snprintf (config_server,
             sizeof(config_server), "$PXRMCFG,%s,Server,", driver);
   config_server_length = strlen(config_server);

   wep = 0;

   printf ("%s\n", config_server); /* Ask for the host to connect to. */
   fflush (stdout);

   for(;;) {

      fd_set reads;

      FD_ZERO(&reads);
      FD_SET(0, &reads);
      if (knet > 0) FD_SET(knet, &reads);

      if (select (fdcount, &reads, NULL, NULL, NULL) == 0) continue;

      if (FD_ISSET(0, &reads)) {

         int  received;
         char info[1024];

         received = read (0, info, sizeof(info));

         if (received <= 0) {
            exit(0); /* RoadMap cut the pipe. */
         }

         if (strncmp (info, config_server, config_server_length) == 0) {

            kismet_host = strdup(info+config_server_length);

            if (kismet_host == NULL || kismet_host[0] == 0) {
               free(kismet_host);
               kismet_host = strdup("127.0.0.1");
            }

            knet = connect_to_kismet (kismet_host);
            if (knet > 0) {
               fdcount = knet + 1;
               sfp = fdopen(knet, "r");
            }
         }
      }

      if ((knet > 0) && FD_ISSET(knet, &reads)) {

         kismet_data[0] = 0;
         fgets(kismet_data, sizeof(kismet_data)-1, sfp);

         if (ferror(sfp)) {

            /* The kismet connection went down. Try to re-establish it. */
            fclose (sfp);
            knet = connect_to_kismet (kismet_host);
            sfp = fdopen(knet, "r");

            continue;
         }

         if(strncmp(kismet_data, "*NETWORK: ", 10))
            continue;

         sscanf(&kismet_data[28],
               "%d %d %lf %lf %d", &chan, &sig, &la, &lo, &wep);

         if(sig > 1 && sig < minsig - 1)
            minsig = sig - 1;
         if(sig > maxsig)
            maxsig = sig;

         for(i = 0; i < 6; i++)
            bssid[i] = strtoul(&kismet_data[10 + 3 * i], NULL, 16);
         bsstop = (bssid[0] << 16) | (bssid[1] << 8) | bssid[2];
         bssbot = (bssid[3] << 16) | (bssid[4] << 8) | bssid[5];


         /* We must now convert the latitude & longitude into the standard
          * NMEA format (since RoadMap uses the NMEA syntax).
          */
         la_hemi = (la < 0) ? 'S' : 'N';
         convert_to_nmea (la, &la_ddmm, &la_mmmm);

         lo_hemi = (lo < 0) ? 'W' : 'E';
         convert_to_nmea (lo, &lo_ddmm, &lo_mmmm);

         if (la_ddmm == 0 && la_mmmm == 0 && lo_ddmm == 0 && lo_mmmm == 0)
            continue;


         /* If this is a new station, we must first declare it. */

         for (i = knlmax - 1; i >= 0; --i) {
            if(bsstop == knl[i].btop && bssbot == knl[i].bbot)
               break;
         }

         if(i == -1) {

            if (knlmax >= MAX_WIRELESS_STATIONS) continue;
            i = knlmax++;
            knl[i].btop = bsstop;
            knl[i].bbot = bssbot;

            printf ("$PXRMADD,ksm%d,"    /* ID for this node. */
                  "%02x:%02x:%02x:%02x:%02x:%02x,Kismet\n", /* MAC address */
                  i,
                  bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
         }


         /* Now provide the current position of this station.
          * The "speed" is actually the station's signal strength
          * and the "course" represents the channel.
          */

         printf ("$PXRMMOV,ksm%d,%d.%04d,%c,%d.%04d,%c,%d,0\n",
                 i,
                 la_ddmm, la_mmmm, la_hemi,
                 lo_ddmm, lo_mmmm, lo_hemi,
                 sig - 170,                  /* Why "170"? */
                 chan + (wep << 8));

         if (gps_mode) {
            /* In this mode, there is no GPS, so we use the kismet data
             * to regenerate some GPS information.
             */
            printf ("$GPGLL,%d.%04d,%c,%d.%04d,%c,,A\n",
                    la_ddmm, la_mmmm, la_hemi,
                    lo_ddmm, lo_mmmm, lo_hemi);
         }
         fflush (stdout);

         if (ferror(stdout)) {
            exit(0); /* RoadMap closed the pipe. */
         }
      }
   }

   return 0; /* Some compilers might not detect the forever loop. */
}

