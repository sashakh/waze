/* roadmap_net.c - Network interface for the RoadMap application.
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
 *   See roadmap_net.h
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "roadmap.h"
#include "roadmap_net.h"


RoadMapSocket roadmap_net_connect (const char *protocol,
                                   const char *name, int default_port) {

   int   s;
   char *hostname;
   char *separator = strchr (name, ':');

   struct hostent *host;
   struct sockaddr_in addr;


   addr.sin_family = AF_INET;

   hostname = strdup(name);
   roadmap_check_allocated(hostname);

   if (separator != NULL) {

      struct servent *service;

      service = getservbyname(separator+1, "tcp");

      if (service == NULL) {

         if (isdigit(separator[1])) {

            addr.sin_port = htons(atoi(separator+1));

            if (addr.sin_port == 0) {
               roadmap_log (ROADMAP_ERROR, "invalid port in '%s'", name);
               goto connection_failure;
            }

         } else {
            roadmap_log (ROADMAP_ERROR, "invalid service in '%s'", name);
            goto connection_failure;
         }

      } else {
         addr.sin_port = service->s_port;
      }

      *(strchr(hostname, ':')) = 0;


   } else {
      addr.sin_port = htons(default_port);
   }


   host = gethostbyname(hostname);

   if (host == NULL) {

      if (isdigit(hostname[0])) {
         addr.sin_addr.s_addr = inet_addr(hostname);
         if (addr.sin_addr.s_addr == INADDR_NONE) {
            roadmap_log (ROADMAP_ERROR, "invalid IP address '%s'", hostname);
            goto connection_failure;
         }
      } else {
         roadmap_log (ROADMAP_ERROR, "invalid host name '%s'", hostname);
         goto connection_failure;
      }

   } else {
      memcpy (&addr.sin_addr, host->h_addr, host->h_length);
   }


   if (strcmp (protocol, "udp") == 0) {
      s = socket (PF_INET, SOCK_DGRAM, 0);
   } else if (strcmp (protocol, "tcp") == 0) {
      s = socket (PF_INET, SOCK_STREAM, 0);
   } else {
      roadmap_log (ROADMAP_ERROR, "unknown protocol %s", protocol);
      goto connection_failure;
   }

   if (s < 0) {
      roadmap_log (ROADMAP_ERROR, "cannot create socket, errno = %d", errno);
      goto connection_failure;
   }

   /* FIXME: this way of establishing the connection is kind of dangerous
    * if the server process is not local: we might fail only after a long
    * delay that will disable RoadMap for a while.
    */
   if (connect (s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {

      /* This is not a local error: the remote application might be
       * unavailable. This is not our fault, don't cry.
       */
      close(s);
      goto connection_failure;
   }

   return (RoadMapSocket)s;


connection_failure:

   free(hostname);
   return (RoadMapSocket)-1;
}


int roadmap_net_send (RoadMapSocket s, const void *data, int length) {

   int sent = write ((int)s, data, length);

   if (sent != length) {
      return -1;
   }

   return sent;
}


int roadmap_net_receive (RoadMapSocket s, void *data, int size) {

   int received = read ((int)s, data, size);

   if (received == 0) {
      return -1; /* On UNIX, this is sign of an error. */
   }

   return received;
}


RoadMapSocket roadmap_net_listen(int port) {

   return -1; // Not yet implemented.
}


RoadMapSocket roadmap_net_accept(RoadMapSocket server_socket) {

   return -1; // Not yet implemented.
}


void roadmap_net_close (RoadMapSocket s) {
   close ((int)s);
}

