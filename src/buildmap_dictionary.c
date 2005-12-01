/* buildmap_dictionary.c - Build a Map dictionary table & index for RoadMap.
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
 *   BuildMapDictionary buildmap_dictionary_open (char *name);
 *   RoadMapString buildmap_dictionary_add
 *                    (BuildMapDictionary d, char *string, int length);
 *   RoadMapString buildmap_dictionary_locate
 *                    (BuildMapDictionary d, char *string);
 *   char *buildmap_dictionary_get (BuildMapDictionary d, RoadMapString index);
 *   void  buildmap_dictionary_save    (void);
 *   void  buildmap_dictionary_summary (void);
 *   void  buildmap_dictionary_reset   (void);
 *
 * These functions are used to build a dictionary of strings from
 * the Tiger maps. The objective is double: (1) reduce the size of
 * the Tiger data by sharing all duplicated strings and (2) serve
 * as the basis for a fast search mechanism by providing an index
 * table designed to help progressive search (i.e. all possible
 * characters following the latest input).
 *
 * Every string in the dictionary can be retrieved using a character
 * search tree:
 * - each node of the tree represents one character in the string.
 * - each node is a list of references to the next level, one
 *   reference for each possible character in the referenced strings.
 * - the references are indexes into an indirection table (to keep
 *   the references small), either the string table (leaf)
 *   or the node table (node).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "buildmap.h"
#include "roadmap_db_dictionary.h"


#define DICTIONARY_INDEX_SIZE 0x10000
#define DICTIONARY_BLOCK_SIZE 0x10000

struct dictionary_reference {

   char character;
   char type;

   unsigned short child;

   struct dictionary_reference *next;
};

struct dictionary_tree {

   struct dictionary_reference *first;
   unsigned short count;
   unsigned short position;
};

struct dictionary_volume {

   char *name;

   struct dictionary_tree tree[DICTIONARY_INDEX_SIZE];
   unsigned int tree_count;
   unsigned int reference_count;

   unsigned int  string_index [DICTIONARY_INDEX_SIZE];
   RoadMapString string_count;

   char *data;
   int   cursor;
   int   size;

   int hits;
};

#define DICTIONARY_MAX_VOLUME_COUNT  16

static struct dictionary_volume *DictionaryVolume[DICTIONARY_MAX_VOLUME_COUNT];
static int DictionaryVolumeCount = 0;

static int DictionaryDebugOn = 0;


static void buildmap_dictionary_print_subtree
               (struct dictionary_volume *dictionary,
                struct dictionary_tree *tree) {

   struct dictionary_reference *reference;

   fprintf (stdout, "%*.*s --- start of tree %d (count = %d)\n",
                    tree->position,
                    tree->position,
                    "",
                    (tree - dictionary->tree),
                    tree->count);

   for (reference = tree->first;
        reference != NULL;
        reference = reference->next) {

      fprintf (stdout, "%*.*s '%c' [%d]",
                       tree->position,
                       tree->position,
                       "",
                       reference->character,
                       tree->position);

      switch (reference->type) {

      case ROADMAP_DICTIONARY_STRING:

         fprintf (stdout, " --> \"%s\"\n",
                          dictionary->data
                             + dictionary->string_index[reference->child]);

         break;

      case ROADMAP_DICTIONARY_TREE:

         fprintf (stdout, "\n");
         buildmap_dictionary_print_subtree
            (dictionary, dictionary->tree + reference->child);

         break;

      default:
         fprintf (stdout, "\n");
         buildmap_fatal (0, "corrupted node type %d when printing tree",
                            reference->type);
      }
   }
}


static void buildmap_dictionary_fatal
               (struct dictionary_volume *dictionary,
                struct dictionary_tree *tree) {

   buildmap_dictionary_print_subtree (dictionary, dictionary->tree);
   buildmap_fatal (0, "end of corrupted tree dump at tree %d.",
                      tree - dictionary->tree);
}


static struct dictionary_reference *
buildmap_dictionary_get_reference (struct dictionary_tree *tree, char c) {

   struct dictionary_reference *reference;

   c = tolower(c);

   if (DictionaryDebugOn) {

      printf ("search for '%c' at position %d in tree 0x%p\n",
              c, tree->position, tree);

      for (reference = tree->first;
           reference != NULL;
           reference = reference->next) {

         printf ("compare to {'%c', %s, %d}\n",
                 reference->character,
                 (reference->type == ROADMAP_DICTIONARY_STRING) ?
                     "string" : "tree",
                 reference->child);

         if (reference->character == c) {
            printf ("   found.\n");
            return reference;
         }
      }
      printf ("   not found.\n");
      return NULL;
   }

   for (reference = tree->first;
        reference != NULL;
        reference = reference->next) {

      if (reference->character == c) {
         return reference;
      }
   }

   return NULL;
}


static void
buildmap_dictionary_add_reference (struct dictionary_volume *dictionary,
                                   struct dictionary_tree *tree,
                                   char character,
                                   char type,
                                   short child) {

   struct dictionary_reference *reference;

   reference = malloc (sizeof(struct dictionary_reference));
   if (reference == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   reference->type = type;
   reference->child = child;
   reference->character = tolower(character);

   reference->next = tree->first;

   tree->first = reference;
   tree->count += 1;

   dictionary->reference_count += 1;
}


static int
buildmap_dictionary_search
   (struct dictionary_volume *dictionary,
    const char *string,
    int   length,
    struct dictionary_tree **last_tree) {

   struct dictionary_tree *tree;
   struct dictionary_reference *reference;

   int   i;
   const char *cursor;
   const char *stored;


   *last_tree = tree = dictionary->tree;

   for (i= 0, cursor = string; i < length; cursor++, i++) {

        if (tree->position != i) {
           buildmap_error
              (0, "corrupted tree position %d (should be %d) in tree %d"
                     " when searching for '%s'",
                  tree->position,
                  i,
                  tree - dictionary->tree,
                  string);
           buildmap_dictionary_fatal (dictionary, tree);
        }

        reference = buildmap_dictionary_get_reference (tree, *cursor);

        if (reference == NULL) {
           return -1;
        }

        switch (reference->type) {

        case ROADMAP_DICTIONARY_STRING:

           stored = dictionary->data
                       + dictionary->string_index[reference->child];

           if ((stored[length] != 0) ||
               (strncasecmp (string, stored, length) != 0)) {
              return -1;
           }
           return reference->child;

        case ROADMAP_DICTIONARY_TREE:

           if (*cursor == 0) {
              buildmap_error (0, "found a subtree after end of string");
              buildmap_dictionary_fatal (dictionary, tree);
           }

           if (reference->child >= dictionary->tree_count) {
              buildmap_error
                 (0, "corrupted tree when searching for %s", string);
              buildmap_dictionary_fatal (dictionary, tree);
           }
           *last_tree = tree = dictionary->tree + reference->child;
           break;

        default:
           buildmap_error (0, "corrupted node when searching for %s", string);
           buildmap_dictionary_fatal (dictionary, tree);
        }
   }

   reference = buildmap_dictionary_get_reference (tree, 0);

   if (reference != NULL) {
      if (reference->type != ROADMAP_DICTIONARY_STRING) {
         buildmap_error (0, "found a subtree after end of string");
         buildmap_dictionary_fatal (dictionary, tree);
      }
      return reference->child;
   }

   return -1;
}


static void  buildmap_dictionary_optimize
                (struct dictionary_volume *dictionary,
                 struct dictionary_tree *tree,
                 int *tree_count,
                 int *reference_count) {

   struct dictionary_tree *subtree;
   struct dictionary_reference *reference;

   /* Remove the tree nodes that lead to a single choice: these
    * are just a waste of memory.
    */
   while (tree->count == 1) {

      reference = tree->first;

      if (reference == NULL) {
         buildmap_error (0, "corrupted tree count when optimizing");
         buildmap_dictionary_fatal (dictionary, tree);
      }
      if (reference->type != ROADMAP_DICTIONARY_TREE) {
         break; /* Not a subtree: stop here. */
      }
      if (reference->next != NULL) {
         buildmap_error (0, "corrupted tree list when optimizing");
         buildmap_dictionary_fatal (dictionary, tree);
      }

      subtree = dictionary->tree + reference->child;

      tree->first = subtree->first;
      tree->count = subtree->count;
      tree->position = subtree->position;
   }

   *tree_count += 1;
   *reference_count += tree->count;

   for (reference = tree->first;
        reference != NULL;
        reference = reference->next) {

      if (reference->type == ROADMAP_DICTIONARY_TREE) {

         subtree = dictionary->tree + reference->child;

         buildmap_dictionary_optimize
            (dictionary, subtree, tree_count, reference_count);
      }
   }
}


