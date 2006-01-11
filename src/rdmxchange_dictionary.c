/* rdmxchange_dictionary.c - Export a Map dictionary table & index.
 *
 * LICENSE:
 *
 *   Copyright 2006 Pascal F. Martin
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_dbread.h"

#include "rdmxchange.h"

#include "roadmap_db_dictionary.h"


struct dictionary_volume {

   char *name;
   struct dictionary_volume *next;

   struct roadmap_dictionary_tree      *tree;
   int                                  tree_count;

   struct roadmap_dictionary_reference *node;
   int                                  node_count;

   unsigned int *string_index;
   int string_count;

   char *data;
   int   size;
};


static struct dictionary_volume *DictionaryVolume = NULL;

static void rdmxchange_dictionary_register_export (void);


static struct dictionary_volume *
         rdmxchange_dictionary_initialize_one (roadmap_db *child) {

   roadmap_db *table;
   struct dictionary_volume *dictionary;


   /* Retrieve all the database sections: */

   dictionary = malloc (sizeof(struct dictionary_volume));

   roadmap_check_allocated(dictionary);

   dictionary->name = roadmap_db_get_name (child);

   table = roadmap_db_get_subsection (child, "data");

   dictionary->data = roadmap_db_get_data (table);
   dictionary->size = roadmap_db_get_size (table);

   table = roadmap_db_get_subsection (child, "tree");

   dictionary->tree =
      (struct roadmap_dictionary_tree *) roadmap_db_get_data (table);
   dictionary->tree_count = roadmap_db_get_count (table);

   table = roadmap_db_get_subsection (child, "node");

   dictionary->node =
      (struct roadmap_dictionary_reference *) roadmap_db_get_data (table);
   dictionary->node_count = roadmap_db_get_count (table);

   table = roadmap_db_get_subsection (child, "index");

   dictionary->string_index = (unsigned int *) roadmap_db_get_data (table);
   dictionary->string_count = roadmap_db_get_count (table);

   return dictionary;
}


static void *rdmxchange_dictionary_map (roadmap_db *parent) {

   roadmap_db *child;
   struct dictionary_volume *volume;
   struct dictionary_volume *first = NULL;


   for (child = roadmap_db_get_first(parent);
        child != NULL;
        child = roadmap_db_get_next(child)) {

       volume = rdmxchange_dictionary_initialize_one (child);
       volume->next = first;

       first = volume;
   }

   rdmxchange_dictionary_register_export();

   return first;
}

static void rdmxchange_dictionary_activate (void *context) {

   DictionaryVolume = (struct dictionary_volume *) context;
}

static void rdmxchange_dictionary_unmap (void *context) {

   struct dictionary_volume *this = (struct dictionary_volume *) context;

   if (this == DictionaryVolume) {
      DictionaryVolume = NULL;
   }

   while (this != NULL) {

      struct dictionary_volume *next = this->next;

      free (this);
      this = next;
   }
}

roadmap_db_handler RoadMapDictionaryExport = {
   "dictionary",
   rdmxchange_dictionary_map,
   rdmxchange_dictionary_activate,
   rdmxchange_dictionary_unmap
};


static void rdmxchange_dictionary_export_head (FILE *file) {

   struct dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      fprintf (file, "table string/%s/node %d\n",
                     volume->name,
                     volume->node_count);

      fprintf (file, "table string/%s/tree %d\n",
                     volume->name,
                     volume->tree_count);

      fprintf (file, "table string/%s/data %d\n",
                     volume->name,
                     volume->size);

      fprintf (file, "table string/%s/index %d\n",
                     volume->name,
                     volume->string_count);
   }
}


static void rdmxchange_dictionary_export_data (FILE *file) {

   int   i;
   char *data;
   char *data_end;
   unsigned int *index;
   struct dictionary_volume *volume;
   RoadMapDictionaryReference *node;
   RoadMapDictionaryTree      *tree;


   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      fprintf (file, "table string/%s/node\n"
                     "character,type,index\n", volume->name);
      node = volume->node;

      for (i = 0; i < volume->node_count; ++i, ++node) {
         if (node->character == 0) {
            fprintf (file, ",%.0d,%.0d\n", node->type, node->index);
            continue;
         }
         fprintf (file, "%c,%.0d,%.0d\n", node->character,
                                          node->type,
                                          node->index);
      }
      fprintf (file, "\n");


      fprintf (file, "table string/%s/tree\n"
                     "first,count,position\n", volume->name);
      tree = volume->tree;

      for (i = 0; i < volume->tree_count; ++i, ++tree) {
         fprintf (file, "%.0d,%.0d,%.0d\n", tree->first,
                                            tree->count,
                                            tree->position);
      }
      fprintf (file, "\n");


      fprintf (file, "table string/%s/data\n"
                     "text\n", volume->name);
      data = volume->data + 1; /* Skip the first (empty) string. */
      data_end = volume->data + volume->size;

      for (; data < data_end; data += (strlen(data) + 1)) {
         fprintf (file, "%s\n", data);
      }
      fprintf (file, "\n");


      fprintf (file, "table string/%s/index\n"
                     "offset\n", volume->name);
      index = volume->string_index;

      for (i = 0; i < volume->string_count; ++i, ++index) {
         fprintf (file, "%d\n", *index);
      }
      fprintf (file, "\n");
   }
}


static RdmXchangeExport RdmXchangeDictionaryExport = {
   "dictionary",
   rdmxchange_dictionary_export_head,
   rdmxchange_dictionary_export_data
};

static void rdmxchange_dictionary_register_export (void) {

   rdmxchange_main_register_export (&RdmXchangeDictionaryExport);
}
