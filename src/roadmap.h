/* roadmap.h - general definitions use by the RoadMap program.
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
 */

#ifndef INCLUDE__ROADMAP__H
#define INCLUDE__ROADMAP__H

#include "roadmap_types.h"

#define ROADMAP_MESSAGE_DEBUG      1
#define ROADMAP_MESSAGE_INFO       2
#define ROADMAP_MESSAGE_WARNING    3
#define ROADMAP_MESSAGE_ERROR      4
#define ROADMAP_MESSAGE_FATAL      5

#define ROADMAP_DEBUG   ROADMAP_MESSAGE_DEBUG,__FILE__,__LINE__
#define ROADMAP_INFO    ROADMAP_MESSAGE_INFO,__FILE__,__LINE__
#define ROADMAP_WARNING ROADMAP_MESSAGE_WARNING,__FILE__,__LINE__
#define ROADMAP_ERROR   ROADMAP_MESSAGE_ERROR,__FILE__,__LINE__
#define ROADMAP_FATAL   ROADMAP_MESSAGE_FATAL,__FILE__,__LINE__

void roadmap_log_push  (const char *description);
void roadmap_log_pop   (void);
void roadmap_log_reset (void);

void roadmap_log (int level, char *source, int line, char *format, ...);

#define roadmap_check_allocated(p) \
            roadmap_check_allocated_with_source_line(__FILE__,__LINE__,p)

#define ROADMAP_SHOW_AREA        1
#define ROADMAP_SHOW_SQUARE      2

int   roadmap_verbosity  (void); /* return a minimum message level. */
int   roadmap_is_visible (int category);
char *roadmap_gps_source (void);

int roadmap_option_width  (const char *name);
int roadmap_option_height (const char *name);

void roadmap_option (int argc, char **argv);


/* This function is hidden by a macro: */
void roadmap_check_allocated_with_source_line
                (char *source, int line, const void *allocated);

#endif // INCLUDE__ROADMAP__H

