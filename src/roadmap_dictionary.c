/* roadmap_dictionary.c - Access a Map dictionary table & index.
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
 * See roadmap_dictionary.h.
 *
 * These functions are used to access a dictionary of strings from
 * a RoadMap database.
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

#include "roadmap_db_dictionary.h"

#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_dictionary.h"


struct dictionary_volume {

   char *name;
   struct dictionary_volume *next;

   struct roadmap_dictionary_tree      *tree;
   struct roadmap_dictionary_reference *reference;

   unsigned int *string_index;
   int string_count;

   char *data;
   int   size;
};


static struct dictionary_volume *DictionaryVolume = NULL;


struct dictionary_cursor {

   struct dictionary_volume *volume;
   int tree_index;
   int complete;
   int completion;
};


static char *roadmap_dictionary_get_string
                (struct dictionary_volume *dictionary, int index) {

   return dictionary->data
             + dictionary->string_index[dictionary->reference[index].index];
}


static void roadmap_dictionary_print_subtree
               (struct dictionary_volume *dictionary, int tree_index) {

   int index;
   struct roadmap_dictionary_tree *tree = dictionary->tree + tree_index;


   fprintf (stdout, "%*.*s --- start of tree %d (count = %d)\n",
                    tree->position,
                    tree->position,
                    "",
                    (tree - dictionary->tree),
                    tree->count);

   for (index = tree->first;
        index < tree->first + tree->count;
        index++) {

      fprintf (stdout, "%*.*s '%c' [%d]",
                       tree->position,
                       tree->position,
                       "",
                       dictionary->reference[index].character,
                       tree->position);

      switch (dictionary->reference[index].type & 0x0f) {

      case ROADMAP_DICTIONARY_STRING:

         fprintf (stdout, " --> [%d] \"%s\"\n",
                          dictionary->reference[index].index,
                          roadmap_dictionary_get_string (dictionary, index));

         break;

      case ROADMAP_DICTIONARY_TREE:

         fprintf (stdout, "\n");
         roadmap_dictionary_print_subtree
            (dictionary, dictionary->reference[index].index);

         break;

      default:
         fprintf (stdout, "\n");
         roadmap_log (ROADMAP_FATAL,
                      "corrupted node type %d",
                      dictionary->reference[index].type & 0x0f);
      }
   }
}


static int roadmap_dictionary_get_reference
              (struct dictionary_volume *dictionary,
               struct roadmap_dictionary_tree *tree, char c) {

   int index;
   int end = tree->first + tree->count;
   struct roadmap_dictionary_reference *reference = dictionary->reference;

   c = tolower(c);

   for (index = tree->first; index < end; index++) {

      if (reference[index].character == c) {
         return index;
      }
   }

   return -1;
}


static int
roadmap_dictionary_search
   (struct dictionary_volume *dictionary, const char *string, int length) {

   int   i;
   int   index;
   char  character;
   const char *cursor;
   char *stored;
   struct roadmap_dictionary_tree *tree;


   tree = dictionary->tree;

   for (i= 0, cursor = string; i <= length; cursor++, i++) {

        if (tree->position > i) continue;

        if (i < length) {
           character = *cursor;
        } else {
           character = 0;
        }
        index = roadmap_dictionary_get_reference (dictionary, tree, character);

        if (index < 0) {
           return 0;
        }

        switch (dictionary->reference[index].type) {

        case ROADMAP_DICTIONARY_STRING:

           stored = roadmap_dictionary_get_string (dictionary, index);

           if ((stored[length] != 0) ||
               (strncasecmp (string, stored, length) != 0)) {
              return -1;
           }
           return dictionary->reference[index].index;

        case ROADMAP_DICTIONARY_TREE:

           if (*cursor == 0) {
              roadmap_log (ROADMAP_FATAL,
                           "found a subtree after end of string");
           }

           tree = dictionary->tree + dictionary->reference[index].index;
           break;

        default:
           roadmap_log (ROADMAP_FATAL,
                        "corrupted node when searching for %s", string);
        }
   }

   return -1;
}


static struct dictionary_volume *
         roadmap_dictionary_initialize_one (roadmap_db *child) {

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

   table = roadmap_db_get_subsection (child, "node");

   dictionary->reference =
      (struct roadmap_dictionary_reference *) roadmap_db_get_data (table);

   table = roadmap_db_get_subsection (child, "index");

   dictionary->string_index = (unsigned int *) roadmap_db_get_data (table);
   dictionary->string_count = roadmap_db_get_count (table);

   return dictionary;
}


static void *roadmap_dictionary_map (roadmap_db *parent) {

   roadmap_db *child;
   struct dictionary_volume *volume;
   struct dictionary_volume *first = NULL;


   for (child = roadmap_db_get_first(parent);
        child != NULL;
        child = roadmap_db_get_next(child)) {

       volume = roadmap_dictionary_initialize_one (child);
       volume->next = first;

       first = volume;
   }

   return first;
}

static void roadmap_dictionary_activate (void *context) {

   DictionaryVolume = (struct dictionary_volume *) context;
}

static void roadmap_dictionary_unmap (void *context) {

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

roadmap_db_handler RoadMapDictionaryHandler = {
   "dictionary",
   roadmap_dictionary_map,
   roadmap_dictionary_activate,
   roadmap_dictionary_unmap
};


RoadMapDictionary roadmap_dictionary_open (char *name) {

   struct dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      if (strcmp (volume->name, name) == 0) {
         return volume;
      }
   }

   return NULL;
}


char *roadmap_dictionary_get (RoadMapDictionary d, RoadMapString index) {

   if (index >= d->string_count) {
      return NULL;
   }

   return d->data + d->string_index[index];
}


int  roadmap_dictionary_count (RoadMapDictionary d) {

   return d->string_count;
}


RoadMapDictionaryCursor roadmap_dictionary_new_cursor (RoadMapDictionary d) {

   RoadMapDictionaryCursor cursor = malloc (sizeof(struct dictionary_cursor));

   if (cursor != NULL) {
      cursor->volume = d;
      cursor->tree_index = 0;
      cursor->complete   = 0;
      cursor->completion = 0;
   }

   return cursor;
}


int roadmap_dictionary_move_cursor (RoadMapDictionaryCursor c, char input) {

   int end;
   int index;
   struct roadmap_dictionary_tree *tree;
   struct roadmap_dictionary_reference *reference;


   if (c->complete) {
      return 0;
   }

   tree = c->volume->tree + c->tree_index;
   reference = c->volume->reference;
   end = tree->first + tree->count;

   input = tolower(input);

   for (index = tree->first; index < end; index++) {

      if (reference[index].character == input) {

         switch (reference[index].type & 0x0f) {

         case ROADMAP_DICTIONARY_STRING:

            c->complete = 1;
            c->completion = index;

            break;

         case ROADMAP_DICTIONARY_TREE:

            c->complete = 0;
            c->tree_index = reference[index].index;

            break;

         default:

            roadmap_log (ROADMAP_FATAL,
                         "corrupted node type %d",
                         reference[index].type & 0x0f);
         }

         return 1;
      }
   }

   return 0;
}


int  roadmap_dictionary_completable (RoadMapDictionaryCursor c) {

   int end;
   int index;
   struct roadmap_dictionary_tree *tree;
   struct roadmap_dictionary_reference *reference;

   if (c->complete) {
      return 1;
   }

   tree = c->volume->tree + c->tree_index;
   end = tree->first + tree->count;
   reference = c->volume->reference;

   if (tree->position == 0) {
      return 0;
   }

   for (index = tree->first; index < end; index++) {

      if (reference[index].character == 0) {
         c->completion = index;
         return 1;
      }
   }

   return 0;
}


void roadmap_dictionary_completion (RoadMapDictionaryCursor c, char *data) {

   int index;
   struct roadmap_dictionary_tree *tree;
   struct roadmap_dictionary_reference *reference;

   tree = c->volume->tree;

   if (c->completion <= 0) {

      reference = c->volume->reference;

      /* Let run tree into the subtree, looking for any of the possible
       * solutions: by truncating the string at the current tree position,
       * we will get our string completion string.
       */

      for (index = tree[c->tree_index].first;
           (reference[index].type & 0x0f) == ROADMAP_DICTIONARY_TREE;
           index = tree[reference[index].index].first) ;

      if ((reference[index].type & 0x0f) != ROADMAP_DICTIONARY_STRING) {
         roadmap_log (ROADMAP_FATAL,
                      "corrupted node type %d",
                      reference[index].type & 0x0f);
      }

      c->completion = index;
   }

   strcpy (data, roadmap_dictionary_get_string (c->volume, c->completion));
   if (!c->complete) {
      data[tree[c->tree_index].position] = 0;
   }
}


