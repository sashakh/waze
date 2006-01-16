/* buildmap_street.h - Build a street table & index for RoadMap.
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

#ifndef INCLUDED__BUILDMAP_STREET__H
#define INCLUDED__BUILDMAP_STREET__H

#include <stdio.h>

int  buildmap_street_add
        (char cfcc,
         RoadMapString fedirp,
         RoadMapString fename,
         RoadMapString fetype,
         RoadMapString fedirs,
         int line);

int  buildmap_street_get_sorted (int street);
void buildmap_street_print_sorted (FILE *file, int street);

int  buildmap_street_count (void);

void buildmap_street_sort (void);

#endif // INCLUDED__BUILDMAP_STREET__H

