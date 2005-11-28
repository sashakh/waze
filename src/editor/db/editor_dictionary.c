/* editor_dictionary.c - Build a Map dictionary table & index for RoadMap.
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
 *   BuildMapDictionary editor_dictionary_open (char *name);
 *   int editor_dictionary_add
 *                    (BuildMapDictionary d, char *string, int length);
 *   int editor_dictionary_locate
 *                    (BuildMapDictionary d, char *string);
 *   char *editor_dictionary_get (BuildMapDictionary d, int index);
 *   void  editor_dictionary_save    (void);
 *   void  editor_dictionary_summary (void);
 *   void  editor_dictionary_reset   (void);
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
#include <assert.h>

#include "roadmap.h"

#include "editor_db.h"
#include "editor_dictionary.h"


#define DICTIONARY_MAX_VOLUME_COUNT  16

static struct ed_dictionary_volume
                   *DictionaryVolume = NULL;

static int DictionaryDebugOn = 0;


static void editor_dictionary_print_subtree
               (struct ed_dictionary_volume *dictionary, int tree_id) {

   struct ed_dictionary_reference *reference = NULL;
   int reference_id;

   struct ed_dictionary_tree *tree =
            (struct ed_dictionary_tree *) editor_db_get_item
               (dictionary->tree_db_section, tree_id, 0, NULL);

   if (tree == NULL) return;

   fprintf (stdout, "%*.*s --- start of tree %d (count = %d)\n",
                    tree->position,
                    tree->position,
                    "",
                    tree_id,
                    editor_db_get_item_count (dictionary->tree_db_section));

   for (reference_id = tree->first;
        reference_id != -1;
        reference_id = reference->next) {

      reference = (struct ed_dictionary_reference *) editor_db_get_item
         (dictionary->reference_db_section, reference_id, 0, NULL);

      fprintf (stdout, "%*.*s '%c' [%d]",
                       tree->position,
                       tree->position,
                       "",
                       reference->character,
                       tree->position);

      switch (reference->type) {

      case ROADMAP_DICTIONARY_STRING:

         fprintf (stdout, " --> \"%s\"\n",
                 (char *) editor_db_get_item
                    (dictionary->data_db_section, reference->child, 0, NULL));
         break;

      case ROADMAP_DICTIONARY_TREE:

         fprintf (stdout, "\n");
         editor_dictionary_print_subtree
            (dictionary, reference->child);

         break;

      default:
         fprintf (stdout, "\n");
         roadmap_log (ROADMAP_FATAL,
               "corrupted node type %d when printing tree",
               reference->type);
      }
   }
}


static void editor_dictionary_fatal
               (struct ed_dictionary_volume *dictionary,
                int tree_id) {

   editor_dictionary_print_subtree (dictionary, 0);
   roadmap_log (ROADMAP_FATAL, "end of corrupted tree dump at tree %d.",
                      tree_id);
}


static struct ed_dictionary_reference *
editor_dictionary_get_reference (struct ed_dictionary_volume *dictionary,
      struct ed_dictionary_tree *tree, char c) {

   struct ed_dictionary_reference *reference;
   int reference_id;

   c = tolower(c);

   if (DictionaryDebugOn) {

      printf ("search for '%c' at position %d in tree 0x%p\n",
              c, tree->position, tree);

      for (reference_id = tree->first;
           reference_id != -1;
           reference_id = reference->next) {

         reference = (struct ed_dictionary_reference *) editor_db_get_item
            (dictionary->reference_db_section, reference_id, 0, NULL);

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

   for (reference_id = tree->first;
        reference_id != -1;
        reference_id = reference->next) {

      reference = (struct ed_dictionary_reference *) editor_db_get_item
            (dictionary->reference_db_section, reference_id, 0, NULL);

      if (reference->character == c) {
         return reference;
      }
   }

   return NULL;
}


static int
editor_dictionary_add_reference (struct ed_dictionary_volume *dictionary,
                                   struct ed_dictionary_tree *tree,
                                   char character,
                                   char type,
                                   int child) {

   struct ed_dictionary_reference reference;
   int reference_id;

   reference.type = type;
   reference.child = child;
   reference.character = tolower(character);
   reference.next = tree->first;

   reference_id =
      editor_db_add_item (dictionary->reference_db_section, &reference);

   if (reference_id == -1) {
      editor_db_grow ();
      reference_id =
         editor_db_add_item (dictionary->reference_db_section, &reference);
   }

   if (reference_id == -1) {
      return -1;
   }

   tree->first = reference_id;
   tree->count += 1;

   return 0;
}


static void editor_dictionary_initialize
               (struct ed_dictionary_volume *dictionary) {

   struct ed_dictionary_tree tree;
   int string_id;
   int tree_id;

   /* Add the empty string. */

   string_id = editor_db_add_item (
         dictionary->data_db_section, "");

   assert (string_id == 0);

   tree.first = -1;
   tree.count = 0;
   tree.position = 0;

   tree_id = editor_db_add_item (dictionary->tree_db_section, &tree);

   assert (tree_id == 0);

   editor_dictionary_add_reference
      (dictionary,
       (struct ed_dictionary_tree *) editor_db_get_item
         (dictionary->tree_db_section, 0, 0, NULL),
       0,
       ROADMAP_DICTIONARY_STRING,
       0);
}


