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
#include "roadmap_file.h"


#define ROADMAP_LOG_STACK_SIZE 256

static const char *RoadMapLogStack[ROADMAP_LOG_STACK_SIZE];
static int         RoadMapLogStackCursor = 0;

static struct {
   int   level;
   int   show_stack;
   int   save_to_file;
   char *prefix;
} RoadMapMessageHead [] = {
   {ROADMAP_MESSAGE_DEBUG,   0, 0, "++"},
   {ROADMAP_MESSAGE_INFO,    0, 0, "--"},
   {ROADMAP_MESSAGE_WARNING, 0, 0, "=="},
   {ROADMAP_MESSAGE_ERROR,   1, 1, "**"},
   {ROADMAP_MESSAGE_FATAL,   1, 1, "##"},
   {0,                       1, 1, "??"}
};


void roadmap_log_push (const char *description) {

   if (RoadMapLogStackCursor < ROADMAP_LOG_STACK_SIZE) {
      RoadMapLogStack[RoadMapLogStackCursor++] = description;
   }
}

void roadmap_log_pop (void) {

   if (RoadMapLogStackCursor > 0) {
      RoadMapLogStackCursor -= 1;
   }
}

void roadmap_log_reset_stack (void) {

   RoadMapLogStackCursor = 0;
}


void roadmap_log_save_all (void) {
    
    int i;
    
    for (i = 0; RoadMapMessageHead[i].level > 0; ++i) {
        RoadMapMessageHead[i].save_to_file = 1;
    }
}


void roadmap_log (int level, char *source, int line, char *format, ...) {

   int i;
   FILE *file;
   va_list ap;

   if (level < roadmap_verbosity()) return;

   for (i = 0; RoadMapMessageHead[i].level != 0; i++) {
      if (RoadMapMessageHead[i].level == level) break;
   }

   if (RoadMapMessageHead[i].save_to_file) {

      char *full_name;

      full_name = roadmap_file_join (roadmap_file_user(), "postmortem");
      file = fopen (full_name, "a");

      if (file == NULL) {
         file = stderr;
      } else {
         fprintf (stderr, "%s %s, line %d: see postmortem info in %s\n",
                  RoadMapMessageHead[i].prefix, source, line, full_name);
      }
   } else {
      file = stderr;
   }

   fprintf (file, "%s %s, line %d: ",
                  RoadMapMessageHead[i].prefix, source, line);

   va_start(ap, format);
   vfprintf(file, format, ap);
   va_end(ap);

   fprintf (file, "\n");

   if (RoadMapMessageHead[i].show_stack && RoadMapLogStackCursor > 0) {

      int indent = 8;

      fprintf (file, "   Call stack:\n");

      for (i = 0; i < RoadMapLogStackCursor; ++i) {
          fprintf (file, "%*.*s %s\n", indent, indent, "", RoadMapLogStack[i]);
          indent += 3;
      }
   }

   if (file != NULL && file != stderr) {
      fclose (file);
   }

   if (level == ROADMAP_MESSAGE_FATAL) {
      exit(1);
   }
}


void roadmap_log_purge (void) {

    char *full_name;

    full_name = roadmap_file_join (roadmap_file_user(), "postmortem");
    
    remove(full_name);
}


void roadmap_check_allocated_with_source_line
                (char *source, int line, const void *allocated) {

    if (allocated == NULL) {
        roadmap_log (ROADMAP_MESSAGE_FATAL, source, line, "no more memory");
    }
}
