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
#include "../roadmap_file.h"
#include "../roadmap_res.h"
#include "../roadmap_sound.h"

#define MAX_LISTS 2

static RoadMapSoundList sound_lists[MAX_LISTS];
static CRITICAL_SECTION SoundCriticalSection;
static HANDLE           SoundEvent;
static HANDLE           SoundRecEvent;
static HANDLE           sound_thread;
static HANDLE           sound_rec_thread;
static int              current_list = -1;

/* Recording stuff */
static WAVEHDR          WaveHeader;
static WAVEFORMATEX     PCMfmt;
static HWAVEIN          hWaveIn;
static char            *RoadMapSoundRecName;

struct WAVE_FORMAT
{
   WORD  wFormatTag;
   WORD  wChannels;
   DWORD dwSamplesPerSec;
   DWORD dwAvgBytesPerSec;
   WORD  wBlockAlign;
   WORD  wBitsPerSample;
};

struct RIFF_HEADER
{
   CHAR szRiffID[4];      // 'R','I','F','F'
   DWORD dwRiffSize;
   CHAR szRiffFormat[4];  // 'W','A','V','E'
};

struct FMT_BLOCK
{
   CHAR    szFmtID[4]; // 'f','m','t',' '
   DWORD    dwFmtSize;
   struct WAVE_FORMAT wavFormat;
};

struct DATA_BLOCK
{
   CHAR szDataID[4];   // 'd','a','t','a'
   DWORD dwDataSize;
};

static int save_wav_file (void *data, unsigned int size);

DWORD WINAPI SoundThread (LPVOID lpParam) {

   SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL);

   while (1) {
      int i;

      if (WaitForSingleObject(SoundEvent, INFINITE) == WAIT_FAILED) {
         return 0;
      }

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

            const char *name = roadmap_sound_list_get (list, i);
            RoadMapSound sound =
                           roadmap_res_get (RES_SOUND, RES_NOCREATE, name);
            if (sound) {
               roadmap_sound_play (sound);
            } else {
               roadmap_sound_play_file (name);
            }
         }

         if (!(list->flags & SOUND_LIST_NO_FREE)) {
            roadmap_sound_list_free (list);
         }
         sound_lists[current_list] = NULL;
      }

   }
}


DWORD WINAPI SoundRecThread (LPVOID lpParam) {

   while (1) {

      DWORD res;

      if (WaitForSingleObject(SoundRecEvent, INFINITE) == WAIT_FAILED) {
         return 0;
      }

      res = waveInUnprepareHeader(hWaveIn, &WaveHeader, sizeof(WAVEHDR));

      res = save_wav_file (WaveHeader.lpData, WaveHeader.dwBytesRecorded);

      free(WaveHeader.lpData);
      WaveHeader.lpData = NULL;

      res = waveInReset(hWaveIn);
      res = waveInClose(hWaveIn);
      hWaveIn = NULL;
      roadmap_sound_play_file ("rec_end.wav");
      res = res;
   }
}


int roadmap_sound_play (RoadMapSound sound) {

   BOOL res;
   void *mem;

   if (!sound) return -1;
   mem = roadmap_file_base ((RoadMapFileContext)sound);

   res = PlaySound((LPWSTR)mem, NULL, SND_SYNC | SND_MEMORY);

   if (res == TRUE) return 0;
   else return -1;
}


int roadmap_sound_play_file (const char *file_name) {

   char full_name[256];
   LPWSTR file_name_unicode;
   BOOL res;

   if (roadmap_path_is_full_path (file_name)) {
      file_name_unicode = ConvertToWideChar(file_name, CP_UTF8);
   } else {

      snprintf (full_name, sizeof(full_name), "%s\\sound\\%s",
                roadmap_path_user (), file_name);

      file_name_unicode = ConvertToWideChar(full_name, CP_UTF8);
   }

   res = PlaySound(file_name_unicode, NULL, SND_SYNC | SND_FILENAME);

   free(file_name_unicode);

   if (res == TRUE) return 0;
   else return -1;
}


RoadMapSound roadmap_sound_load (const char *path, const char *file, int *mem) {

   char *full_name = roadmap_path_join (path, file);
   const char *seq;
   RoadMapFileContext sound;

   seq = roadmap_file_map (NULL, full_name, NULL, "r", &sound);

   roadmap_path_free (full_name);

   if (seq == NULL) {
      *mem = 0;
      return NULL;
   }

   *mem = roadmap_file_size (sound);

   return (RoadMapSound) sound;
}


int roadmap_sound_free (RoadMapSound sound) {

   roadmap_file_unmap (&(RoadMapFileContext)sound);

   return 0;
}


