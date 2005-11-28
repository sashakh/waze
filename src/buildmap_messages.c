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
#include "roadmap.h"

static char *SourceFile = NULL;
static int   SourceLine = 0;
static int   ErrorCount = 0;
static int   ErrorTotal = 0;

static int   LastProgress = 0;


void buildmap_set_source (char *name) {

   char *p;

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


void buildmap_error (int column, char *format, ...) {

   va_list ap;
   FILE    *log;

   fprintf (stderr, "** %s, line %d, column %d: ",
                    SourceFile, SourceLine, column+1);

   log = fopen ("buildmap_errors.log", "a");
   if (log != NULL) {
      fprintf (log, "** %s, line %d, column %d: ",
                    SourceFile, SourceLine, column+1);
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


void buildmap_fatal (int column, char *format, ...) {

   va_list ap;
   FILE    *log;

   fprintf (stderr, "## %s, line %d, column %d: ",
                    SourceFile, SourceLine, column+1);

   log = fopen ("buildmap_errors.log", "a");
   if (log != NULL) {
      fprintf (log, "## %s, line %d, column %d: ",
                    SourceFile, SourceLine, column+1);
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


void buildmap_info (char *format, ...) {

   va_list ap;

   if (SourceLine > 0) {
      fprintf (stderr, "-- %s, line %d: ", SourceFile, SourceLine);
   } else {
      fprintf (stderr, "-- %s: ", SourceFile);
   }

   va_start(ap, format);
   vfprintf(stderr, format, ap);
   va_end(ap);

   fprintf (stderr, "\n");
}


void buildmap_summary (int verbose, char *format, ...) {

   va_list ap;

   if (ErrorCount > 0) {
      if (ErrorCount > 1) {
         fprintf (stderr, "-- %s: %d errors", SourceFile, ErrorCount);
      } else {
         fprintf (stderr, "-- %s: 1 error", SourceFile);
      }
      verbose = 1;
      ErrorCount = 0;
   } else if (verbose) {
     fprintf (stderr, "-- %s: %d lines, no error", SourceFile, SourceLine);
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

