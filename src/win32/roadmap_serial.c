/* wince_serial.c - serial connection management.
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
#include "../roadmap_serial.h"

RoadMapSerial roadmap_serial_open(const char *name, const char *mode,
		int baud_rate)
{
	HANDLE hCommPort = INVALID_HANDLE_VALUE;
	LPWSTR url_unicode = ConvertToWideChar(name, CP_UTF8);
	COMMTIMEOUTS ct;
	DCB dcb;

	hCommPort = CreateFile (url_unicode,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	free(url_unicode);

	if(hCommPort == INVALID_HANDLE_VALUE) {
		return (HANDLE)-1;
	}

	ct.ReadIntervalTimeout = MAXDWORD;
	ct.ReadTotalTimeoutMultiplier = 0;
	ct.ReadTotalTimeoutConstant = 0;
	ct.WriteTotalTimeoutMultiplier = 10;
	ct.WriteTotalTimeoutConstant = 1000;
	if(!SetCommTimeouts(hCommPort, &ct)) {
		roadmap_serial_close(hCommPort);
		return (HANDLE)-1;
	}

	dcb.DCBlength = sizeof(DCB);
	if(!GetCommState(hCommPort, &dcb)) {
		roadmap_serial_close(hCommPort);
		return (HANDLE)-1;
	}

	dcb.fBinary			   = TRUE;
	dcb.BaudRate	       = baud_rate;
	dcb.fOutxCtsFlow       = TRUE;
	dcb.fRtsControl        = RTS_CONTROL_DISABLE;
	dcb.fDtrControl        = DTR_CONTROL_DISABLE;
	dcb.fOutxDsrFlow       = FALSE;
	dcb.fOutX              = FALSE;
	dcb.fInX               = FALSE;
	dcb.ByteSize           = 8;
	dcb.Parity             = NOPARITY;
	dcb.StopBits           = ONESTOPBIT;

	if(!SetCommState(hCommPort, &dcb)) {
		roadmap_serial_close(hCommPort);
		return (HANDLE)-1;
	}

	return hCommPort;
}


void roadmap_serial_close(RoadMapSerial serial)
{
	if (serial != INVALID_HANDLE_VALUE) {
		CloseHandle(serial);
	}
}


int roadmap_serial_read(RoadMapSerial serial, void *data, int size)
{
   DWORD dwBytesRead;

   if(!ReadFile((HANDLE)serial,
                data,
                size,
                &dwBytesRead,
                NULL)) {
      return -1;
   }

   return dwBytesRead;
}

int roadmap_serial_write (RoadMapSerial serial, const void *data, int length)
{
   DWORD dwBytesWritten;

   if(!WriteFile((HANDLE)serial,
                data,
                length,
                &dwBytesWritten,
                NULL)) {
      return -1;
   }

   return dwBytesWritten;
}

