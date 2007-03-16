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
#include <time.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_file.h"


#define ROADMAP_LOG_STACK_SIZE 256

static const char *RoadMapLogStack[ROADMAP_LOG_STACK_SIZE];
static int         RoadMapLogStackCursor = 0;

static struct roadmap_message_descriptor {
   int   level;
   int   show_stack;
   int   save_to_file;
   int   do_exit;
   char *prefix;
} RoadMapMessageHead [] = {
   {ROADMAP_MESSAGE_DEBUG,   0, 0, 0, "++"},
   {ROADMAP_MESSAGE_INFO,    0, 0, 0, "--"},
   {ROADMAP_MESSAGE_WARNING, 0, 0, 0, "=="},
   {ROADMAP_MESSAGE_ERROR,   1, 1, 0, "**"},
   {ROADMAP_MESSAGE_FATAL,   1, 1, 1, "##"},
   {0,                       1, 1, 1, "??"}
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


void roadmap_log_save_none (void) {
    
    int i;
    
    for (i = 0; RoadMapMessageHead[i].level > 0; ++i) {
        RoadMapMessageHead[i].save_to_file = 0;
    }
    roadmap_log_purge();
}


int  roadmap_log_enabled (int level, char *source, int line) {
   return (level >= roadmap_verbosity());
}


static void roadmap_log_one (struct roadmap_message_descriptor *category,
                             FILE *file,
                             char  saved,
                             char *source,
                             int line,
                             char *format,
                             va_list ap) {

   int i;

#ifndef J2ME
#ifndef _WIN32
   time_t now;
   struct tm *tms;

   time (&now);
   tms = localtime (&now);

   printf ("%d:%d:%d %c%s %s, line %d ",
         tms->tm_hour, tms->tm_min, tms->tm_sec,
         saved, category->prefix, source, line);
#else

   SYSTEMTIME st;
	GetSystemTime(&st);

   printf ("%d/%d %d:%d:%d %c%s %s, line %d ",
         st.wDay, st.wMonth, st.wHour, st.wMinute, st.wSecond,
         saved, category->prefix, source, line);

#endif
#endif
   if (!category->show_stack && (RoadMapLogStackCursor > 0)) {
      printf ("(%s): ", RoadMapLogStack[RoadMapLogStackCursor-1]);
   }
   vprintf(format, ap);
   printf ("\n");

   if (category->show_stack && RoadMapLogStackCursor > 0) {

      int indent = 8;

      printf ("   Call stack:\n");

      for (i = 0; i < RoadMapLogStackCursor; ++i) {
          printf ("%*.*s %s\n", indent, indent, "", RoadMapLogStack[i]);
          indent += 3;
      }
   }
}

void roadmap_log (int level, char *source, int line, char *format, ...) {

#ifndef J2ME
   static FILE *file;
#endif
   va_list ap;
   char saved = ' ';
   struct roadmap_message_descriptor *category;
   char *debug;

   if (level < roadmap_verbosity()) return;

#ifdef DEBUG
   return;
#endif

   debug = roadmap_debug();

   if ((debug[0] != 0) && (strcmp (debug, source) != 0)) return;

   for (category = RoadMapMessageHead; category->level != 0; ++category) {
      if (category->level == level) break;
   }

   va_start(ap, format);

#ifndef J2ME
   if (category->save_to_file) {

      if (file == NULL) {

         file = roadmap_file_fopen (roadmap_path_user(), "postmortem", "sa");
      }

      if (file != NULL) {

         roadmap_log_one (category, file, ' ', source, line, format, ap);
         fflush (file);
         //fclose (file);

         va_end(ap);
         va_start(ap, format);

         saved = 's';
      }
   }
#endif
   roadmap_log_one (category, stderr, saved, source, line, format, ap);

   va_end(ap);

   if (category->do_exit) exit(1);
}


void roadmap_log_purge (void) {

    roadmap_file_remove (roadmap_path_user(), "postmortem");
}


void roadmap_check_allocated_with_source_line
                (char *source, int line, const void *allocated) {

    if (allocated == NULL) {
        roadmap_log (ROADMAP_MESSAGE_FATAL, source, line, "no more memory");
    }
}