static int buildmap_dictionary_save_subtree
               (struct dictionary_volume *dictionary,
                struct dictionary_tree *tree,
                int *tree_cursor,
                int *reference_cursor,
                struct roadmap_dictionary_tree *db_tree,
                struct roadmap_dictionary_reference *db_reference) {

   int this_tree;
   int this_reference;
   int next_reference;
   struct dictionary_tree *subtree;
   struct dictionary_reference *reference;

   /* Allocate space for this tree and its references: */

   this_tree = *tree_cursor;
   this_reference = *reference_cursor;
   next_reference = this_reference + tree->count;

   *tree_cursor += 1;
   *reference_cursor += tree->count;

   db_tree[this_tree].first = this_reference;
   db_tree[this_tree].count = tree->count;
   db_tree[this_tree].position = tree->position;


   /* Fill in the references and process all subtrees : */

   for (reference = tree->first;
        reference != NULL;
        reference = reference->next) {

      db_reference[this_reference].character = reference->character;
      db_reference[this_reference].type = reference->type;

      switch (reference->type) {

      case ROADMAP_DICTIONARY_STRING:

         db_reference[this_reference].index = reference->child;

         break;

      case ROADMAP_DICTIONARY_TREE:

         subtree = dictionary->tree + reference->child;

         db_reference[this_reference].index =
            buildmap_dictionary_save_subtree
               (dictionary, subtree,
                tree_cursor, reference_cursor, db_tree, db_reference);
      }

      this_reference += 1;
   }

   if (this_reference != next_reference) {
      buildmap_error (0, "corrupted tree count when saving to disk");
      buildmap_dictionary_fatal (dictionary, tree);
   }

   return this_tree;
}


