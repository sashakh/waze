/*
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright (c) 2008, 2009, Danny Backx.
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
 * @brief roadmap_line.c - retrieve the points that make the lines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_db_line.h"

#include "roadmap_math.h"
#include "roadmap_point.h"
#include "roadmap_line.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"
#include "roadmap_layer.h"

static char *RoadMapLineType = "RoadMapLineContext";

/**
 * @brief
 */
typedef struct {

   char *type;

   RoadMapLine *Line;
   int          LineCount;
   RoadMapLine2 *Line2;

   RoadMapLineBySquare *LineBySquare1;
   int                  LineBySquare1Count;

   int                 *LineByLayer1;
   int                  LineByLayer1Count;

   RoadMapLineBySquare *LineBySquare2;
   int                  LineBySquare2Count;

   RoadMapLongLine     *LongLines;
   int                  LongLinesCount;

   int                 *LineByLayer2;
   int                  LineByLayer2Count;

   int *LineIndex2;
   int  LineIndex2Count;

   RoadMapLineByPoint1	*LineByPoint1;
   int			LineByPoint1Count;
   RoadMapLineByPoint2	*LineByPoint2;
   int			LineByPoint2Count;
} RoadMapLineContext;

static RoadMapLineContext *RoadMapLineActive = NULL;


static void *roadmap_line_map (roadmap_db *root) {

   RoadMapLineContext *context;

   roadmap_db *line_table;
   roadmap_db *line2_table;
   roadmap_db *index2_table;
   roadmap_db *square1_table;
   roadmap_db *square2_table;
   roadmap_db *layer1_table;
   roadmap_db *layer2_table;
   roadmap_db *long_lines_table;
   roadmap_db *by_point1_table;
   roadmap_db *by_point2_table;


   context = (RoadMapLineContext *) malloc (sizeof(RoadMapLineContext));
   if (context == NULL) {
      roadmap_log (ROADMAP_ERROR, "no more memory");
      return NULL;
   }
   context->type = RoadMapLineType;

   line_table    = roadmap_db_get_subsection (root, "data");
   line2_table    = roadmap_db_get_subsection (root, "data2");
   square1_table = roadmap_db_get_subsection (root, "bysquare1");
   layer1_table = roadmap_db_get_subsection (root, "bylayer1");
   square2_table = roadmap_db_get_subsection (root, "bysquare2");
   layer2_table = roadmap_db_get_subsection (root, "bylayer2");
   index2_table  = roadmap_db_get_subsection (root, "index2");
   long_lines_table  = roadmap_db_get_subsection (root, "longlines");
   by_point1_table  = roadmap_db_get_subsection (root, "bypoint1");
   by_point2_table  = roadmap_db_get_subsection (root, "bypoint2");

   context->Line = (RoadMapLine *) roadmap_db_get_data (line_table);
   context->LineCount = roadmap_db_get_count (line_table);

   if (roadmap_db_get_size (line_table) !=
       context->LineCount * sizeof(RoadMapLine)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/data structure");
      goto roadmap_line_map_abort;
   }
   /* Backwards compatibility : cope with data2 absence */
   if (line2_table) {
	   context->Line2 = (RoadMapLine2 *) roadmap_db_get_data (line2_table);
	   if (roadmap_db_get_size (line2_table) !=
	       context->LineCount * sizeof(RoadMapLine2)) {
	      roadmap_log (ROADMAP_ERROR, "invalid line/data2 structure");
	      goto roadmap_line_map_abort;
	   }
   }

   context->LineBySquare1 =
      (RoadMapLineBySquare *) roadmap_db_get_data (square1_table);
   context->LineBySquare1Count = roadmap_db_get_count (square1_table);

   if (roadmap_db_get_size (square1_table) !=
       context->LineBySquare1Count * sizeof(RoadMapLineBySquare)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/bysquare1 structure");
      goto roadmap_line_map_abort;
   }

   context->LineByLayer1 = (int *) roadmap_db_get_data (layer1_table);
   context->LineByLayer1Count = roadmap_db_get_count (layer1_table);

   if (roadmap_db_get_size (layer1_table) !=
       context->LineByLayer1Count * sizeof(int)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/bylayer1 structure");
      goto roadmap_line_map_abort;
   }

   context->LineIndex2 = (int *) roadmap_db_get_data (index2_table);
   context->LineIndex2Count = roadmap_db_get_count (index2_table);

   if (roadmap_db_get_size (index2_table) !=
       context->LineIndex2Count * sizeof(int)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/index2 structure");
      goto roadmap_line_map_abort;
   }

   context->LineByLayer2 = (int *) roadmap_db_get_data (layer2_table);
   context->LineByLayer2Count = roadmap_db_get_count (layer2_table);

   if (roadmap_db_get_size (layer2_table) !=
       context->LineByLayer2Count * sizeof(int)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/bylayer2 structure");
      goto roadmap_line_map_abort;
   }

   context->LineBySquare2 =
      (RoadMapLineBySquare *) roadmap_db_get_data (square2_table);
   context->LineBySquare2Count = roadmap_db_get_count (square2_table);

   if (roadmap_db_get_size (square2_table) !=
       context->LineBySquare2Count * sizeof(RoadMapLineBySquare)) {
      roadmap_log (ROADMAP_ERROR, "invalid line/bysquare2 structure");
      goto roadmap_line_map_abort;
   }

   if (long_lines_table) {
      context->LongLines =
         (RoadMapLongLine *) roadmap_db_get_data (long_lines_table);
      context->LongLinesCount = roadmap_db_get_count (long_lines_table);

      if (roadmap_db_get_size (long_lines_table) !=
         context->LongLinesCount * sizeof(RoadMapLongLine)) {
         roadmap_log (ROADMAP_ERROR, "invalid long lines structure");
         goto roadmap_line_map_abort;
      }
   } else {
      context->LongLinesCount = 0;
   }

   if (by_point1_table && by_point2_table) {
	   roadmap_log(ROADMAP_DEBUG, "***** This map has line by point *****");
     context->LineByPoint1 = (RoadMapLineByPoint1 *) roadmap_db_get_data (by_point1_table);
     context->LineByPoint1Count = roadmap_db_get_count (by_point1_table);
	   roadmap_log(ROADMAP_DEBUG, "***** This map has line by point1 (%d) *****",
			   context->LineByPoint1Count);

     if (roadmap_db_get_size (by_point1_table) !=
		     context->LineByPoint1Count * sizeof(int)) {
       roadmap_log (ROADMAP_ERROR, "invalid line/bypoint1 structure");
       goto roadmap_line_map_abort;
     }

     context->LineByPoint2 = (RoadMapLineByPoint2 *) roadmap_db_get_data (by_point2_table);
     context->LineByPoint2Count = roadmap_db_get_count (by_point2_table);
	   roadmap_log(ROADMAP_DEBUG, "***** This map has line by point2 (%d) *****",
			   context->LineByPoint2Count);

     if (roadmap_db_get_size (by_point2_table) !=
		     context->LineByPoint2Count * sizeof(int)) {
       roadmap_log (ROADMAP_ERROR, "invalid line/bypoint2 structure");
       goto roadmap_line_map_abort;
     }
   } else {
	   context->LineByPoint1 = 0;
	   context->LineByPoint1Count = 0;
	   context->LineByPoint2 = 0;
	   context->LineByPoint2Count = 0;
   }

   return context;

roadmap_line_map_abort:

   free(context);
   return NULL;
}