static int
editor_dictionary_search
   (struct ed_dictionary_volume *dictionary,
    const char *string,
    int   length,
    int *last_tree_id) {

   struct ed_dictionary_tree *tree;
   struct ed_dictionary_reference *reference;

   int   i;
   const char *cursor;
   char *stored;


   *last_tree_id = 0;

   tree = 
       (struct ed_dictionary_tree *) editor_db_get_item
          (dictionary->tree_db_section, 0, 0, NULL);

   if (tree == NULL) {
       /* this is a new tree. Initialize it. */
       editor_dictionary_initialize (dictionary);
       tree = 
           (struct ed_dictionary_tree *) editor_db_get_item
                  (dictionary->tree_db_section, *last_tree_id, 0, NULL);
   }

   for (i= 0, cursor = string; i < length; cursor++, i++) {

        tree = 
            (struct ed_dictionary_tree *) editor_db_get_item
               (dictionary->tree_db_section, *last_tree_id, 0, NULL);

        if (tree == NULL) {
           /* this is a new tree. Initialize it. */
           editor_dictionary_initialize (dictionary);

           tree = 
               (struct ed_dictionary_tree *) editor_db_get_item
                     (dictionary->tree_db_section, *last_tree_id, 0, NULL);
        }

        if (tree->position != i) {
           roadmap_log
              (ROADMAP_ERROR, "corrupted tree position %d (should be %d) in tree %d"
                     " when searching for '%s'",
                  tree->position,
                  i,
                  *last_tree_id,
                  string);
           editor_dictionary_fatal (dictionary, *last_tree_id);
        }

        reference = editor_dictionary_get_reference
                              (dictionary, tree, *cursor);

        if (reference == NULL) {
           return -1;
        }

        switch (reference->type) {

        case ROADMAP_DICTIONARY_STRING:

           stored = (char *) editor_db_get_item
                    (dictionary->data_db_section, reference->child, 0, NULL);

           if ((stored[length] != 0) ||
               (strncasecmp (string, stored, length) != 0)) {
              return -1;
           }
           return reference->child;

        case ROADMAP_DICTIONARY_TREE:

           if (*cursor == 0) {
              roadmap_log (ROADMAP_ERROR, "found a subtree after end of string");
              editor_dictionary_fatal (dictionary, *last_tree_id);
           }

           if (reference->child >=
                 editor_db_get_item_count (dictionary->tree_db_section)) {
              roadmap_log
                 (ROADMAP_ERROR, "corrupted tree when searching for %s", string);
              editor_dictionary_fatal (dictionary, *last_tree_id);
           }
           *last_tree_id = reference->child;
           break;

        default:
           roadmap_log (ROADMAP_ERROR, "corrupted node when searching for %s", string);
           editor_dictionary_fatal (dictionary, *last_tree_id);
        }
   }

   tree = 
       (struct ed_dictionary_tree *) editor_db_get_item
          (dictionary->tree_db_section, *last_tree_id, 0, NULL);

   reference = editor_dictionary_get_reference (dictionary, tree, 0);

   if (reference != NULL) {
      if (reference->type != ROADMAP_DICTIONARY_STRING) {
         roadmap_log (ROADMAP_ERROR, "found a subtree after end of string");
         editor_dictionary_fatal (dictionary, *last_tree_id);
      }
      return reference->child;
   }

   return -1;
}


static struct ed_dictionary_volume *
         editor_dictionary_initialize_one (roadmap_db *child) {

   roadmap_db *table;
   struct ed_dictionary_volume *dictionary;


   /* Retrieve all the database sections: */

   dictionary = malloc (sizeof(struct ed_dictionary_volume));

   roadmap_check_allocated(dictionary);

   dictionary->name = roadmap_db_get_name (child);

   table = roadmap_db_get_subsection (child, "data");

   dictionary->data_db_section =
      (editor_db_section *)roadmap_db_get_data (table);

   table = roadmap_db_get_subsection (child, "trees");

   dictionary->tree_db_section =
      (editor_db_section *) roadmap_db_get_data (table);

   table = roadmap_db_get_subsection (child, "references");

   dictionary->reference_db_section =
      (editor_db_section *) roadmap_db_get_data (table);

   dictionary->hits = 0;

   return dictionary;
}


static void *editor_dictionary_map (roadmap_db *parent) {

   roadmap_db *child;
   struct ed_dictionary_volume *volume;
   struct ed_dictionary_volume *first = NULL;


   for (child = roadmap_db_get_first(parent);
        child != NULL;
        child = roadmap_db_get_next(child)) {

       volume = editor_dictionary_initialize_one (child);
       volume->next = first;

       first = volume;
   }

   return first;
}

