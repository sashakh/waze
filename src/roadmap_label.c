/* roadmap_label.c - Manage map labels.
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *
 *   This code was mostly taken from UMN Mapserver
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
 *   See roadmap_label.h.
 */

#include <stdlib.h>

#include "roadmap_config.h"
#include "roadmap_math.h"
#include "roadmap_plugin.h"
#include "roadmap_canvas.h"

#include "roadmap_label.h"

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

static RoadMapConfigDescriptor RoadMapConfigMinFeatureSize =
                        ROADMAP_CONFIG_ITEM("Labels", "MinFeatureSize");

static RoadMapConfigDescriptor RoadMapConfigLabelsColor =
                        ROADMAP_CONFIG_ITEM("Labels", "Color");

static labelCacheObj RoadMapLabelCache;
static int RoadMapLabelCacheFull = 0;
static RoadMapPen RoadMapLabelPen;

static int rect_overlap (RoadMapGuiRect *a, RoadMapGuiRect *b) {

   if(a->minx > b->maxx) return 0;
   if(a->maxx < b->minx) return 0;
   if(a->miny > b->maxy) return 0;
   if(a->maxy < b->miny) return 0;

   return 1;
}

int segment_intersect (RoadMapGuiPoint *f1, RoadMapGuiPoint *t1,
			   RoadMapGuiPoint *f2, RoadMapGuiPoint *t2,
			   RoadMapGuiPoint *isect) {

   double a1,b1;
   double a2,b2;

   if (f1->x == t1->x) {

      a1 = 0;
      b1 = f1->y;
   } else {
      a1 = 1.0 * (f1->y - t1->y) / (f1->x - t1->x);
      b1 = f1->y - 1.0 * a1 * f1->x;
   }

   if ((f2->x - t2->x) == 0) {
      a2 = 0;
      b2 = f2->y;
   } else {
      a2 = 1.0 * (f2->y - t2->y) / (f2->x - t2->x);
      b2 = f2->y - 1.0 * a2 * f2->x;
   }

   if (a1 == a2) return 0;

   isect->x = (int) ((b1 - b2) / (a2 - a1));
   isect->y = (int) (b1 + isect->x * a1);

   return 1;
}

static int point_in_bbox( RoadMapGuiPoint *p, RoadMapGuiRect *bb) {

   if ((p->x < bb->minx) || (p->x > bb->maxx) ||
       (p->y > bb->maxy) || (p->y < bb->miny))
      return 0;
 
   return 1;
}

static int poly_overlap (labelCacheMemberObj *c1, labelCacheMemberObj *c2) {

   RoadMapGuiPoint *a = c1->poly;
   RoadMapGuiPoint *b = c2->poly;
   RoadMapGuiPoint isect;
   int ai, bi;
   for (ai = 0; ai < 4; ai++) {
      for (bi = 0; bi < 4; bi++) {
	 if (segment_intersect( &a[ai], &a[(ai+1)%4],
                                &b[bi], &b[(bi+1)%4], &isect)) {
	    if (point_in_bbox(&isect, &c1->bbox) &&
		point_in_bbox(&isect, &c2->bbox))
	       return 1;
	 }
      }
   }

   return 0;
}

   
static void compute_bbox(RoadMapGuiPoint *poly, RoadMapGuiRect *bbox) {

   int i;

   bbox->minx = bbox->maxx = poly[0].x;
   bbox->miny = bbox->maxy = poly[0].y;

   for(i=1; i<4; i++) {
      bbox->minx = MIN(bbox->minx, poly[i].x);
      bbox->maxx = MAX(bbox->maxx, poly[i].x);
      bbox->miny = MIN(bbox->miny, poly[i].y);
      bbox->maxy = MAX(bbox->maxy, poly[i].y);
   }
}


static RoadMapGuiPoint get_metrics(labelCacheMemberObj *c, 
					RoadMapGuiRect *rect) {
   RoadMapGuiPoint q;
   int x1=0, y1=0;
   RoadMapGuiPoint *poly = c->poly;
   int w, h;
   int buffer = 0;
   int lines = 2;
   int angle = c->angle;
   RoadMapGuiPoint *p = &c->point;

   w = rect->maxx - rect->minx;
   h = rect->maxy - rect->miny;

   /* position CC */
   x1 = -(int)(w/2.0) + 0; // ox;
   y1 = (int)(h/2.0) + 0; // oy;

   q.x = x1 - rect->minx;
   q.y = rect->miny - y1;

   roadmap_math_rotate_point (&q, p, angle);

   poly[0].x = x1 - buffer; /* ll */
   poly[0].y = -(y1 + buffer);
   roadmap_math_rotate_point (&poly[0], p, angle);

   poly[1].x = x1 - buffer; /* ul */
   poly[1].y = -(y1 -h - buffer);
   roadmap_math_rotate_point (&poly[1], p, angle);

   poly[2].x = x1 + w + buffer; /* ur */
   poly[2].y = -(y1 -h - buffer);
   roadmap_math_rotate_point (&poly[2], p, angle);

   poly[3].x = x1 + w + buffer; /* lr */
   poly[3].y = -(y1 + buffer);
   roadmap_math_rotate_point (&poly[3], p, angle);

   lines = 4;

   /* roadmap_canvas_draw_multiple_lines(1, &lines, poly, 1); */

   compute_bbox(poly, &c->bbox);

   return q;
}


