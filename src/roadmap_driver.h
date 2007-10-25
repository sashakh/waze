/* roadmap_driver.h - a driver manager for the RoadMap application.
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
 *    This module is, as a matter of fact, an extension to roadmap_gps.c
 *    that deals with managing external drivers and exchanging information
 *    with these drivers. The information is coded using the NMEA syntax,
 *    so that we benefit from all the existing infrastructure.
 *
 *    The function roadmap_driver_subscribe () should be called when
 *    a given NMEA client wants to accept RoadMap driver extensions
 *    sentences from its data source. This does not require the NMEA
 *    client to know or handle any of these extension sentences: the
 *    processing of the extension sentences will be transparent. A
 *    typical use is when a GPS source knows RoadMap and sends some
 *    RoadMap sentences: when such a source is selected, roadmap_gps.c
 *    should call roadmap_driver_subsribe().
 *
 *    The other function are similar to the roadmap_gps.c equivalents.
 */

#ifndef INCLUDED__ROADMAP_DRIVER__H
#define INCLUDED__ROADMAP_DRIVER__H

#include "roadmap_io.h"
#include "roadmap_gps.h"
#include "roadmap_nmea.h"

void roadmap_driver_initialize (void);

void roadmap_driver_subscribe (RoadMapNmeaAccount account);

void roadmap_driver_register_link_control
        (roadmap_gps_link_control add, roadmap_gps_link_control remove);

void roadmap_driver_register_server_control
        (roadmap_gps_link_control add, roadmap_gps_link_control remove);

void roadmap_driver_activate (void);

void roadmap_driver_input (RoadMapIO *io);

void roadmap_driver_accept (RoadMapIO *io);

void roadmap_driver_shutdown (void);

#endif // INCLUDED__ROADMAP_DRIVER__H

