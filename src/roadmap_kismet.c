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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define MAX_WIRELESS_HOT_SPOTS   16384

struct knetlist {
    int btop, bbot;
};

static struct knetlist knl[MAX_WIRELESS_HOT_SPOTS];
static int knlmax = 0;


int main(int argc, char *argv[])
{
   FILE *sfp;
   int knet = -1;
   int i, kfd;
   struct sockaddr_in skin;
   long temp;
   char buf[256];
   double la, lo;
   unsigned int chan, sig, wep, maxsig = 0, minsig = 255;
   unsigned char bssid[6];
   unsigned int bsstop, bssbot;
   int ilat, ilon;

   temp = inet_addr("127.0.0.1");

   memset((char *) &skin, 0, sizeof(skin));
   skin.sin_family = AF_INET;	/* host->h_addrtype; */
   skin.sin_port = htons(2501);
   memcpy(&skin.sin_addr, &temp, sizeof(temp));
   knet = socket(AF_INET, SOCK_STREAM, 0);
   if(knet < 0)
      exit(errno);
   if((i = connect(knet, (struct sockaddr *) &skin, sizeof(skin))) < 0)
      exit(errno);
   sleep(1);
   i = read(knet, buf, 255);
   buf[i] = 0;
   sleep(1);

   {
      char xx[] = "!2 ENABLE NETWORK bssid,channel,bestsignal,bestlat,bestlon,wep\n";
      write(knet, xx, strlen(xx));
   }
   i = read(knet, buf, 255);
   sleep(1);

   sfp = fdopen(knet, "r");
   wep = 0;

   for(;;) {

      buf[0] = 0;
      fgets(buf, 255, sfp);

      if (ferror(sfp))
         exit(errno);

      if(strncmp(buf, "*NETWORK: ", 10))
         continue;

      sscanf(&buf[28], "%d %d %lf %lf %d", &chan, &sig, &la, &lo, &wep);

      if(sig > 1 && sig < minsig - 1)
         minsig = sig - 1;
      if(sig > maxsig)
         maxsig = sig;

      for(i = 0; i < 6; i++)
         bssid[i] = strtoul(&buf[10 + 3 * i], NULL, 16);
      bsstop = (bssid[0] << 16) | (bssid[1] << 8) | bssid[2];
      bssbot = (bssid[3] << 16) | (bssid[4] << 8) | bssid[5];

      la *= 1000000.0;
      lo *= 1000000.0;
      la += 0.5;
      lo += 0.5;
      ilat = la;
      ilon = lo;
      if(!ilat || !ilon)
         continue;

      for(i = knlmax - 1; i >= 0; i--) {
         if(bsstop == knl[i].btop && bssbot == knl[i].bbot)
            break;
      }
      if(i == -1) {
         if (knlmax >= MAX_WIRELESS_HOT_SPOTS) continue;
         i = knlmax++;
         knl[i].btop = bsstop;
         knl[i].bbot = bssbot;
         printf ("$PRDMADD,KISMET%x.%x,Channel:%d,kismet,%d,%d\n",
                 bsstop, bssbot,  chan + (wep << 8), ilat, ilon);
      } else {
         printf ("$PRDMMOV,KISMET%x.%x,%d,%d\n", bsstop, bssbot, ilat, ilon);
      }
      fflush (stdout);

      if (ferror(stdout))
         exit(0); /* RoadMap closed the pipe. */
   }
   return 0; /* Some compilers might not detect the forever loop. */
}

