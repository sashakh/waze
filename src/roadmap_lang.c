/* roadmap_lang.c - i18n
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
 * SYNOPSYS:
 *
 *   See roadmap_lang.h.
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_config.h"
#include "roadmap_hash.h"
#include "roadmap_lang.h"

#define INITIAL_ITEMS_SIZE 50

struct RoadMapLangItem {

   const char *name;
   const char *value;
};

static struct RoadMapLangItem *RoadMapLangItems;
static int RoadMapLangSize;
static int RoadMapLangCount;
static RoadMapHash *RoadMapLangHash;
static int RoadMapLangLoaded = 0;
static int RoadMapLangRTL = 0;


static void roadmap_lang_allocate (void) {

   if (RoadMapLangSize == 0) {
      RoadMapLangSize = INITIAL_ITEMS_SIZE;
      RoadMapLangItems = calloc(RoadMapLangSize, sizeof(struct RoadMapLangItem));
      RoadMapLangHash = roadmap_hash_new ("lang_hash", RoadMapLangSize);
   } else {
      RoadMapLangSize *= 2;
      RoadMapLangItems =
         realloc(RoadMapLangItems,
                 RoadMapLangSize * sizeof(struct RoadMapLangItem));
      roadmap_hash_resize (RoadMapLangHash, RoadMapLangSize);
   }

   if (RoadMapLangItems == NULL) {
      roadmap_log (ROADMAP_FATAL, "No memory.");
   }
}


static void roadmap_lang_new_item (const char *name, const char *value) {

   int hash = roadmap_hash_string (name);

   if (RoadMapLangCount == RoadMapLangSize) {
      roadmap_lang_allocate ();
   }

   RoadMapLangItems[RoadMapLangCount].name  = name;
   RoadMapLangItems[RoadMapLangCount].value = value;

   roadmap_hash_add (RoadMapLangHash, hash, RoadMapLangCount);

   RoadMapLangCount++;
}


static int roadmap_lang_load (const char *path) {

   char *p;
   FILE *file;
   char  line[1024];

   char *name;
   char *value;

   file = roadmap_file_fopen (path, "lang", "sr");
   if (file == NULL) return 0;

   while (!feof(file)) {

        /* Read the next line, skip empty lines and comments. */

        if (fgets (line, sizeof(line), file) == NULL) break;

        p = roadmap_config_extract_data (line, sizeof(line));
        if (p == NULL) continue;

        /* Decode the line (name= value). */
        
        name = p;

        p = roadmap_config_skip_until (p, '=');
        if (*p != '=') continue;
        *(p++) = 0;

        p = roadmap_config_skip_spaces (p);
        value = p;

        p = roadmap_config_skip_until (p, 0);
        *p = 0;

        name  = strdup (name);
        value = strdup (value);

        roadmap_lang_new_item (name, value);
    }
    fclose (file);

    return 1;
}


void roadmap_lang_initialize (void) {

   const char *p;

   roadmap_lang_allocate ();

#ifndef J2ME
   p = roadmap_path_user ();
#else
   p = NULL;
#endif

   RoadMapLangLoaded = roadmap_lang_load (p);
   RoadMapLangRTL = (strcasecmp(roadmap_lang_get ("RTL"), "Yes") == 0);
}


const char* roadmap_lang_get (const char *name) {

   int hash;
   int i;
   
   if (!RoadMapLangLoaded) return name;
   
   hash = roadmap_hash_string (name);

   for (i = roadmap_hash_get_first (RoadMapLangHash, hash);
        i >= 0;
        i = roadmap_hash_get_next (RoadMapLangHash, i)) {

      if (!strcmp(name, RoadMapLangItems[i].name)) {
         
         return RoadMapLangItems[i].value;
      }
   }

   return name;
}


int roadmap_lang_rtl (void) {
   return RoadMapLangRTL;
}

