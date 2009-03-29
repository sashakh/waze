/*
 * LICENSE:
 *
 *   Copyright 2007 Paul Fox
 *   Copyright (c) 2009, Danny Backx.
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
 * @brief a module to read OSM Mobile Binary format
 *
 * The protocol is documented in
 *    http://wiki.openstreetmap.org/index.php/OSM_Mobile_Binary_Protocol ,
 * and
 *    http://wiki.openstreetmap.org/index.php/OSM_Binary_Format ,
 * the current OSM protocol is in
 *    http://wiki.openstreetmap.org/index.php/OSM_Protocol_Version_0.5 .
 * More generic protocol documentation:
 *    http://wiki.openstreetmap.org/index.php/Protocol .
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>


#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_osm.h"
#include "roadmap_line.h"

#include "buildmap.h"
#include "buildmap_zip.h"
#include "buildmap_city.h"
#include "buildmap_square.h"
#include "buildmap_point.h"
#include "buildmap_line.h"
#include "buildmap_street.h"
#include "buildmap_range.h"
#include "buildmap_area.h"
#include "buildmap_shape.h"
#include "buildmap_polygon.h"

#include "buildmap_layer.h"
#include "buildmap_osm_common.h"
#include "buildmap_osm_binary.h"

extern char *BuildMapResult;

/**
 * @brief
 * @param d
 * @param string
 * @return
 */
static RoadMapString str2dict (BuildMapDictionary d, const char *string) {

   if (!strlen(string)) {
      return buildmap_dictionary_add (d, "", 0);
   }

   return buildmap_dictionary_add (d, (char *) string, strlen(string));
}

/* the major options coming back from the server can indicate
 * four types of values.  those with a value that is itself an
 * enumerated type, and those with arbitrary numeric, time, or
 * string values.
 */
#define key_enumerated_start    1
#define key_numeric_start       128
#define key_date_start          176
#define key_string_start        192

#define STRING_NAME 0
#define STRING_REF 9

static unsigned char *
read_4_byte_int(unsigned char *p, int *r)
{
    *r = (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];
    return p + 4;
}

static unsigned char *
read_2_byte_int(unsigned char *p, int *r)
{
    short s;
    s = (p[1] << 8) + p[0];
    *r = s;
    return p + 2;
}

/**
 * @brief look ahead and figure out the additional info (options)
 *  like the road type
 * @param cur
 * @param end
 * @param name
 * @param ref
 * @param flagp
 * @return the layer that this way is in
 */
static int
buildmap_osm_binary_parse_options
        (unsigned char *cur, unsigned char *end,
            char *name, char *ref, int *flagp)
{

    int key, slen;
    int layer = 0;

    if (buildmap_is_verbose()) {
        fprintf(stderr, "option data is\n");
        int i;
        for (i = 0; i < end - cur; i++) {
            fprintf(stderr, " %02x", cur[i]);
        }
        fprintf(stderr, "\n");

    }

    if (name != NULL)
        *name = '\0';
    if (ref != NULL)
        *ref = '\0';

    while (cur < end) {
        key = *cur++;
        if (key >= key_string_start) {

            key -= key_string_start;
            slen = *cur++;

            buildmap_verbose("'%s' is %.*s", stringtype[key], slen, cur);

            if (key == STRING_NAME && name != NULL) { /* regular "name" */
                memcpy(name, cur, slen);
                name[slen] = '\0';
            }
            if (key == STRING_REF && ref != NULL) { /* usually route number */
                int i;
                memcpy(ref, cur, slen);
                for (i = 0; i < slen; i++) {
                    /* multiple route numbers separated by ';' in tag */
                    if (ref[i] == ';') ref[i] = '/';
                }
                ref[slen] = '\0';
            }
            cur += slen;

        } else if (key >= key_date_start) {
            time_t tim;
            int itime;

            key -= key_date_start;

            cur = read_4_byte_int(cur, &itime);
            tim = (time_t)itime;

            buildmap_verbose
                ("'%s' is %s", datetype[key], asctime(gmtime(&tim)));

        } else if (key >= key_numeric_start) {
            int val;

            key -= key_numeric_start;
            cur = read_4_byte_int(cur, &val);

            buildmap_verbose("'%s' is %ld", stringtype[key], val);

        } else {
            int val;

            /* the rest are enumerated types, and we have lists for
             * the ones we're interested in.
             */
            val = *cur++;

            buildmap_verbose
                ("list type is '%s', subtype ", list_info[key].name);

            if (list_info[key].list) {
                buildmap_verbose("'%s'", list_info[key].list[val].name);

                if (list_info[key].list[val].layerp) {
                    layer = *list_info[key].list[val].layerp;
                    if (flagp) *flagp = list_info[key].list[val].flags;
                    buildmap_verbose("maps with list to layer %d", layer);
                }
            } else {
                buildmap_verbose("%d", val);

                if (list_info[key].layerp) {
                    layer = *list_info[key].layerp;
                    buildmap_verbose("maps to layer %d", layer);
                }
            }

        }
    }
    return layer;
}

