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
 *   the Free Software Foundation, as of version 2 of the License.
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
   "string",
   rdmxchange_dictionary_map,
   rdmxchange_dictionary_activate,
   rdmxchange_dictionary_unmap
};


static void rdmxchange_dictionary_export_head (FILE *file) {

   struct dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      fprintf (file, "string/%s/tree,%d\n",
                     volume->name,
                     volume->tree_count);

      fprintf (file, "string/%s/node,%d\n",
                     volume->name,
                     volume->node_count);

      fprintf (file, "string/%s/index,%d\n",
                     volume->name,
                     volume->string_count);

      fprintf (file, "string/%s/data,%d\n",
                     volume->name,
                     volume->size);
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

      fprintf (file, "string/%s/tree\n"
                     "first,count,position\n", volume->name);
      tree = volume->tree;

      for (i = 0; i < volume->tree_count; ++i, ++tree) {
         fprintf (file, "%.0d,%.0d,%.0d\n", tree->first,
                                            tree->count,
                                            tree->position);
      }
      fprintf (file, "\n");


      fprintf (file, "string/%s/node\n"
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


      fprintf (file, "string/%s/index\n"
                     "offset\n", volume->name);
      index = volume->string_index;

      for (i = 0; i < volume->string_count; ++i, ++index) {
         fprintf (file, "%d\n", *index);
      }
      fprintf (file, "\n");


      fprintf (file, "string/%s/data\n"
                     "text\n", volume->name);
      data = volume->data + 1; /* Skip the first (empty) string. */
      data_end = volume->data + volume->size;

      for (; data < data_end; data += (strlen(data) + 1)) {
         fprintf (file, "%s\n", data);
      }
      fprintf (file, "\n");
   }
}


static RdmXchangeExport RdmXchangeDictionaryExport = {
   "string",
   rdmxchange_dictionary_export_head,
   rdmxchange_dictionary_export_data
};

static void rdmxchange_dictionary_register_export (void) {

   rdmxchange_main_register_export (&RdmXchangeDictionaryExport);
}


/* The import side. ----------------------------------------------------- */

static int  DictionaryCursor = 0;
static struct dictionary_volume *DictionaryCurrentVolume = 0;


static void rdmxchange_dictionary_save_one (buildmap_db *root,
                                            struct dictionary_volume *volume) {

   int i;

   struct roadmap_dictionary_tree *db_tree;
   struct roadmap_dictionary_reference *db_node;
   unsigned int *db_index;
   char *db_data;

   buildmap_db *child;
   buildmap_db *table_tree;
   buildmap_db *table_node;
   buildmap_db *table_index;
   buildmap_db *table_data;


   if (volume->tree == NULL ||
         volume->node == NULL ||
         volume->string_index == NULL ||
         volume->data == NULL) {
      buildmap_fatal (1, "incomplete dictionary volume %s", volume->name);
   }

   /* Create the tables. */

   child = buildmap_db_add_section (root, volume->name);
   if (child == NULL) {
      buildmap_fatal (0, "Cannot add new section %s", volume->name);
   }

   table_tree =
      buildmap_db_add_child (child,
                             "tree",
                             volume->tree_count,
                             sizeof(struct roadmap_dictionary_tree));

   table_node =
      buildmap_db_add_child (child,
                             "node",
                             volume->node_count,
                             sizeof(struct roadmap_dictionary_reference));

   table_index = buildmap_db_add_child (child,
                                        "index",
                                        volume->string_count,
                                         sizeof(unsigned int));

   table_data = buildmap_db_add_child (child, "data", volume->size, 1);

   db_tree =
      (struct roadmap_dictionary_tree *) buildmap_db_get_data (table_tree);
   db_node = (struct roadmap_dictionary_reference *)
      buildmap_db_get_data (table_node);
   db_index = (unsigned int *) buildmap_db_get_data (table_index);
   db_data  = buildmap_db_get_data (table_data);


   /* Fill the data in. */

   for (i = 0; i < volume->tree_count; ++i) {
      db_tree[i] = volume->tree[i];
   }
   for (i = 0; i < volume->node_count; ++i) {
      db_node[i] = volume->node[i];
   }
   for (i = 0; i < volume->string_count; ++i) {
      db_index[i] = volume->string_index[i];
   }
   memcpy (db_data, volume->data, volume->size);

   /* Do not save this data ever again. */
   free (volume->tree);
   free (volume->node);
   free (volume->string_index);
   free (volume->data);
   free (volume->name);
   free (volume);
}


static void rdmxchange_dictionary_save (void) {

   buildmap_db *root;

   struct dictionary_volume *volume;
   struct dictionary_volume *next;


   root  = buildmap_db_add_section (NULL, "string");
   if (root == NULL) buildmap_fatal (0, "Can't add a new section");

   for (volume = DictionaryVolume; volume != NULL; volume = next) {

      next = volume->next;
      rdmxchange_dictionary_save_one (root, volume);
   }
}


static buildmap_db_module RdmXchangeDictionaryModule = {
   "string",
   NULL,
   rdmxchange_dictionary_save,
   NULL,
   NULL
};


static struct dictionary_volume *rdmxchange_dictionary_find (const char *name) {

   struct dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      if (strcmp (name, volume->name) ==0) return volume;
   }
   return NULL;
}


