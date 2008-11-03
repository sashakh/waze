/*
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

/**
 * @file
 * @brief Manage screen signs.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_time.h"
#include "roadmap_message.h"
#include "roadmap_display.h"


static char *RoadMapMessageParameters[128] = {NULL};

static int RoadMapMessagesUseTime;

int roadmap_message_time_in_use(void) {

    return RoadMapMessagesUseTime;
}

int roadmap_message_format (char *text, int length, const char *format) {

    char *f;
    char *p = text;
    char *end = text + length - 1;
    
    while (*format) {
        
        if (*format == '%') {
            
            format += 1;
            if (*format <= 0) {
                break;
            }
            
            f = RoadMapMessageParameters[(int)(*format)];
            if (*format == 'T') {
                RoadMapMessagesUseTime = 1;
            }
            format += 1;
            if (f != NULL) {
                while (*f && p < end) {
                    *(p++) = *(f++);
                }
            } else {
                format = strchr (format, '|');
                
                if (format == NULL) {
                    return 0; /* Cannot build the string. */
                }
                format += 1;
                p = text; /* Restart. */
            }

        } else if (*format == '|') {
            
            break; /* We completed this alternative successfully. */
            
        } else {

            *(p++) = *(format++);
        }
        
        if (p >= end) {
            break;
        }
    }

    *p = 0;

    return p > text;
}

/**
 * @brief set a numbered message to the given string
 * @param parameter indicates which message to set
 * @param format this and the next parameters are printf-style
 */
void roadmap_message_set (int parameter, const char *format, ...) {
    
    va_list ap;
    char    value[256];
    
    if (parameter <= 0) {
        roadmap_log (ROADMAP_ERROR, "invalid parameter code %d", parameter);
        return;
    }
    
    va_start(ap, format);
    vsnprintf(value, sizeof(value), format, ap);
    va_end(ap);
    
    if (RoadMapMessageParameters[parameter] != NULL) {
        free (RoadMapMessageParameters[parameter]);
    }
    if (value[0] == 0) {
        RoadMapMessageParameters[parameter] = NULL;
    } else {
        RoadMapMessageParameters[parameter] = strdup (value);
    }
}

char *roadmap_message_get (int parameter) {
    
    return RoadMapMessageParameters[parameter] ? 
    	    RoadMapMessageParameters[parameter] : "";
}


void roadmap_message_unset (int parameter) {
    
    if (parameter <= 0) {
        roadmap_log (ROADMAP_ERROR, "invalid parameter code %d", parameter);
        return;
    }
    
    if (RoadMapMessageParameters[parameter] != NULL) {
        free (RoadMapMessageParameters[parameter]);
        RoadMapMessageParameters[parameter] = NULL;
    }
}
