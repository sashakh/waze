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

void roadmap_log (int level, char *source, int line, char *format, ...);

void roadmap_log_save_all  (void);
void roadmap_log_save_none (void);
void roadmap_log_purge     (void);

int  roadmap_log_enabled (int level, char *source, int line);

#define roadmap_check_allocated(p) \
            roadmap_check_allocated_with_source_line(__FILE__,__LINE__,p)

typedef void (* RoadMapLogRedirect) (const char *message);

RoadMapLogRedirect roadmap_log_redirect (int level,
                                         RoadMapLogRedirect redirect);


#define ROADMAP_SHOW_AREA        1
#define ROADMAP_SHOW_SQUARE      2



void roadmap_option_initialize (void);

int  roadmap_option_is_synchronous (void);

char **roadmap_debug (void);

int   roadmap_verbosity  (void); /* return a minimum message level. */
int   roadmap_is_visible (int category);
char *roadmap_gps_source (void);

int roadmap_option_cache  (void);
int roadmap_option_width  (const char *name);
int roadmap_option_height (const char *name);


typedef void (*RoadMapUsage) (const char *section);

void roadmap_option (int argc, char **argv, RoadMapUsage usage);


/* This function is hidden by a macro: */
void roadmap_check_allocated_with_source_line
                (char *source, int line, const void *allocated);

typedef void (* RoadMapCallback) (void);

#ifdef ROADMAP_DBG_TIME

#define DBG_TIME_FULL 0
#define DBG_TIME_DRAW_SQUARE 1
#define DBG_TIME_DRAW_ONE_LINE 2
#define DBG_TIME_SELECT_PEN 3
#define DBG_TIME_DRAW_LINES 4
#define DBG_TIME_CREATE_PATH 5
#define DBG_TIME_ADD_PATH 6
#define DBG_TIME_FLIP 7
#define DBG_TIME_TEXT_FULL 8
#define DBG_TIME_TEXT_CNV 9
#define DBG_TIME_TEXT_LOAD 10
#define DBG_TIME_TEXT_ONE_LETTER 11
#define DBG_TIME_TEXT_GET_GLYPH 12
#define DBG_TIME_TEXT_ONE_RAS 13

#define DBG_TIME_LAST_COUNTER 14

void dbg_time_start(int type);
void dbg_time_end(int type);

#else

#define dbg_time_start(x)
#define dbg_time_end(x)

#endif  // ROADMAP_DBG_TIME

#endif // INCLUDE__ROADMAP__H

