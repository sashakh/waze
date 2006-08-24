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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

   RoadMapLogRedirect redirect;

} RoadMapMessageHead [] = {
   {ROADMAP_MESSAGE_DEBUG,   0, 0, 0, "++", NULL},
   {ROADMAP_MESSAGE_INFO,    0, 0, 0, "--", NULL},
   {ROADMAP_MESSAGE_WARNING, 0, 1, 0, "==", NULL},
   {ROADMAP_MESSAGE_ERROR,   1, 1, 0, "**", NULL},
   {ROADMAP_MESSAGE_FATAL,   1, 1, 1, "##", NULL},
   {0,                       1, 1, 1, "??", NULL}
};


static void roadmap_log_noredirect (const char *message) {}


static struct roadmap_message_descriptor *roadmap_log_find (int level) {

   struct roadmap_message_descriptor *category;

   for (category = RoadMapMessageHead; category->level != 0; ++category) {
      if (category->level == level) break;
   }
   return category;
}


void roadmap_log_push (const char *description) {

#ifdef DEBUG
   roadmap_log(ROADMAP_DEBUG, "push of %s", description);
#endif
   if (RoadMapLogStackCursor < ROADMAP_LOG_STACK_SIZE) {
      RoadMapLogStack[RoadMapLogStackCursor++] = description;
   }
}

void roadmap_log_pop (void) {

   if (RoadMapLogStackCursor > 0) {
      RoadMapLogStackCursor -= 1;
#ifdef DEBUG
      roadmap_log(ROADMAP_DEBUG, "pop of %s",
	 RoadMapLogStack[RoadMapLogStackCursor]);
#endif
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

   fprintf (file, "%c%s %s, line %d: ", saved, category->prefix, source, line);
   vfprintf(file, format, ap);
   fprintf (file, "\n");

   if (category->show_stack && RoadMapLogStackCursor > 0) {

      int indent = 8;

      fprintf (file, "   Call stack:\n");

      for (i = 0; i < RoadMapLogStackCursor; ++i) {
          fprintf (file, "%*.*s %s\n", indent, indent, "", RoadMapLogStack[i]);
          indent += 3;
      }
   }
}

static void roadmap_redirect_one (struct roadmap_message_descriptor *category,
                                  char *format,
                                  va_list ap) {

   char message[1024];

   if (category->redirect != NULL) {

      vsnprintf(message, sizeof(message)-1, format, ap);
      message[0] = toupper(message[0]);

      category->redirect (message);
   }
}


void roadmap_log (int level, char *source, int line, char *format, ...) {

   FILE *file;
   va_list ap;
   char saved;
   struct roadmap_message_descriptor *category;
   char **enabled;

   if (level < roadmap_verbosity()) return;

   enabled = roadmap_debug();

   if (enabled != NULL) {

      int i;
      char *debug;

      for (i = 0, debug = enabled[0]; debug != NULL; debug = enabled[++i]) {
         if (strcmp (debug, source) == 0) break;
      }

      if (debug == NULL) return;
   }

   category = roadmap_log_find (level);

   va_start(ap, format);

   saved = ' ';

   if (category->save_to_file) {

      file = roadmap_file_fopen (roadmap_path_user(), "postmortem", "sa");

      if (file != NULL) {

         roadmap_log_one (category, file, ' ', source, line, format, ap);
         fclose (file);

         va_end(ap);
         va_start(ap, format);

         saved = 's';
      }
   }

   roadmap_log_one (category, stderr, saved, source, line, format, ap);
   roadmap_redirect_one (category, format, ap);

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


RoadMapLogRedirect roadmap_log_redirect (int level,
                                         RoadMapLogRedirect redirect) {

   RoadMapLogRedirect old;
   struct roadmap_message_descriptor *category;


   category = roadmap_log_find (level);

   old = category->redirect;
   category->redirect = redirect;

   if (old == NULL) old = roadmap_log_noredirect;

   return old;
}


#ifdef ROADMAP_DBG_TIME

static unsigned long dbg_time_rec[DBG_TIME_LAST_COUNTER];
static unsigned long dbg_time_tmp[DBG_TIME_LAST_COUNTER];

#ifdef __WIN32

void dbg_time_start(int type) {
   dbg_time_tmp[type] = GetTickCount();
}

void dbg_time_end(int type) {
   dbg_time_rec[type] += GetTickCount() - dbg_time_tmp[type];
}

#else
#include <time.h>
#include <sys/time.h>

unsigned long tv_to_msec(struct timeval *tv)
{
    return (tv->tv_sec & 0xffff) * 1000 + tv->tv_usec/1000;
}

void dbg_time_start(int type) {
   struct timeval tv;
   gettimeofday(&tv, NULL);
   dbg_time_tmp[type] = tv_to_msec(&tv);
}

void dbg_time_end(int type) {
   struct timeval tv;
   gettimeofday(&tv, NULL);
   dbg_time_rec[type] += tv_to_msec(&tv) - dbg_time_tmp[type];
}

#endif // __WIN32

#endif // ROADMAP_DBG_TIME
