/* roadmap_win32.c - Windows specific utility functions.
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_win32.c
 */

#include <windows.h>
#include "roadmap_win32.h"


LPWSTR ConvertToWideChar(LPCSTR string, UINT nCodePage)
{
	int len = MultiByteToWideChar(nCodePage, 0, string, -1, 0, 0);

	if (len < 0) {
		return _T("");
	} else {
		LPWSTR result = malloc(len * sizeof(WCHAR) + 2);
		MultiByteToWideChar(nCodePage, 0, string, -1, result, len);
		return result;
	}
}


char* ConvertToMultiByte(const LPWSTR s, UINT nCodePage)
{
   int len;
	int nSize = wcslen(s);

	len = WideCharToMultiByte(nCodePage, 0, s, nSize+1, 0, 0, NULL, NULL);

  	if (len < 0) {
	   return "";
   } else {
      char *pAnsiString = malloc(len+1);

   	WideCharToMultiByte(nCodePage, 0, s, nSize+1, pAnsiString, len+1,
					NULL, NULL);
      return pAnsiString;
   }
}

