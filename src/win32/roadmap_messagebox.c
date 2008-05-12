/* roadmap_messagebox.c - message box implementation
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright 2008 Danny Backx
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
 *   See roadmap_messagebox.h
 */

#include <windows.h>
#include "../roadmap.h"
#include "../roadmap_start.h"
#define __ROADMAP_MESSAGEBOX_NO_LANG
#include "../roadmap_messagebox.h"
#include "resource.h"

static HWND	mb;
extern HINSTANCE g_hInst;
extern HWND	RoadMapMainWindow;

static BOOL roadmap_messagebox_proc (HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch (m) {
		case WM_COMMAND:
			switch (LOWORD(w)) {
				case IDOK:
				case IDCANCEL:
					DestroyWindow(h);
					mb = 0;
					return TRUE;
			}
			break;
	}
	return FALSE;
}


static void roadmap_messagebox_show (const char *title,
                                     const char *text, int options)
{
	LPWSTR u_title, u_text;
	if (mb == 0)
		mb = CreateDialog(g_hInst, ROADMAP_MESSAGE_DIALOG,
			RoadMapMainWindow, roadmap_messagebox_proc);

	u_title = ConvertToUNICODE(title);
	u_text = ConvertToUNICODE(text);
	SetDlgItemText(mb, ROADMAP_DIALOG_TEXT, u_text);
	SetWindowText(mb, u_title);
	free(u_title);
	free(u_text);

	ShowWindow(mb, SW_SHOW);
}

void *roadmap_messagebox (const char *title, const char *text)
{
   roadmap_messagebox_show (title, text, 0);
   return mb;	/* Nothing to return, the box is already popped down */
}

void *roadmap_messagebox_wait (const char *title, const char *text)
{
   roadmap_messagebox_show (title, text, MB_ICONERROR);
   return mb;	/* Nothing to return, the box is already popped down */
}

void roadmap_messagebox_hide (void *handle)
{
	/* This is a no op if we're using MessageBox() */
	if (mb)
		DestroyWindow(mb);
	mb = 0;
}
