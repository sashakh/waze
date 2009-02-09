/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001-2006 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Header: /cvsroot/roadmap/roadmap/src/win32/CEDevice.c,v 1.1 2009/02/09 19:39:17 dannybackx Exp $
 *
 */

#include "windows.h"
#include "CEDevice.h"

/**
 * @file
 * @brief Windows CE power management code
 * @ingroup windows
 *
 * This code uses three techniques :
 * - Call SetPowerRequirement once on the backlight device BLK1: to keep the light on
 * - Periodically call SystemIdleTimerReset to keep the system from powering down
 * - Periodically call SHIdleTimerReset to keep the shell from locking the device or
 *   reverting to the Today screen.
 *
 * Power management code borrowed from MoDaCo & Betaplayer. Thanks !
 */
static void (WINAPI* _SHIdleTimerReset)(void) = NULL;
static HANDLE (WINAPI* _SetPowerRequirement)(const void *,int,ULONG,PVOID,ULONG) = NULL;
static DWORD (WINAPI* _ReleasePowerRequirement)(HANDLE) = NULL;
static HANDLE _hPowerManagement = NULL;
static DWORD _lastTime = 0;

#define TIMER_TRIGGER 9000

void ce_device_init(void)
{
	HINSTANCE dll = LoadLibrary(TEXT("aygshell.dll"));
	if (dll) {
		*(FARPROC*)&_SHIdleTimerReset = GetProcAddress(dll, MAKEINTRESOURCE(2006));
	}
	dll = LoadLibrary(TEXT("coredll.dll"));
	if (dll) {
		*(FARPROC*)&_SetPowerRequirement = GetProcAddress(dll, TEXT("SetPowerRequirement"));
		*(FARPROC*)&_ReleasePowerRequirement = GetProcAddress(dll, TEXT("ReleasePowerRequirement"));

	}
	if (_SetPowerRequirement)
		_hPowerManagement = _SetPowerRequirement(TEXT("BKL1:"), 0, 1, NULL, 0);
	_lastTime = GetTickCount();
}

void ce_device_end(void)
{
	if (_ReleasePowerRequirement && _hPowerManagement) {
		_ReleasePowerRequirement(_hPowerManagement);
	}
}

void ce_device_wakeup(void)
{
	DWORD currentTime = GetTickCount();
	if (currentTime > _lastTime + TIMER_TRIGGER) {
		_lastTime = currentTime;
		SystemIdleTimerReset();
		if (_SHIdleTimerReset)
			_SHIdleTimerReset();
	}
}
