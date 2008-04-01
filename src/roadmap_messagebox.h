/* roadmap_messagebox.h - Display a small message window for RoadMap.
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

#ifndef INCLUDE__ROADMAP_MESSAGEBOX__H
#define INCLUDE__ROADMAP_MESSAGEBOX__H

void roadmap_messagebox_hide (void *handle);
void *roadmap_messagebox        (const char *title, const char *message);
void *roadmap_messagebox_wait   (const char *title, const char *message);
#ifdef LANG_SUPPORT
#ifndef __ROADMAP_MESSAGEBOX_NO_LANG
#include "roadmap_lang.h"

static __inline void *roadmap_messagebox_i (const char *title, 
                                           const char *message) {
  return roadmap_messagebox(roadmap_lang_get(title),
                     roadmap_lang_get(message));
}

static __inline void *roadmap_messagebox_wait_i (const char *title,
                                                const char *message) {
  return roadmap_messagebox_wait(roadmap_lang_get(title), 
                          roadmap_lang_get(message));
}

#define roadmap_messagebox      roadmap_messagebox_i
#define roadmap_messagebox_wait      roadmap_messagebox_wait_i
#endif /* __ROADMAP_MESSAGEBOX_NO_LANG */
#endif /* LANG_SUPPORT */



#endif // INCLUDE__ROADMAP_MESSAGEBOX__H
