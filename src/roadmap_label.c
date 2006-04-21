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

#include "roadmap_config.h"
#include "roadmap_math.h"
#include "roadmap_plugin.h"

#include "roadmap_label.h"

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

static RoadMapConfigDescriptor RoadMapConfigMinFeatureSize =
                        ROADMAP_CONFIG_ITEM("Labels", "MinFeatureSize");

static labelCacheObj RoadMapLabelCache;
static int RoadMapLabelCacheFull = 0;

static int rect_overlap (RoadMapGuiRect *a, RoadMapGuiRect *b) {

   if(a->minx > b->maxx) return 0;
   if(a->maxx < b->minx) return 0;
   if(a->miny > b->maxy) return 0;
   if(a->maxy < b->miny) return 0;

   return 1;
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


static RoadMapGuiPoint get_metrics(RoadMapGuiPoint *p, RoadMapGuiRect *rect,
                                   int ox, int oy, int angle,
                                   RoadMapGuiRect *bbox) {
   RoadMapGuiPoint q;
   int x1=0, y1=0;
   RoadMapGuiPoint poly[4];
   int w, h;
   int buffer = 0;
   int lines = 2;

   w = rect->maxx - rect->minx;
   h = rect->maxy - rect->miny;

   /* position CC */
   x1 = -(int)(w/2.0) + ox;
   y1 = (int)(h/2.0) + oy;

   q.x = x1 - rect->minx;
   q.y = rect->maxy - y1;

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

   //roadmap_canvas_draw_multiple_lines(1, &lines, poly);

   compute_bbox(poly, bbox);

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
      roadmap_log (ROADMAP_ERROR, "Too many lables on screen.");
      RoadMapLabelCacheFull = 1;
      return -1;
   }

   cachePtr = &(RoadMapLabelCache.labels[RoadMapLabelCache.numlabels]);

   cachePtr->featuresize = featuresize;
   cachePtr->line = *line;
   cachePtr->angle = angle;
   cachePtr->point = *point;
   cachePtr->status = 0;

   RoadMapLabelCache.numlabels++;

   return 0;
}


int roadmap_label_draw_cache (void) {

   int l;
   int i;
   int width;
   int ascent;
   int descent;
   RoadMapGuiPoint p;
   RoadMapGuiRect r;
   int label_offsetx = 0;
   int label_offsety = 0;
   const char *text;

   labelCacheMemberObj *cachePtr=NULL;

   for(l=0; l<RoadMapLabelCache.numlabels; l++) {

      PluginStreetProperties properties;

      cachePtr = &(RoadMapLabelCache.labels[l]);

      roadmap_plugin_get_street_properties (&cachePtr->line, &properties);

      if (!properties.street || !*properties.street) {
         continue;
      }

      text = properties.street;
      cachePtr->street = properties.plugin_street;

      roadmap_canvas_get_text_extents(text, &width, &ascent, &descent);
      r.minx = 0;
      r.maxx=width;
      r.miny = -ascent;
      r.maxy = descent;
      //FIXME
      width *= 1.5;

      /*
         label_offsety += MS_NINT(((bbox[5] + bbox[1]) + size) / 2);
         label_offsetx += MS_NINT(bbox[0] / 2);
         */

      /* text is too long for this feature */
      if ((width >> 2)> cachePtr->featuresize) {
         continue;
      }

      cachePtr->status = 1; /* assume label *can* be drawn */

      /* The stored point is not screen orieneted, rotate is needed */
      roadmap_math_rotate_coordinates (1, &cachePtr->point);
      cachePtr->angle += roadmap_math_get_orientation ();

      while (cachePtr->angle > 360) cachePtr->angle -= 360;
      while (cachePtr->angle < 0) cachePtr->angle += 360;
      if (cachePtr->angle >= 180) cachePtr->angle -= 180;

      cachePtr->angle -= 90;

      //cachePtr->angle = (int)(cachePtr->angle / 18) * 18;

      p = get_metrics (&(cachePtr->point), &r, label_offsetx, label_offsety,
            cachePtr->angle, &cachePtr->bbox);

      /*
         if(!labelPtr->partials) {
         if(labelInImage(img->sx, img->sy, cachePtr->poly, labelPtr->buffer+map_edge_buffer) == 0)
         cachePtr->status = 0;
         }
         */

      if(!cachePtr->status)
         continue; /* next label */

      for(i=0; i<l; i++) { /* compare against rendered label */
         if(RoadMapLabelCache.labels[i].status == 1) { /* compare bounding polygons and check for duplicates */

            /* MIN_DISTANCE */
            if(roadmap_plugin_same_street(&cachePtr->street, &RoadMapLabelCache.labels[i].street)) { /* label is a duplicate */
               cachePtr->status = 0;
               break;
            }

            if(rect_overlap (&RoadMapLabelCache.labels[i].bbox, &cachePtr->bbox)) {
               cachePtr->status = 0;
               break;
            }
         }
      }

      /* imagePolyline(img, cachePtr->poly, 1, 0, 0); */

      if(!cachePtr->status)
         continue; /* next label */

      roadmap_canvas_draw_string_angle (&p, &cachePtr->point, cachePtr->angle, text);

   } /* next label */

   RoadMapLabelCache.numlabels = 0;
   RoadMapLabelCacheFull = 0;
   return 0;
}


int roadmap_label_initialize (void) {

   roadmap_config_declare
       ("preferences", &RoadMapConfigMinFeatureSize,  "25");

   return 0;
}

