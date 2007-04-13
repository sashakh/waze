/* roadmap_db_range.h - the format of the address range table used by RoadMap.
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
 *   The RoadMap address ranges are described by the following tables:
 *
 *   range.bystreet   for each street, the ranges that belong to the street.
 *   range.bycity     the list of ranges by street, brocken for each city.
 *   range.addr       for each range, the street address numbers.
 *   range.noaddr     reference to the street for the lines with no address.
 *   range.byzip      for each range, the ZIP code.
 *   range.place      for each place, the city it belongs to.
 *   range.bysquare   for each square, a street search accelerator.
 *
 *   The ZIP codes are stored in a separate table to make the database more
 *   compact: the ZIP value is a 8 bit value that references the zip table.
 */

#ifndef _ROADMAP_DB_RANGE__H_
#define _ROADMAP_DB_RANGE__H_

#include "roadmap_types.h"

#define CONTINUATION_FLAG   0x80000000

#define HAS_CONTINUATION(r) ((r->line & CONTINUATION_FLAG) != 0)

typedef struct {   /* table range.addr */

   int line; /* Sign is used to mark a "wide range" record. */

   unsigned short fradd;
   unsigned short toadd;

} RoadMapRange;


typedef struct {  /* table range.bystreet */
   int first_range;
   int first_city;
#ifndef J2MEMAP
   int first_zip;
#endif   
   int count_range;
} RoadMapRangeByStreet;


typedef struct {  /* table range.bycity */

   RoadMapString city;
   unsigned short count;

} RoadMapRangeByCity;


typedef struct { /* table range.byzip */

   RoadMapZip     zip;
   unsigned short count;

} RoadMapRangeByZip;


typedef struct {  /* table range.place */

   RoadMapString place;
   RoadMapString city;

} RoadMapRangePlace;


typedef struct {

   unsigned short excluded;
   unsigned short included;

} RoadMapRangeHole;

#define ROADMAP_RANGE_HOLES 32

typedef struct {  /* table range.bysquare */

   RoadMapRangeHole hole[ROADMAP_RANGE_HOLES];

   int noaddr_start;
   int noaddr_count;

} RoadMapRangeBySquare;


#endif // _ROADMAP_DB_RANGE__H_

