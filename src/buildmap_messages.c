/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2008, Danny Backx.
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
 */
/**
 * @file
 * @brief a module for managing uniform error & info messages.
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

#define BUILDMAP_MESSAGE_VERBOSE   5
#define BUILDMAP_MESSAGE_PROGRESS  4
#define BUILDMAP_MESSAGE_INFO      3
#define BUILDMAP_MESSAGE_ERROR     2
#define BUILDMAP_MESSAGE_FATAL     1
int BuildMapMessageLevel = 4; /* + for more verbosity, - for less */

void buildmap_message_adjust_level (int level) {

    BuildMapMessageLevel += level;

#ifndef	_WIN32
    if (BuildMapMessageLevel >= BUILDMAP_MESSAGE_VERBOSE)
        setbuf(stdout, NULL);
#endif
}

/**
 * @brief set the name of the file we're processing, to be used in error reporting
 * @param name the file name, or NULL to erase previous setting
 */
void buildmap_set_source (const char *name) {

   const char *p;

   if (name == NULL) {
	   if (SourceFile)
		   free(SourceFile);
	   SourceFile = NULL;
	   return;
   }
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

/**
 * @brief indicate which line number we're processing
 * @param line the number
 */
void buildmap_set_line (int line) {

   SourceLine = line;
}

/**
 * @brief
 * @param output
 * @param preamble
 * @param column
 */
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

/**
 * @brief print an error message
 * @param column
 * @param format
 */
void buildmap_error (int column, const char *format, ...) {

   va_list ap;
   FILE    *log;

   if (BuildMapMessageLevel >= BUILDMAP_MESSAGE_ERROR)
      buildmap_show_source (stderr, "**", column);

   log = fopen ("buildmap_errors.log", "a");
   if (log != NULL) {
      buildmap_show_source (log, "**", column);
   }

   va_start(ap, format);
   if (BuildMapMessageLevel >= BUILDMAP_MESSAGE_ERROR)
      vfprintf(stderr, format, ap);

   if (log != NULL) {
      vfprintf(log, format, ap);
   }
   va_end(ap);

   if (BuildMapMessageLevel >= BUILDMAP_MESSAGE_ERROR)
      fprintf (stderr, "\n");

   if (log != NULL) {
      fprintf(log, "\n");
      fclose (log);
   }

   ErrorCount += 1;
   ErrorTotal += 1;
}

/**
 * @brief print an error message and terminate the application
 * @param column
 * @param format
 */
void buildmap_fatal (int column, const char *format, ...) {

   va_list ap;
   FILE    *log;

   if (BuildMapMessageLevel >= BUILDMAP_MESSAGE_FATAL)
      buildmap_show_source (stderr, "##", column);

   log = fopen ("buildmap_errors.log", "a");
   if (log != NULL) {
      buildmap_show_source (log, "##", column);
   }

   va_start(ap, format);
   if (BuildMapMessageLevel >= BUILDMAP_MESSAGE_FATAL)
      vfprintf(stderr, format, ap);
   if (log != NULL) {
      vfprintf(log, format, ap);
   }
   va_end(ap);

   if (BuildMapMessageLevel >= BUILDMAP_MESSAGE_FATAL)
      fprintf (stderr, "\n");
   if (log != NULL) {
      fprintf(log, "\n");
      fclose (log);
   }

   exit (1);
}

/**
 * @brief
 * @param done
 * @param estimated
 */
void buildmap_progress (int done, int estimated) {

   int this;

   if (BuildMapMessageLevel >= BUILDMAP_MESSAGE_PROGRESS) {

      this = (100 * done) / estimated;

      if (this != LastProgress) {
         LastProgress = this;
        fprintf (stdout, "-- %s: %3d%% done\r", SourceFile, this);
      }
   }
}

/**
 * @brief print an informational message
 * @param format
 */
void buildmap_info (const char *format, ...) {

   va_list ap;

   if (BuildMapMessageLevel >= BUILDMAP_MESSAGE_INFO) {

      buildmap_show_source (stdout, "--", -1);

      va_start(ap, format);
      vfprintf(stdout, format, ap);
      va_end(ap);

      fprintf (stdout, "\n");
   }

}

/**
 * @brief
 * @return
 */
int buildmap_is_verbose (void) {

   return (BuildMapMessageLevel >= BUILDMAP_MESSAGE_VERBOSE);
}

/**
 * @brief
 * @param format
 */
void buildmap_verbose (const char *format, ...) {

   va_list ap;

   if (BuildMapMessageLevel < BUILDMAP_MESSAGE_VERBOSE)
      return;

   va_start(ap, format);
   vfprintf(stdout, format, ap);
   va_end(ap);

   fprintf (stdout, "\n");
}

/**
 * @brief
 * @param verbose
 * @param format
 */
void buildmap_summary (int verbose, const char *format, ...) {

   va_list ap;

   if (BuildMapMessageLevel < BUILDMAP_MESSAGE_INFO)
      return;

   if (ErrorCount > 0 || verbose) {
      SourceLine = 0;
      buildmap_show_source (stdout, "--", -1);
   }

   if (ErrorCount > 0) {
      if (ErrorCount > 1) {
         fprintf (stdout, "%d errors", ErrorCount);
      } else {
         fprintf (stdout, "1 error");
      }
      verbose = 1;
      ErrorCount = 0;
   } else if (verbose) {
     fprintf (stdout, "no error");
   }

   if (verbose) {
      fprintf (stdout, ", ");

      va_start(ap, format);
      vfprintf(stdout, format, ap);
      va_end(ap);

      fprintf (stdout, "\n");
   }
}

/**
 * @brief
 * @return
 */
int buildmap_get_error_count (void) {

   return ErrorCount;
}
/**
 * @brief
 * @return
 */
int buildmap_get_error_total (void) {

   return ErrorTotal;
}
/**
 * @brief
 * @param source
 * @param line
 * @param allocated
 */
void buildmap_check_allocated_with_source_line
                (char *source, int line, const void *allocated) {

    if (allocated == NULL)
        buildmap_fatal (0, "out of memory, %s:%d", source, line);
}

