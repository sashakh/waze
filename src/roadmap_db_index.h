/* roadmap_db_index.h - the format of the index table used by RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2005 Pascal F. Martin
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
 *   The RoadMap index is described by the following table:
 *
 *   index/authority   The authority body. Links to territories.
 *   index/territory   The territory. Sorted by authority, links to maps.
 *   index/map         A map file (a specific territory & class).
 *   index/name        Describes the names of a specific authority.
 *   index/citiek      Links the name of a city with its associated territory.
 *
 * NOTE:
 *
 *   The format of the wtid field is defined using decimal digits:
 *
 *   - The wtid is made of 10 decimal digits. There are two major classes
 *   of wti format: one is defined by legal boundaries and the other is
 *   defined by a longitude/latitude grid.
 *
 *   - The legal-boundaries wtid for the USA has the format 00000SSCCC where
 *   SSCCC is the county's FIPS code. Note that, in this case, the value of
 *   the wtid exactly matches the value of the FIPS code. A CCC of 000 means
 *   the whole state. A wtid value of 0 represents the whole US.
 *
 *   - the legal-boundaries wtid for other countries has the format 0CCCCXXXXX,
 *   where CCCC is the telephone dialing code for the country covered by the
 *   maps and XXXXX is a subdivision inside that country, wich format depends
 *   on the country's legal organization. A XXXXX of 00000 represents the
 *   whole country.
 *
 *   - The grid wtid have the format: 1CCLLLLllll where LLLL is a number that
 *   represents the longitude span covered by the map and llll is a number
 *   that represents the latitude covered by the map. CC identifies the type
 *   of grid used to define the map; value 00 represents the USGS "quad" grid.
 *   When the USGS quad gris is used, LLLL and llll are formatted as DDDQ,
 *   where DDD is the decimal degree (range 0-360 for longitude and 0-180
 *   for latitude) and Q is the quad index (range 1-8, where 1 represents
 *   the low order longitude or latitude value and 8 the high order value).
 *
 *   The details of the format of the wtid is never used by RoadMap: its
 *   sole purpose is to ensure the generation of a unique and consistent
 *   map identification code.
 *
 *   The main goal of the wtid format is to serve for identifying maps
 *   using 32 bits entities. A map will be uniquely identified in RoadMap
 *   using two integers: the wtid and the class index. The class index is
 *   defined when RoadMap starts and is never stored on disk. The wtid is
 *   predefined (see above) and is stored in the map files.
 *
 *   The international dialing codes are defined in the ITU Operational
 *   Bulletin No. 835. For more information, go to:
 *
 *      http://www.itu.int/itu-t/bulletin/annex.html.
 *
 *   The code name of a map mimics the wtid using ASCII codes. The format
 *   is <country> / <subdivision>, where subdivision may also be a slash-
 *   separated list of legal entities. For example, the format of the code
 *   name for the US is: us/<state code> (the code name for California is
 *   thus "us/ca"). The code name must be suitable for address search.
 *   The country code name follow the iso 3166-1 standard. See:
 *
 *      http://www.iso.org/iso/en/prods-services/iso3166ma/02iso-3166-code-lists/list-en1.html
 *
 *   The goal of the city names and postal code range is to allow for
 *   the search by address.
 *
 *   Note that it is legal for the same authority to appear more than
 *   once. This can happen for optimization reason, when the authority
 *   covers multiple non-contiguous areas. Instead of listing a single,
 *   huge, area, one can list the same authority multiple times, one per
 *   area covered.
 */

#ifndef INCLUDE__ROADMAP_DB_INDEX__H
#define INCLUDE__ROADMAP_DB_INDEX__H

#include "roadmap_types.h"

typedef struct { /* table index/authority */

   RoadMapString symbol;    /* Relative to the index's parent. */
   RoadMapString pathname;  /* Relative or absolute path for the maps. */

   RoadMapArea edges;       /* Bounding earth area for this authority. */

   unsigned short name_first;
   unsigned short name_count; /* Cannot be 0. */

   unsigned short territory_first;
   unsigned short territory_count; /* If 0, it must have a sub-index file. */

} RoadMapAuthority;


typedef struct {  /* table index/territory */

   int wtid;                /* Stands for "world territory ID". */

   RoadMapString name;
   RoadMapString pathname;  /* Relative or absolute path for the map files. */

   RoadMapArea edges;       /* Bounding earth area for this territory. */

   unsigned short map_first;
   unsigned short map_count; /* Cannot be 0. */

   unsigned short city_first;
   unsigned short city_count; /* 0 if no city is listed. */

   unsigned int postal_low;   /* Lowest postal code in that territory. */
   unsigned int postal_high;  /* Highest postal code in that territory. */

} RoadMapTerritory;


typedef struct {  /* table index/map */

   RoadMapString class;
   RoadMapString filename;  /* ... relative to the authority/territory path. */

} RoadMapMap;

/* table index/name is an array of RoadMapString sorted by authority. */

/* table index/city is an array of RoadMapString sorted by map. */

#endif // INCLUDE__ROADMAP_DB_INDEX__H

