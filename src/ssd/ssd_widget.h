/* ssd_widget.h - defines a generic widget
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 */

#ifndef __SSD_WIDGET_H_
#define __SSD_WIDGET_H_
  
#include "roadmap_gui.h"

#define SSD_ALIGN_CENTER    0x1
#define SSD_ALIGN_RIGHT     0x2
#define SSD_START_NEW_ROW   0x4
#define SSD_END_ROW         0x8

/* Buttons flags */
#define SSD_BUTTON_KEY        0x100
#define SSD_BUTTON_TEXT       0x200
#define SSD_BUTTON_TEXT_BELOW 0x400


typedef struct ssd_size {
   unsigned short width;
   unsigned short height;
} SsdSize;

struct ssd_widget;
typedef struct ssd_widget *SsdWidget;

/* Widget callback. If it returns non zero, the new_value should not be
 * applied.
 */
typedef int (*SsdCallback) (const char *name, const char *new_value,
                            void *context);

struct ssd_widget {

   char *typeid;

   struct ssd_widget *next;

   const char *name;

   char *value;

   struct ssd_widget *children;

   SsdSize size;

   int flags;

   SsdCallback callback;
   void *context;  /* References a caller-specific context. */

   RoadMapGuiPoint position;
   void *data; /* Widget specific data */

   int  (*set_value)    (SsdWidget widget, const char *value);
   void (*draw)         (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*pointer_down) (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*short_click)  (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*long_click)   (SsdWidget widget, const RoadMapGuiPoint *point);
};

SsdWidget ssd_widget_new (const char *name, int flags);
SsdWidget ssd_widget_get (SsdWidget child, const char *name);
void ssd_widget_draw (SsdWidget w, const RoadMapGuiRect *rect);
void ssd_widget_set_callback (SsdWidget widget, SsdCallback callback);
int  ssd_widget_rtl (void);

void ssd_widget_pointer_down (SsdWidget widget, RoadMapGuiPoint *point);
void ssd_widget_short_click  (SsdWidget widget, RoadMapGuiPoint *point);
void ssd_widget_long_click   (SsdWidget widget, RoadMapGuiPoint *point);

const char *ssd_widget_get_value (const SsdWidget widget, const char *name);
int ssd_widget_set_value (const SsdWidget widget, const char *name,
                          const char *value);
#endif // __SSD_WIDGET_H_
