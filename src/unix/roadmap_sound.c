/*
 * LICENSE:
 *
 *   Copyright (c) 2008, Danny Backx.
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

/**
 * @file
 * @brief Play sound on unix.
 */

#include "../roadmap.h"
#include "../roadmap_path.h"
#include "../roadmap_file.h"
#include "../roadmap_res.h"
#include "../roadmap_sound.h"

#define MAX_LISTS 2

#ifdef NEEDED_LATER

static RoadMapSoundList sound_lists[MAX_LISTS];

static int save_wav_file (void *data, unsigned int size);

#endif

int roadmap_sound_play (RoadMapSound sound)
{
	   return -1;
}


int roadmap_sound_play_file (const char *file_name)
{
   return -1;
}


RoadMapSound roadmap_sound_load (const char *path, const char *file, int *mem)
{
   return NULL;
}


int roadmap_sound_free (RoadMapSound sound)
{
   return 0;
}


RoadMapSoundList roadmap_sound_list_create (int flags)
{
   return NULL;
}


int roadmap_sound_list_add (RoadMapSoundList list, const char *name)
{
   return -1;
}


int roadmap_sound_list_count (const RoadMapSoundList list)
{
   return -1;
}

const char *roadmap_sound_list_get (const RoadMapSoundList list, int i)
{
   return NULL;
}

void roadmap_sound_list_free (RoadMapSoundList list)
{
}

int roadmap_sound_play_list (const RoadMapSoundList list)
{
   return 0;
}

void roadmap_sound_initialize (void)
{
}

void roadmap_sound_shutdown (void)
{
}

#ifdef NEEDED_LATER
/* Recording */
static int allocate_rec_buffer(int seconds)
{
   return 0;
}

static int save_wav_file (void *data, unsigned int size)
{
   return 0;
}
#endif

int roadmap_sound_record (const char *file_name, int seconds)
{
   return 0;
}