RoadMapSoundList roadmap_sound_list_create (int flags) {

   RoadMapSoundList list =
         (RoadMapSoundList) calloc (1, sizeof(struct roadmap_sound_list_t));

   list->flags = flags;

   return list;
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
		  if (!(sound_lists[next]->flags & SOUND_LIST_NO_FREE)) {
			  roadmap_sound_list_free (sound_lists[next]);
		  }
      }

      sound_lists[next] = list;
   }

   LeaveCriticalSection (&SoundCriticalSection);

   return 0;
}


void roadmap_sound_initialize (void) {

   SoundEvent    = CreateEvent (NULL, FALSE, FALSE, NULL);
   SoundRecEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
   InitializeCriticalSection (&SoundCriticalSection);

   sound_thread = CreateThread(NULL, 0, SoundThread, NULL, 0, NULL);

   /* Recording */

   PCMfmt.cbSize=0;
   PCMfmt.wFormatTag=WAVE_FORMAT_PCM;
   PCMfmt.nChannels=1;
   PCMfmt.nSamplesPerSec=11025;
   PCMfmt.wBitsPerSample=8;
   PCMfmt.nBlockAlign=1;
   PCMfmt.nAvgBytesPerSec=11025;
}


void roadmap_sound_shutdown (void) {

   DWORD res;

   CloseHandle (SoundEvent);
   if (SoundRecEvent) CloseHandle (SoundRecEvent);
   DeleteCriticalSection (&SoundCriticalSection);

   if (hWaveIn != NULL) {
      res = waveInReset(hWaveIn);
      res = waveInClose(hWaveIn);
      hWaveIn = NULL;
   }

}


/* Recording */
static int allocate_rec_buffer(int seconds) {
   
   int length = PCMfmt.nSamplesPerSec*PCMfmt.nChannels*
                PCMfmt.wBitsPerSample*seconds/8;

   void *buffer = malloc(length);

   if (!buffer) return -1;

   WaveHeader.lpData=buffer;
   WaveHeader.dwBufferLength=length;
   WaveHeader.dwBytesRecorded=0;
   WaveHeader.dwUser=0;
   WaveHeader.dwFlags=0;
   WaveHeader.reserved=0;
   WaveHeader.lpNext=0;

   return 0;
}


static int save_wav_file (void *data, unsigned int size) {
   struct RIFF_HEADER rh = { {'R', 'I', 'F', 'F'},
                             size - 8,
                             {'W', 'A', 'V', 'E'}};

   struct FMT_BLOCK fb = { {'f', 'm', 't', ' '}, sizeof(struct WAVE_FORMAT), {0}};
   struct DATA_BLOCK db = { {'d', 'a', 't', 'a'}, size};

   FILE *file;

   fb.wavFormat.wFormatTag       = PCMfmt.wFormatTag;
   fb.wavFormat.wChannels        = PCMfmt.nChannels;
   fb.wavFormat.dwSamplesPerSec  = PCMfmt.nSamplesPerSec;
   fb.wavFormat.dwAvgBytesPerSec = PCMfmt.nAvgBytesPerSec;
   fb.wavFormat.wBlockAlign      = PCMfmt.nBlockAlign;
   fb.wavFormat.wBitsPerSample   = PCMfmt.wBitsPerSample;

   file = roadmap_file_fopen (NULL, RoadMapSoundRecName, "w");
   if (!file) return -1;

   {
      int test = sizeof(rh) + sizeof(fb) + sizeof(db);
      test = test;
   }
   fwrite(&rh, sizeof(rh), 1, file);
   fwrite(&fb, sizeof(fb), 1, file);
   fwrite(&db, sizeof(db), 1, file);

   fwrite(data, size, 1, file);

   fclose (file);

   return 0;
}


int roadmap_sound_record (const char *file_name, int seconds) {

   DWORD res;

   if (hWaveIn != NULL) return -1;

   if (sound_rec_thread == NULL) {
      sound_rec_thread = CreateThread(NULL, 0, SoundRecThread, NULL, 0, NULL);
   }
   
   if (RoadMapSoundRecName != NULL) {
      free(RoadMapSoundRecName);
   }
   RoadMapSoundRecName = strdup(file_name);

   res = waveInOpen(&hWaveIn, (UINT) WAVE_MAPPER, &PCMfmt,
         (DWORD) SoundRecEvent, (DWORD) 0, CALLBACK_EVENT);   

   res = allocate_rec_buffer (seconds);

   res = waveInPrepareHeader(hWaveIn, &WaveHeader, sizeof(WAVEHDR));

   res = waveInAddBuffer(hWaveIn, &WaveHeader, sizeof(WAVEHDR));

   roadmap_sound_play_file ("rec_start.wav");

   res = waveInStart(hWaveIn);

   return 0;
}

