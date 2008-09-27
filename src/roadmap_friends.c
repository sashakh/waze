/* roadmap_friends.c - RoadMap driver for GpsDrive's friends protocol.
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
 *   This program is a partial implementation of the GpsDrive's friends
 *   protocol (reverse engineered from GpsDrive version 2.09).
 *
 *   It is my understanding that the friend protocol is defined as follow:
 *
 *   Assigned numbers: port addres is 50123
 *
 *   Protocol data units:
 *
 *   All data units are represented by a line of ASCII text ending with '\n'.
 *   Each PDU is transported sperately in its own UDP packet.
 *
 *   SND: id name text
 *
 *         Send a text message to station "name". This message is sent by
 *         the client and echoed by the server.
 *
 *         id: unique identifier of the message.
 *         name: message destination (i.e. name of a friend).
 *
 *   POS: id name latitude longitude time speed direction
 *
 *         Advertise the current position of the named friend. This message
 *         is sent by the client and echoed by the server.
 *
 *         id: unique identifier of the message (string).
 *         name: message source (string).
 *         latitude, longitude: position of the source, decimal degrees.
 *         time: system time when the message was originated (integer).
 *         speed: speed of the source, in km/h (integer).
 *         direction: heading of the source, degree (integer).
 *
 *   SRV: id name latitude longitude time speed direction
 *
 *         Advertise the current position of the friends server. This PDU
 *         is sent by the server only. It is otherwise the same as POS.
 *
 *   ACK: id
 *
 *         Acknowledge the message indentified by its id. This PDU is sent
 *         by the client only.
 *
 *   $START:$
 *
 *         Beginning of the server's transmission. This PDU is sent by the
 *         server only.
 *
 *   $END:$
 *
 *         End of the server's transmission. This PDU is sent by the server
 *         only.
 *
 *   Data flow:
 *
 *   The friend server receives UDP messages from all friends clients. It
 *   memorizes each message for a defined amount of time, or until the
 *   message has been acknowledged (assuming the clients only acknowledge
 *   the "SND" messages). The server echo all messages to each client after
 *   having received and processed any message from that client.
 *
 *   The friend client sends its position periodically, or a text message.
 *   It is not obvious when the client code acknowledge a text message, but
 *   we will suppose it does, since this is the only way not to receive the
 *   same message again and again.
 *
 *   After sending its message, the friends client waits for the server
 *   to reply back with the list of all messages. This is where the protocol
 *   assumes that UDP is a reliable transport, i.e. the packets arrive in
 *   the correct order (probably true for a direct WIFI connection) and there
 *   is no loss of message (a risky proposition..).
 *
 *   Variations:
 *
 *   The RoadMap friends driver differs from the GpsDrive's friends client
 *   as follow:
 *
 *   1- the RoadMap driver is asynchronous and event driven. There is no
 *   assumption that the server transmission occurs as a reply to our own
 *   messages and the driver accepts server messages at any time.
 *
 *   2- the $STAT:$ and $END:$ message are ignored. They are useless anyway.
 *
 *   3- a text message sent to the local user is always acknowledged each
 *   time it is received. The driver memorizes the last text message PDU
 *   received so that it will ignore repeats (takes care of loosing the ACK).
 *
 *   4- the driver assumes that messages may arrive in any order.
 *
 *   5- the driver supports other port numbers.
 *
 *   6- The driver assumes that we are using the US real numbers format
 *   (i.e. a '.' and not a ',' to separate the decimal part).
 *
 *   This program is normally launched by RoadMap.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/select.h>
#endif

#include "roadmap.h"
#include "roadmap_net.h"


static char *friends_local_name = NULL;

static char  **friends_names = NULL;
static time_t *friends_detected = NULL;
static int     friends_max = 0;
static int     friends_count = 0;


static int dec2bin (char c) {

   if ((c >= '0') && (c <= '9')) {
      return c - '0';
   }

   /* Invalid character. */
   roadmap_log (ROADMAP_ERROR, "invalid decimal character '%c'", c);
   return 0;
}


