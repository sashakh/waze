/* roadmap_copy.c - Download RoadMap maps using file copy.
 *
 * LICENSE:
 *
 *   Copyright 2003 Pascal Martin.
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
 * SYNOPSYS:
 *
 *   See roadmap_copy.h
 *
 *
 * DESCRIPTION:
 *
 *   This module handles the local file name and "file://[...]" URLs
 *   for the map download feature of RoadMap. This module is designed
 *   to be linked to RoadMap either statically or dynamically. In the
 *   later case, the following linker option should be used when
 *   creating the shared library on Linux:
 *
 *       --defsym roadmap_plugin_init=roadmap_copy_init
 *
 *   The intend of this module is to provide a small example for the
 *   implementation of a download protocol module.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_download.h"
#include "roadmap_messagebox.h"

#include "roadmap_copy.h"


static int roadmap_copy (RoadMapDownloadCallbacks *callbacks,
                         const char *source,
                         const char *destination) {

   int size;
   RoadMapFileContext input;

   if (roadmap_file_map (source, NULL, &input) == NULL) {
      roadmap_messagebox ("Error", "Cannot find the download source file");
      return 0;
   }

   size = roadmap_file_size (input);

   if (! callbacks->size (size)) {
      roadmap_file_unmap (&input);
      return 0;
   }

   roadmap_file_save (NULL, destination,
                      roadmap_file_base (input), roadmap_file_size (input));

   roadmap_file_unmap (&input);

   callbacks->progress (size);

   return 1;
}


static int roadmap_copy_url (RoadMapDownloadCallbacks *callbacks,
                             const char *source,
                             const char *destination) {

   return roadmap_copy (callbacks, source + strlen("file://"), destination);
}


void roadmap_copy_init (RoadMapDownloadSubscribe subscribe) {

   subscribe ("/",       roadmap_copy);
   subscribe ("file://", roadmap_copy_url);
}

