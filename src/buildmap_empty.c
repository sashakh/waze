/* buildmap_empty.c - Create an empty map file.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright (c) 2009, Danny Backx
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
 *   see buildmap_empty.h
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_path.h"
#include "roadmap_line.h"

#include "buildmap.h"
#include "buildmap_point.h"
#include "buildmap_line.h"

static int decode_points (const char *str, int *points) {

   char num[100];
   int j;
   const char *itr = str;

   for (j=0; j<4; j++) {
      int i = 0;
      
      while (*itr && !isdigit(*itr) && (*itr != '-')) itr++;

      if (!*itr) {
         buildmap_fatal (0, "Can't decode parameter: '%s'", str);
      }
   
      while (*itr && (isdigit(*itr) || (*itr == '.') || (*itr == '-'))) {
         num[i++] = *itr;
         
         if (i==sizeof(num)) {
            buildmap_fatal (0, "Number is too long: '%s'", str);
         }
         itr++;
      }
      num[i+1] = '\0';
      points[j] = (int) (atof(num) * 1000000);
   }

   return 0;
}   


void buildmap_empty_process (const char *source) {

   int points[4];
   int p1, p2, p3, p4;
   int cfcc = 11;

   decode_points (source, points);

   buildmap_info ("Using points: %d %d, %d %d",
         points[0], points[1], points[2], points[3]);

   p1 = buildmap_point_add (points[0], points[1]);
   p2 = buildmap_point_add (points[2], points[1]);
   p3 = buildmap_point_add (points[2], points[3]);
   p4 = buildmap_point_add (points[0], points[3]);

   buildmap_line_add (1, cfcc, p1, p2, ROADMAP_LINE_DIRECTION_BOTH);
   buildmap_line_add (2, cfcc, p2, p3, ROADMAP_LINE_DIRECTION_BOTH);
   buildmap_line_add (3, cfcc, p3, p4, ROADMAP_LINE_DIRECTION_BOTH);
   buildmap_line_add (4, cfcc, p4, p1, ROADMAP_LINE_DIRECTION_BOTH);

   buildmap_dictionary_open ("prefix");
   buildmap_dictionary_open ("street");
   buildmap_dictionary_open ("type");
   buildmap_dictionary_open ("suffix");
   buildmap_dictionary_open ("city");
}

