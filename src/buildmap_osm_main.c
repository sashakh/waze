/*
 * LICENSE:
 *
 *   Copyright 2007 Paul G. Fox
 *   Copyright (c) 2008, 2009, Danny Backx
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
 * @brief The map builder tool for openstreetmap maps.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_hash.h"
#include "roadmap_path.h"
#include "roadmap_math.h"
#include "roadmap_file.h"


#include "buildmap.h"
#include "buildmap_opt.h"
#include "buildmap_metadata.h"
#include "buildmap_layer.h"
#include "buildmap_osm_common.h"
#include "buildmap_osm_binary.h"
#include "buildmap_osm_text.h"

#include "roadmap_osm.h"
#include "roadmap_iso.h"

int BuildMapNoLongLines;

#define BUILDMAP_FORMAT_OSMBIN    1

static char *BuildMapFormat;
static int   BuildMapReplaceAll = 0;
static char *BuildMapFileName = 0;

char *BuildMapResult;


struct opt_defs options[] = {
   {"format", "f", opt_string, "osmbinary",
        "Input format (OSM protocol)"},
   {"class", "c", opt_string, "default/All",
        "The class file to create the map for"},
   {"bits", "b", opt_int, ROADMAP_OSM_DEFAULT_BITS,
        "The number of bits of quadtile address"},
   {"replace", "r", opt_flag, "0",
        "Re-request maps that are already present"},
   {"maps", "m", opt_string, "",
        "Location for the generated map files"},
   {"source", "s", opt_string, "osmgetbmap",
        "commandname or URL for accessing map data"},
   {"tileid", "t", opt_int, "",
        "Fetch the given numeric tileid"},
   {"decode", "d", opt_string, "",
        "Analyze given tileid (or quadtile filename) (hex only)"},
   {"encode", "e", opt_string, "",
        "Report tileid for given lat,lon"},
   {"quiet", "q", opt_flag, "0",
        "Show less progress information"},
   {"verbose", "v", opt_flag, "0",
        "Show more progress information"},
   {"inputfile", "i", opt_string, "",
        "Convert this OSM file into a map"},
   {"outputfile", "o", opt_string, "",
        "Write output in this file"},
   OPT_DEFS_END
};

/**
 * @brief save our things in a custom file name
 * @param filename
 * @param writeit
 */
static void buildmap_osm_save_custom (const char *filename, int writeit) {

   char *parent;

   parent = roadmap_path_parent(BuildMapResult, filename);
   roadmap_path_create (parent);
   roadmap_path_free(parent);

   if (buildmap_db_open (BuildMapResult, filename) < 0) {
      buildmap_fatal (0, "cannot create database %s", filename);
   }

   if (writeit) {
        buildmap_info("writing results to %s", filename);
        buildmap_db_save ();
   } else {
        buildmap_info("no results to write to %s", filename);
   }

   buildmap_db_close ();
}

/**
 * @brief save our things, if writeit is true. File name based on tileid
 * @param tileid
 * @param writeit
 */
static void buildmap_osm_save (int tileid, int writeit) {

   char db_name[128];

   roadmap_osm_filename(db_name, 1, tileid);

   buildmap_osm_save_custom(db_name, writeit);
}

/**
 * @brief
 * @param llp
 * @param end
 * @return
 */
static int
strtolatlon(char *llp, char **end) {
    int ll;
    double dll;

    /* allow user to leave out decimal point, to enter microdegrees */
    dll = strtod(llp, end);
    if (dll < 181 && dll > -181)
        return dll * 1000000;

    ll = strtol(llp, end, 10);
    return ll;
}

#if NEEDED
/* this routine figures out which tiles we already have, so we
 * can tell the server about our neighbors.  but this gets hard
 * to manage if we re-request a tile -- it now has neighbors that
 * weren't there when we first requested it.  so instead, we simply
 * always tell the server that half our neighbors (the ones to the
 * south and east) are present, for all tiles.
 */