static void rdmxchange_dictionary_import_table (const char *name, int count) {

   char *p;
   struct dictionary_volume *volume;


   buildmap_db_register (&RdmXchangeDictionaryModule);

   if (strncmp (name, "string/", 7) == 0) {

      /* Separate the volume name. */

      char *volume_name = strdup (name + 7);

      p = strchr (volume_name, '/');
      if (p == NULL) buildmap_fatal (1, "invalid table name %s", name);
      *(p++) = 0;

      /* Retrieve the volume, or create a new one. */

      volume = rdmxchange_dictionary_find (volume_name);
      if (volume == NULL) {
         volume = calloc (1, sizeof(struct dictionary_volume));
         volume->name = volume_name;
         volume->next = DictionaryVolume;
         DictionaryVolume = volume;
      } else {
         free (volume_name); /* We already have a copy in volume->name. */
      }

      /* Now gather the information about the specific table. */

      if (strcmp (p, "tree") == 0) {

         if (volume->tree != NULL) {
            buildmap_fatal (1, "duplicate tree in %s", volume->name);
         }
         volume->tree = calloc (count, sizeof(struct roadmap_dictionary_tree));
         volume->tree_count = count;

      } else if (strcmp (p, "node") == 0) {

         if (volume->node != NULL) {
            buildmap_fatal (1, "duplicate node in %s", volume->name);
         }
         volume->node = calloc (count,
                                sizeof(struct roadmap_dictionary_reference));
         volume->node_count = count;

      } else if (strcmp (p, "index") == 0) {

         if (volume->string_index != NULL) {
            buildmap_fatal (1, "duplicate index in %s", volume->name);
         }
         volume->string_index = calloc (count, sizeof (unsigned int));
         volume->string_count = count;

      } else if (strcmp (p, "data") == 0) {

         if (volume->data != NULL) {
            buildmap_fatal (1, "duplicate data in %s", volume->name);
         }
         volume->data = calloc (count, sizeof (char));
         volume->size = count;

      } else {

         buildmap_fatal (1, "invalid table name %s", name);
      }

   } else {

      buildmap_fatal (1, "invalid table name %s", name);
   }
}


static int rdmxchange_dictionary_import_schema (const char *table,
                                                char *fields[], int count) {

   struct dictionary_volume *volume;

   DictionaryCursor = 0;

   if (strncmp (table, "string/", 7) == 0) {

      /* Separate the volume name. */

      char *volume_name = strdup (table + 7);
      char *p = strchr (volume_name, '/');

      if (p == NULL) buildmap_fatal (1, "invalid table name %s", table);
      *(p++) = 0;

      /* Retrieve the volume (it must exist). */

      volume = rdmxchange_dictionary_find (volume_name);
      if (volume == NULL) {
         buildmap_fatal (1, "%s: unknown volume", table);
      }
      DictionaryCurrentVolume = volume;

      if (strcmp (p, "tree") == 0) {

         if (volume->tree == NULL ||
               count != 3 ||
               strcmp(fields[0], "first") != 0 ||
               strcmp(fields[1], "count") != 0 ||
               strcmp(fields[2], "position") != 0) {
            buildmap_fatal (1, "invalid schema for table %s", table);
         }
         return 1;

      } else if (strcmp (p, "node") == 0) {

         if (volume->node == NULL ||
               count != 3 ||
               strcmp(fields[0], "character") != 0 ||
               strcmp(fields[1], "type") != 0 ||
               strcmp(fields[2], "index") != 0) {
            buildmap_fatal (1, "invalid schema for table %s", table);
         }
         return 2;

      } else if (strcmp (p, "index") == 0) {

         if (volume->string_index == NULL ||
               count != 1 || strcmp (fields[0], "offset") != 0) {
            buildmap_fatal (1, "invalid schema for table %s", table);
         }
         return 3;

      } else if (strcmp (p, "data") == 0) {

         if (volume->data == NULL ||
               count != 1 || strcmp (fields[0], "text") != 0) {
            buildmap_fatal (1, "invalid schema for table %s", table);
         }
         volume->data[0] = 0;
         DictionaryCursor = 1;
         return 4;
      }
   }

   buildmap_fatal (1, "invalid table name %s", table);
   return 0;
}


static void rdmxchange_dictionary_import_data (int table,
                                        char *fields[], int count) {

   switch (table) {

      case 1:

         if (count != 3) {
            buildmap_fatal (count, "invalid string tree record");
         }
         DictionaryCurrentVolume->tree[DictionaryCursor].first =
            (unsigned short) rdmxchange_import_int (fields[0]);
         DictionaryCurrentVolume->tree[DictionaryCursor].count =
            (unsigned char) rdmxchange_import_int (fields[1]);
         DictionaryCurrentVolume->tree[DictionaryCursor].position =
            (unsigned char) rdmxchange_import_int (fields[2]);
         break;

      case 2:

         if (count != 3) {
            buildmap_fatal (count, "invalid string node record");
         }
         DictionaryCurrentVolume->node[DictionaryCursor].character = *(fields[0]);
         DictionaryCurrentVolume->node[DictionaryCursor].type =
            (unsigned char) rdmxchange_import_int (fields[1]);
         DictionaryCurrentVolume->node[DictionaryCursor].index =
            (unsigned short) rdmxchange_import_int (fields[2]);
         break;

      case 3:

         if (count != 1) {
            buildmap_fatal (count, "invalid string index record");
         }
         DictionaryCurrentVolume->string_index[DictionaryCursor] =
            rdmxchange_import_int (fields[0]);
         break;

      case 4:

         if (count != 1) {
            buildmap_fatal (count, "invalid string data record");
         }
         strcpy (DictionaryCurrentVolume->data + DictionaryCursor, fields[0]);
         DictionaryCursor += strlen(fields[0]);
         break;

      default:

          buildmap_fatal (1, "invalid table");
   }

   DictionaryCursor += 1;
}


RdmXchangeImport RdmXchangeDictionaryImport = {
   "string",
   rdmxchange_dictionary_import_table,
   rdmxchange_dictionary_import_schema,
   rdmxchange_dictionary_import_data
};

