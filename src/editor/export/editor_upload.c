/* editor_upload.c - Upload a gpx file to freemap.co.il
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 * SYNOPSYS:
 *
 *   See editor_upload.h
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_net.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_httpcopy.h"
#include "roadmap_messagebox.h"
#include "roadmap_dialog.h"
#include "roadmap_config.h"
#include "roadmap_main.h"
#include "roadmap_fileselection.h"

#include "editor_upload.h"

static FILE *dbg_file;

#define ROADMAP_HTTP_MAX_CHUNK 4096

static RoadMapConfigDescriptor RoadMapConfigTarget =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Target");

static RoadMapConfigDescriptor RoadMapConfigUser =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "User Name");

static RoadMapConfigDescriptor RoadMapConfigPassword =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Password");

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char *RoadMapUploadCurrentName = "";
static int  RoadMapUploadCurrentFileSize = 0;
static int  RoadMapUploadUploaded = -1;


static void encodeblock (unsigned char in[3], unsigned char out[4], int len) {
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}


static int editor_upload_request (int size) {

   RoadMapUploadCurrentFileSize = size;
   RoadMapUploadUploaded = -1;
   return 1;
}


static void editor_upload_format_size (char *image, int value) {

   if (value > (10 * 1024 * 1024)) {
      sprintf (image, "%dMB", value / (1024 * 1024));
   } else {
      sprintf (image, "%dKB", value / 1024);
   }
}


static void editor_upload_error (const char *format, ...) {

   va_list ap;
   char message[2048];

   va_start(ap, format);
   vsnprintf (message, sizeof(message), format, ap);
   va_end(ap);

   roadmap_messagebox ("Upload Error", message);
}


static void editor_upload_progress (int loaded) {

   char image[32];

   int progress = (100 * loaded) / RoadMapUploadCurrentFileSize;


   /* Avoid updating the dialog too often: this may slowdown the upload. */

   if (progress == RoadMapUploadUploaded) return;

   RoadMapUploadUploaded = progress;

   if (roadmap_dialog_activate ("Uploading", NULL)) {

      roadmap_dialog_new_label  (".file", "Name");
      roadmap_dialog_new_label  (".file", "Size");
      roadmap_dialog_new_label  (".file", "Upload");

      roadmap_dialog_complete (0);
   }

   roadmap_dialog_set_data (".file", "Name", RoadMapUploadCurrentName);

   editor_upload_format_size (image, RoadMapUploadCurrentFileSize);
   roadmap_dialog_set_data (".file", "Size", image);

   editor_upload_format_size (image, loaded);
   roadmap_dialog_set_data (".file", "Upload", image);

   roadmap_main_flush ();
}


#if 0
static RoadMapDownloadCallbacks RoadMapUploadCallbackFunctions = {
   roadmap_upload_request,
   roadmap_upload_progress,
   roadmap_upload_error
};
#endif

static int editor_http_send (RoadMapSocket socket,
                              RoadMapDownloadCallbackError error,
                              const char *format, ...)
{
   va_list ap;
   int     length;
   char    buffer[ROADMAP_HTTP_MAX_CHUNK];
   
   if (!dbg_file) dbg_file = fopen ("/tmp/post3.gpx", "w");
   va_start(ap, format);
   vsnprintf (buffer, sizeof(buffer), format, ap);
   va_end(ap);

   length = strlen(buffer);
   fwrite (buffer, length, 1, dbg_file);
   if (roadmap_net_send (socket, buffer, length) != length) {
      error ("send error on: %s", buffer);
      return 0;
   }
   return length;
}


static void send_auth(const char *user, const char *pw, RoadMapSocket fd,
                      RoadMapDownloadCallbackError error) {
    unsigned char in[3], out[4];
    int i, len;
    char auth_string[255];
    char auth_encoded[sizeof(auth_string) * 2];
    char *itr = auth_string;
    char *encoded_itr = auth_encoded;
    
    snprintf (auth_string, sizeof(auth_string), "%s:%s", user, pw);
    auth_string[sizeof(auth_string) - 1] = 0;

    while(*itr) {
        len = 0;
        for( i = 0; i < 3; i++ ) {
            in[i] = (unsigned char) *itr;
            if (*itr) {
               itr++;
               len++;
            }
        }

        if( len ) {
            encodeblock( in, out, len );
            for( i = 0; i < 4; i++ ) {
                *encoded_itr++ = out[i];
            }
        }
    }
    *encoded_itr = 0;

    editor_http_send (fd, error, "Authorization: Basic %s\r\n", auth_encoded);
}


