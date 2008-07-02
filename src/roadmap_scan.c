/* roadmap_scan.c - a module to scan for files using path lists.
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
 *
 *
 * DESCRIPTION:
 *
 *   The roadmap_scan_first() function returns a single directory path
 *   where the given file exists.
 *
 *   The roadmap_scan_next() function returns all successive directories
 *   where to find the given file.
 */

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_file.h"

#include "roadmap_scan.h"


const char *roadmap_scan_next (const char *set,
                               const char *name,
                               const char *sequence) {

   if (roadmap_path_is_full_path(name)) {

      if (roadmap_file_exists (NULL, name)) {
         return "";
      }

   } else {

      if (sequence == NULL) {
         sequence = roadmap_path_first(set);
      } else {
         sequence = roadmap_path_next(set, sequence);
      }
      while (sequence != NULL) {

         roadmap_log (ROADMAP_DEBUG, "searching for %s file %s in %s",
                      set,
                      name,
                      sequence);

         if (roadmap_file_exists (sequence, name)) {
            roadmap_log
               (ROADMAP_DEBUG, "found file %s in %s", name, sequence);
            return sequence;
         }

         sequence = roadmap_path_next(set, sequence);
      }
   }


   return NULL; /* Could not find anything. */
}


const char *roadmap_scan (const char *set, const char *name) {

   return roadmap_scan_next (set, name, NULL);
}

