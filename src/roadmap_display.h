/* roadmap_display.h - Manage screen signs.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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

#ifndef INCLUDED__ROADMAP_DISPLAY__H
#define INCLUDED__ROADMAP_DISPLAY__H

#include "roadmap_canvas.h"
#include "roadmap_plugin.h"

void roadmap_display_initialize (void);

void roadmap_display_page (const char *name);

int roadmap_display_activate (const char *title,
                              PluginLine *line,
                              const RoadMapPosition *position,
                              PluginStreet *street);

void roadmap_display_text (const char *title, const char *format, ...);

void roadmap_display_hide (const char *title);

void roadmap_display_signs (void);

const char *roadmap_display_get_id (const char *title);

int roadmap_display_is_refresh_needed (void);

#ifdef LANG_SUPPORT
#ifndef __ROADMAP_DISPLAY_NO_LANG
#include "roadmap_lang.h"
#include <stdarg.h>

static __inline void roadmap_display_text_i (const char *title, 
                                        const char *format, ...) {
   char text[1024];
   char format_i[1024]; 
   va_list parameters;

   sprintf (format_i,"%s",roadmap_lang_get(format));
   va_start(parameters, format);
   vsnprintf (text, sizeof(text), format_i, parameters);
   va_end(parameters);

  roadmap_display_text (roadmap_lang_get(title),
                        text);
}

static __inline void roadmap_display_hide_i (const char *title) {
  roadmap_display_hide(roadmap_lang_get(title));
}

#define roadmap_display_text	roadmap_display_text_i
#define roadmap_display_hide	roadmap_display_hide_i
#endif /* __ROADMAP_DISPLAY_NO_LANG */
#endif /* LANG_SUPPORT */

#endif // INCLUDED__ROADMAP_DISPLAY__H