static void  buildmap_dictionary_save_one
                (struct dictionary_volume *dictionary,
                 buildmap_db *parent) {

   int tree_count = 0;
   int reference_count = 0;

   buildmap_db *child;
   buildmap_db *table_tree;
   buildmap_db *table_node;
   buildmap_db *table_index;
   buildmap_db *table_data;
   struct roadmap_dictionary_tree *db_tree;
   struct roadmap_dictionary_reference *db_reference;
   unsigned int *db_index;
   char *db_data;


   /* Make the dictionary as small as possible and compute its
    * final size.
    */
   buildmap_dictionary_optimize
      (dictionary, dictionary->tree, &tree_count, &reference_count);


   /* Allocate all the database space: */

   child = buildmap_db_add_section (parent, dictionary->name);
   if (child == NULL) buildmap_fatal (0, "Can't add a new section");

   table_tree = buildmap_db_add_child
         (child, "tree", tree_count, sizeof(struct roadmap_dictionary_tree));

   table_node =
      buildmap_db_add_child (child,
                             "node", 
                             reference_count,
                             sizeof(struct roadmap_dictionary_reference));

   table_index = buildmap_db_add_child
         (child, "index", dictionary->string_count, sizeof(unsigned int));

   table_data = buildmap_db_add_child (child, "data", dictionary->cursor, 1);

   db_tree =
      (struct roadmap_dictionary_tree *) buildmap_db_get_data (table_tree);
   db_reference = (struct roadmap_dictionary_reference *)
      buildmap_db_get_data (table_node);
   db_index = (unsigned int *) buildmap_db_get_data (table_index);
   db_data  = buildmap_db_get_data (table_data);

   tree_count = 0;
   reference_count = 0;

   buildmap_dictionary_save_subtree
      (dictionary, dictionary->tree,
       &tree_count, &reference_count, db_tree, db_reference);

   memcpy (db_index, dictionary->string_index,
           dictionary->string_count * sizeof(unsigned int));
   memcpy (db_data, dictionary->data, dictionary->cursor);
}