/**
 * @brief
 * @param data
 * @param len
 * @return
 */
static int
buildmap_osm_binary_node(unsigned char *data, int len)
{
    int id, lon, lat;
    unsigned char *dp = data;
    int prop;
    int layer;

    dp = read_4_byte_int(dp, &id);
    dp = read_4_byte_int(dp, &lon);
    dp = read_4_byte_int(dp, &lat);
    prop = *dp++;

    buildmap_verbose("node: id %ld, lon %ld, lat %ld, prop %d",
                id, lon, lat, prop);

    if (dp < data + len)
        layer = buildmap_osm_binary_parse_options
                    (dp, data + len, NULL, NULL, NULL);

    /* currently, RoadMap has no ability to display simple
     * points, or "nodes" in OSM vocabulary.  so we do this
     * parsing, then drop it on the floor.
     */

    return 0;
}


struct shapeinfo {
    int lineid;
    int count;
    int *lons;
    int *lats;
};

static int numshapes;
static struct shapeinfo *shapes;

/**
 * @brief
 * @param data
 * @param len
 * @return
 */
static int
buildmap_osm_binary_way(unsigned char *data, int len)
{
    int id, count, savecount;
    int *lonp, *latp;
    unsigned char *dp, *op;
    int layer, flags, line, street, j;
    int frlong, frlat, tolong, tolat, from_point, to_point;
    char name[257];
    char ref[257];
    char compound_name[513];
    char *n;
    static int *lonsbuf, *latsbuf;
    static int lineid = 1;

    RoadMapString rms_dirp = str2dict(DictionaryPrefix, "");
    RoadMapString rms_dirs = str2dict(DictionarySuffix, "");
    RoadMapString rms_type = str2dict(DictionaryType, "");
    RoadMapString rms_name;

    dp = data;
    dp = read_4_byte_int(dp, &id);
    dp = read_4_byte_int(dp, &count);

    /* look ahead to find the options */
    op = dp + 4 + 4 + (count - 1) * (2 + 2);
    if (op >= data + len)
        return 0;

    layer = buildmap_osm_binary_parse_options
                (op, data + len, name, ref, &flags);

    buildmap_verbose("'%s' (ref '%s') is in layer %d", name, ref, layer);

    if (*ref) {
        if (*name) {
            sprintf(compound_name, "%s/%s", ref, name);
            n = compound_name;
        } else {
            n = ref;
        }
    } else {
        n = name;
    }

    if (!layer)
        return 0;

    savecount = count;

    lonp = lonsbuf = realloc(lonsbuf, count * sizeof(long));
    buildmap_check_allocated(lonsbuf);
    dp = read_4_byte_int(dp, lonp);

    latp = latsbuf = realloc(latsbuf, count * sizeof(long));
    buildmap_check_allocated(latsbuf);
    dp = read_4_byte_int(dp, latp);

    buildmap_verbose("way: id %ld", id);
    buildmap_verbose("    lon %ld, lat %ld", *lonp, *latp);

    count--;
    while (count--) {
        int tmp;

        dp = read_2_byte_int(dp, &tmp);
        *(lonp + 1) = *lonp + tmp;

        dp = read_2_byte_int(dp, &tmp);
        *(latp + 1) = *latp + tmp;

        lonp++;
        latp++;
        buildmap_verbose("    lon %ld, lat %ld", *lonp, *latp);
    }

    frlong = *lonsbuf;
    frlat = *latsbuf;

    tolong = *lonp;
    tolat = *latp;

    if ((flags & AREA) && (frlong == tolong) && (frlat == tolat)) {
        static int landid;
        static int polyid;
        static int cenid;

        buildmap_verbose("looks like a polygon");

        buildmap_polygon_add_landmark
            (++landid, layer, 0 /* RoadMapString name */ );

        buildmap_polygon_add(landid, ++cenid, ++polyid);

        count = savecount - 1;
        frlong = *lonsbuf;
        frlat = *latsbuf;
        from_point = buildmap_point_add(frlong, frlat);
        lonp = lonsbuf;
        latp = latsbuf;
        while (count--) {

            tolong = *(lonp + 1);
            tolat = *(latp + 1);

            to_point = buildmap_point_add(tolong, tolat);
            line = buildmap_line_add(++lineid, layer, from_point, to_point,
			    ROADMAP_LINE_DIRECTION_BOTH);

            buildmap_verbose("from: %d, %d        to: %d, %d      lineid %d",
                   frlong, frlat, tolong, tolat, lineid);

            buildmap_polygon_add_line
                (cenid, polyid, lineid, POLYGON_SIDE_RIGHT);
            frlong = tolong;
            frlat = tolat;
            from_point = to_point;
            lonp++;
            latp++;
        }
    } else {

        from_point = buildmap_point_add(frlong, frlat);
        to_point = buildmap_point_add(tolong, tolat);
        line = buildmap_line_add(++lineid, layer, from_point, to_point,
			ROADMAP_LINE_DIRECTION_BOTH);

        buildmap_verbose("from: %d, %d    to: %d, %d      lineid %d",
               frlong, frlat, tolong, tolat, lineid);

        rms_name = str2dict(DictionaryStreet, n);
        street =
            buildmap_street_add(layer, rms_dirp, rms_name, rms_type,
                                rms_dirs, line);

        buildmap_range_add_no_address(line, street);

        for (j = 0; j < lonp - lonsbuf + 1; j++) {
            buildmap_square_adjust_limits(lonsbuf[j], latsbuf[j]);
        }

        shapes = realloc(shapes,
                    ((numshapes + 1024) & ~1023) * sizeof(*shapes));
        buildmap_check_allocated(shapes);
        shapes[numshapes].lons = lonsbuf;
        shapes[numshapes].lats = latsbuf;
        shapes[numshapes].count = lonp - lonsbuf + 1;
        shapes[numshapes].lineid = lineid;
        lonsbuf = latsbuf = 0;
        numshapes++;
    }

    return 1;
}

