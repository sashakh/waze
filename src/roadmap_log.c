/* roadmap_log.c - a module for managing uniform error & info messages.
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
 *   #include "roadmap.h"
 *
 *   void roadmap_log (int level, char *format, ...);
 *
 * This module is used to control and manage the appearance of messages
 * printed by the roadmap program. The goals are (1) to produce a uniform
 * look, (2) have a central point of control for error management and
 * (3) have a centralized control for routing messages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "roadmap.h"


static struct {
   int level;
   char *text;
} RoadMapMessageHead [] = {
   {ROADMAP_MESSAGE_DEBUG,   "++"},
   {ROADMAP_MESSAGE_INFO,    "--"},
   {ROADMAP_MESSAGE_WARNING, "=="},
   {ROADMAP_MESSAGE_ERROR,   "**"},
   {ROADMAP_MESSAGE_FATAL,   "##"},
   {0,                       "??"}
};

void roadmap_log (int level, char *source, int line, char *format, ...) {

   int i;
   va_list ap;

   if (level < roadmap_verbosity()) return;

   for (i = 0; RoadMapMessageHead[i].level != 0; i++) {
      if (RoadMapMessageHead[i].level == level) break;
   }

   fprintf (stderr, "%s %s, line %d: ",
                    RoadMapMessageHead[i].text, source, line);

   va_start(ap, format);
   vfprintf(stderr, format, ap);
   va_end(ap);

   fprintf (stderr, "\n");

   if (level == ROADMAP_MESSAGE_FATAL) {
      exit(1);
   }
}