static double coordinate_to_friends (char *value, char *hemispher) {

   int degrees;
   double result;

   char *end;
   char *dot = strchr (value, '.');

   if (dot == NULL) {
      if (value[0] == 0) return 0;
      dot = value + strlen(value);
   }

   end = dot - 2;

   degrees = 0;
   while (value < end) {
      degrees = dec2bin(*value) + (10 * degrees);
      ++value;
   }

   if (*dot == '.') {
      result = atof(value);
   } else {
      result = 1.0 * atoi(value);
   }
   result = (result / 60) + degrees;

   if (*hemispher == 'W' || *hemispher == 'S') {

         return 0.0 - result;
   }

   return result;
}


static void send_position_to_friends (char *sentence, int socket) {

   int   argc = 0;
   char *argv[16];

   char buffer[1024];

   while (sentence) {

      argv[argc++] = sentence;

      sentence = strchr (sentence, ',');
      if (sentence != NULL) {
         *(sentence++) = 0;
      }
   }

   snprintf (buffer, sizeof(buffer), "POS: %s %s %.6f %.6f %ld %d %d",
             friends_local_name, friends_local_name,
             coordinate_to_friends (argv[3], argv[4]),
             coordinate_to_friends (argv[5], argv[6]),
             time(NULL),
             atoi(argv[7]),
             atoi(argv[8]));

   roadmap_net_send (socket,  buffer, strlen(buffer), 1);
}


static void coordinate_to_nmea (double value, int *ddmm, int *mmmm) {

   int degrees;
   int minutes;
   int value_i = (int) (value * 1000000);

   value_i = abs(value_i);
   degrees = value_i / 1000000;
   value_i = 60 * (value_i % 1000000);
   minutes = value_i / 1000000;

   *ddmm = degrees * 100 + minutes;
   *mmmm = (value_i % 1000000) / 100;
}

static void forward_position_from_friends (char *friend_position) {

   char id[30];
   char name[40];
   double latitude;
   double longitude;
   int latitude_ddmm;
   int latitude_mmmm;
   int longitude_ddmm;
   int longitude_mmmm;
   int speed;
   int steering;
   int friend_time;

   sscanf (friend_position,
           "%s %s %lf %lf %d %d %d", id,
           name,
           &latitude,
           &longitude,
           &friend_time,
           &speed,
           &steering);

   /* Ignore our own position. */

   if (strcmp (name, friends_local_name) != 0) {

      int i = friends_count;
      int available = -1;

      if (friends_names != NULL) {
         for (i = 0; i < friends_count; ++i) {
            if (friends_names[i] == NULL) {
               available = i;
            } else if (strcmp (name, friends_names[i]) == 0) {
               break;
            }
         }
      }

      if (i < friends_count) {

         /* Old friend. */

         friends_detected[i] = time(NULL);

      } else {

         /* New friend. */

         if (available >= 0) {

            friends_names[available] = strdup(name);
            friends_detected[available] = time(NULL);

         } else {
            
            if (friends_count >= friends_max) {

               friends_max += 10;
               friends_names = realloc (friends_names,
                                        sizeof(char *) * friends_max);
               friends_detected = realloc (friends_detected,
                                           sizeof(time_t) * friends_max);
            }
            friends_names[friends_count] = strdup(name);
            friends_detected[friends_count] = time(NULL);
            ++friends_count;
         }

         /* Announce the new friend to Roadmap. */

         printf ("$PXRMADD,%s,%s,Friend\n", name, name);
      }

      /* Announce this friend's new position to RoadMap. */

      coordinate_to_nmea (latitude, &latitude_ddmm, &latitude_mmmm);
      coordinate_to_nmea (longitude, &longitude_ddmm, &longitude_mmmm);

      printf ("$PXRMMOV,%s,%d.%04d,%c,%d.%04d,%c,%d,%d\n",
              name,
              latitude_ddmm, latitude_mmmm, latitude >= 0.0 ? 'N' : 'S',
              longitude_ddmm, longitude_mmmm, longitude >= 0.0 ? 'E' : 'W',
              speed,
              steering);
      fflush(stdout);
   }
}


static void detect_defunct_friends (void) {

   int i;
   time_t deadline = time(NULL) - 30;

   for (i = 0; i < friends_count; ++i) {
      if (friends_names[i] != NULL) {
         if (friends_detected[i] < deadline) {

            /* Announce this friend's disapearance to RoadMap. */
            printf ("$PXRMDEL,%s\n", friends_names[i]);

            free(friends_names[i]);
            friends_names[i] = NULL;
         }
      }
   }
   fflush(stdout);
}


