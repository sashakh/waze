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

#define SSD_MIN_SIZE -1
#define SSD_MAX_SIZE -2

#define SSD_ALIGN_CENTER    0x1
#define SSD_ALIGN_RIGHT     0x2
#define SSD_START_NEW_ROW   0x4
#define SSD_END_ROW         0x8
#define SSD_ALIGN_VCENTER   0x10
#define SSD_ALIGN_GRID      0x20
#define SSD_GET_SIZE        0x40
#define SSD_WIDGET_SPACE    0x80
#define SSD_WIDGET_HIDE     0x100
#define SSD_ALIGN_BOTTOM    0x200
#define SSD_ALIGN_LTR       0x400

/* Buttons flags */
#define SSD_BUTTON_KEY        0x1000
#define SSD_BUTTON_TEXT       0x2000
#define SSD_BUTTON_TEXT_BELOW 0x4000

/* Dialogs */
#define SSD_DIALOG_FLOAT        0x10000
#define SSD_DIALOG_TRANSPARENT  0x20000

/* Container flags */
#define SSD_CONTAINER_BORDER 0x1000
#define SSD_CONTAINER_TITLE  0x2000

/* Text flags */
#define SSD_TEXT_LABEL 0x1000 /* Adds a ':' sign */

typedef struct ssd_size {
   short width;
   short height;
} SsdSize;

struct ssd_widget;
typedef struct ssd_widget *SsdWidget;

/* Widget callback. If it returns non zero, the new_value should not be
 * applied.
 */
typedef int (*SsdCallback) (SsdWidget widget, const char *new_value);

struct ssd_widget {

   char *typeid;

   struct ssd_widget *parent;
   struct ssd_widget *next;

   const char *name;

   char *value;

   struct ssd_widget *children;

   SsdSize size;
   SsdSize cached_size;

   int offset_x;
   int offset_y;

   int flags;

   const char *fg_color;
   const char *bg_color;

   SsdCallback callback;
   void *context;  /* References a caller-specific context. */

   RoadMapGuiPoint position;
   void *data; /* Widget specific data */

   const char * (*get_value) (SsdWidget widget);
   const void * (*get_data)  (SsdWidget widget);

   int  (*set_value)    (SsdWidget widget, const char *value);
   int  (*set_data)     (SsdWidget widget, const void *value);
   void (*draw)         (SsdWidget widget, RoadMapGuiRect *rect, int flags);
   int  (*pointer_down) (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*short_click)  (SsdWidget widget, const RoadMapGuiPoint *point);
   int  (*long_click)   (SsdWidget widget, const RoadMapGuiPoint *point);
};

SsdWidget ssd_widget_new (const char *name, int flags);
SsdWidget ssd_widget_get (SsdWidget child, const char *name);
void ssd_widget_draw (SsdWidget w, const RoadMapGuiRect *rect,
                      int parent_flags);
void ssd_widget_set_callback (SsdWidget widget, SsdCallback callback);
int  ssd_widget_rtl (SsdWidget parent);

int ssd_widget_pointer_down (SsdWidget widget, const RoadMapGuiPoint *point);
int ssd_widget_short_click  (SsdWidget widget, const RoadMapGuiPoint *point);
int ssd_widget_long_click   (SsdWidget widget, const RoadMapGuiPoint *point);

const char *ssd_widget_get_value (const SsdWidget widget, const char *name);
const void *ssd_widget_get_data (const SsdWidget widget, const char *name);
int ssd_widget_set_value (const SsdWidget widget, const char *name,
                          const char *value);
int ssd_widget_set_data (const SsdWidget widget, const char *name,
                          const void *value);

void ssd_widget_add (SsdWidget parent, SsdWidget child);

void ssd_widget_set_flags   (SsdWidget widget, int flags);
void ssd_widget_set_size    (SsdWidget widget, int width, int height);
void ssd_widget_set_offset  (SsdWidget widget, int x, int y);
void ssd_widget_set_context (SsdWidget widget, void *context);
void ssd_widget_get_size    (SsdWidget w, SsdSize *size, const SsdSize *max);
void ssd_widget_set_color   (SsdWidget w, const char *fg_color,
                             const char *bg_color);

void ssd_widget_container_size (SsdWidget dialog, SsdSize *size);

void *ssd_widget_get_context (SsdWidget w);

void ssd_widget_reset_cache (SsdWidget w);

void ssd_widget_hide (SsdWidget w);
void ssd_widget_show (SsdWidget w);

SsdWidget ssd_widget_find_by_pos (SsdWidget widget,
                                  const RoadMapGuiPoint *point);

#endif // __SSD_WIDGET_H_