int roadmap_label_add (const RoadMapGuiPoint *point, int angle,
                       int featuresize, const PluginLine *line) {

   labelCacheMemberObj *cachePtr;

   if (RoadMapLabelCacheFull) return -1;

   if (featuresize <
         roadmap_config_get_integer (&RoadMapConfigMinFeatureSize)) {
      return -1;
   }


   if(RoadMapLabelCache.numlabels == MAX_LABELS) {
      roadmap_log (ROADMAP_WARNING, "Too many labels on screen.");
      RoadMapLabelCacheFull = 1;
      return -1;
   }

   cachePtr = RoadMapLabelCache.labels[RoadMapLabelCache.numlabels];
   if (!cachePtr) {
      cachePtr = malloc (sizeof (*cachePtr));
      roadmap_check_allocated (cachePtr);
      RoadMapLabelCache.labels[RoadMapLabelCache.numlabels] = cachePtr;
   }

   cachePtr->featuresize = featuresize;
   cachePtr->line = *line;
   cachePtr->angle = angle;
   cachePtr->point = *point;
   cachePtr->status = 0;

   RoadMapLabelCache.numlabels++;

   return 0;
}


int roadmap_label_draw_cache (int angles) {

   int l;
   int i;
   int width;
   int ascent;
   int descent;
   RoadMapGuiPoint p;
   RoadMapGuiRect r;
   // int label_offsetx = 0;
   // int label_offsety = 0;
   const char *text;

   labelCacheMemberObj *cachePtr=NULL;

   roadmap_canvas_select_pen (RoadMapLabelPen);

   for(l=0; l<RoadMapLabelCache.numlabels; l++) {

      PluginStreetProperties properties;

      cachePtr = RoadMapLabelCache.labels[l];

      roadmap_plugin_get_street_properties (&cachePtr->line, &properties);

      if (!properties.street || !*properties.street) {
         goto recycle;
      }

      text = properties.street;
      cachePtr->street = properties.plugin_street;

      roadmap_canvas_get_text_extents(text, -1, &width, &ascent, &descent);
      r.minx = 0;
      r.maxx=width+1;
      r.miny = 0;
      r.maxy = ascent + descent + 1;

      /* text is too long for this feature */
      if ((width >> 2) > cachePtr->featuresize) {
         goto recycle;
      }

      cachePtr->status = 1; /* assume label *can* be drawn */

      /* The stored point is not screen orieneted, rotate is needed */
      roadmap_math_rotate_coordinates (1, &cachePtr->point);

      if (angles) {
         cachePtr->angle += roadmap_math_get_orientation ();

         while (cachePtr->angle > 360) cachePtr->angle -= 360;
         while (cachePtr->angle < 0) cachePtr->angle += 360;
         if (cachePtr->angle >= 180) cachePtr->angle -= 180;

         cachePtr->angle -= 90;

         // p = get_metrics (&(cachePtr->point), &r, cachePtr->angle, &cachePtr->bbox);
         p = get_metrics (cachePtr, &r);
      } else {
         /* Text will be horizontal, so bypass a lot of math.
          * (and compensate for eventual centering of text.)  */
         p = cachePtr->point;
         cachePtr->bbox.minx = r.minx + p.x - (r.maxx - r.minx)/2;
         cachePtr->bbox.maxx = r.maxx + p.x - (r.maxx - r.minx)/2;
         cachePtr->bbox.miny = r.miny + p.y - (r.maxy - r.miny)/2;
         cachePtr->bbox.maxy = r.maxy + p.y - (r.maxy - r.miny)/2;
      }

      for(i=0; i<l; i++) { /* compare against rendered label */
         /* compare bounding polygons and check for duplicates */

         if(roadmap_plugin_same_street(&cachePtr->street,
                 &RoadMapLabelCache.labels[i]->street)) {
            /* label is a duplicate */
            cachePtr->status = 0;
            break;
         }


         if(rect_overlap (&RoadMapLabelCache.labels[i]->bbox,
                 &cachePtr->bbox)) {

	    /* if labels are horizontal, bbox check is sufficient.  else... */
            if(!angles ||
		poly_overlap (RoadMapLabelCache.labels[i], cachePtr)) {

               cachePtr->status = 0;
               break;
	    }
         }
      }

      if(!cachePtr->status) {
      recycle:
         /* move us out of the "under consideration" group */
         if (l < RoadMapLabelCache.numlabels - 1) {
            labelCacheMemberObj * tmpPtr = cachePtr;
            cachePtr = RoadMapLabelCache.labels[RoadMapLabelCache.numlabels-1];
            RoadMapLabelCache.labels[RoadMapLabelCache.numlabels-1] = tmpPtr;;
            RoadMapLabelCache.labels[l--] = cachePtr;
            RoadMapLabelCache.numlabels--;
         }
         continue; /* next label */
      }

      if (angles) {
         roadmap_canvas_draw_string_angle
                 (&p, &cachePtr->point, cachePtr->angle, text);
      } else {
         roadmap_canvas_draw_string
                 (&cachePtr->point, ROADMAP_CANVAS_CENTER, text);
      }

   } /* next label */

   RoadMapLabelCache.numlabels = 0;
   RoadMapLabelCacheFull = 0;
   return 0;
}


int roadmap_label_activate (void) {

   RoadMapLabelPen = roadmap_canvas_create_pen ("labels.main");
   roadmap_canvas_set_foreground
      (roadmap_config_get (&RoadMapConfigLabelsColor));
    
   return 0;
}


int roadmap_label_initialize (void) {

   roadmap_config_declare
       ("preferences", &RoadMapConfigMinFeatureSize,  "25");

   roadmap_config_declare
       ("preferences", &RoadMapConfigLabelsColor,  "#000000");
    
   return 0;
}

