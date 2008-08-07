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
#ifdef _WIN32
#include "win32/roadmap_win32.h"
#endif


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

void roadmap_log_push        (const char *description);
void roadmap_log_pop         (void);
void roadmap_log_reset_stack (void);

void roadmap_log (int level, const char *source, int line, const char *format, ...);

void roadmap_log_save_all  (void);
void roadmap_log_save_none (void);
void roadmap_log_purge     (void);

int  roadmap_log_enabled (int level, char *source, int line);

#define roadmap_check_allocated(p) \
            roadmap_check_allocated_with_source_line(__FILE__,__LINE__,p)

typedef void (* RoadMapLogRedirect) (const char *message);

RoadMapLogRedirect roadmap_log_redirect (int level,
                                         RoadMapLogRedirect redirect);

/* Cancel all redirect, to be used in child processes only. */
void roadmap_log_cancel_redirect (void);

#define ROADMAP_SHOW_AREA          1
#define ROADMAP_SHOW_SQUARE        2
#define ROADMAP_SHOW_GLOBAL_SQUARE 3



void roadmap_option_initialize (void);

int  roadmap_option_is_synchronous (void);

char **roadmap_debug (void);

int   roadmap_verbosity  (void); /* return a minimum message level. */
int   roadmap_is_visible (int category);
char *roadmap_gps_source (void);
char *roadmap_extra_config (void);
char *roadmap_userpath (void);
char *roadmap_icon_path (void);

int roadmap_option_cache  (void);
int roadmap_option_width  (const char *name);
int roadmap_option_height (const char *name);


typedef void (*RoadMapUsage) (const char *section);

void roadmap_option (int argc, char **argv, int pass, RoadMapUsage usage);


/* This function is hidden by a macro: */
void roadmap_check_allocated_with_source_line
                (const char *source, int line, const void *allocated);

typedef void (* RoadMapCallback) (void);

typedef enum {
    ROADMAP_CURSOR_NORMAL,
    ROADMAP_CURSOR_CROSS,
    ROADMAP_CURSOR_WAIT,
    ROADMAP_CURSOR_WAIT_WITH_DELAY
} RoadMapCursor;

#endif // INCLUDE__ROADMAP__H

