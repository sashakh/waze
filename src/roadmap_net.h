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

#ifndef _ROADMAP_NET__H_
#define _ROADMAP_NET__H_

#ifdef _WIN32

#include <winsock.h>

typedef SOCKET RoadMapSocket; /* WIN32 style. */
#define ROADMAP_NET_IS_VALID(s) (s != INVALID_SOCKET)

#else

typedef int RoadMapSocket; /* UNIX style. */
#define ROADMAP_NET_IS_VALID(s) (s != (RoadMapSocket)-1)

#endif /* _WIN32 */

RoadMapSocket roadmap_net_connect (const char *protocol,
                                   const char *name, int default_port);

int roadmap_net_receive (RoadMapSocket s, void *data, int size);
int roadmap_net_send    (RoadMapSocket s, const void *data, int length);

void roadmap_net_close  (RoadMapSocket s);

#endif // _ROADMAP_NET__H_

