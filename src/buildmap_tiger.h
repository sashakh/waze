/* buildmap_tiger.h - Read the US Census bureau's TIGER files.
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

#ifndef _BUILDMAP_TIGER__H_
#define _BUILDMAP_TIGER__H_

void buildmap_tiger_set_format (int year);
void buildmap_tiger_set_prefix (char *prefix);

void buildmap_tiger_initialize (int verbose, int canals, int rivers);

void buildmap_tiger_read_rt1 (char *path, char *name, int verbose);
void buildmap_tiger_read_rt2 (char *path, char *name, int verbose);
void buildmap_tiger_read_rtc (char *path, char *name, int verbose);
void buildmap_tiger_read_rt7 (char *path, char *name, int verbose);
void buildmap_tiger_read_rt8 (char *path, char *name, int verbose);
void buildmap_tiger_read_rti (char *path, char *name, int verbose);

void buildmap_tiger_sort (void);

void buildmap_tiger_summary (void);

#endif // _BUILDMAP_TIGER__H_