static int
buildmap_osm_neighbors_to_mask(int tileid) {
    int dir, n;
    char filename[128];
    int bit = 1, have = 0;

    for (dir = TILE_SOUTHEAST; dir < TILE_NORTHWEST+1; dir++) {
        bit = (1 << dir);
        n = roadmap_osm_tileid_to_neighbor(tileid, dir);
        roadmap_osm_filename(filename, 1, n);
        if (roadmap_file_exists (BuildMapResult, filename))
            have |= bit;
    }
    return have;
}
#endif

#define DEFAULT_NEIGHBORS \
    ( 1<<TILE_EAST | 1<<TILE_SOUTHEAST | 1<<TILE_SOUTH | 1<<TILE_SOUTHWEST )

/**
 * @brief query the web server using the OSM binary protocol, and process it
 * @param tileid
 * @param source
 * @param cmdfmt is a printf style format string used to format the web query
 * @return
 */
static int
buildmap_osm_process_one_tile
        (int tileid, const char *source, const char *cmdfmt)
{

    char urlcmd[256];
    FILE *fdata;
    int have = 0;
    int ret, bits, trutile;

    bits = tileid2bits(tileid);
    buildmap_verbose("called for tileid 0x%x, bits %d", tileid, bits);

    have = DEFAULT_NEIGHBORS;

    buildmap_verbose ("tileid %d, have 0x%02x", tileid, have);

    trutile = tileid2trutile(tileid);
    snprintf(urlcmd, sizeof(urlcmd), cmdfmt, source, trutile, bits, have);

    buildmap_info("command is \"%s\"", urlcmd);

    fdata = popen(urlcmd, "r");
    if (fdata == NULL) {
        buildmap_fatal(0, "couldn't open \"%s\"", urlcmd);
    }

    buildmap_osm_common_find_layers();

    ret = buildmap_osm_binary_read(fdata);

    if (pclose(fdata) != 0) {
        buildmap_error(0, "problem fetching data");
        ret = -1;
    }

    return ret;
     
}

/**
 * @brief process a .osm text file
 * @param fn the file name
 * @return success indicator
 */
int buildmap_osm_text_process_file(char *fn)
{
    int         n, ret = 0;
    FILE        *f;
    char        country[6], division[6];
    int         fips, country_num = 0, division_num = 0;

    f = fopen(fn, "r");
    if (f == NULL) {
            buildmap_fatal(0, "couldn't open \"%s\"", fn);
            return -1;
    }

    if ((n = buildmap_osm_filename_iso(fn, country, division, ".osm"))) {
            buildmap_metadata_add_attribute ("Territory", "Id", country);
            buildmap_metadata_add_attribute ("Territory", "Parent", country);
            buildmap_metadata_add_value ("Territory", "Parent", country);

            country_num = roadmap_iso_alpha_to_num(country);
            if (n == 2)
                    division_num =
                        roadmap_iso_division_to_num(country, division);
    } else if (buildmap_osm_filename_usc(fn, &fips)) {
            /* ?? */
            buildmap_metadata_add_attribute ("Territory", "Id", fn);
            buildmap_metadata_add_attribute ("Territory", "Parent", fn);
            buildmap_metadata_add_value ("Territory", "Parent", fn);

            country_num = fips / 1000;
            division_num = fips % 1000;
    } else {
            buildmap_fatal(0,
                "Invalid file name \"%s\" should be ISO or USC shape", fn);
    }

    buildmap_metadata_add_attribute ("Class", "Name",   "All");
    buildmap_metadata_add_attribute ("Data",  "Source", "OSM");

    buildmap_osm_common_find_layers();
    ret = buildmap_osm_text_read(f, country_num, division_num);
    if (fclose(f) != 0) {
        buildmap_error(0, "problem fetching data");
        ret = -1;
    }

    buildmap_db_sort();
    buildmap_osm_save_custom(BuildMapFileName, ret);

    return ret;
     
}

