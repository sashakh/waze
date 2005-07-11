/* roadmap_file.c - a module to open/read/close a roadmap database file.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *
 *   Based on an implementation by Pascal F. Martin.
 *   This file is based part of RoadMap.
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
 *   See roadmap_file.h.
 */

#include <stdio.h>
#include "../roadmap.h"
#include "../roadmap_path.h"
#include "../roadmap_file.h"


struct RoadMapFileContextStructure {

	HANDLE  hFile;
	void *base;
	int   size;
};


FILE *roadmap_file_fopen (const char *path, const char *name, const char *mode) {
	int   silent;
	FILE *file;
	const char *full_name = roadmap_path_join (path, name);

	if (mode[0] == 's') {
		/* This special mode is a "lenient" read: do not complain
		* if the file does not exist.
		*/
		silent = 1;
		++mode;
	} else {
		silent = 0;
	}

	file = fopen (full_name, mode);

	if ((file == NULL) && (! silent)) {
		roadmap_log (ROADMAP_ERROR, "cannot open file %s", full_name);
	}

	roadmap_path_free (full_name);
	return file;
}


void roadmap_file_remove (const char *path, const char *name)
{
	const char *full_name = roadmap_path_join (path, name);

#ifdef _UNICODE
	{
		LPWSTR full_name_unicode = ConvertToUNICODE(full_name);
		DeleteFile(full_name_unicode);
		free(full_name_unicode);
	}
#else
	DeleteFile(full_name);
#endif
	roadmap_path_free (full_name);
}


