/* roadmap_osm.c - helper utilities for OpenStreetMap maps
 *
 * LICENSE:
 *
 *   Copyright 2007 Paul G. Fox
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
 * These functions are used to retrieve which map the given entity belongs to.
 *
 * These functions are used both by roadmap and by buildmap.
 */

#define ROADMAP_OSM_DEFAULT_BITS "19"  // yes, it's a string

#define MaxLon 180000000
#define MaxLat 90000000

/* we have 32 bits.  we reserve the sign bit, and we reserve a
 * nibble for the bitcount.  that leaves 27 bits for quadtile
 * addressing. */

#define TILE_MINBITS 12
#define TILE_MAXBITS 27

/* bitcount is encoded in the last nibble.  range is 12 through 27 */
#define tileid2bits(tileid)     \
    (((tileid) & 0xf) + 12)

/* tileid starts at bit 31 and grows to the right with increasing bitcount */
#define tileid2trutile(tileid)  \
    ((tileid) >> (4 + (27 - tileid2bits(tileid))))

#define mktileid(tru, bits)     \
    (((tru) << (4 + (27 - (bits)))) | ((bits) - 12))

int roadmap_osm_latlon2tileid(int lat, int lon, int bits);
void roadmap_osm_tileid_to_bbox(int tileid, RoadMapArea *edges);
char *roadmap_osm_filename(char *buf, int dirpath, int tileid);
void roadmap_osm_tilesplit(int tileid, int *ntiles, int howmanybits);

int  roadmap_osm_by_position
        (const RoadMapPosition *position,
         const RoadMapArea *focus, int **fips, int in_count);

int  roadmap_osm_by_position_munged
        (const RoadMapPosition *position,
         const RoadMapArea *focus, int **fips, int in_count);

void roadmap_osm_initialize(void);
   

int roadmap_osm_tileid_to_neighbor(int tileid, int dir);
#define TILE_SOUTHEAST 0
#define TILE_SOUTH 1
#define TILE_SOUTHWEST 2
#define TILE_EAST 3
#define TILE_WEST 4
#define TILE_NORTHEAST 5
#define TILE_NORTH 6
#define TILE_NORTHWEST 7

//int roadmap_osm_get_bits (void);
//void roadmap_osm_set_bits (int bits);

