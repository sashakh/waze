/* buildmap_area.c - Build a area table & index for RoadMap.
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
 *   void buildmap_area_initialize (void);
 *   int buildmap_area_add
 *          (char cfcc,
 *           RoadMapString fedirp,
 *           RoadMapString fename,
 *           RoadMapString fetype,
 *           RoadMapString fedirs,
 *           int tlid);
 *   void  buildmap_area_sort (void);
 *   void  buildmap_area_save (void);
 *   void  buildmap_area_summary (void);
 *
 * These functions are used to build a table of areas from
 * the Tiger maps. The objective is double: (1) reduce the size of
 * the Tiger data by sharing all duplicated information and
 * (2) produce the index data to serve as the basis for a fast
 * search mechanism for areas in roadmap.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "buildmap.h"
#include "buildmap_area.h"


void buildmap_area_initialize (void) {

}


int buildmap_area_add
        (char cfcc,
         RoadMapString fedirp,
         RoadMapString fename,
         RoadMapString fetype,
         RoadMapString fedirs,
         int tlid) {

   return 0; // TBD
}


void buildmap_area_sort (void) {

}


void buildmap_area_save (void) {

}


void buildmap_area_summary (void) {

}

