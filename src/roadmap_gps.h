/* roadmap_gps.h - GPS interface for the RoadMap application.
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

#ifndef _ROADMAP_GPS__H_
#define _ROADMAP_GPS__H_

/* The listener is a function to be called each time a valid GPS position
 * has been received.
 */
typedef void (*roadmap_gps_listener)
                   (RoadMapPosition *position, int speed, int direction);

/* The logger is a function to be called each time data has been
 * received from the GPS link (good or bad).
 */
typedef void (*roadmap_gps_logger)   (char *sentence);

/* The link control functions are hooks designed to let the NMEA reception
 * to be managed from within a GUI main loop. When data is detected from
 * the GPS link, roadmap_gps_input() should be called.
 */
typedef void (*roadmap_gps_link_control) (int fd);


void roadmap_gps_initialize (roadmap_gps_listener listener);
void roadmap_gps_open       (void);

void roadmap_gps_register_logger (roadmap_gps_logger logger);

void roadmap_gps_register_link_control
        (roadmap_gps_link_control add, roadmap_gps_link_control remove);

void roadmap_gps_input (int fd);

int  roadmap_gps_active (void);

#endif // _ROADMAP_GPS__H_

