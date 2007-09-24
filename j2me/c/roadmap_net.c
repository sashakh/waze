/* roadmap_net.c - Network interface for the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <javax/microedition/io.h>

#include "roadmap.h"
#include "roadmap_net.h"


struct roadmap_socket_t {
   int http;
   NOPH_Connection_t conn;
   NOPH_InputStream_t is;
   NOPH_OutputStream_t os;
};


/*
static void NOPH_setter_exception_handler(NOPH_Exception_t ex, void *arg)
{
  int *p = (int*)arg;
  *p = 1;

  NOPH_delete(ex);
}
*/


RoadMapSocket roadmap_net_connect (const char *protocol,
                                   const char *name, int default_port) {

   char url[255];
   char *separator = strchr (name, ':');
   char *hostname = strdup(name);
   RoadMapSocket s = (RoadMapSocket)calloc(1, sizeof(struct roadmap_socket_t));

   int error = 0;

   roadmap_check_allocated(hostname);
   roadmap_check_allocated(s);

   if (separator != NULL) {

      if (isdigit(separator[1])) {

         default_port = atoi(separator+1);

      }

      *(strchr(hostname, ':')) = 0;
   }

   if (strcmp (protocol, "http") == 0) {
      NOPH_try(NOPH_setter_exception_handler, (void*)&error) {
         s->http = 1;
         snprintf(url, sizeof(url), "http://%s", name);
         s->conn = NOPH_Connector_open(url);
         NOPH_HttpConnection_setRequestMethod((NOPH_HttpConnection_t)s->conn, "POST");
      } NOPH_catch();

      if (error || !s->conn) goto connection_failure;
   } else {

      snprintf (url, sizeof(url), "socket://%s:%d", hostname, default_port);

      if (strcmp (protocol, "udp") == 0) {
         snprintf (url, sizeof(url), "datagram://%s:%d", hostname, default_port);
      } else if (strcmp (protocol, "tcp") == 0) {
         snprintf (url, sizeof(url), "socket://%s:%d", hostname, default_port);
      } else {
         roadmap_log (ROADMAP_ERROR, "unknown protocol %s", protocol);
         goto connection_failure;
      }

      /* FIXME: this way of establishing the connection is kind of dangerous
       * if the server process is not local: we might fail only after a long
       * delay that will disable RoadMap for a while.
       */
      NOPH_try(NOPH_setter_exception_handler, (void*)&error) {
         s->conn = NOPH_Connector_open(url);
         s->is = NOPH_SocketConnection_openInputStream((NOPH_SocketConnection_t)s->conn);
         s->os = NOPH_SocketConnection_openOutputStream((NOPH_SocketConnection_t)s->conn);
      } NOPH_catch();

      if (error || !s->conn || !s->is || !s->os) goto connection_failure;
   }

   free(hostname);
   return s;


connection_failure:

   free(hostname);
   roadmap_net_close (s);
   return (RoadMapSocket)NULL;
}


int roadmap_net_send (RoadMapSocket s, const void *ptr, int in_size, int wait) {

  char *c;

  if (s->http && !s->os) {
     char *line = strdup((const char *)ptr);
     char *value;
     if (strcmp(line, "\r\n") == 0) {
        s->os = NOPH_HttpConnection_openOutputStream((NOPH_HttpConnection_t)s->conn);
        free (line);
        return in_size;
     }

     value = strchr(line, ':');
     *value = '\0';
     value++;
     while (*value == ' ') value++;
     *(strchr(value, '\r')) = '\0';

     NOPH_HttpConnection_setRequestProperty ((NOPH_HttpConnection_t)s->conn, line, value);
     free (line);
     return in_size;
  }

  /* One byte at a time */
  for ( c = (char *)ptr; c < (char*)ptr + in_size; c++)
    NOPH_OutputStream_write(s->os, *c);

  return in_size;
}


int roadmap_net_receive (RoadMapSocket s, void *ptr, int in_size) {

  size_t count = 0;
  size_t data_size = 0;
  short eof = 0;

  if (s->http && !s->is) {
     int status;

     s->is = NOPH_HttpConnection_openInputStream((NOPH_HttpConnection_t)s->conn);
     status = NOPH_HttpConnection_getResponseCode((NOPH_HttpConnection_t)s->conn);
     data_size = NOPH_HttpConnection_getLength((NOPH_HttpConnection_t)s->conn);

     return snprintf((char *)ptr, in_size, "HTTP/1.1 %d STATUS\r\nContent-Length: %d\r\n\r\n", status, data_size);
  }

  /* Cached file reading - read into a temporary buffer */
  while (in_size > 0)
    {
      size_t size = in_size < 4096 ? in_size : 4096;
      int n;

      /* Read the data into the buffer, potentially setting fp->eof */
      n = NOPH_InputStream_read_into(s->is, ptr, size, &eof);
      count += n;
      in_size -= n;
      ptr += n;
      if (eof)
        break;
    }

  return count;
}


RoadMapSocket roadmap_net_listen(int port) {

   return NULL; // Not yet implemented.
}


RoadMapSocket roadmap_net_accept(RoadMapSocket server_socket) {

   return NULL; // Not yet implemented.
}


void roadmap_net_close (RoadMapSocket s) {
   if (s->os) NOPH_OutputStream_close(s->os);
   if (s->is) NOPH_InputStream_close(s->is);
   if (s->conn) {
      if (s->http) NOPH_HttpConnection_close(s->conn);
      else NOPH_SocketConnection_close(s->conn);
   }

   free (s);
}

