/* roadmap_input.h - Decode an ASCII data stream.
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
 * PURPOSE:
 *
 *   This module is used when decoding line-oriented ASCII data input.
 *   The caller's decoder is called for each line received, except for
 *   empty lines and comments (if enabled).
 */

#ifndef INCLUDED__ROADMAP_INPUT__H
#define INCLUDED__ROADMAP_INPUT__H

typedef void (*RoadMapInputLogger)  (const char *data);
typedef int  (*RoadMapInputReceive) (void *context, char *data, int size);
typedef int  (*RoadMapInputDecode)  (void *context, char *line);

typedef struct roadmap_input_context {

   const char *title;
   void       *user_context;

   RoadMapInputLogger  logger;
   RoadMapInputReceive receiver;
   RoadMapInputDecode  decoder;

   char data[1024];
   int  cursor;

} RoadMapInputContext;

int roadmap_input_split (char *text, char separator, char *field[], int max);

int roadmap_input (RoadMapInputContext *context);

#endif // INCLUDED__ROADMAP_INPUT__H

