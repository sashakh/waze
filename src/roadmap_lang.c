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

#ifdef LANG_SUPPORT

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

/* if enabled roadmap will write all the values it does not find
 * to the lang file
 * this is usefull when generating a template for translators
 * there must be a langs directory inside the user preferences folder for 
 * this to work
 */
/* #define WRITE_MISSING */

static RoadMapConfigDescriptor RoadMapConfigLanguage =
                         ROADMAP_CONFIG_ITEM("General", "Language");


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


static int roadmap_lang_load (const char *path, const char *language) {

   char *p;
   FILE *file;
   char line[1024];

   char *name;
   char *value;

   if (language == NULL) return 0; 
   if (strlen(language)<=0) return 0;

   file = roadmap_file_fopen (path, language, "sr");
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
        *p = 0;
        p++;
        p = roadmap_config_skip_spaces (p);
        value = p;

        p = roadmap_config_skip_until (p, 0);
        *p = 0;
	
        if (strlen(name)==0) continue;

        name  = strdup (name);
        value = strdup (value);
        roadmap_lang_new_item (name, value);
    }
    fclose (file);

    return 1;
}


void roadmap_lang_initialize (void) {

   const char *p,*l,*path;
   int index;
   char **values;

   roadmap_lang_allocate ();

   l = roadmap_config_get (&RoadMapConfigLanguage);

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigLanguage,
                                          "",NULL);

   for (p = roadmap_path_first("config"); p != NULL;
                     p = roadmap_path_next("config", p)) {

       path = roadmap_path_join(p,"langs");
       RoadMapLangLoaded  = RoadMapLangLoaded | roadmap_lang_load(path,l);

       values = roadmap_path_list(path,"");
       index = 0;
       while ( values[index]!=NULL ) {
         roadmap_config_append_to_enumeration ("preferences",
                                   &RoadMapConfigLanguage, values[index]);
         index++;
       }
    }

    path = roadmap_path_join(roadmap_path_user(),"langs");
    RoadMapLangLoaded  = RoadMapLangLoaded | roadmap_lang_load(path,l);

    values = roadmap_path_list(path,"");
    index = 0;
    while ( values[index]!=NULL ) {
      roadmap_config_append_to_enumeration ("preferences", 
                                &RoadMapConfigLanguage, values[index]);
      index++;
    }


#ifdef WRITE_MISSING
  RoadMapLangLoaded=1;
#endif

   RoadMapLangRTL = (strcasecmp(roadmap_lang_get ("RTL"), "Yes") == 0);

}

#ifdef WRITE_MISSING
/* appends to the lang file the missing translation */
void roadmap_lang_write_missing(const char *name) {

   FILE *file;
   char *lang_path,*lang_file;

   lang_path = roadmap_path_join(roadmap_path_user(), "langs");
   lang_file = roadmap_config_get(&RoadMapConfigLanguage);
   
   if (strlen(lang_file) == 0) return;
   if (strlen(name) == 0) return ;

   file = roadmap_file_fopen (lang_path, lang_file, "a+");
   if (file == NULL) return;
  
   fprintf(file, "%s=%s\n",name,name); 
   fclose(file);
   /* add it to the hash so it won't be written more than once */
   roadmap_lang_new_item(name,name);
}
#endif

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

   /* translation not found */
#ifdef WRITE_MISSING
  roadmap_lang_write_missing(name);
#endif
   return name;
}


int roadmap_lang_rtl (void) {
   return RoadMapLangRTL;
}

#else /* LANG_SUPPORT */

void roadmap_lang_initialize (void) {
}

const char* roadmap_lang_get (const char *name) {
  return name;
}

int roadmap_lang_rtl(void) {
  return 0;
}

#endif