static int editor_http_send_header (const char *target,
                                     const char *full_name,
                                     size_t size,
                                     const char *user,
                                     const char *pw,
                                     RoadMapDownloadCallbackError error) {

   RoadMapSocket fd;
   char *host;
   char *path;


   /* Get a local copy of the URL, for us to decode. */

   target += 7; /* Skip the "http://" prefix. */

   host = strdup(target);
   if (host == NULL) {
      error ("no more memory");
      return 0;
   }


   /* Separate the file path from the server's address. */

   path = strchr (host, '/');
   if (path == NULL) {
      error ("bad URL syntax in: %s", host);
      free (host);
      return 0;
   }
   *path = 0; /* Isolate the host string. */

   path = strchr (target, '/');
   if (path == NULL) {
      error ("bad URL syntax in: %s", target);
      free (host);
      return 0;
   }

   /* Connect to the server (roadmap_net understands the "host:port"
    * syntax).
    */
   fd = roadmap_net_connect ("tcp", host, 80);
   if (fd >= 0) {
      const char *filename = roadmap_path_skip_directories (full_name);
      char *p = strchr(host, ':');

      if (p != NULL) *p = 0; /* remove the port/service info. */

      editor_http_send (fd, error, "POST %s HTTP/1.0\r\n", path);
      editor_http_send (fd, error, "Host: %s\r\n", host);
      
      send_auth (user, pw, fd, error);

      editor_http_send (fd, error, "Content-Type: multipart/form-data; boundary=---------------------------10424402741337131014341297293\r\n");
      editor_http_send (fd, error, "Content-Length: %d\r\n", size + 261 + strlen(filename));
      editor_http_send (fd, error, "\r\n");
      editor_http_send (fd, error, "-----------------------------10424402741337131014341297293\r\n");
      editor_http_send (fd, error, "Content-disposition: form-data; name=\"file_0\"; filename=\"%s\"\r\n", filename);
      editor_http_send (fd, error, "Content-type: application/octet-stream\r\n");
      editor_http_send (fd, error, "Content-Transfer-Encoding: binary\r\n\r\n");
   }

   free (host);

   return fd;
}


static int editor_http_decode_response (RoadMapSocket fd,
                                        char *buffer,
                                        int  *sizeof_buffer,
                                        RoadMapDownloadCallbackError error) {

   int  received_status = 0;
   int  shift;

   int  size;
   int  total;
   int  received;

   char *p;
   char *next;

   size = 0;
   total = 0;

   for (;;) {

      buffer[total] = 0;
      if (strchr (buffer, '\n') == NULL) {

         /* We do not have a full line: we need more data. */

         received =
            roadmap_net_receive
               (fd, buffer + total, *sizeof_buffer - total - 1);

         if (received <= 0) {
            error ("Receive error");
            return 0;
         }
         total += received;
         buffer[total] = 0;
      }

      shift = 2;
      next = strstr (buffer, "\r\n");
      if (next == NULL) {
         shift = 1;
         next = strchr (buffer, '\n');
      }

      if (next != NULL) {

         *next = 0;

         if (! received_status) {

            if (next != buffer) {
               if (strstr (buffer, " 200 ") == NULL) {
                  error ("received bad status: %s", buffer);
                  return 0;
               }
               received_status = 1;
            }

         } else {

            if (next == buffer) {

               /* An empty line signals the end of the header. Any
                * reminder data is part of the upload: save it.
                */
               next += shift;
               received = (buffer + total) - next;
               if (received) memcpy (buffer, next, received);
               *sizeof_buffer = received;

               return size;
            }

            if (strncmp (buffer,
                        "Content-Length", sizeof("Content-Length")-1) == 0) {

               p = strchr (buffer, ':');
               if (p == NULL) {
                  error ("bad formed header: %s", buffer);
                  return 0;
               }

               while (*(++p) == ' ') ;
               size = atoi(p);
               if (size <= 0) {
                  error ("bad formed header: %s", buffer);
                  return 0;
               }
            }
         }

         /* Move the remaining data to the beginning of the buffer
          * and wait for more.
          */
         next += shift;
         received = (buffer + total) - next;
         if (received) memcpy (buffer, next, received);
         total = received;
      }
   }

   error ("No valid header received");
   return 0;
}