/**
 * @brief populates a list of tileids necessary to give coverage
 *  for the given position
 * @param position
 * @param focus
 * @param fips
 * @param bits
 * @return
 */
int buildmap_osm_by_position
        (const RoadMapPosition *position,
         const RoadMapArea *focus, int **fips, int bits) {

    int i, d, found, width;
    int tileid;
    char eswn[] = { TILE_EAST, TILE_SOUTH, TILE_WEST, TILE_NORTH};
    RoadMapArea tileedges;
    int ofound;
    int *fipslist;

    fipslist = *fips;

    found = 0;
    tileid = roadmap_osm_latlon2tileid
            (position->latitude, position->longitude, bits);
    fipslist = realloc(fipslist, (found + 1) * sizeof(int));
    buildmap_check_allocated(fipslist);
    fipslist[found++] = tileid;


    width = 1;
    ofound = -1;
    while (found != ofound) {
        /* we keep checking until a trip around the block says
         * that we haven't found any visibility overlaps
         * in a full circuit.
         */
        ofound = found;
        tileid = roadmap_osm_tileid_to_neighbor(tileid, TILE_NORTHWEST);
        width += 2;
        for (d = 0; d < 4; d++) {
            for (i = 0; i < width-1; i++) {
                tileid = roadmap_osm_tileid_to_neighbor(tileid, eswn[d]);
                roadmap_osm_tileid_to_bbox(tileid, &tileedges);
                if (roadmap_math_areas_intersect (&tileedges, focus)) {
                    fipslist = realloc(fipslist, (found + 1) * sizeof(int));
                    buildmap_check_allocated(fipslist);
                    fipslist[found++] = tileid;
                }
            }
        }
    }

    *fips = fipslist;

    return found;

}

/**
 * @brief return tileids needed to for given positoin
 * @param latlonarg
 * @param tilesp
 * @param bits
 * @return
 *
 * Called directly from main to convert lat/lon command line
 * arguments into a tiles list.  The area covered can be specified
 * in several formats.
 */
static int
buildmap_osm_which_tiles(const char *latlonarg, int **tilesp, int bits)
{

    char *end, *latp = 0, *lonp = 0, *latp2 = 0, *lonp2 = 0;
    char *latlon;
    int lat, lon, lat2, lon2;
    int tileid;
    int *tiles = NULL;
    int count;
    RoadMapArea processingEdges;

    latlon = strdup(latlonarg);
    latp = strtok(latlon, ",");
    if (latp != NULL) {
        lonp = strtok(NULL, ":");
        if (lonp != NULL) {
            latp2 = strtok(NULL, ",");
            if (latp2 != NULL) {
                lonp2 = strtok(NULL, ",");
            }
        }
    }

    if (!lonp || !latp)
        buildmap_fatal(0, "bad lat,lon specified");

    lat = strtolatlon(latp, &end);
    if (*end != '\0')
        buildmap_fatal(0, "bad latitude specified");

    lon = strtolatlon(lonp, &end);
    if (*end != '\0')
        buildmap_fatal(0, "bad longitude specified");

    if (latp2) {

        RoadMapPosition pos;

        if (latp2 && lonp2) { /* two lat/lon pairs were given */

            lat2 = strtolatlon(latp2, &end);
            if (*end != '\0')
                buildmap_fatal(0, "bad second latitude specified");

            lon2 = strtolatlon(lonp2, &end);
            if (*end != '\0')
                buildmap_fatal(0, "bad second longitude specified");

            processingEdges.east = (lon >= lon2) ? lon : lon2;
            processingEdges.west = (lon < lon2) ? lon : lon2;
            processingEdges.north = (lat >= lat2) ? lat : lat2;
            processingEdges.south = (lat < lat2) ? lat : lat2;

            pos.latitude = (lat + lat2) / 2;
            pos.longitude = (lon + lon2) / 2;


        } else { /* one lat/lon pair, plus a "radius" specifier */
            double distance;
            distance = strtod(latp2, &end);
            if (strcasecmp(end, "km") && strcasecmp(end, "mi")) {
                buildmap_fatal(0, "bad distance from point specified");
            }

            pos.latitude = lat;
            pos.longitude = lon;
            distance *= 2;
            roadmap_math_bbox_around_point
                (&processingEdges, &pos, distance, end);
        }

        count = buildmap_osm_by_position(&pos, &processingEdges, &tiles, bits);

        buildmap_info("will process %d tiles", count);

    } else {

        /* just a single lat/lon pair was given, so the user gets
         * just a single tile.
         */
        tileid = roadmap_osm_latlon2tileid(lat, lon, bits);
        tiles = malloc(sizeof(tileid));
        buildmap_check_allocated(tiles);
        tiles[0] = tileid;
        count = 1;
    }

    free(latlon);

    *tilesp = tiles;
    return count;
}