static void remove_end_of_line (char *buffer) {

   char *p = strchr (buffer, '\n');

   if (p != NULL) *p = 0;

   p = strchr (buffer, '\r');

   if (p != NULL) *p = 0;
}


int main(int argc, char *argv[]) {

   char *driver = "Friends";

   char config_name[256];
   int  config_name_length;

   char config_server[256];
   int  config_server_length;

   int socket = -1;
   int fdcount = 1;
   int received;

   char host_name[128];
   char buffer[2048];   /* Probably larger than anything we might receive. */


   if (argc > 1 && strncmp (argv[1], "--driver=", 9) == 0) {
      driver = argv[1] + 9;
   }
   snprintf (config_name, sizeof(config_name), "$PXRMCFG,%s,Name,", driver);
   config_name_length = strlen(config_name);

   snprintf (config_server,
             sizeof(config_server), "$PXRMCFG,%s,Server,", driver);
   config_server_length = strlen(config_server);


   if (gethostname (host_name, sizeof(host_name)) < 0) {
      fprintf (stderr, "FATAL error: gethostname() failed\n");
      exit(1);
   }

   /* Note we send only one configuration inquiry at a time. Doing
    * otherwise would lead to 2 lines received at once, something
    * this simplistic code does not handle.
    */
   printf ("$PXRMSUB,RMC\n");
   printf ("%s%s\n", config_name, host_name);
   fflush(stdout);


   for(;;) {

      fd_set reads;

      FD_ZERO(&reads);
      FD_SET(0, &reads);      /* config from RoadMap. */
      if (socket > 0) FD_SET(socket, &reads); /* from the friends server. */

      if (select (fdcount, &reads, NULL, NULL, NULL) == 0) continue;

      if (FD_ISSET(0, &reads)) {

         /* We received a message from RoadMap. ------------------------ */

         received = read (0, buffer, sizeof(buffer));

         if (received <= 0) {
            exit(0); /* RoadMap cut the pipe. */
         }
         buffer[received] = 0;
         remove_end_of_line (buffer);

         if (strncmp (buffer, "$GPRMC,", 7) == 0) {

            /* decode this position and forward a POS message to
             * the friends server, if any.
             */
            if (socket > 0) {
               send_position_to_friends (buffer, socket);
            }
         }
         else if (strncmp (buffer, config_name, config_name_length) == 0) {

            friends_local_name = strdup(buffer + config_name_length);

            /* Send the next configuration inquiry now. */

            printf ("%s%s\n", config_server, host_name);
            fflush(stdout);
         }
         else if (strncmp (buffer, config_server, config_server_length) == 0) {

            char *servername = buffer + config_server_length;
            char *p = strchr (buffer, '\n');

            if (p != NULL) *p = 0;

            /* Create the UDP socket used to communicate with the friends
             * server.
             */
            socket = roadmap_net_connect ("udp", servername, 50123);
            if (socket > 0) {
               fdcount = socket + 1;
            }
         }
         else {

            /* Ignore. */
         }

         detect_defunct_friends ();
      }

      if (socket > 0 && FD_ISSET(socket, &reads)) {

         /* We received a message from the friends server. ------------- */

         received = roadmap_net_receive (socket, buffer, sizeof(buffer));

         if (received <= 0) {
            roadmap_net_close (socket);
            exit(1); /* No more friends, life is not worth it anymore.. */
         }
         buffer[received] = 0;
         remove_end_of_line (buffer);

         if (strncmp (buffer, "SND: ", 5) == 0) {

            /* This is a text message. If we are the destination, acknowledge
             * and forward to RoadMap (but only if this is not a repeat).
             * NOTE THAT WE DO NOT FORWARD THE TEXT MESSAGE: TBD.
             */
            char *id = buffer + 5;
            char *p = strchr (id, ' ');

            if (p != NULL) {
               p[0] = '\n';
               p[1] = 0;
            }
            buffer[0] = 'A'; buffer[1] = 'C'; buffer[2] = 'K';
            roadmap_net_send (socket,  buffer, strlen(buffer), 1);

         } else if (strncmp (buffer, "POS: ", 5) == 0 ||
                    strncmp (buffer, "SRV: ", 5) == 0) {

            forward_position_from_friends (buffer + 5);
         }

         if (ferror(stdout)) {
            exit(0); /* RoadMap closed the pipe. */
         }
      }
   }
   return 0; /* Some compilers might not detect the forever loop. */
}

