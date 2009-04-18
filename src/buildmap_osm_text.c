/*
 * LICENSE:
 *
 *   Copyright (c) 2008, 2009, Danny Backx.
 *
 *   Based on code Copyright 2007 Paul Fox that interprets the OSM
 *   binary stream.
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
 * @brief a module to read OSM text format
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

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
#include "buildmap_osm_text.h"


/**
 * @brief a couple of variables to keep track of the way we're dealing with
 */
static int      in_way = 0;             /**< are we in a way (its id) */
static int      nWayNodes = 0;          /**< number of nodes known for 
                                                        this way */
static int      nWayNodeAlloc = 0;      /**< size of the allocation of
                                                        the array */
static int      *WayNodes = 0;          /**< the array to keep track of
                                                        this way's nodes */
static int      WayLayer = 0;           /**< the layer for this way */
static char     *WayStreetName = 0;     /**< the street name */
static char	*WayStreetRef = 0;	/**< street code,
					  to be used when no name (e.g. motorway) */
static int      WayFlags = 0;           /**< properties of this way, from
                                                        the table flags */
static int      WayInvalid = 0;         /**< this way contains invalid nodes */
static int	WayIsOneWay = ROADMAP_LINE_DIRECTION_BOTH;
					/**< is this way one direction only */

/**
 * @brief variables referring to the current node
 */
static int      NodeId = 0;             /**< which node */
static char     *NodePlace = 0;         /**< what kind of place is this */
static char     *NodeTownName = 0;      /**< which town */
static char     *NodePostalCode = 0;    /**< postal code */
static int      NodeLon,                /**< coordinates */
                NodeLat;                /**< coordinates */

static int      NodeFakeFips;           /**< fake postal code */
static int      WayLandUseNotInteresting = 0;   /**< land use not interesting
                                                        for RoadMap */

/**
 * @brief some global variables
 */
static int      LineNo;                 /**< line number in the input file */
static int      nPolygons = 0;          /**< current polygon id (number of
                                                        polygons until now) */

static int      LineId = 0;             /**< for buildmap_line_add */
static int      nsplits = 0;            /**< number of times we've split
                                                        a way */
static int      WaysSplit = 0,  /**< Number of ways that were split */
                WaysNotSplit = 0;       /**< Number of ways not split */

/**
 * @brief allow the user to specify a bounding box
 */
#if 0
        /* Mallorca ? */
int     HaveLonMin = 1,
        HaveLonMax = 1,
        HaveLatMin = 1,
        HaveLatMax = 1,
        LonMin =  2200000,
        LonMax =  3150000,
        LatMin = 39100000,
        LatMax = 39950000;
#else
int     HaveLonMin = 0,
        HaveLonMax = 0,
        HaveLatMin = 0,
        HaveLatMax = 0,
        LonMin = 0,
        LonMax = 0,
        LatMin = 0,
        LatMax = 0;
#endif

/**
 * @brief table for translating the names in XML strings into readable format
 */