/**
 * @brief
 * @param tileid
 * @return
 */
static int roadmap_osm_tile_has_coverage(int tileid) {

    int childid, bits, j;

    if (roadmap_file_exists (BuildMapResult,
            roadmap_osm_filename(NULL, 1, tileid))) {
        return 1;
    }

    bits = tileid2bits(tileid);
    if (bits >= TILE_MAXBITS-1)
        return 0;

    for (j = 0; j < 4; j++) {
        childid = mktileid( (tileid2trutile(tileid) << 2) | j, bits+2);
        if (!roadmap_osm_tile_has_coverage(childid))
            return 0;
    }

    return 1;
}

/**
 * @brief called directly from main to convert a tiles list
 * @param tiles
 * @param bits
 * @param count
 * @param source
 * @param cmdfmt
 * @return
 */
static int
buildmap_osm_process_tiles (int *tiles, int bits, int count,
                const char *source, const char *cmdfmt)
{
    int i, ret = 0;
    int nbits;
    char name[64];

    for (i = 0; i < count; i++) {

        int tileid = tiles[i];

        roadmap_osm_filename(name, 1, tileid);
        if (!BuildMapReplaceAll && roadmap_osm_tile_has_coverage(tileid)) {
            buildmap_info("have coverage for tile %s already, skipping", name);
            continue;
        }

        if (strcasecmp(BuildMapFormat, "osmbinary") == 0) {

            buildmap_info("");
            buildmap_info
                ("processing tileid '%d', for file '%s'", tileid, name);

            ret = buildmap_osm_process_one_tile (tileid, source, cmdfmt);

            if (ret == -2) {
                if (bits >= TILE_MAXBITS-1) {
                    buildmap_info("can't split tile 0x%x further", tileid);
                    continue;
                }
                /* we got a "tile too big" error.  try for four
                 * subtiles instead -- put them at the end of the list.
                 */
                nbits = tileid2bits(tileid);
                buildmap_info
                    ("splitting tile 0x%x, new bits %d", tileid, nbits+2);
                count += 4;
                tiles = realloc(tiles, sizeof(*tiles) * count);
                buildmap_check_allocated(tiles);
                roadmap_osm_tilesplit(tileid, &tiles[count - 4], 2);
                continue;
            }
    
            if (ret < 0) break;
        }

        if (ret > 0) buildmap_db_sort();

        if (buildmap_is_verbose()) {
           roadmap_hash_summary();
           buildmap_db_summary();
        }

        buildmap_osm_save(tileid, ret);

        buildmap_db_reset();
        roadmap_hash_reset();
    }

    return ret < 0;
}

/**
 * @brief prints as much information as possible about the given hex tileid
 * @param decode
 * @return
 */
