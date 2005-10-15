/* wince_input_mon.c - monitor for inputs (serial / network / file).
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
 */

#include <windows.h>
#include "../roadmap.h"
#include "../roadmap_io.h"
#include "../roadmap_serial.h"
#include "wince_input_mon.h"

extern HWND RoadMapMainWindow;

DWORD WINAPI SerialMonThread(LPVOID lpParam)
{
	roadmap_main_io *data = (roadmap_main_io*)lpParam;
	RoadMapIO *io = data->io;
	DWORD fdwCommMask;
	HANDLE hCommPort = data->io->os.serial;

	while(io->subsystem != ROADMAP_IO_INVALID)
	{
		SetCommMask (hCommPort, EV_RXCHAR);
		if(!WaitCommEvent (hCommPort, &fdwCommMask, 0))
		{
			if(GetLastError() == ERROR_INVALID_HANDLE) {
				roadmap_log (ROADMAP_INFO,
						"Com port is closed.");
			} else {
				roadmap_log (ROADMAP_ERROR,
						"Error in WaitCommEvent.");
			}
		
			/* Ok, we got some error. We continue to the same path
			 * as if input is available. The read attempt should
			 * fail, and result in removing this input.
			 */
		}

		/* Check if this input was unregistered while we were
		 * sleeping.
		 */
		if (io->subsystem == ROADMAP_IO_INVALID) {
			break;
		}

		/* Send a message to main window so it can read. */
		SendMessage(RoadMapMainWindow, WM_USER_READ, (WPARAM)data, 0);
	}

	free(io);

	return 0;
}


DWORD WINAPI SocketMonThread(LPVOID lpParam)
{
	roadmap_main_io *data = (roadmap_main_io*)lpParam;
	RoadMapIO *io = data->io;
	SOCKET fd = data->io->os.socket;
	fd_set set;

	FD_ZERO(&set);
	while(io->subsystem != ROADMAP_IO_INVALID)
	{
		FD_SET(fd, &set);
		if(select(fd+1, &set, NULL, NULL, NULL) == SOCKET_ERROR) {
			roadmap_log (ROADMAP_ERROR,
					"Error in select.");
		}

		/* Check if this input was unregistered while we were
		 * sleeping.
		 */
		if (io->subsystem == ROADMAP_IO_INVALID) {
			break;
		}

		/* Send a message to main window so it can read. */
		SendMessage(RoadMapMainWindow, WM_USER_READ, (WPARAM)data, 0);
	}

	free(io);

	return 0;
}


/* This is not a real monitor thread. As this is a file we assume that input
 * is always ready and so we loop until a request to remove this input is
 * received.
 */
DWORD WINAPI FileMonThread(LPVOID lpParam)
{
	roadmap_main_io *data = (roadmap_main_io*)lpParam;
	RoadMapIO *io = data->io;

	while(io->subsystem != ROADMAP_IO_INVALID)
	{
		/* Send a message to main window so it can read. */
		SendMessage(RoadMapMainWindow, WM_USER_READ, (WPARAM)data, 0);
	}

	free(io);

	return 0;
}