int roadmap_dictionary_get_result (RoadMapDictionaryCursor c) {

   if (c->completion <= 0) {
      return 0;
   }

   return c->volume->reference[c->completion].index;
}


void roadmap_dictionary_get_next (RoadMapDictionaryCursor c, char *set) {

   int end;
   int index;
   struct roadmap_dictionary_tree *tree;
   struct roadmap_dictionary_reference *reference;

   if (! c->complete) {

      tree = c->volume->tree + c->tree_index;
      end  = tree->first + tree->count;

      reference = c->volume->reference;

      for (index = tree->first; index < end; index++) {

         if (reference[index].character != 0) {
            *(set++) = reference[index].character;
         }
      }
   }

   *set = 0;
}


void roadmap_dictionary_free_cursor (RoadMapDictionaryCursor c) {
   free (c);
}


RoadMapString roadmap_dictionary_locate (RoadMapDictionary d,
                                         const char *string) {

   int result;

   result = roadmap_dictionary_search (d, string, strlen(string));

   if (result < 0) {
      return 0;
   }

   return (RoadMapString)result;
}


void roadmap_dictionary_dump (void) {

   struct dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      printf ("Dictionary %s:\n", volume->name);
      roadmap_dictionary_print_subtree (volume, 0);
   }
}


void roadmap_dictionary_dump_volume (char *name) {

   struct dictionary_volume *volume;

   for (volume = DictionaryVolume; volume != NULL; volume = volume->next) {

      if (strcmp (volume->name, name) == 0) {
         printf ("Dictionary %s:\n", volume->name);
         roadmap_dictionary_print_subtree (volume, 0);
      }
   }
}