static struct XmlIsms {
        char    *code;          /**< the piece between & and ; */
        char    *string;        /**< translates into this */
} XmlIsms[] = {
        { "apos",       "'" },
        { "gt",         ">" },
        { "lt",         "<" },
        { "quot",       """" },
        { "amp",        "&" },
        { NULL,         NULL }  /* end */
};

/**
 * @brief remove XMLisms such as &apos; and strdup
 * limitation : names must be shorter than 256 bytes
 * @param s input string
 * @return duplicate, to be freed elsewhere
 */
static char *FromXmlAndDup(const char *s)
{
        int             i, j, k, l, found;
        static char     res[256];

        for (i=j=0; s[i]; i++)
                if (s[i] == '&') {
                        found = 0;
                        for (k=0; XmlIsms[k].code && !found; k++) {
                                for (l=0; s[l+i+1] == XmlIsms[k].code[l]; l++)
                                        ;
                                /* When not equal, must be at end of code */
                                if (XmlIsms[k].code[l] == '\0' &&
                                                s[l+i+1] == ';') {
                                        found = 1;
                                        i += l+1;
                                        for (l=0; XmlIsms[k].string[l]; l++)
                                                res[j++] = XmlIsms[k].string[l];
                                }
                        }
                        if (!found)
                                res[j++] = s[i];
                } else {
                        res[j++] = s[i];
                }
        res[j] = '\0';
        return strdup(res);
}

/**
 * @brief reset all the info about this way
 */
static void
buildmap_osm_text_reset_way(void)
{
        in_way = 0;
        nWayNodes = 0;
        free(WayStreetName); WayStreetName = 0;
        free(WayStreetRef); WayStreetRef = 0;
        WayFlags = 0;
        WayInvalid = 0;
	WayIsOneWay = ROADMAP_LINE_DIRECTION_BOTH;
}

/**
 * @brief reset all the info about this node
 */
static void
buildmap_osm_text_reset_node(void)
{
        NodeId = 0;
        free(NodePlace); NodePlace = 0;
        free(NodeTownName); NodeTownName = 0;
        free(NodePostalCode); NodePostalCode = 0;
}

/**
 * @brief
 * @param d
 * @param string
 * @return
 */
static RoadMapString str2dict (BuildMapDictionary d, const char *string) {

   if (string == 0 || !strlen(string)) {
      return buildmap_dictionary_add (d, "", 0);
   }

   return buildmap_dictionary_add (d, (char *) string, strlen(string));
}

/**
 * @brief simplistic way to gather point data
 */
static int nPointsAlloc = 0;
static int nPoints = 0;
static struct points {
        int     id;
        int     npoint;
} *points = 0;

static void
buildmap_osm_text_point_add(int id, int npoint)
{
        if (nPoints == nPointsAlloc) {
                nPointsAlloc += 10000;
                points = realloc(points, sizeof(struct points) * nPointsAlloc);
                if (!points)
                        buildmap_fatal(0, "allocate %d points", nPointsAlloc);
        }

        points[nPoints].id = id;
        points[nPoints++].npoint = npoint;
}

/**
 * @brief look up a point by id
 * @param id the external representation
 * @return the internal index
 */
static int
buildmap_osm_text_point_get(int id)
{
        int     i;

        /* fix me, this needs to be speed up */
        for (i=0; i<nPoints; i++)
                if (points[i].id == id)
                        return points[i].npoint;
        return -1;
}

/**
 * @brief collect node data in pass 1
 * @param data
 * @return an error indicator
 *
 * The input line is discarded if a bounding box is specified and
 * the node is outside.
 *
 * Example input line :
 *   <node id="123295" timestamp="2005-07-05T03:26:11Z" user="LA2"
 *      lat="50.4443626" lon="3.6855288"/>
 */
int
buildmap_osm_text_node(char *data)
{
    int         npoints, nchars, r;
    double      flat, flon;
    char        *p;
    static char *tag = 0, *value = 0;
    int         NodeLatRead, NodeLonRead;

    sscanf(data, "node id=%*[\"']%d%*[\"']%n", &NodeId, &nchars);
    p = data + nchars;

    /* Initialize/allocate these only once, never free them. */
    if (! tag)
            tag = malloc(512);
    if (! value)
            value = malloc(512);

    NodeLat = NodeLon = 0;
    NodeLatRead = NodeLonRead = 0;

    while (NodeLatRead == 0 || NodeLonRead == 0) {
            for (; *p && isspace(*p); p++) ;
            r = sscanf(p, "%[a-zA-Z0-9_]=%*[\"']%[^\"']%*[\"']%n",
                        tag, value, &nchars);

            if (strcmp(tag, "lat") == 0) {
                    sscanf(value, "%lf", &flat);
                    NodeLat = flat * 1000000;
                    NodeLatRead++;
            } else if (strcmp(tag, "lon") == 0) {
                    sscanf(value, "%lf", &flon);
                    NodeLon = flon * 1000000;
                    NodeLonRead++;
            }

            p += nchars;
    }

    if ((HaveLonMin && (NodeLon < LonMin))
    ||  (HaveLonMax && (NodeLon > LonMax))
    ||  (HaveLatMin && (NodeLat < LatMin))
    ||  (HaveLatMax && (NodeLat > LatMax))) {

            /* Outside the specified bounding box, ignore this node */
            NodeLat = NodeLon = 0;
            NodeLatRead = NodeLonRead = 0;

            return 1;
    }

    npoints = buildmap_point_add(NodeLon, NodeLat);
    buildmap_osm_text_point_add(NodeId, npoints);       /* hack */

    return 0;
}

/**
 * @brief At the end of a node, process its data
 * @param data point to the line buffer
 * @return error indication
 */
int
buildmap_osm_text_node_end(char *data)
{
        if (NodePlace && strcmp(NodePlace, "town") == 0) {
                /* We have a town, process it */
                if (NodeTownName && NodePostalCode) {
                        buildmap_verbose("Node %d town %s postal %s",
                                        NodeId, NodeTownName, NodePostalCode);
                }

                if (NodeTownName) {
                        NodeFakeFips++;
//                      buildmap_verbose ("buildmap_osm_text_node_end: "
//                                          "fake fips %d\n", NodeFakeFips);
                        int year = 2008;
#if 0
                        buildmap_verbose("Node %d town %s, no postal code",
                                        NodeId, NodeTownName);
#endif
                        RoadMapString s;
                        s = buildmap_dictionary_add (DictionaryCity,
                                (char *) NodeTownName, strlen(NodeTownName));
                        buildmap_city_add(NodeFakeFips, year, s);
                }
                if (NodePostalCode) {
                        int zip = 0;
                        sscanf(NodePostalCode, "%d", &zip);
                        if (zip)
                                buildmap_zip_add(zip, NodeLon, NodeLat);
                }
        }

        buildmap_osm_text_reset_node();
        return 0;
}

/**
 * @brief this structure keeps shape data for a postprocessing step
 */
struct shapeinfo {
    int lineid;
    int count;
    int *lons;
    int *lats;
};

static int numshapes = 0;
static int nallocshapes = 0;

/**
 * @brief ?
 */
static struct shapeinfo *shapes;

/*
 * Count nodes
 * Watch out : should be called in Pass 1.
 */
struct node_counter {
        int     count;
        int     node;
};
struct node_counter_row {
        int     max, alloc;
        struct node_counter *row;
};

static struct node_counter_row nc[256];
static int node_counter_init = 0;

/**
 * @brief add one to the use counter of this node
 * @param node a node id
 */
static void CountNode(int node)
{
        int     i, row;

        if (! node_counter_init) {
                node_counter_init++;
                for (i=0; i<256; i++) {
                        nc[i].max = nc[i].alloc = 0;
                        nc[i].row = NULL;
                }
        }

        row = node % 256;
        for (i=0; i<nc[row].max; i++)
                if (node == nc[row].row[i].node) {
                        nc[row].row[i].count++;
                        return;
                }

        if (nc[row].alloc == nc[row].max) {
                nc[row].alloc += 256;
                nc[row].row = realloc(nc[row].row,
                                sizeof(struct node_counter) * nc[row].alloc);
        }

        i = nc[row].max;
        nc[row].row[i].node = node;
        nc[row].row[i].count = 1;
        nc[row].max++;
}

/**
 * @brief report the number of uses of this node
 * @param node the node id
 * @return the number of uses
 */
static int NodeReportUse(int node)
{
        int row = node % 256;
        int i;

        for (i=0; i<nc[row].max; i++)
                if (node == nc[row].row[i].node) {
                        return nc[row].row[i].count;
		}
        buildmap_fatal(0, "NodeReportUse %d", node);
        return -1;
}

/**
 * @brief
 * @param data
 * @return
 *
 * Sample XML :
 *   <way id="75146" timestamp="2006-04-28T15:24:05Z" user="Mercator">
 *     <nd ref="997466"/>
 *     <nd ref="997470"/>
 *     <nd ref="1536769"/>
 *     <nd ref="997472"/>
 *     <nd ref="1536770"/>
 *     <nd ref="997469"/>
 *     <tag k="highway" v="residential"/>
 *     <tag k="name" v="Rue de Thiribut"/>
 *     <tag k="created_by" v="JOSM"/>
 *   </way>
 */
static int
buildmap_osm_text_way(char *data)
{
        /* Severely cut in pieces.
         * This only remembers which way we're in...
         */
        sscanf(data, "way id=%*[\"']%d%*[\"']", &in_way);
        WayLandUseNotInteresting = 0;

        if (in_way == 0)
                buildmap_fatal(0, "buildmap_osm_text_way(%s) error", data);
        return 0;
}

/**
 * @brief
 * @param data points into the line of text being processed
 * @return error indication
 *
 * Example line :
 *     <nd ref="997470"/>
 */
static int
buildmap_osm_text_nd_pass1(char *data)
{
        int     node;

        if (sscanf(data, "nd ref=%*[\"']%d%*[\"']", &node) != 1)
                return -1;

        CountNode(node);
        return 0;
}

/**
 * @brief
 * @param data points into the line of text being processed
 * @return error indication
 *
 * Example line :
 *     <nd ref="997470"/>
 */
static int
buildmap_osm_text_nd(char *data)
{
        int     node, ix;
        float   lon, lat;

        if (! in_way)
                buildmap_fatal(0, "Wasn't in a way (%s)", data);

        if (sscanf(data, "nd ref=%*[\"']%d%*[\"']", &node) != 1)
                return -1;

        ix = buildmap_osm_text_point_get(node);
        if (ix < 0) {
                /* Inconsistent OSM file, this node is not defined */
                WayInvalid = 1;
                return 0;
        }
        lon = buildmap_point_get_longitude(ix);
        lat = buildmap_point_get_latitude(ix);

        if (nWayNodes == nWayNodeAlloc) {
                nWayNodeAlloc += 100;
                WayNodes = 
                    (int *)realloc(WayNodes, sizeof(int) * nWayNodeAlloc);
                if (WayNodes == 0)
                        buildmap_fatal
                            (0, "allocation failed for %d ints", nWayNodeAlloc);
        }
        WayNodes[nWayNodes++] = node;

        return 0;
}

/**
 * @brief deal with tag lines outside of ways
 * @param data points into the line of text being processed
 * @return error indication
 */
static int
buildmap_osm_text_node_tag(char *data)
{
        static char *tagk = 0;
        static char *tagv = 0;

        if (! tagk)
                tagk = malloc(512);
        if (! tagv)
                tagv = malloc(512);

        sscanf(data, "tag k=%*['\"]%[^\"']%*['\"] v=%*['\"]%[^\"']%*['\"]",
                        tagk, tagv);

        if (strcmp(tagk, "postal_code") == 0) {
                /* <tag k="postal_code" v="3020"/> */
                if (NodePostalCode)
                        free(NodePostalCode);
                NodePostalCode = strdup(tagv);
        } else if (strcmp(tagk, "place") == 0) {
                /* <tag k="place" v="town"/> */
                if (NodePlace)
                        free(NodePlace);
                NodePlace = strdup(tagv);
        } else if (strcmp(tagk, "name") == 0) {
                /* <tag k="name" v="Herent"/> */
                if (NodeTownName)
                        free(NodeTownName);
                NodeTownName = FromXmlAndDup(tagv);
        }
        return 0;
}

/**
 * @brief deal with tag lines inside a <way> </way> pair
 * @param data points into the line of text being processed
 * @return error indication
 *
 * Example line :
 *     <tag k="highway" v="residential"/>
 *     <tag k="name" v="Rue de Thiribut"/>
 *     <tag k="created_by" v="JOSM"/>
 *     <tag k="ref" v="E40">
 *     <tag k="int_ref" v="E 40">
 */
static int
buildmap_osm_text_tag(char *data)
{
	static char	*tag = 0, *value = 0;
	int		i, found;
	layer_info_t	*list;
	int		ret = 0;

	if (! in_way) {
		/* Deal with tags outside ways */
		return buildmap_osm_text_node_tag(data);
	}

	if (! tag) tag = malloc(512);
	if (! value) value = malloc(512);

	sscanf(data, "tag k=%*[\"']%[^\"']%*[\"'] v=%*[\"']%[^\"']%*[\"']", tag, value);

	/* street names */
	if (strcmp(tag, "name") == 0) {
		if (WayStreetName)
			free(WayStreetName);
		WayStreetName = FromXmlAndDup(value);
		return 0;	/* FIX ME ?? */
	} else if (strcmp(tag, "landuse") == 0) {
		WayLandUseNotInteresting = 1;
//		buildmap_info("discarding way %d, landuse %s", in_way, data);
	} else if (strcmp(tag, "oneway") == 0 && strcmp(value, "yes") == 0) {
		WayIsOneWay = ROADMAP_LINE_DIRECTION_ONEWAY;
	} else if (strcmp(tag, "ref") == 0) {
		if (WayStreetRef)
			free(WayStreetRef);
		WayStreetRef = FromXmlAndDup(value);
		return 0;	/* FIX ME ?? */
	}

	/* Scan list_info
	 *
	 * This will map tags such as highway and cycleway.
	 */
	found = 0;
	for (i=1; found == 0 && list_info[i].name != 0; i++) {
		if (strcmp(tag, list_info[i].name) == 0) {
			list = list_info[i].list;
			found = 1;
			break;
		}
	}

	if (found) {
		if (list) {
			for (i=1; list[i].name; i++) {
				if (strcmp(value, list[i].name) == 0) {
					WayFlags = list[i].flags;
					if (list[i].layerp)
						ret = *(list[i].layerp);
				}
			}
		} else {
			/* */
		}
	}

	/* FIX ME When are we supposed to do this */
	if (ret)
		WayLayer = ret;

        return ret;
}

/**
 * @brief We found an end tag for a way, so we must have read all
 *  the required data.  Process it.
 * @param data points to the line of text being processed
 * @return error indication
 */
static int
buildmap_osm_text_way_end(char *data)
{
        int             from_point, to_point, line, street;
        int             fromlon, tolon, fromlat, tolat;
        int             j;
        int             was_split = 0;

        if (WayInvalid)
                return 0;

        if (in_way == 0)
                buildmap_fatal(0, "Wasn't in a way (%s)", data);

        if (WayLandUseNotInteresting) {
#if 0
                buildmap_info("discarding way %d, landuse %s", in_way, data);
#endif
                WayLandUseNotInteresting = 0;
                buildmap_osm_text_reset_way();
                return 0;
        }

        RoadMapString rms_dirp = str2dict(DictionaryPrefix, "");
        RoadMapString rms_dirs = str2dict(DictionarySuffix, "");
        RoadMapString rms_type = str2dict(DictionaryType, "");
        RoadMapString rms_name = 0;

        from_point = buildmap_osm_text_point_get(WayNodes[0]);
        to_point = buildmap_osm_text_point_get(WayNodes[nWayNodes-1]);

        fromlon = buildmap_point_get_longitude(from_point);
        fromlat = buildmap_point_get_latitude(from_point);
        tolon = buildmap_point_get_longitude(to_point);
        tolat = buildmap_point_get_latitude(to_point);

        if ((WayFlags & AREA)  && (fromlon == tolon) && (fromlat == tolat)) {
                static int polyid = 0;
                static int cenid = 0;
                int line;

                /*
                 * Detect an AREA -> create a polygon
                 */
                nPolygons++;
                cenid++;
                polyid++;

                rms_name = str2dict(DictionaryStreet, WayStreetName);
                buildmap_polygon_add_landmark (nPolygons, WayLayer, rms_name);
                buildmap_polygon_add(nPolygons, cenid, polyid);

                for (j=1; j<nWayNodes; j++) {
                        int prevpoint =
                            buildmap_osm_text_point_get(WayNodes[j-1]);
                        int point =
                            buildmap_osm_text_point_get(WayNodes[j]);

                        LineId++;
                        line = buildmap_line_add
                                (LineId, WayLayer, prevpoint, point,
				 ROADMAP_LINE_DIRECTION_BOTH);
                }
        } else {
                /*
                 * Register the way
                 *
                 * Need to do this several different ways :
                 * - begin and end points form a "line"
                 * - register street name
                 * - adjust the square
                 * - keep memory of the point coordinates so we can
                 *   postprocess the so called shapes (otherwise we only have
                 *   straight lines)
                 */

                int     *lonsbuf, *latsbuf;
                int     from_ix;

                /* Map begin and end points to internal point id */
                from_point = buildmap_osm_text_point_get(WayNodes[0]);
                to_point = buildmap_osm_text_point_get(WayNodes[nWayNodes-1]);

                /* Street name */
                if (WayStreetName)
                        rms_name = str2dict(DictionaryStreet, WayStreetName);
		else if (WayStreetRef)
                        rms_name = str2dict(DictionaryStreet, WayStreetRef);

                /*
                 * Loop over the points of the way.
                 *
                 * If we find a (non-endpoint) node that is also referenced
                 * in another way, then this way needs to be split.  In
                 * case of a split, and in case of the end node :  create
                 * all the right stuff for the map.
                 */
                from_ix = 0;
                for (j=1; j<nWayNodes-1; j++) {
                        int point = WayNodes[j];
                        if (NodeReportUse(point) <= 1)
                                continue;

                        int     k, num;

                        /* Keep track of what we did */
                        nsplits++;
                        was_split = 1;

                        /* Create a way */
                        int from_point =
                            buildmap_osm_text_point_get(
                                                WayNodes[from_ix]);
                        int to_point =
                            buildmap_osm_text_point_get(WayNodes[j]);

                        LineId++;
                        line = buildmap_line_add(
                            LineId, WayLayer, from_point, to_point,
			    WayIsOneWay);

                        street = buildmap_street_add(WayLayer,
                                        rms_dirp, rms_name, rms_type,
                                        rms_dirs, line);
                        buildmap_range_add_no_address(line, street);

                        /*
                         * Pass stuff to the shape module.
                         *
                         * We're passing too much here - the
                         * endpoints of the line don't need to be
                         * passed to the shape module.  We're
                         * keeping them here just to be on the safe
                         * side, they'll be ignored in
                         * buildmap_osm_text_ways_shapeinfo().
                         *
                         * The lonsbuf/latsbuf are never freed,
                         * need to be preserved for shape
                         * registration which happens at the end of
                         * the program run, so exit() will free
                         * this for us.
                         */
                        num = j - from_ix + 1;

                        lonsbuf = calloc(num, sizeof(int));
                        latsbuf = calloc(num, sizeof(int));

                        for (k=0; k<num; k++) {
                                int point = buildmap_osm_text_point_get(
                                                WayNodes[from_ix+k]);
                                int lon = buildmap_point_get_longitude(point);
                                int lat = buildmap_point_get_latitude(point);

                                /* Keep info for the shapes */
                                lonsbuf[k] = lon;
                                latsbuf[k] = lat;

                                buildmap_square_adjust_limits(lon, lat);
                        }

                        if (numshapes == nallocshapes) {
                                /* Allocate additional space (in big
                                 * chunks) when needed */
                                nallocshapes += 1000;
                                shapes = realloc(shapes, 
                                    nallocshapes * sizeof(struct shapeinfo));
                                buildmap_check_allocated(shapes);
                        }

                        /* Keep info for the shapes */
                        shapes[numshapes].lons = lonsbuf;
                        shapes[numshapes].lats = latsbuf;
                        shapes[numshapes].count = num;
                        shapes[numshapes].lineid = LineId;

                        numshapes++;

                        /* Prepare for next iteration */
                        from_ix = j;
                }

                if (was_split) {
                        int     k, num;
                        /*
                         * Need to create an additional way for the last piece
                         * This is from from_ix to nWayNodes-1
                         */
                        j = nWayNodes-1;

                        /* Create a way */
                        int from_point =
                            buildmap_osm_text_point_get(WayNodes[from_ix]);
                        int to_point =
                            buildmap_osm_text_point_get(WayNodes[j]);

                        LineId++;
                        line = buildmap_line_add(LineId,
                                WayLayer, from_point, to_point, WayIsOneWay);

                        street = buildmap_street_add(WayLayer,
                                        rms_dirp, rms_name, rms_type,
                                        rms_dirs, line);
                        buildmap_range_add_no_address(line, street);

                        /*
                         * Pass stuff to the shape module.
                         *
                         * We're passing too much here - the endpoints of
                         * the line don't need to be passed to the shape
                         * module.  We're keeping them here just to be on
                         * the safe side, they'll be ignored in
                         * buildmap_osm_text_ways_shapeinfo().
                         *
                         * The lonsbuf/latsbuf are never freed, need to be
                         * preserved for shape registration which happens
                         * at the end of the program run, so exit() will
                         * free this for us.
                         */
                        num = j - from_ix + 1;

                        lonsbuf = calloc(num, sizeof(int));
                        latsbuf = calloc(num, sizeof(int));

                        for (k=0; k<num; k++) {
                                int point = buildmap_osm_text_point_get(
                                                WayNodes[from_ix+k]);
                                int lon = buildmap_point_get_longitude(point);
                                int lat = buildmap_point_get_latitude(point);

                                /* Keep info for the shapes */
                                lonsbuf[k] = lon;
                                latsbuf[k] = lat;

                                buildmap_square_adjust_limits(lon, lat);
                        }

                        if (numshapes == nallocshapes) {
                                /* Allocate additional space (in big
                                 * chunks) when needed */
                                nallocshapes += 1000;
                                shapes = realloc(shapes,
                                    nallocshapes * sizeof(struct shapeinfo));
                                buildmap_check_allocated(shapes);
                        }

                        /* Keep info for the shapes */
                        shapes[numshapes].lons = lonsbuf;
                        shapes[numshapes].lats = latsbuf;
                        shapes[numshapes].count = num;
                        shapes[numshapes].lineid = LineId;

                        numshapes++;
                }

                /* If we didn't split, process the whole way in one piece */
                if (! was_split) {
                        int from_point =
                            buildmap_osm_text_point_get(WayNodes[0]);
                        int to_point =
                            buildmap_osm_text_point_get(WayNodes[nWayNodes-1]);

                        LineId++;
                        line = buildmap_line_add(LineId,
                                WayLayer, from_point, to_point, WayIsOneWay);

                        street = buildmap_street_add(WayLayer,
                                        rms_dirp, rms_name, rms_type,
                                        rms_dirs, line);
                        buildmap_range_add_no_address(line, street);

                        /*
                         * We're passing too much here - the endpoints of
                         * the line don't need to be passed to the shape
                         * module.  We're keeping them here just to be on
                         * the safe side, they'll be ignored in
                         * buildmap_osm_text_ways_shapeinfo().
                         *
                         * The lonsbuf/latsbuf are never freed, need to be
                         * preserved for shape registration which happens
                         * at the end of the program run, so exit() will
                         * free this for us.
                         */

                        lonsbuf = calloc(nWayNodes, sizeof(int));
                        latsbuf = calloc(nWayNodes, sizeof(int));

                        for (j=0; j<nWayNodes; j++) {
                                int point =
                                    buildmap_osm_text_point_get(WayNodes[j]);
                                int lon = buildmap_point_get_longitude(point);
                                int lat = buildmap_point_get_latitude(point);

                                /* Keep info for the shapes */
                                lonsbuf[j] = lon;
                                latsbuf[j] = lat;

                                buildmap_square_adjust_limits(lon, lat);
                        }

                        if (numshapes == nallocshapes) {
                                /* Allocate additional space (in big
                                 * chunks) when needed */
                                nallocshapes += 1000;
                                shapes = realloc(shapes,
                                    nallocshapes * sizeof(struct shapeinfo));
                                buildmap_check_allocated(shapes);
                        }

                        /* Keep info for the shapes */
                        shapes[numshapes].lons = lonsbuf;
                        shapes[numshapes].lats = latsbuf;
                        shapes[numshapes].count = nWayNodes;
                        shapes[numshapes].lineid = LineId;

                        numshapes++;
                }
        }

        buildmap_osm_text_reset_way();

        if (was_split)
                WaysSplit++;
        else
                WaysNotSplit++;

        return 0;
}

/**
 * @brief a postprocessing step to load shape info
 *
 * this needs to be a separate step because lines need to be sorted
 *
 * The shapes don't include the first and last points of a line, hence the
 * strange loop beginning/end for the inner loop.
 */
static int
buildmap_osm_text_ways_shapeinfo(void)
{
    int i, j, count, lineid;
    int *lons, *lats;
    int line_index;

    buildmap_info("loading shape info (from %d ways) ...", numshapes);

    buildmap_line_sort();

    for (i = 0; i < numshapes; i++) {

        count = shapes[i].count;

        if (count <= 2)
            continue;

        lineid = shapes[i].lineid;
        line_index = buildmap_line_find_sorted(lineid);

        if (line_index >= 0) {
            lons = shapes[i].lons;
            lats = shapes[i].lats;

            /* Add the shape points here, don't include beginning and end
             * point */
            for (j = 1; j < count - 1; j++) {
                buildmap_shape_add
                    (line_index, i, lineid, j - 1, lons[j], lats[j]);
            }
        }
    }

    return 1;
}

/**
 * @brief This is the gut of buildmap_osm_text : parse an OSM XML file
 * @param fdata an open file pointer, this will get read twice
 * @param country_num the country id that we're working for
 * @param division_num the country subdivision id that we're working for
 * @return error indication
 *
 * This is a simplistic approach to parsing the OSM text (XML) files.
 * It scans the XML twice, to cope with out of order information.
 * (Note that this simplistic approach may raise eyebrows, but this is
 * not a big time consumer !)
 *
 * Pass 1 deals with node definitions only.
 * Pass 2 interprets ways and a few tags.
 * All underlying processing is passed to other functions.
 */
int
buildmap_osm_text_read(FILE * fdata, int country_num, int division_num)
{
    char *got;
    static char buf[LINELEN];
    int ret = 0;
    char *p;
    time_t      t0, t1, t2, t3;

    NodeFakeFips = 1000000 + country_num * 1000 + division_num;

    (void) time(&t0);

    DictionaryPrefix = buildmap_dictionary_open("prefix");
    DictionaryStreet = buildmap_dictionary_open("street");
    DictionaryType = buildmap_dictionary_open("type");
    DictionarySuffix = buildmap_dictionary_open("suffix");
    DictionaryCity = buildmap_dictionary_open("city");

    buildmap_osm_common_find_layers ();

    /*
     * Pass 1
     */
    LineNo = 0;

    while (! feof(fdata)) {
        buildmap_set_line(++LineNo);
        got = fgets(buf, LINELEN, fdata);
        if (got == NULL) {
            if (feof(fdata))
                break;
            buildmap_fatal(0, "short read (length)");
        }

        /* Figure out the XML */
        for (p=buf; *p && isspace(*p); p++) ;
        if (*p == '\n' || *p == '\r') 
                continue;
        if (*p != '<')
                buildmap_fatal(0, "invalid XML (line %d, %s)", LineNo, buf);

        p++; /* point to character after '<' now */
        for (; *p && isspace(*p); p++) ;

        if (strncasecmp(p, "osm", 3) == 0) {
                continue;
        } else if (strncasecmp(p, "?xml", 4) == 0) {
                continue;
        } else if (strncasecmp(p, "way", 3) == 0) {
//              ret += buildmap_osm_text_way(p);
                continue;
        } else if (strncasecmp(p, "/way", 4) == 0) {
//              ret += buildmap_osm_text_way_end(p);
                continue;
        } else if (strncasecmp(p, "node", 4) == 0) {
                ret += buildmap_osm_text_node(p);
                continue;
        } else if (strncasecmp(p, "/node", 5) == 0) {
                ret += buildmap_osm_text_node_end(p);
                continue;
        } else if (strncasecmp(p, "nd", 2) == 0) {
                ret += buildmap_osm_text_nd_pass1(p);
                continue;
        } else if (strncasecmp(p, "tag", 3) == 0) {
                ret += buildmap_osm_text_tag(p);
                continue;
        } else if (strncasecmp(p, "relation", 8) == 0) {
                continue;
        } else if (strncasecmp(p, "/relation", 9) == 0) {
                continue;
        } else if (strncasecmp(p, "member", 6) == 0) {
                continue;
        } else if (strncasecmp(p, "/member", 7) == 0) {
                continue;
        } else if (strncasecmp(p, "bound", 5) == 0) {
                continue;
        } else if (strncasecmp(p, "bounds", 6) == 0) {
                continue;
        } else if (strncasecmp(p, "/bounds", 7) == 0) {
                continue;
        } else if (strncasecmp(p, "/osm", 4) == 0) {
                continue;
        } else {
                buildmap_fatal(0, "invalid XML token (%s)", p);
        }
    }

    (void) time(&t1);
    buildmap_info("Pass 1 : %d lines read (%d seconds)", LineNo, t1 - t0);

    /*
     * Pass 2
     */
    LineNo = 0;
    fseek(fdata, 0L, SEEK_SET);

    while (! feof(fdata)) {
        buildmap_set_line(++LineNo);
        got = fgets(buf, LINELEN, fdata);
        if (got == NULL) {
            if (feof(fdata))
                break;
            buildmap_fatal(0, "short read (length)");
        }

        /* Figure out the XML */
        for (p=buf; *p && isspace(*p); p++) ;
        if (*p == '\n' || *p == '\r') 
                continue;
        if (*p != '<')
                buildmap_fatal(0, "invalid XML");

        p++; /* point to character after '<' now */
        for (; *p && isspace(*p); p++) ;

        if (strncasecmp(p, "osm", 3) == 0) {
                continue;
        } else if (strncasecmp(p, "?xml", 4) == 0) {
                continue;
        } else if (strncasecmp(p, "way", 3) == 0) {
                ret += buildmap_osm_text_way(p);
                continue;
        } else if (strncasecmp(p, "/way", 4) == 0) {
                ret += buildmap_osm_text_way_end(p);
                continue;
        } else if (strncasecmp(p, "node", 4) == 0) {
//              ret += buildmap_osm_text_node(p);
                continue;
        } else if (strncasecmp(p, "/node", 5) == 0) {
//              ret += buildmap_osm_text_node_end(p);
                continue;
        } else if (strncasecmp(p, "nd", 2) == 0) {
                ret += buildmap_osm_text_nd(p);
                continue;
        } else if (strncasecmp(p, "tag", 3) == 0) {
                ret += buildmap_osm_text_tag(p);
                continue;
        } else if (strncasecmp(p, "relation", 8) == 0) {
                continue;
        } else if (strncasecmp(p, "/relation", 9) == 0) {
                continue;
        } else if (strncasecmp(p, "member", 6) == 0) {
                continue;
        } else if (strncasecmp(p, "/member", 7) == 0) {
                continue;
        } else if (strncasecmp(p, "bound", 5) == 0) {
                continue;
        } else if (strncasecmp(p, "bounds", 6) == 0) {
                continue;
        } else if (strncasecmp(p, "/bounds", 7) == 0) {
                continue;
        } else if (strncasecmp(p, "/osm", 4) == 0) {
                continue;
        } else {
                buildmap_fatal(0, "invalid XML token (%s)", p);
        }
    }

    (void) time(&t2);
    buildmap_info("Pass 2 : %d lines read (%d seconds)", LineNo, t2 - t1);

    /*
     * End pass 2
     */
    buildmap_osm_text_ways_shapeinfo();
    (void) time(&t3);
    buildmap_info("Shape info processed (%d seconds)", t3 - t2);


    buildmap_info("Splits %d, ways split %d, not split %d",
        nsplits, WaysSplit, WaysNotSplit);

    return ret;
}
