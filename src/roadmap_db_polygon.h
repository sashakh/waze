/* roadmap_polygon.h - the format of the polygon table used by RoadMap.
 *
 * The RoadMap polygons are described by the following tables:
 *
 * polygon.bysquare the list of polygons located in each square.
 * polygon.head     for each polygon, the category and list of lines.
 * polygon.points   the list of points defining the polygons border.
 */

#ifndef _ROADMAP_DB_POLYGON__H_
#define _ROADMAP_DB_POLYGON__H_

#include "roadmap_types.h"

typedef struct {  /* table polygon.head */

   unsigned short first;
   unsigned short count;

   RoadMapString name;
   char  cfcc;
   unsigned char filler;

   int   north;
   int   west;
   int   east;
   int   south;

} RoadMapPolygon;

typedef struct {  /* table polygon.lines */

   int point;

} RoadMapPolygonPoint;

#endif // _ROADMAP_DB_POLYGON__H_