static void editor_dictionary_activate (void *context) {

   DictionaryVolume = (struct ed_dictionary_volume *) context;
}

static void editor_dictionary_unmap (void *context) {

   struct ed_dictionary_volume *this = (struct ed_dictionary_volume *) context;

   if (this == DictionaryVolume) {
      DictionaryVolume = NULL;
   }

   while (this != NULL) {

      struct ed_dictionary_volume *next = this->next;

      free (this);
      this = next;
   }
}

roadmap_db_handler EditorDictionaryHandler = {
   "dictionary",
   editor_dictionary_map,
   editor_dictionary_activate,
   editor_dictionary_unmap
};


struct ed_dictionary_volume *editor_dictionary_open (char *name) {

   struct ed_dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      if (strcmp (volume->name, name) == 0) {
         return volume;
      }
   }

   return NULL;
}


int editor_dictionary_add
                 (struct ed_dictionary_volume *dictionary,
                  const char *string, int length) {

   char character;

   int tree_id;
   int string_id;
   char *db_string;
   struct ed_dictionary_tree *tree;


/* Can't just return 0 as the dictionary may not yet be initialized. */
#if 0
   if (length == 0) {
      return 0; /* Empty string optimization (quite frequent). */
   }
#endif
   string_id = editor_dictionary_search
      (dictionary, string, length, &tree_id);

   tree = 
       (struct ed_dictionary_tree *) editor_db_get_item
          (dictionary->tree_db_section, tree_id, 0, NULL);

   if (string_id < 0) {

      struct ed_dictionary_reference *reference;

      if (DictionaryDebugOn) {
         printf ("  string not found: adding it.\n");
      }

      string_id =
         editor_db_allocate_items
            (dictionary->data_db_section, length+1);

      if (string_id == -1) {
         editor_db_grow ();

         string_id =
            editor_db_allocate_items
               (dictionary->data_db_section, length+1);
      }

      if (string_id == -1) {
         roadmap_log (ROADMAP_ERROR, "dictionary data full");
         return -1;
      }

      db_string = (char *) editor_db_get_item (
         dictionary->data_db_section, string_id, 0, NULL);

      strncpy (db_string, string, length);
      db_string[length] = 0;

      /* Update the search tree. */

      if (tree->position < length) {
         character = string[tree->position];
      } else {
         character = 0;
      }

      reference = editor_dictionary_get_reference (dictionary, tree, character);

      while (reference != NULL) {

         /* There was a similar string: we must create a new tree item
          * to distinguish between the two.
          */
         int existing = reference->child;
         int position = tree->position;
         char *referenced =
            db_string = (char *) editor_db_get_item (
               dictionary->data_db_section, existing, 0, NULL);

         if (reference->type != ROADMAP_DICTIONARY_STRING) {
            roadmap_log (ROADMAP_ERROR, "dictionary corrupted (uncomplete search)");
            editor_dictionary_fatal (dictionary, tree_id);
         }

         tree_id = editor_db_add_item (dictionary->tree_db_section, NULL);

         if (tree_id == -1) {
            roadmap_log (ROADMAP_ERROR, "dictionary tree full");
            return -1;
         }

         if (DictionaryDebugOn) {
            printf ("  pruning string \"%s\".\n", referenced);
         }
         reference->type  = ROADMAP_DICTIONARY_TREE;
         reference->child = tree_id;

         tree =
            (struct ed_dictionary_tree *) editor_db_get_item
               (dictionary->tree_db_section, tree_id, 0, NULL);

         tree->count = 0;
         tree->first = -1;
         tree->position = position + 1;

         if (tree->position < length) {
            character = string[tree->position];
         } else {
            character = 0;
         }

         editor_dictionary_add_reference
            (dictionary,
             tree,
             referenced[tree->position], ROADMAP_DICTIONARY_STRING, existing);

         reference = editor_dictionary_get_reference (dictionary, tree, character);
      }

      editor_dictionary_add_reference
         (dictionary, tree,
          character, ROADMAP_DICTIONARY_STRING, string_id);

   } else {

      if (DictionaryDebugOn) {
         printf ("  string found.\n");
      }
      dictionary->hits += 1;
   }

   return string_id;
}


char *editor_dictionary_get
         (struct ed_dictionary_volume *dictionary, int index) {

   if (index >= editor_db_get_item_count (dictionary->data_db_section)) {
      return NULL;
   }

   return (char *) editor_db_get_item
      (dictionary->data_db_section, index, 0, NULL);
}


int editor_dictionary_locate
          (struct ed_dictionary_volume *dictionary, const char *string) {

   int tree_id;

   return editor_dictionary_search
             (dictionary, string, strlen(string), &tree_id);
}