static void buildmap_dictionary_free_subtree
               (struct dictionary_volume *dictionary,
                struct dictionary_tree *tree) {

   struct dictionary_reference *reference;
   struct dictionary_reference *next;

   for (reference = tree->first; reference != NULL; reference = next) {

      next = reference->next;

      switch (reference->type) {

      case ROADMAP_DICTIONARY_STRING:

         break;

      case ROADMAP_DICTIONARY_TREE:

         buildmap_dictionary_free_subtree
            (dictionary, dictionary->tree + reference->child);

         break;

      default:
         buildmap_fatal (0, "corrupted node type %d when releasing tree",
                            reference->type);
      }
      free (reference);
   }
}


BuildMapDictionary buildmap_dictionary_open (char *name) {

   int i;
   struct dictionary_volume *dictionary;

   for (i = 0; i < DictionaryVolumeCount; i++) {
      if (strcmp (name, DictionaryVolume[i]->name) == 0) {
         return DictionaryVolume[i];
      }
   }

   /* This is a new dictionary volume. */

   if (DictionaryVolumeCount >= DICTIONARY_MAX_VOLUME_COUNT) {
      buildmap_fatal (0, "out of dictionary slots");
   }

   dictionary = malloc (sizeof(struct dictionary_volume));

   if (dictionary == NULL) {
      buildmap_fatal (0, "out of memory");
   }

   DictionaryVolume[DictionaryVolumeCount] = dictionary;
   DictionaryVolumeCount += 1;

   dictionary->name = strdup(name);
   dictionary->data = malloc (DICTIONARY_BLOCK_SIZE);

   if (dictionary->data == NULL) {
      buildmap_fatal (0, "no more memory");
   }

   dictionary->size = DICTIONARY_BLOCK_SIZE;

   dictionary->cursor = 0;
   dictionary->hits = 0;
   dictionary->reference_count = 0;

   for (i = 0; i < DICTIONARY_INDEX_SIZE; i++) {

      dictionary->tree[i].first = NULL;
      dictionary->tree[i].count = 0;
      dictionary->tree[i].position = -1;

      dictionary->string_index[i] = 0xffffffff;
   }

   dictionary->tree[0].position = 0;

   dictionary->tree_count = 1;


   /* Add the empty string. */

   dictionary->data[0] = 0;
   dictionary->cursor  = 1;
   dictionary->string_index[0] = 0;
   dictionary->string_count = 1;

   buildmap_dictionary_add_reference
      (dictionary, dictionary->tree, 0, ROADMAP_DICTIONARY_STRING, 0);

   return dictionary;
}


RoadMapString buildmap_dictionary_add
           (BuildMapDictionary dictionary, const char *string, int length) {

   int  cursor;
   char character;
   int  result;

   struct dictionary_tree *tree;
   struct dictionary_tree *start_tree;


   if (length == 0) {
      return 0; /* Empty string optimization (quite frequent). */
   }

   result = buildmap_dictionary_search (dictionary, string, length, &tree);
   start_tree = tree;

   if (result < 0) {

      struct dictionary_reference *reference;

      if (dictionary->string_count >= DICTIONARY_INDEX_SIZE-1) {
         buildmap_dictionary_summary ();
         buildmap_fatal (0, "dictionary full (index)");
      }
      if (DictionaryDebugOn) {
         printf ("  string not found: adding it.\n");
      }
      result = dictionary->string_count;

      cursor = dictionary->cursor + length + 1;

      if (cursor >= 0x1000000) {
         buildmap_fatal (0, "dictionary full (data, %d entries)",
                            dictionary->string_count);
      }

      while (cursor >= dictionary->size) {
         dictionary->size += DICTIONARY_BLOCK_SIZE;
         dictionary->data = realloc (dictionary->data, dictionary->size);
      }

      strncpy (dictionary->data + dictionary->cursor, string, length);
      dictionary->data[dictionary->cursor + length] = 0;

      dictionary->string_index[result] = dictionary->cursor;

      /* Update the search tree. */

      if (tree->position < length) {
         character = string[tree->position];
      } else {
         character = 0;
      }

      reference = buildmap_dictionary_get_reference (tree, character);

      while (reference != NULL) {

         /* There was a similar string: we must create a new tree item
          * to distinguish between the two.
          */
         int existing = reference->child;
         int position = tree->position;
         char *referenced = dictionary->data
                               + dictionary->string_index[existing];

         if (reference->type != ROADMAP_DICTIONARY_STRING) {
            buildmap_error (0, "dictionary corrupted (uncomplete search)");
            buildmap_dictionary_fatal (dictionary, tree);
         }
         if (dictionary->tree_count >= DICTIONARY_INDEX_SIZE) {
            buildmap_dictionary_summary ();
            buildmap_fatal (0, "dictionary tree full");
         }

         if (DictionaryDebugOn) {
            printf ("  pruning string \"%s\".\n", referenced);
         }
         reference->type  = ROADMAP_DICTIONARY_TREE;
         reference->child = dictionary->tree_count;
         dictionary->tree_count += 1;

         tree = dictionary->tree + reference->child;
         tree->count = 0;
         tree->first = NULL;
         tree->position = position + 1;

         if (tree->position < length) {
            character = string[tree->position];
         } else {
            character = 0;
         }

         buildmap_dictionary_add_reference
            (dictionary,
             tree,
             referenced[tree->position], ROADMAP_DICTIONARY_STRING, existing);

         reference = buildmap_dictionary_get_reference (tree, character);
      }

      buildmap_dictionary_add_reference
         (dictionary, tree,
          character, ROADMAP_DICTIONARY_STRING, result);

      dictionary->string_count += 1;
      dictionary->cursor = cursor;

   } else {

      if (DictionaryDebugOn) {
         printf ("  string found.\n");
      }
      dictionary->hits += 1;
   }

   return (RoadMapString)result;
}


