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
 * @brief win32/roadmap_time.c - Manage time information & display.
 * @ingroup windows
 */

#include "../roadmap.h"
#include "../roadmap_time.h"

/**
 * @brief convert time_t into a string
 * @param gmt the input time
 * @return the string that can be printed
 */
char *roadmap_time_get_date_hours_minutes (time_t gmt) {

    static char image[32];
    struct tm *tm;

    tm = localtime (&gmt);
    snprintf (image, sizeof(image), "%4d.%02d.%02d %2d:%02d",
		    1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
		    tm->tm_hour, tm->tm_min);

    return image;
}

char *roadmap_time_get_hours_minutes (time_t gmt) {

    static char image[32];
    struct tm *tm;

    tm = localtime (&gmt);
    snprintf (image, sizeof(image), "%2d:%02d", tm->tm_hour, tm->tm_min);

    return image;
}

/**
 * @brief Get some kind of time indication in milliseconds, on Windows it's the time since boot
 * @brief Get the time since last boot
 * @return return the time in milliseconds
 */
unsigned long roadmap_time_get_millis(void) {

   return (unsigned long) GetTickCount();

}
