/* roadmap_time.c - Manage time information & display.
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
 *   See roadmap_time.h
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_time.h"


char *roadmap_time_get_hours_minutes (void) {
    
    static char image[16];
    
    time_t now;
    struct tm *tm;
    
    now = time(NULL);
    tm = localtime (&now);
    snprintf (image, sizeof(image), "%2d:%02d", tm->tm_hour, tm->tm_min);
    
    return image;
}