static void roadmap_line_activate (void *context) {

   RoadMapLineContext *line_context = (RoadMapLineContext *) context;

   if ((line_context != NULL) &&
       (line_context->type != RoadMapLineType)) {
      roadmap_log (ROADMAP_FATAL, "invalid line context activated");
   }
   RoadMapLineActive = line_context;
}

static void roadmap_line_unmap (void *context) {

   RoadMapLineContext *line_context = (RoadMapLineContext *) context;

   if (line_context->type != RoadMapLineType) {
      roadmap_log (ROADMAP_FATAL, "unmapping invalid line context");
   }
   free (line_context);
}

roadmap_db_handler RoadMapLineHandler = {
   "dictionary",
   roadmap_line_map,
   roadmap_line_activate,
   roadmap_line_unmap
};

/**
 * @brief get the list of lines in a square, and a specified layer
 * @param square input parameter
 * @param layer input parameter
 * @param first return index of first line that complies
 * @param last return index of last line that complies
 * @return success indicator
 */
int roadmap_line_in_square (int square, int layer, int *first, int *last) {

   int *index;

   if (RoadMapLineActive == NULL) return 0; /* No line. */

   square = roadmap_square_index(square);
   if (square < 0) {
      return 0;   /* This square is empty. */
   }

   if (layer <= 0 || layer > RoadMapLineActive->LineBySquare1[square].count) {
       return 0;
   }
   index = RoadMapLineActive->LineByLayer1
              + RoadMapLineActive->LineBySquare1[square].first;

   *first = index[layer-1];
   *last = index[layer] - 1;

   return (*first <= *last);
}


