/* roadmap_library.c - a low level module to manage plugins for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright (c) 2008 Danny Backx - added an actual implementation.
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
 *   See roadmap_library.h
 */

#include <windows.h>

#include "../roadmap.h"
#include "../roadmap_library.h"
#include "../roadmap_file.h"

struct roadmap_library_context {
	const char	*name;
	HINSTANCE	handle;
};

/*
 * HINSTANCE dll;
 * dll = LoadLibrary(TEXT("coredll.dll"));
 * *(FARPROC*)&_ReleasePowerRequirement = GetProcAddress(dll, TEXT("ReleasePowerRequirement"));
 */
RoadMapLibrary roadmap_library_load (const char *pathname)
{
	HINSTANCE	h;
	RoadMapLibrary	library;
	wchar_t		*wcs;

	wcs = ConvertToUNICODE(pathname);
	h = LoadLibrary(wcs);
	free(wcs);
	if (h == NULL)
		return NULL;

	library = (RoadMapLibrary) malloc (sizeof(struct roadmap_library_context));
	roadmap_check_allocated(library);

	library->handle = h;
	library->name = strdup(pathname);

	return library;
}


int roadmap_library_exists (const char *pathname)
{
	return 0;
}


void *roadmap_library_symbol (RoadMapLibrary library, const char *symbol)
{
	void	*r = NULL;
	wchar_t		*wcs;

	if (library == NULL || library->handle == NULL)
		return NULL;

	wcs = ConvertToUNICODE(symbol);
	r = GetProcAddress(library->handle, wcs);
	free(wcs);

	return r;
}