static int editor_post_file (const char *target,
                             const char *file_name,
                             const char *user_name,
                             const char *password) {

   RoadMapSocket fd;
   int size;
   int loaded;
   int uploaded;

   char buffer[ROADMAP_HTTP_MAX_CHUNK];

   RoadMapFile file = roadmap_file_open (file_name, "r");

   if (!ROADMAP_FILE_IS_VALID(file)) {
      editor_upload_error ("Can't open file: %s\n", file_name);
      return 0;
   }

   size = roadmap_file_length (NULL, file_name);

   editor_upload_request (size);
   RoadMapUploadCurrentName = file_name;

   fd = editor_http_send_header (target, file_name, size, user_name, password, editor_upload_error);
   if (fd < 0) {
      return 0;
   }

   uploaded = 0;
   editor_upload_progress (uploaded);

   loaded = uploaded;

   while (loaded < size) {

      uploaded = roadmap_file_read (file, buffer, sizeof(buffer));
   fwrite (buffer, uploaded, 1, dbg_file);
      uploaded = roadmap_net_send (fd, buffer, uploaded);

      if (uploaded <= 0) {
         editor_upload_error ("Receive error after %d data bytes", loaded);
         goto cancel_upload;
      }
      loaded += uploaded;

   editor_upload_progress (loaded);
   }

   editor_http_send (fd, editor_upload_error, "\r\n-----------------------------10424402741337131014341297293--\r\n");
   fflush (dbg_file);
   if (!editor_http_decode_response
             (fd, buffer, &loaded, editor_upload_error)) {
      goto cancel_upload;
   }

   roadmap_net_close (fd);
   roadmap_dialog_hide ("Uploading");
   buffer[loaded] = 0;

   roadmap_messagebox ("Upload done.", buffer);
   return 1;

cancel_upload:

   roadmap_net_close (fd);
   roadmap_dialog_hide ("Uploading");

   return 0;
}


static void editor_upload_ok (const char *name, void *context) {

   const char *filename;
   const char *target;
   const char *username;
   const char *password;

   filename = roadmap_dialog_get_data (".file", "Name");

   target = roadmap_dialog_get_data (".file", "To");
   roadmap_config_set (&RoadMapConfigTarget, target);

   username = roadmap_dialog_get_data (".file", "User Name");
   roadmap_config_set (&RoadMapConfigUser, username);

   password = roadmap_dialog_get_data (".file", "Password");
   roadmap_config_set (&RoadMapConfigPassword, password);

   roadmap_dialog_hide (name);

   editor_post_file (target, filename, username, password);
}


static void editor_upload_cancel (const char *name, void *context) {

   roadmap_dialog_hide (name);
}


void editor_upload_file (const char *filename) {

   static char s_file[500];
   strncpy (s_file, filename, sizeof(s_file));
   s_file[sizeof(s_file)-1] = 0;

   if (roadmap_dialog_activate ("Upload gpx file", NULL)) {

      roadmap_dialog_new_label  (".file", "Name");
      roadmap_dialog_new_entry  (".file", "To", NULL);
      roadmap_dialog_new_entry  (".file", "User Name", NULL);
      roadmap_dialog_new_entry  (".file", "Password", NULL);

      roadmap_dialog_add_button ("OK", editor_upload_ok);
      roadmap_dialog_add_button ("Cancel", editor_upload_cancel);

      roadmap_dialog_complete (0);
   }

   roadmap_dialog_set_data (".file", "Name", s_file);

   roadmap_dialog_set_data (".file", "To",
         roadmap_config_get (&RoadMapConfigTarget));
   roadmap_dialog_set_data (".file", "User Name",
         roadmap_config_get (&RoadMapConfigUser));
   roadmap_dialog_set_data (".file", "Password",
         roadmap_config_get (&RoadMapConfigPassword));

}


static void editor_upload_file_dialog_ok
                           (const char *filename, const char *mode) {

   editor_upload_file (filename);
}


void editor_upload_select (void) {
                                
   roadmap_fileselection_new ("Upload file",
                              "gpx",
                              roadmap_path_user (),
                              "r",
                              editor_upload_file_dialog_ok);
}

void editor_upload_initialize (void) {

   roadmap_config_declare
      ("preferences",
      &RoadMapConfigTarget,
      "http://www.freemap.co.il/upload/upload_rm.php");

   roadmap_config_declare ("preferences", &RoadMapConfigUser, "");
   roadmap_config_declare ("preferences", &RoadMapConfigPassword, "");
}