int roadmap_line_in_square2 (int square, int layer, int *first, int *last) {

   int layer_first;
   int *index;

   if (RoadMapLineActive == NULL) return 0; /* No line. */

   square = roadmap_square_index(square);
   if (square < 0) {
      return 0;   /* This square is empty. */
   }

   if (layer <= 0 || layer > RoadMapLineActive->LineBySquare2[square].count) {
       return 0;
   }
   index = RoadMapLineActive->LineByLayer2
              + RoadMapLineActive->LineBySquare2[square].first;

   layer_first = index[layer-1];
   if (layer_first < 0 || layer_first >= RoadMapLineActive->LineIndex2Count) {
      return 0;
   }
   *first = layer_first;
   *last = index[layer] - 1;

   return (*first <= *last);
}


int roadmap_line_get_from_index2 (int index) {

   return RoadMapLineActive->LineIndex2[index];
}

/**
 * @brief store the position of the "from" point of the line
 * @param line the line
 * @param position the position to store
 */
void roadmap_line_from (int line, RoadMapPosition *position) {

#ifdef ROADMAP_INDEX_DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif
   roadmap_point_position (RoadMapLineActive->Line[line].from, position);
}

/**
 * @brief store the position of the "to" point of the line
 * @param line the line
 * @param position the position to store
 */
void roadmap_line_to  (int line, RoadMapPosition *position) {

#ifdef ROADMAP_INDEX_DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif
   roadmap_point_position (RoadMapLineActive->Line[line].to, position);
}


int  roadmap_line_count (void) {

   if (RoadMapLineActive == NULL) return 0; /* No line. */

   return RoadMapLineActive->LineCount;
}


int roadmap_line_length (int line) {

   RoadMapPosition p1;
   RoadMapPosition p2;
   int length = 0;

   int square;
   int first_shape_line;
   int last_shape_line;
   int first_shape;
   int last_shape;
   int i;

   roadmap_point_position (RoadMapLineActive->Line[line].from, &p1);

   square = roadmap_square_search (&p1);
   if (square < 0) return 0;

   if (roadmap_shape_in_square (square, &first_shape_line,
                                        &last_shape_line) > 0) {

      if (roadmap_shape_of_line (line, first_shape_line,
                                       last_shape_line,
                                       &first_shape, &last_shape) > 0) {

         p2 = p1;
         for (i = first_shape; i <= last_shape; i++) {

            roadmap_shape_get_position (i, &p2);
            length += roadmap_math_distance (&p1, &p2);
            p1 = p2;
         }
      }
   }

   roadmap_point_position (RoadMapLineActive->Line[line].to, &p2);
   length += roadmap_math_distance (&p1, &p2);

   return length;
}

/**
 * @brief query the points at both ends of the given line
 * @param line this line is to be queried
 * @param from return the point at one end of the line
 * @param to return the point at the other end of the line
 */
void roadmap_line_points (int line, int *from, int *to) {

#ifdef ROADMAP_INDEX_DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
      roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif

   *from = RoadMapLineActive->Line[line].from;
   *to = RoadMapLineActive->Line[line].to;
}

/**
 * @brief
 * @param index
 * @param line_id
 * @param area
 * @param cfcc
 * @return
 */
int roadmap_line_long (int index, int *line_id, RoadMapArea *area, int *cfcc) {

   if (RoadMapLineActive == NULL) return 0; /* No lines */

   if (index >= RoadMapLineActive->LongLinesCount) return 0;

   *line_id = RoadMapLineActive->LongLines[index].line;
   *area = RoadMapLineActive->LongLines[index].area;
   *cfcc = RoadMapLineActive->LongLines[index].layer;

   return 1;
}

/**
 * @brief OLD, TO BE REMOVED EXCEPT FOR BACKWARDS COMPATIBILITY
 * determine the layer that some line is in
 * note: rewritten completely, I guess Ehud's data model is different from trunk
 * @param line_id the line whose layer we want to query
 * @return the layer
 */