int buildmap_osm_decode(char *decode) {

    char *p, *end;
    int tileid;
    RoadMapArea e[1];
    char lat[16], lon[16];
    RoadMapPosition pos1, pos2;

    /* strip leading "0x", or non-hex characters */
    if (strncasecmp("0x", decode, 2) == 0) {
        decode += 2;
    } else {
        while (!isxdigit(*decode)) decode++;
    }

    if (!*decode) return 1;

    /* strip any suffix */
    p = strchr(decode, '.');

    if (p)
        *p-- = '\0';
    else
        p = &decode[strlen(decode) - 1];

    /* strip any trailing non-hex */
    while (p > decode && !isxdigit(*p)) p--;

    if (p == decode) return 1;

    tileid = strtol(decode, &end, 16);

    if (*end) return 1;

    roadmap_osm_tileid_to_bbox(tileid, e);

    printf("tileid:\t0x%08x\t%d\n", tileid, tileid);
    printf("true tileid:\t0x%08x\t%d\n", 
        tileid2trutile(tileid), tileid2trutile(tileid));
    printf("bits:\t%d\n", tileid2bits(tileid));

    pos1.latitude = (e->north + e->south)/2;
    pos1.longitude = (e->west + e->east)/2;
    printf("center:\t%s\t%s\n", 
        roadmap_math_to_floatstring(lat, pos1.latitude, MILLIONTHS),
        roadmap_math_to_floatstring(lon, pos1.longitude, MILLIONTHS));
    printf("north:\t%s\n", 
        roadmap_math_to_floatstring(NULL, e->north, MILLIONTHS));
    printf("south:\t%s\n", 
        roadmap_math_to_floatstring(NULL, e->south, MILLIONTHS));
    printf("east:\t%s\n", 
        roadmap_math_to_floatstring(NULL, e->east, MILLIONTHS));
    printf("west:\t%s\n", 
        roadmap_math_to_floatstring(NULL, e->west, MILLIONTHS));

    roadmap_math_use_metric ();
    roadmap_math_set_center (&pos1);
    /* upper left */
    pos1.latitude = e->north;
    pos1.longitude = e->west;
    /* upper right */
    pos2.latitude = e->north;
    pos2.longitude = e->east;

    printf("width km:\t%s\n",
        roadmap_math_to_floatstring
            (NULL, roadmap_math_to_trip_distance_tenths
                        (roadmap_math_distance(&pos1, &pos2)) , TENTHS));
    /* lower left */
    pos2.latitude = e->south;
    pos2.longitude = e->west;
    printf("height km:\t%s\n",
        roadmap_math_to_floatstring
            (NULL, roadmap_math_to_trip_distance_tenths
                        (roadmap_math_distance(&pos1, &pos2)) , TENTHS));
    
    return 0;

}

/**
 * @brief converts position to tileids
 * @param latlon
 * @param bits
 * @return
 *
 * Prints the RoadMap and OSM (true) tileids corresponding to the
 * given position and bitlength.
 */
int buildmap_osm_encode(char *latlon, int bits) {

    char *latp, *lonp = NULL;
    char *end;
    int lat, lon;
    int tileid;

    latp = strtok(latlon, ",");
    if (latp != NULL) {
        lonp = strtok(NULL, "");
    }

    if (!lonp || !latp) {
        buildmap_fatal(0, "bad lat,lon specified");
        return 1;
    }

    lat = strtolatlon(latp, &end);
    if (*end != '\0') {
        buildmap_fatal(0, "bad latitude specified");
        return 1;
    }

    lon = strtolatlon(lonp, &end);
    if (*end != '\0') {
        buildmap_fatal(0, "bad longitude specified");
        return 1;
    }

    tileid = roadmap_osm_latlon2tileid(lat, lon, bits);

    printf("tileid:\t0x%08x\t%d\n", tileid, tileid);
    printf("true tileid:\t0x%08x\t%d\n",
                    tileid2trutile(tileid),
                    tileid2trutile(tileid));
    printf("bits:\t%d\n", bits);
    return 0;
}

