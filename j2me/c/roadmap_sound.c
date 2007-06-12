/* roadmap_sound.c - Play sound.
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "sound_mgr.h"
#include "roadmap.h"
#include "roadmap_sound.h"

static NOPH_SoundMgr_t SM;

int roadmap_sound_play (RoadMapSound sound) { assert(0); return 0; }

int roadmap_sound_play_file (const char *file_name) { assert(0); return 0; }

RoadMapSound roadmap_sound_load (const char *path, const char *file, int *mem) {
   assert(0);
   return NULL;
}

int roadmap_sound_free (RoadMapSound sound) {
   assert(0);
   return 0;
}

RoadMapSoundList roadmap_sound_list_create (int flags) {

   return NOPH_SoundMgr_listCreate(SM, flags);
}


int roadmap_sound_list_add (RoadMapSoundList list, const char *name) {

   return NOPH_SoundMgr_listAdd(SM, list, name);
}


int roadmap_sound_list_count (const RoadMapSoundList list) {

   return NOPH_SoundMgr_listCount(SM, list);
}


void roadmap_sound_list_free (RoadMapSoundList list) {

   NOPH_SoundMgr_listFree(SM, list);
}


int roadmap_sound_play_list (const RoadMapSoundList list) {
   NOPH_SoundMgr_playList(SM, list);

   return 0;
}


void roadmap_sound_initialize (void) {
   SM = NOPH_SoundMgr_getInstance();
}


void roadmap_sound_shutdown (void) {
}


int roadmap_sound_record (const char *file_name, int seconds) {
   assert(0);
   return 0;
}

