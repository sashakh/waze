/*
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright (c) 2008, 2009 Danny Backx
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
 * @brief monitor for inputs (serial / network / file) and power management events.
 */

#include <windows.h>
#include <msgqueue.h>
#include <pm.h>

#include "../roadmap.h"
#include "../roadmap_io.h"
#include "../roadmap_serial.h"
#include "wince_input_mon.h"
#include "roadmap_time.h"

extern HWND RoadMapMainWindow;

/**
 * @brief
 * @param lpParam
 * @return
 */
DWORD WINAPI SerialMonThread(LPVOID lpParam)
{
	roadmap_main_io *data = (roadmap_main_io*)lpParam;
	RoadMapIO *io = data->io;
	DWORD fdwCommMask;
	HANDLE hCommPort = data->io->os.serial;

	while(io->subsystem != ROADMAP_IO_INVALID)
	{
		if (SetCommMask (hCommPort, EV_RXCHAR) == 0) {
			DWORD e = GetLastError();
			roadmap_log (ROADMAP_DEBUG, "SetCommMask %p %d\n", hCommPort, e);
			/* Returning here terminates the thread,
			 * which is probably the best thing to do. */
			return 0;
		}

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

	roadmap_log (ROADMAP_WARNING, "SerialMonThread: terminate");
	return 0;
}

/**
 * @brief
 * @param lpParam
 * @return
 */
DWORD WINAPI SocketMonThread(LPVOID lpParam)
{
	roadmap_main_io *data = (roadmap_main_io*)lpParam;
	RoadMapIO *io = data->io;
	SOCKET fd = data->io->os.socket;
	fd_set set;

	FD_ZERO(&set);
	while (data->is_valid && io->subsystem != ROADMAP_IO_INVALID)
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

	roadmap_log (ROADMAP_WARNING, "SocketMonThread: terminate");
	return 0;
}

/**
 * @brief This is not a real monitor thread.
 * As this is a file we assume that input is always ready and so we loop until
 * a request to remove this input is received.
 * @param lpParam
 * @return
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

	roadmap_log (ROADMAP_WARNING, "FileMonThread: terminate");
	return 0;
}

static HANDLE	power_msg_handle = NULL,
		power_req_handle = NULL;
static HANDLE	power_thread = NULL;
static POWER_BROADCAST	buf[4];	/* enough */

/**
 * @brief list of functions to call after suspend/resume cycle
 */
static roadmap_power_callback *power_callbacks = NULL;
static int npower_callbacks = 0,
	   maxpower_callbacks = 0;

static void roadmap_main_power_suspend(void)
{
	int	i;

	roadmap_log (ROADMAP_WARNING, "Success (%s) !\r\nMsg %d flags %d length %d",
			roadmap_time_get_hours_minutes(time(NULL)),
			buf[0].Message,
			buf[0].Flags,
			buf[0].Length);

	for (i=0; i<npower_callbacks; i++)
		(*power_callbacks[i])();
}

/**
 * @brief
 * @param func
 */
void roadmap_power_register(roadmap_power_callback func)
{
	if (npower_callbacks == maxpower_callbacks) {
		maxpower_callbacks += 4;
		power_callbacks = (roadmap_power_callback *)realloc((void *)power_callbacks,
				sizeof(roadmap_power_callback) * maxpower_callbacks);
	}
	power_callbacks[npower_callbacks++] = func;
}

/**
 * @brief Monitor power
 * @param lpParam
 * @return
 */
DWORD WINAPI PowerMonThread(LPVOID lpParam)
{
	DWORD		nread;	/**< number of bytes read */
	DWORD		dwflags;

	roadmap_log (ROADMAP_WARNING, "PowerMonThread");

	while (1) {
		if (ReadMsgQueue(power_msg_handle, buf, sizeof(buf),
					&nread, INFINITE, &dwflags) == FALSE) {
			DWORD e = GetLastError();
			roadmap_log (ROADMAP_WARNING, "Power: ReadMsgQueue failed %d", e);
			return 0;	/* stop thread */
		}
		if (buf[0].Message == PBT_RESUME) {
			roadmap_main_power_suspend();
		} else {
			; /* ignore */
		}
	}
	return 0;	/* never happens */
}

/**
 * @brief start something to monitor power changes
 */
void roadmap_main_power_monitor_start(void)
{
	MSGQUEUEOPTIONS	mqo;

	roadmap_log (ROADMAP_WARNING, "roadmap_main_power_monitor_start");
	mqo.dwSize = sizeof(mqo);
	mqo.dwFlags = MSGQUEUE_NOPRECOMMIT;
	mqo.dwMaxMessages = 0;
	mqo.cbMaxMessage = 100; /* ?? */
	mqo.bReadAccess = TRUE; /* read access to the queue */

	power_msg_handle = CreateMsgQueue(NULL, &mqo);
	if (power_msg_handle == NULL) 
		return;

	power_req_handle = RequestPowerNotifications(power_msg_handle, PBT_RESUME);

	power_thread = CreateThread(NULL, 0, PowerMonThread, NULL, 0, NULL);
}

/**
 * @brief stop monitoring power changes
 */
void roadmap_main_power_monitor_stop(void)
{
	roadmap_log (ROADMAP_WARNING, "roadmap_main_power_monitor_stop");
	if (power_req_handle)
		StopPowerNotifications(power_req_handle);
	power_req_handle = NULL;

	if (power_msg_handle)
		if (CloseMsgQueue(power_msg_handle) == FALSE) {
			; /* FIX ME */
		}
	power_msg_handle = NULL;
}
