/* roadmap_input.c - Decode an ASCII data stream.
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
 *   See roadmap_input.h
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _WIN32
#include <errno.h>
#endif

#include "roadmap.h"
#include "roadmap_input.h"


static char *roadmap_input_end_of_line (RoadMapInputContext *context,
                                        char *from) {

   char *data_end = context->data + context->cursor;

   for (; from < data_end; ++from) {

      if (*from < ' ') {

         if (*from == '\n' || *from == '\r') {
            return from;
         }
      }
   }

   return NULL;
}


static void roadmap_input_shift_to_next_line (RoadMapInputContext *context,
                                              char *from) {

   char *data_end = context->data + context->cursor;

   /* Skip the trailing end of line characters, if any. */
   while ((*from < ' ') && (from < data_end)) ++from;

   if (from > context->data) {
         
      context->cursor -= (int)(from - context->data);

      if (context->cursor <= 0) {
         context->cursor = 0; /* Play it safe. */
         return;
      }

      memmove (context->data, from, context->cursor);
      context->data[context->cursor] = 0;
   }
}


int roadmap_input_split (char *text, char separator, char *field[], int max) {

   int   i;
   char *p;

   field[0] = text;
   p = strchr (text, separator);

   for (i = 1; p != NULL && *p != 0; ++i) {

      *p = 0;
      if (i >= max) return -1;

      field[i] = ++p;
      p = strchr (p, separator);
   }
   return i;
}


int roadmap_input (RoadMapInputContext *context) {

   int result;
   int received;

   char *line_start;
   char *data_end;


   /* Receive more data if available. */

   if (context->cursor >= (int)sizeof(context->data) - 1) {
      /* This buffer is full. As a stop-gap measure, let
       * empty it now.
       */
      context->cursor = 0;
      context->data[0] = 0;
   }

   if (context->cursor < (int)sizeof(context->data) - 1) {

      received =
         roadmap_io_read (context->io,
                          context->data + context->cursor,
                          sizeof(context->data) - context->cursor - 1);

      if (received < 0) {

         /* We lost that connection. */
         roadmap_log (ROADMAP_INFO, "lost %s", context->title);
         return -1;

      } else {

         context->data[context->cursor + received] = 0;
         roadmap_log (ROADMAP_DEBUG,
                      "received: '%s'", context->data + context->cursor);

         context->cursor += received;
      }
   }


   /* Remove the leading end of line characters, if any.
    * We do the same in between lines (see end of loop).
    */
   line_start = context->data;
   data_end   = context->data + context->cursor;
   while ((*line_start < ' ') && (line_start < data_end)) ++line_start;


   /* process each complete line in this buffer. */

   result = 0;

   while (line_start < data_end) {

      char *line_end;


      /* Find the first end of line character coming after this line. */

      line_end = roadmap_input_end_of_line (context, line_start);

      if (line_end == NULL) {

         /* This line is not complete: shift the remaining data
          * to the beginning of the buffer and then stop.
          */
         roadmap_input_shift_to_next_line (context, line_start);
         roadmap_log (ROADMAP_DEBUG, "data left: '%s'", context->data);
         return result;
      }


      /* Process this line. */

      *line_end = 0; /* Separate this line from the next. */

      roadmap_log (ROADMAP_DEBUG, "processing: '%s'", line_start);

      if (context->logger != NULL) {
         context->logger (line_start);
      }
      result |= context->decoder (context->user_context,
                                  context->decoder_context, line_start);


      /* Move to the next line. */

      line_start = line_end + 1;
      while ((*line_start < ' ') && (line_start < data_end)) ++line_start;
   }


   context->cursor = 0; /* The buffer is now empty. */
   context->data[0] = 0;

   roadmap_log (ROADMAP_DEBUG, "buffer empty");
   return result;
}