char *buildmap_dictionary_get
         (BuildMapDictionary dictionary, RoadMapString index) {

   if (index >= dictionary->string_count) {
      return NULL;
   }

   return dictionary->data + dictionary->string_index[index];
}


RoadMapString buildmap_dictionary_locate
                 (BuildMapDictionary dictionary, const char *string) {

   struct dictionary_tree *tree;

   return buildmap_dictionary_search
             (dictionary, string, strlen(string), &tree);
}


void buildmap_dictionary_summary (void) {

   int i;
   int size;
   int total_size = 0;
   int total_strings = 0;
   int total_hits = 0;
   int total_references = 0;
   int total_trees = 0;
   struct dictionary_volume *dictionary;


   fprintf (stderr, "-- Dictionary statistics:\n");

   for (i = 0; i < DictionaryVolumeCount; i++) {

      dictionary = DictionaryVolume[i];

      size = dictionary->cursor
                   + (dictionary->reference_count
                         * sizeof(struct roadmap_dictionary_reference))
                   + (dictionary->tree_count
                         * sizeof(struct roadmap_dictionary_tree))
                   + (dictionary->string_count * sizeof(int));

      fprintf (stderr, "--    %s: %d items, %d hits, %d bytes\n"
                    "                     %d references, %d nodes\n",
                    dictionary->name,
                    dictionary->string_count,
                    dictionary->hits,
                    size,
                    dictionary->reference_count,
                    dictionary->tree_count);

      total_size += size;
      total_strings += dictionary->string_count;
      total_hits += dictionary->hits;
      total_references += dictionary->reference_count;
      total_trees += dictionary->tree_count;
   }

   fprintf (stderr,
            "-- totals: %d items, %d hits, %d bytes, %d references, %d nodes\n",
            total_strings,
            total_hits,
            total_size,
            total_references,
            total_trees);
}


void  buildmap_dictionary_save (void) {

   int i;

   buildmap_db *names = buildmap_db_add_section (NULL, "string");
   if (names == NULL) buildmap_fatal (0, "Can't add a new section");

   buildmap_info ("saving dictionary...");

   for (i = 0; i < DictionaryVolumeCount; i++) {

      if (DictionaryVolume[i]->name[0] != '.') {
         buildmap_dictionary_save_one (DictionaryVolume[i], names);
      }
   }
}


void buildmap_dictionary_reset (void) {

   int i;

   for (i = 0; i < DICTIONARY_MAX_VOLUME_COUNT; i++) {

       if (DictionaryVolume[i] != NULL) {

          buildmap_dictionary_free_subtree
             (DictionaryVolume[i], DictionaryVolume[i]->tree);

          free (DictionaryVolume[i]->data);
          free (DictionaryVolume[i]);
          DictionaryVolume[i] = NULL;
       }
   }

   DictionaryVolumeCount = 0;
   DictionaryDebugOn = 0;
}

