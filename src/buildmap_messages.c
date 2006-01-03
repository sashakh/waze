/* buildmap_messages.c - a module for managing uniform error & info messages.
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
 *   void buildmap_set_source (char *name);
 *   void buildmap_set_line (int line);
 *   void buildmap_error (int column, char *format, ...);
 *   void buildmap_fatal (int column, char *format, ...);
 *   void buildmap_progress (int done, int estimated);
 *   void buildmap_info (char *format, ...);
 *   void buildmap_summary (int verbose, char *format, ...);
 *   int buildmap_get_error_count (void);
 *   int buildmap_get_error_total (void);
 *
 * This module is used to control and manage the appearance of messages
 * printed by the buildmap tool. The goals are (1) to produce a uniform
 * look, (2) have a central point of control for error management and
 * (3) have a centralized control for routing messages.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "buildmap.h"


static char *SourceFile = NULL;
static int   SourceLine = 0;
static int   ErrorCount = 0;
static int   ErrorTotal = 0;
static int   LastProgress = 0;


void buildmap_set_source (const char *name) {

   const char *p;

   /* Get the file's base name (for error display purpose). */

   p = strrchr (name, '/');

   if (p == NULL) {
      p = name;
   } else {
      p += 1;
   }

   if (SourceFile != NULL) {
      free(SourceFile);
   }
   SourceFile = strdup(p);

   SourceLine = 0;
   LastProgress = 0;
}

void buildmap_set_line (int line) {

   SourceLine = line;
}


static void buildmap_show_source (FILE *output,
                                  const char *preamble, int column) {

   if (SourceFile == NULL) {
      fprintf (output, "%s ", preamble);
   } else if (SourceLine > 0) {
      if (column >= 0) {
          fprintf (output, "%s %s, line %d, column %d: ",
                         preamble, SourceFile, SourceLine, column+1);
      } else {
          fprintf (output, "%s %s, line %d: ",
                         preamble, SourceFile, SourceLine);
      }
   } else {
       fprintf (output, "%s %s: ", preamble, SourceFile);
   }
}


void buildmap_error (int column, const char *format, ...) {

   va_list ap;
   FILE    *log;

   buildmap_show_source (stderr, "**", column);

   log = fopen ("buildmap_errors.log", "a");
   if (log != NULL) {
      buildmap_show_source (log, "**", column);
   }

   va_start(ap, format);
   vfprintf(stderr, format, ap);
   if (log != NULL) {
      vfprintf(log, format, ap);
   }
   va_end(ap);

   fprintf (stderr, "\n");
   if (log != NULL) {
      fprintf(log, "\n");
      fclose (log);
   }

   ErrorCount += 1;
   ErrorTotal += 1;
}


void buildmap_fatal (int column, const char *format, ...) {

   va_list ap;
   FILE    *log;

   buildmap_show_source (stderr, "##", column);

   log = fopen ("buildmap_errors.log", "a");
   if (log != NULL) {
      buildmap_show_source (log, "##", column);
   }

   va_start(ap, format);
   vfprintf(stderr, format, ap);
   if (log != NULL) {
      vfprintf(log, format, ap);
   }
   va_end(ap);

   fprintf (stderr, "\n");
   if (log != NULL) {
      fprintf(log, "\n");
      fclose (log);
   }

   exit (1);
}


void buildmap_progress (int done, int estimated) {

   int this;

   this = (100 * done) / estimated;

   if (this != LastProgress) {
      LastProgress = this;
      fprintf (stderr, "-- %s: %3d%% done\r", SourceFile, this);
   }
}


void buildmap_info (const char *format, ...) {

   va_list ap;

   buildmap_show_source (stderr, "--", -1);

   va_start(ap, format);
   vfprintf(stderr, format, ap);
   va_end(ap);

   fprintf (stderr, "\n");
}


void buildmap_summary (int verbose, const char *format, ...) {

   va_list ap;

   SourceLine = 0;
   buildmap_show_source (stderr, "--", -1);

   if (ErrorCount > 0) {
      if (ErrorCount > 1) {
         fprintf (stderr, "%d errors", ErrorCount);
      } else {
         fprintf (stderr, "1 error");
      }
      verbose = 1;
      ErrorCount = 0;
   } else if (verbose) {
     fprintf (stderr, "no error");
   }

   if (verbose) {
      fprintf (stderr, ", ");

      va_start(ap, format);
      vfprintf(stderr, format, ap);
      va_end(ap);

      fprintf (stderr, "\n");
   }
}


int buildmap_get_error_count (void) {

   return ErrorCount;
}

int buildmap_get_error_total (void) {

   return ErrorTotal;
}