int roadmap_file_exists (const char *path, const char *name)
{
	HANDLE  file;
	const char *full_name = roadmap_path_join (path, name);

	LPWSTR full_name_unicode = ConvertToUNICODE(full_name);

	file = CreateFile (full_name_unicode,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	roadmap_path_free (full_name);
	free(full_name_unicode);

	if (file != INVALID_HANDLE_VALUE) {
		CloseHandle(file);
		return 1;
	} else {
		return 0;
	}
}


int roadmap_file_length (const char *path, const char *name)
{
	HANDLE  file;
	const char *full_name = roadmap_path_join (path, name);

	LPWSTR full_name_unicode = ConvertToUNICODE(full_name);

	file = CreateFile (full_name_unicode,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	roadmap_path_free (full_name);
	free(full_name_unicode);

	if (file != INVALID_HANDLE_VALUE) {
		DWORD file_size = GetFileSize(file, NULL);
		CloseHandle(file);
		if (file_size != INVALID_FILE_SIZE) {
			return file_size;
		}
	}

	return -1;
}


void roadmap_file_save (const char *path, const char *name, void *data,
						int length)
{
	HANDLE  file;
	const char *full_name = roadmap_path_join (path, name);

	LPWSTR full_name_unicode = ConvertToUNICODE(full_name);

	file = CreateFile (full_name_unicode,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL);

	roadmap_path_free (full_name);
	free(full_name_unicode);

	if (file != INVALID_HANDLE_VALUE) {
		DWORD res;
		WriteFile(file, data, length, &res, NULL);
		CloseHandle(file);
	}
}


void roadmap_file_append (const char *path, const char *name,
						  void *data, int length)
{
	HANDLE  file;
	const char *full_name = roadmap_path_join (path, name);

	LPWSTR full_name_unicode = ConvertToUNICODE(full_name);

	file = CreateFile (full_name_unicode,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		0,
		NULL);

	SetFilePointer (file, 0, NULL, FILE_END);
	roadmap_path_free (full_name);
	free(full_name_unicode);

	if (file != INVALID_HANDLE_VALUE) {
		DWORD res;
		WriteFile(file, data, length, &res, NULL);
		CloseHandle(file);
	}
}


const char *roadmap_file_unique (const char *base)
{
	static int   UniqueNameCounter = 0;
	static char *UniqueNameBuffer = NULL;
	static int   UniqueNameBufferLength = 0;
	FILETIME ft[4];

	int length;

	length = strlen(base + 16);

	if (length > UniqueNameBufferLength) {

		if (UniqueNameBuffer != NULL) {
			free(UniqueNameBuffer);
		}
		UniqueNameBuffer = malloc (length);

		roadmap_check_allocated(UniqueNameBuffer);

		UniqueNameBufferLength = length;
	}

	GetThreadTimes(GetCurrentThread(), &ft[0], &ft[1], &ft[2], &ft[3]);

	sprintf (UniqueNameBuffer,
		"%s%d_%d", base, ft[0].dwLowDateTime%10000, UniqueNameCounter);

	UniqueNameCounter += 1;

	return UniqueNameBuffer;
}


const char *roadmap_file_map (const char *set,
							  const char *name,
							  const char *sequence,
							  RoadMapFileContext *file)
{
	RoadMapFileContext context;
	DWORD file_size;

	context = malloc (sizeof(*context));
	roadmap_check_allocated(context);

	context->hFile = INVALID_HANDLE_VALUE;
	context->base = NULL;
	context->size = 0;

	if (name[0] == '\\') {
		LPWSTR name_unicode = ConvertToUNICODE(name);
		context->hFile = CreateFileForMapping(
			name_unicode,
			GENERIC_READ ,
			FILE_SHARE_READ, NULL, OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, NULL);
		free(name_unicode);
		sequence = ""; /* Whatever, but NULL. */

	} else {

		char *full_name;
		LPWSTR full_name_unicode;

		int full_name_size;
		int name_size = strlen(name);
		int size;


		if (sequence == NULL) {
			sequence = roadmap_path_first(set);
		} else {
			sequence = roadmap_path_next(set, sequence);
		}
		if (sequence == NULL) {
			return NULL;
		}

		full_name_size = 512;
		full_name = malloc (full_name_size);
		roadmap_check_allocated(full_name);

		do {
			size = strlen(sequence) + name_size + 2;

			if (size >= full_name_size) {
				full_name = realloc (full_name, size);
				roadmap_check_allocated(full_name);
				full_name_size = size;
			}

			strcpy (full_name, sequence);
			strcat (full_name, "\\");
			strcat (full_name, name);

			full_name_unicode = ConvertToUNICODE(full_name);
			context->hFile = CreateFileForMapping(
				full_name_unicode,
				GENERIC_READ ,
				FILE_SHARE_READ, NULL, OPEN_EXISTING, 
				FILE_ATTRIBUTE_NORMAL, NULL);
			free(full_name_unicode);

			if (context->hFile != INVALID_HANDLE_VALUE) break;

			sequence = roadmap_path_next(set, sequence);

		} while (sequence != NULL);

		free (full_name);
	}

	if (context->hFile == INVALID_HANDLE_VALUE ) {
		if (sequence == 0) {
			roadmap_log (ROADMAP_INFO, "cannot open file %s", name);
		}
		roadmap_file_unmap (&context);
		return NULL;
	}

	file_size = GetFileSize(context->hFile, NULL);

	if (file_size == INVALID_FILE_SIZE) {
		if (sequence == 0) {
			roadmap_log (ROADMAP_ERROR, "cannot stat file %s", name);
		}
		roadmap_file_unmap (&context);
		return NULL;
	}

	context->size = file_size;

	context->hFile = CreateFileMapping(
		context->hFile, 
		NULL,
		PAGE_READONLY,
		0,0,0);

	if (context->hFile == INVALID_HANDLE_VALUE) {
		if (sequence == 0) {
			roadmap_log (ROADMAP_INFO, "cannot open file %s", name);
		}
		roadmap_file_unmap (&context);
		return NULL;
	}

	context->base =
		MapViewOfFile(
		context->hFile,
		FILE_MAP_READ,
		0,0,0 );

	if (context->base == NULL) {
		roadmap_log (ROADMAP_ERROR, "cannot map file %s", name);
		roadmap_file_unmap (&context);
		return NULL;
	}

	*file = context;

	return sequence; /* Indicate the next directory in the path. */
}


void *roadmap_file_base (RoadMapFileContext file)
{
	if (file == NULL) {
		return NULL;
	}
	return file->base;
}


int roadmap_file_size (RoadMapFileContext file)
{
	if (file == NULL) {
		return 0;
	}
	return file->size;
}


void roadmap_file_unmap (RoadMapFileContext *file)
{
	RoadMapFileContext context = *file;

	if (context->base != NULL) {
		UnmapViewOfFile(context->base);
	}

	if (context->hFile != INVALID_HANDLE_VALUE ) {
		CloseHandle(context->hFile);
	}
	free(context);
	*file = NULL;
}


RoadMapFile roadmap_file_open(const char *name, const char *mode)
{
	HANDLE file = INVALID_HANDLE_VALUE;
	LPWSTR url_unicode;
	
	DWORD os_mode;
	
	if (strcmp(mode, "r") == 0) {
		os_mode = GENERIC_READ;
	} else if (strchr (mode, 'w') != NULL) {
		os_mode = GENERIC_READ | GENERIC_WRITE;
	} else {
		roadmap_log (ROADMAP_ERROR,
			"%s: invalid file access mode %s", name, mode);
		return INVALID_HANDLE_VALUE;
	}
	
	url_unicode = ConvertToUNICODE(name);
	
	file = CreateFile (url_unicode,
		os_mode,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	
	free(url_unicode);

	return file;
}


int roadmap_file_read(RoadMapFile file, void *data, int size)
{
	DWORD num_bytes;
	
	if (!ReadFile((HANDLE)file, data, size, &num_bytes, NULL)) {
		return -1;
	} else {
		return num_bytes;
	}
}


int roadmap_file_write(RoadMapFile file, const void *data, int length)
{
 	DWORD num_bytes;
	
	if (!WriteFile((HANDLE)file, data, length, &num_bytes, NULL)) {
		return -1;
	} else {
		return num_bytes;
	}
}


void  roadmap_file_close(RoadMapFile file)
{
   CloseHandle((HANDLE)file);
}