/**
 * @brief print help text
 * @param progpath
 * @param msg
 */
void usage(char *progpath, const char *msg) {

    char *prog = strrchr(progpath, '/');

    if (prog)
        prog++;
    else
        prog = progpath;

    if (msg)
        fprintf(stderr, "%s: %s\n", prog, msg);
    fprintf(stderr,
        "usage: %s [options] lat,lon[:lat,lon] -or- lat,lon:NN{mi|km}\n"
	"  Use '--' after the options if the 'lat' value is negative.\n",
	prog);
    opt_desc(options, 1);
    exit(1);
}

/**
 * @brief main program
 * @param argc
 * @param argv
 * @return
 */
int
main(int argc, char **argv)
{

    int error;
    int verbose, quiet;
    int osm_bits;
    int count;
    int *tileslist;
    char *decode, *encode;
    int tileid;
    char *class, *latlonarg, *source, *cmdfmt, *inputfile;

    BuildMapResult = strdup(roadmap_path_preferred("maps")); /* default. */

    /* parse the options */
    error = opt_parse(options, &argc, argv, 0);
    if (error) usage(argv[0], opt_strerror(error));

    /* then, fetch the option values */
    error = opt_val("verbose", &verbose) ||
            opt_val("quiet", &quiet) ||
            opt_val("format", &BuildMapFormat) ||
            opt_val("class", &class) ||
            opt_val("bits", &osm_bits) ||
            opt_val("replace", &BuildMapReplaceAll) ||
            opt_val("maps", &BuildMapResult) ||
            opt_val("source", &source) ||
            opt_val("tileid", &tileid) ||
            opt_val("decode", &decode) ||
            opt_val("encode", &encode) ||
            opt_val("outputfile", &BuildMapFileName) ||
            opt_val("inputfile", &inputfile);
    if (error)
        usage(argv[0], opt_strerror(error));

    if (osm_bits < TILE_MINBITS || TILE_MAXBITS < osm_bits) {
        fprintf (stderr, "bits value %d out of range (%d to %d)",
                osm_bits, TILE_MINBITS, TILE_MAXBITS);
        exit(1);
    }

    if (*decode) {
        exit ( buildmap_osm_decode(decode) );
    }

    if (*encode) {
        exit ( buildmap_osm_encode(encode, osm_bits) );
    }

    if (strcasecmp(BuildMapFormat, "osmtext") != 0) {
    } else if (strcasecmp(BuildMapFormat, "osmbinary") != 0) {
        fprintf (stderr,
            "unsupported protocol input format %s", BuildMapFormat);
        exit(1);
    }

    if (!*source) {
        usage (argv[0], "missing source (commandname or URL) option");
    }

    if (strncmp(source, "http:", 5) == 0) {
        cmdfmt = "wget -q -O - '%s?tile=%d&ts=%d&have=%d'";
    } else {
        cmdfmt = "%s -t %d -b %d -h %d";
    }

    if (verbose || quiet)
        buildmap_message_adjust_level (verbose - quiet);

    buildmap_layer_load(class);

    buildmap_metadata_add_attribute ("MapFormat", "Version", "1.3 alpha");

    buildmap_verbose("processing with bits '%d'", osm_bits);

    if (tileid) {

        tileslist = malloc(sizeof(int));
        buildmap_check_allocated(tileslist);
        *tileslist = tileid;
        count = 1;

    } else if (*inputfile) {
            exit(buildmap_osm_text_process_file(inputfile));
    } else {

        if (argc < 2)
            usage(argv[0], "missing required arguments");

        if (argc > 2)
            usage(argv[0], "too many arguments");

        latlonarg = argv[1];

        count = buildmap_osm_which_tiles(latlonarg, &tileslist, osm_bits);
    }

    error = buildmap_osm_process_tiles
                (tileslist, osm_bits, count, source, cmdfmt);

    free (tileslist);

    if (error) exit(1);

    exit(0);
}
