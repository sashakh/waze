/* editor_dictionary.h - database layer
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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

#ifndef INCLUDE__EDITOR_DICTIONARY__H
#define INCLUDE__EDITOR_DICTIONARY__H

#define DICTIONARY_INDEX_SIZE 0x10000
#define DICTIONARY_DATA_SIZE 0x10000

#define ROADMAP_DICTIONARY_NULL   0
#define ROADMAP_DICTIONARY_TREE   1
#define ROADMAP_DICTIONARY_STRING 2

struct ed_dictionary_reference {

   char character;
   char type;

   int child;

   int next;
};


struct ed_dictionary_tree {

   int first;
   unsigned short count;
   unsigned short position;
};

struct editor_db_section_s;

struct ed_dictionary_volume {

   char *name;
 
   struct ed_dictionary_volume *next;

   struct editor_db_section_s *reference_db_section;
   struct editor_db_section_s *tree_db_section;
   struct editor_db_section_s *data_db_section;

   int hits;
};

typedef int EditorString;
typedef struct ed_dictionary_volume *EditorDictionary;

int editor_dictionary_add
                 (struct ed_dictionary_volume *dictionary,
                  const char *string, int length);

char *editor_dictionary_get
         (struct ed_dictionary_volume *dictionary, int index);

int editor_dictionary_locate
          (struct ed_dictionary_volume *dictionary, const char *string);

EditorDictionary editor_dictionary_open (char *name);

extern roadmap_db_handler EditorDictionaryHandler;

#endif // INCLUDE__EDITOR_DICTIONARY__H

