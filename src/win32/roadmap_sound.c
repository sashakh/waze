/* roadmap_sound.c - Play sound.
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
 *   See roadmap_sound.h
 */

#include <windows.h>

#include "../roadmap.h"
#include "../roadmap_path.h"
#include "../roadmap_sound.h"

#define MAX_LISTS 2

static RoadMapSoundList sound_lists[MAX_LISTS];
static CRITICAL_SECTION SoundCriticalSection;
static HANDLE           SoundEvent;
static HANDLE           sound_thread;
static int              current_list = -1;

DWORD WINAPI SoundThread (LPVOID lpParam) {

   SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL);

   while (1) {
      int i;

      WaitForSingleObject(SoundEvent, INFINITE);

      while (1) {
         RoadMapSoundList list;

         EnterCriticalSection (&SoundCriticalSection);

         current_list = (current_list + 1) % MAX_LISTS;

         if (sound_lists[current_list] == NULL) {

            /* nothing to play */
            current_list = -1;
         }

         LeaveCriticalSection (&SoundCriticalSection);

         if (current_list == -1) break;

         list = sound_lists[current_list];

         for (i=0; i<roadmap_sound_list_count (list); i++) {

            roadmap_sound_play (roadmap_sound_list_get (list, i));
         }

         roadmap_sound_list_free (list);
         sound_lists[current_list] = NULL;
      }

   }
}


int roadmap_sound_play (const char *file_name) {

   char full_name[256];
   LPWSTR file_name_unicode;
   BOOL res;

   snprintf (full_name, sizeof(full_name), "%s\\sound\\%s",
             roadmap_path_user (), file_name);

   file_name_unicode = ConvertToWideChar(full_name, CP_UTF8);

   res = PlaySound(file_name_unicode, NULL, SND_SYNC | SND_FILENAME);

   free(file_name_unicode);

   if (res == TRUE) return 0;
   else return -1;
}


RoadMapSoundList roadmap_sound_list_create (void) {

   return (RoadMapSoundList) calloc (1, sizeof(struct roadmap_sound_list_t));
}


int roadmap_sound_list_add (RoadMapSoundList list, const char *name) {

   if (list->count == MAX_SOUND_LIST) return -1;

   strncpy (list->list[list->count], name, sizeof(list->list[0]));
   list->list[list->count][sizeof(list->list[0])-1] = '\0';
   list->count++;

   return list->count - 1;
}


int roadmap_sound_list_count (const RoadMapSoundList list) {

   return list->count;
}


const char *roadmap_sound_list_get (const RoadMapSoundList list, int i) {

   if (i >= MAX_SOUND_LIST) return NULL;

   return list->list[i];
}


void roadmap_sound_list_free (RoadMapSoundList list) {

   free(list);
}


int roadmap_sound_play_list (const RoadMapSoundList list) {

   EnterCriticalSection (&SoundCriticalSection);

   if (current_list == -1) {
      /* not playing */

      sound_lists[0] = list;
      SetEvent(SoundEvent);

   } else {

      int next = (current_list + 1) % MAX_LISTS;

      if (sound_lists[next] != NULL) {
         roadmap_sound_list_free (sound_lists[next]);
      }

      sound_lists[next] = list;
   }

   LeaveCriticalSection (&SoundCriticalSection);

   return 0;
}


void roadmap_sound_initialize (void) {

   SoundEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
   InitializeCriticalSection (&SoundCriticalSection);

	sound_thread = CreateThread(NULL, 0, SoundThread, NULL, 0, NULL);
}


void roadmap_sound_shutdown (void) {

   CloseHandle (SoundEvent);
   DeleteCriticalSection (&SoundCriticalSection);
}


