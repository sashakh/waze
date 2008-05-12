/* roadmap_spawn.c - Process control interface for the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright 2008 Danny Backx
 *
 *   Based on an implementation by Pascal F. Martin.
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
 *   See roadmap_spawn.h
 */

#include <windows.h>
#include "../roadmap.h"
#include "../roadmap_list.h"
#include "../roadmap_spawn.h"


static char *RoadMapSpawnPath = NULL;

static RoadMapList RoadMapSpawnActive;


static int roadmap_spawn_child (const char *name, const char *command_line,
		RoadMapPipe pipes[2])
{
	PROCESS_INFORMATION pi;
	char full_name[MAX_PATH];
	LPWSTR full_path_unicode;
	LPWSTR command_line_unicode;

	memset(&pi, 0, sizeof(pi));

	if (*name != '\\') {
		snprintf(full_name, MAX_PATH, "%s\\%s", RoadMapSpawnPath, name);
		full_path_unicode = ConvertToUNICODE(full_name);
	} else {
		full_path_unicode = ConvertToUNICODE(name);
	}

	if (command_line != NULL) {
		command_line_unicode = ConvertToUNICODE(command_line);
	} else {
		command_line_unicode = NULL;
	}

	if (!CreateProcess(full_path_unicode, command_line_unicode, NULL,
				NULL, FALSE, 0, NULL, NULL, NULL, & pi)) {
			free(full_path_unicode);
			free(command_line_unicode);
			roadmap_log (ROADMAP_ERROR, "CreateProcess(\"%s\") failed",
					full_name);
			return -1;
	}

	free(full_path_unicode);

	if (command_line_unicode != NULL) {
		free(command_line_unicode);
	}

	CloseHandle(pi.hThread);

	if (pipes != NULL) {
		pipes[0] = ROADMAP_SPAWN_INVALID_PIPE;
		pipes[1] = ROADMAP_SPAWN_INVALID_PIPE;
	}

	return (int)pi.hProcess;
}


void roadmap_spawn_initialize (const char *argv0)
{
	WCHAR path_unicode[MAX_PATH];
	char *path;
	char *tmp;

	GetModuleFileName(NULL, path_unicode,
		sizeof(path_unicode)/sizeof(path_unicode[0]));
	path = ConvertToANSI(path_unicode, CP_ACP);
	tmp = strrchr (path, '\\');
	if (tmp != NULL) {
		*tmp = '\0';
	}
	RoadMapSpawnPath = path;
	ROADMAP_LIST_INIT(&RoadMapSpawnActive);
}


int roadmap_spawn (const char *name, const char *command_line)
{
	return roadmap_spawn_child (name, command_line, NULL);
}


int  roadmap_spawn_with_feedback (const char *name, const char *command_line,
		RoadMapFeedback *feedback)
{
	roadmap_list_append (&RoadMapSpawnActive, &feedback->link);
	feedback->child = roadmap_spawn_child (name, command_line, NULL);

	return feedback->child;
}


int  roadmap_spawn_with_pipe (const char *name, const char *command_line,
		RoadMapPipe pipes[2], RoadMapFeedback *feedback)
{
	roadmap_list_append (&RoadMapSpawnActive, &feedback->link);
	feedback->child = roadmap_spawn_child (name, command_line, pipes);

	return feedback->child;
}


void roadmap_spawn_check (void)
{
	RoadMapListItem *item, *tmp;
	RoadMapFeedback *feedback;

	ROADMAP_LIST_FOR_EACH (&RoadMapSpawnActive, item, tmp) {

		feedback = (RoadMapFeedback *)item;

		if (WaitForSingleObject((HANDLE)feedback->child, 0) == WAIT_OBJECT_0) {
			CloseHandle((HANDLE)feedback->child);
			roadmap_list_remove (&feedback->link);
			feedback->handler (feedback->data);
		}
	}
}


void roadmap_spawn_command (const char *command)
{
   roadmap_spawn(command, NULL);
}


int roadmap_spawn_write_pipe (RoadMapPipe pipe, const void *data, int length)
{
   return -1;
}


int roadmap_spawn_read_pipe (RoadMapPipe pipe, void *data, int size)
{
   return -1;
}


void roadmap_spawn_close_pipe (RoadMapPipe pipe)
{
} 