int roadmap_line_get_layer_old (int line_id)
{
   int *index;
   int  first, last, layer, i;
   int square;
   RoadMapPosition pos;
   int	last_road_layer;

   /*
    * This is hacked for now, we can't call roadmap_layer_() functions from here
    * because this would pull roadmap_canvas_() functions into executables that
    * aren't linked with that (e.g. roadgps).
    */
#define roadmap_layer_road_last()	11
#warning Hack for roadmap_layer_road_last

   last_road_layer = roadmap_layer_road_last();

   if (RoadMapLineActive == NULL)
	   return 0; /* No line. */

   roadmap_point_position(RoadMapLineActive->Line[line_id].from, &pos);
   square = roadmap_square_search (&pos);

   square = roadmap_square_index(square);
   if (square < 0) {
      return 0;   /* This square is empty. */
   }

   index = RoadMapLineActive->LineByLayer1 + RoadMapLineActive->LineBySquare1[square].first;

   for (layer = 1; layer < last_road_layer; layer++) {
	   first = index[layer-1];
	   last = index[layer]-1;

	   for (i=first; i<last; i++)
		   if (i == line_id) {
			   return layer;
		   }
   }
   return 0;
}

/**
 * @brief determine the layer that some line is in
 * @param line_id the line whose layer we want to query
 * @return the layer
 */
int roadmap_line_get_layer (int line)
{
#ifdef ROADMAP_INDEX_DEBUG
    if (line < 0 || line >= RoadMapLineActive->LineCount) {
	    roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
    }
#endif
    if (RoadMapLineActive->Line2 == 0) {
	static int once = 1;

	if (once) {
	   once = 0;
	   roadmap_log (ROADMAP_WARNING, "Map without data2 -> no layer info");

	}
	return roadmap_line_get_layer_old(line);
    }

#if 0
    /* debug */
    int ol = roadmap_line_get_layer_old(line),
	nl = RoadMapLineActive->Line2[line].layer;
    if (ol != nl)
	    roadmap_log (ROADMAP_WARNING, "roadmap_line_get_layer(%d) old %d new %d",
			    line, ol, nl);
#endif
    return RoadMapLineActive->Line2[line].layer;
}

/**
 * @brief determine the layer that some line is in
 * @param line_id the line whose layer we want to query
 * @return the layer
 */
int roadmap_line_get_oneway (int line)
{
#ifdef ROADMAP_INDEX_DEBUG
    if (line < 0 || line >= RoadMapLineActive->LineCount) {
	    roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
    }
#endif
    if (RoadMapLineActive->Line2 == 0)
	    return 0;
    return RoadMapLineActive->Line2[line].oneway;
}

#if defined(HAVE_NAVIGATE_PLUGIN)
/**
 * @brief look up the point that a line is coming "from"
 * @param line the id of this line
 * @return the point that the line is coming "from"
 */
int roadmap_line_from_point (int line)
{ 
#ifdef ROADMAP_INDEX_DEBUG
    if (line < 0 || line >= RoadMapLineActive->LineCount) {
	    roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
    }
#endif
    return RoadMapLineActive->Line[line].from;
}
/**
 * @brief look up the point that a line is going "to"
 * @param line the id of this line
 * @return the point that the line is going to
 */
int roadmap_line_to_point (int line)
{
#ifdef ROADMAP_INDEX_DEBUG
   if (line < 0 || line >= RoadMapLineActive->LineCount) {
	   roadmap_log (ROADMAP_FATAL, "illegal line index %d", line);
   }
#endif
   return RoadMapLineActive->Line[line].to;
}
#endif

/**
 * @brief Get a line adjacent to a given point
 * @param point this is the point referred to
 * @param ix the index in the list of lines adjacent to this point
 * @return a line id, or 0 if out of bounds
 */
int roadmap_line_point_adjacent(int point, int ix)
{
	int	i;
	int	*p, *q;

	if (RoadMapLineActive == NULL)
		return 0; /* No line. */
	if (RoadMapLineActive->LineByPoint1 == 0)
		return 0;	/* No map data */
	if (RoadMapLineActive->LineByPoint2 == 0)
		return 0;	/* No map data */

	if (ix < 0)
		return 0;
	if (point <= 0 || RoadMapLineActive->LineByPoint1Count < point)
		return 0;

	q = (int *)RoadMapLineActive->LineByPoint1;
	q += point;

	p = (int *)RoadMapLineActive->LineByPoint2;
	p += *q;

	/* expensive check ? */
	for (i=0; i<ix; i++)
		if (p[i] == 0) {
			roadmap_log (ROADMAP_WARNING, "Didn't expect this !");
			return 0;
		}
	return p[ix];
}

/**
 * @brief return the fips for this line, based on the from point's position
 * @param line the line id
 * @return the fips
 */
int roadmap_line_get_fips(int line)
{
	static int	*fl = NULL;
	RoadMapPosition	pos;

	roadmap_line_from(line, &pos);
	if (roadmap_locator_by_position(&pos, &fl) <= 0)
		return -1;
	return fl[0];
}