/**
 * @brief
 */
static int
buildmap_osm_binary_ways_pass2(void)
{
    int i, j, count, lineid;
    int *lons, *lats;
    int line_index;
    buildmap_info("loading shape info ...");
    buildmap_line_sort();
    for (i = 0; i < numshapes; i++) {

        lons = shapes[i].lons;
        lats = shapes[i].lats;
        count = shapes[i].count;
        lineid = shapes[i].lineid;
        if (count <= 2)
            continue;
        line_index = buildmap_line_find_sorted(lineid);
        if (line_index >= 0) {

            // Add the shape points here
            for (j = 1; j < count - 1; j++) {
                buildmap_shape_add
                    (line_index, i, lineid, j - 1, lons[j], lats[j]);
            }
        }
        if ((i & 0xff) == 0) {
            buildmap_progress(i, numshapes);
        }

    }
    return 1;
}

/**
 * @brief
 * @param data
 * @return
 */
static int
buildmap_osm_binary_time(unsigned char *data)
{
    int itime;
    time_t time;

    /* we do nothing with this (for now?) */
    read_4_byte_int(data, &itime);

    time = (time_t)itime;

    buildmap_verbose("Creation time is '%d', '%s'",
               itime, asctime(gmtime(&time)));
    return 0;
}

/**
 * @brief
 * @param data
 * @return
 */
static int
buildmap_osm_binary_error(unsigned char *data)
{
    int error = *data++;
    int slen = *data++;
    buildmap_error(0, "Error %d: '%.*s'", error, slen, data);
    if (error == 5)
        return -2;
    return -1;
}

/**
 * @brief
 * @param fdata
 * @return
 */
int
buildmap_osm_binary_read(FILE * fdata)
{
    unsigned char *block = NULL;
    int length, got;
    unsigned char type;
    unsigned char buf[5];;
    int n;
    int ret = 0, ranways = 0;

    DictionaryPrefix = buildmap_dictionary_open("prefix");
    DictionaryStreet = buildmap_dictionary_open("street");
    DictionaryType = buildmap_dictionary_open("type");
    DictionarySuffix = buildmap_dictionary_open("suffix");
    DictionaryCity = buildmap_dictionary_open("city");

    n = 1;

    while (ret >= 0) {
        buildmap_set_line(n);
        got = (int)fread(buf, 1, 5, fdata);
        if (got != 5) {
            if (feof(fdata))
                break;
            buildmap_fatal(0, "short read (length)");
        }

        /* four bytes of length */
        read_4_byte_int(buf, &length);

        buildmap_verbose("length is %ld", length);

        /* fifth byte is type */
        type = buf[4];
        buildmap_verbose("type is %c", type);

        block = realloc(block, length - 1);
        buildmap_check_allocated(block);
        got = (int)fread(block, 1, length - 1, fdata);
        if (got != length - 1) {
            if (feof(fdata))
                break;
            buildmap_fatal(0, "short read (data)");
        }

        if (0 && buildmap_is_verbose()) {
            fprintf(stderr, "data is");
            int i;
            for (i = 0; i < length - 1; i++) {
                fprintf(stderr, " %02x", block[i]);
            }
            fprintf(stderr, "\n");
        }

        switch (type) {

        case 'n':               // node
            ret += buildmap_osm_binary_node(block, length - 1);
            break;

        case 'w':               // way
            if (buildmap_osm_binary_way(block, length - 1)) {
                ret++;
                ranways = 1;
            }
            break;

        case 'o':               // omit
            // ret += buildmap_osm_binary_omit(block);
            break;

        case 't':               // time
            ret += buildmap_osm_binary_time(block);
            break;

        case 'f':               // failure
            ret = buildmap_osm_binary_error(block);
            break;

        default:
            break;
        }
    }

    if (ranways) buildmap_osm_binary_ways_pass2();

    if (block) free(block);

    return ret;
}
