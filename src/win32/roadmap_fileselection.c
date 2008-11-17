/*
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

/**
 * @file
 * @brief win32/roadmap_fileselection.c - manage the Widget used in roadmap dialogs.
 */

#include <windows.h>
#include <commdlg.h>

#include "../roadmap.h"
#include "../roadmap_types.h"
#include "../roadmap_file.h"
#include "../roadmap_fileselection.h"


/**
 * @brief pop up a dialog asking the user which file to load/save
 * @param title the title of this dialog
 * @param filter optional filter string, defaults to *.*
 * @param path not used
 * @param mode should contain 'r' for reading a file
 * @param callback this function is called (on success) with the filename and mode as arguments
 */
void roadmap_fileselection_new (const char *title,
				const char *filter,
				const char *path,
				const char *mode,
				RoadMapFileCallback callback)
{
	WCHAR filename[MAX_PATH] = {0};
	WCHAR strFilter[MAX_PATH] = {0};
	LPWSTR title_unicode = ConvertToUNICODE(title);
	BOOL res;

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename) / sizeof(filename[0]);
	ofn.Flags = OFN_EXPLORER;
	ofn.lpstrTitle = title_unicode;
	if (filter != NULL) {
		/*
		 * Need to get the string converted into WCS and then
		 * stored twice with a couple of NULLs in between and at the end.
		 */
		LPWSTR fltr = ConvertToUNICODE(filter);
		int l = wcslen(fltr);
		wcscpy(strFilter, fltr);
		strFilter[l] = L'\0';	/* one more zero */
		wcscpy(strFilter+l+1, fltr);
		strFilter[2*l+1] = L'\0';	/* one more zero */
		strFilter[2*l+2] = L'\0';	/* one more zero */
		free(fltr);
		ofn.lpstrFilter = strFilter;
	} else {
		ofn.lpstrFilter = TEXT("*.*\0*.*\0");
	}

	if (strchr(mode, 'r') != NULL) {
		ofn.Flags |= OFN_FILEMUSTEXIST;
		res = GetOpenFileName(&ofn);

	} else {
		ofn.Flags |= OFN_OVERWRITEPROMPT;
		res = GetSaveFileName(&ofn);
	}

	free((char*)ofn.lpstrTitle);

	if (res) {
		char *name = ConvertToANSI(filename, CP_UTF8);
		(*callback)(name, mode);
		free(name);
	}
}
