/* roadmap_history.c - manage the roadmap address history.
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
 * SYNOPSYS:
 *
 *   See roadmap_history.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_file.h"


static RoadMapConfigDescriptor RoadMapConfigHistoryDepth =
                        ROADMAP_CONFIG_ITEM("History", "Depth");

struct roadmap_history_log_entry {

   struct roadmap_history_log_entry *before;
   struct roadmap_history_log_entry *after;
   char data[1];
};


static int RoadMapHistoryChanged = 0;
static int RoadMapHistoryCount = 0;
static struct roadmap_history_log_entry *RoadMapLatest = NULL;
static struct roadmap_history_log_entry *RoadMapOldest = NULL;


static void roadmap_history_remove_entry
               (struct roadmap_history_log_entry *entry) {
                   
    if (entry->after != NULL) {
        entry->after->before = entry->before;
    } else {
        if (entry != RoadMapLatest) {
            roadmap_log (ROADMAP_FATAL, "invalid lastest entry");
        }
        RoadMapLatest = entry->before;
    }
    
    if (entry->before != NULL) {
        entry->before->after = entry->after;
    } else {
        if (entry != RoadMapOldest) {
            roadmap_log (ROADMAP_FATAL, "invalid oldest entry");
        }
        RoadMapOldest = entry->after;
    }
    
    RoadMapHistoryCount -= 1;
    RoadMapHistoryChanged = 1;
}


static void roadmap_history_add_entry (const char *data) {

   struct roadmap_history_log_entry *entry = NULL;

   if (RoadMapLatest != NULL) {
       
      if (strcasecmp (data, RoadMapLatest->data) == 0) {
         return; /* Same entry as before. */
      }
      
      for (entry = RoadMapLatest->before;
           entry != NULL;
           entry = entry->before) {
               
        if (strcasecmp (data, entry->data) == 0) {
            
            roadmap_history_remove_entry (entry);
            break;
        }
      }
   }

   if (entry == NULL) {
       
       entry = malloc (strlen(data) +
                       sizeof(struct roadmap_history_log_entry));
       roadmap_check_allocated(entry);

       strcpy (entry->data, data);
   }

   entry->before = RoadMapLatest;
   entry->after  = NULL;

   if (RoadMapLatest != NULL) {
       RoadMapLatest->after = entry;
   } else {
       RoadMapOldest = entry;
   }

   RoadMapLatest = entry;
   RoadMapHistoryCount += 1;

   if (RoadMapHistoryCount >
       roadmap_config_get_integer(&RoadMapConfigHistoryDepth)) {
           
      entry = RoadMapOldest;
      roadmap_history_remove_entry (entry);
      free (entry);
   }
   
   RoadMapHistoryChanged = 1;
}


static void roadmap_history_get_from_entry
               (struct roadmap_history_log_entry *entry,
                char **number,
                char **street,
                char **city,
                char **state) {

   static char data[1024];
   char *p;


   *number = "";
   *street = "";
   *city   = "";
   *state  = "";

   if (entry == NULL) return;


   strncpy (data, entry->data, sizeof(data));

   *number = data;
   p = strchr (data, ',');

   if (p != NULL) {

      *p = 0;
      *street = ++p;
      p = strchr (p, ',');

      if (p != NULL) {

         *p = 0;
         *city = ++p;
         p = strchr (p, ',');

         if (p != NULL) {

            *p = 0;
            *state = ++p;
         }
      }
   }
}


static void roadmap_history_save_entry
               (FILE *file, struct roadmap_history_log_entry *entry) {

   if (entry->before != NULL) {
      roadmap_history_save_entry (file, entry->before);
   }
   fprintf (file, "%s\n", entry->data);
}


void roadmap_history_initialize (void) {

   roadmap_config_declare ("preferences", &RoadMapConfigHistoryDepth, "100");
}


void roadmap_history_load (void) {

   static int loaded = 0;

   FILE *file;
   char *p;
   char  line[1024];


   if (loaded) return;

   file = roadmap_file_open (roadmap_file_user(), "history", "sr");

   if (file != NULL) {

      while (! feof(file)) {

         if (fgets (line, sizeof(line), file) == NULL) break;

         p = roadmap_config_extract_data (line, sizeof(line));

         if (p == NULL) continue;

         roadmap_history_add_entry (p);
      }

      fclose (file);
   }

   loaded = 1;
}


void roadmap_history_add
        (char *number, char *street, char *city, char *state) {

   char data[1024];

   snprintf (data, sizeof(data), "%s,%s,%s,%s", number, street, city, state);

   roadmap_history_add_entry (data);
}


void *roadmap_history_get_latest
         (char **number, char **street, char **city, char **state) {

   roadmap_history_get_from_entry
             (RoadMapLatest, number, street, city, state);

   return RoadMapLatest;
}


void *roadmap_history_get_before
         (void *reference,
          char **number, char **street, char **city, char **state) {

   struct roadmap_history_log_entry *entry =
             (struct roadmap_history_log_entry *)reference;

   if (entry != NULL) {
      if (entry->before != NULL) {
         entry = entry->before;
      }
   } else {
     entry = RoadMapLatest;
   }
   roadmap_history_get_from_entry (entry, number, street, city, state);

   return entry;
}


void *roadmap_history_get_after
         (void *reference,
          char **number, char **street, char **city, char **state) {

   struct roadmap_history_log_entry *entry =
             (struct roadmap_history_log_entry *)reference;

   if (entry != NULL) {
      entry = entry->after;
   }
   roadmap_history_get_from_entry (entry, number, street, city, state);

   return entry;
}


void roadmap_history_purge (int count) {

   struct roadmap_history_log_entry *entry;

   while (RoadMapHistoryCount > count) {
       entry = RoadMapOldest;
       roadmap_history_remove_entry (entry);
       free(entry);
       RoadMapHistoryChanged = 1;
   }
}


void roadmap_history_save (void) {

   FILE *file;

   if (RoadMapLatest == NULL) return; /* Nothing to save. */

   if (! RoadMapHistoryChanged) return; /* Nothing new to save. */

   file = roadmap_file_open (roadmap_file_user(), "history", "w");

   if (file != NULL) {

      roadmap_history_save_entry (file, RoadMapLatest);

      fclose (file);
   }
}

