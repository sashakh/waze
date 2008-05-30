/* wince_serial.c - serial connection management.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright (c) 2008, Danny Backx.
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

/* 10 ones to take 10 possible COMx: ports into account */
static int serial_ports[MAX_SERIAL_ENUMS] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

RoadMapSerial roadmap_serial_open(const char *name, const char *mode,
		int baud_rate)
{
	HANDLE hCommPort = INVALID_HANDLE_VALUE;
	LPWSTR url_unicode = ConvertToUNICODE(name);
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
		roadmap_log (ROADMAP_WARNING, "roadmap_serial_open(%s,%d) -> invalid handle",
				name, baud_rate);
		return (HANDLE)-1;
	}

	ct.ReadIntervalTimeout = MAXDWORD;
	ct.ReadTotalTimeoutMultiplier = 0;
	ct.ReadTotalTimeoutConstant = 0;
	ct.WriteTotalTimeoutMultiplier = 10;
	ct.WriteTotalTimeoutConstant = 1000;
	if(!SetCommTimeouts(hCommPort, &ct)) {
		roadmap_serial_close(hCommPort);
		roadmap_log (ROADMAP_WARNING, "roadmap_serial_open(%s,%d) -> SetCommTimeouts",
				name, baud_rate);
		return (HANDLE)-1;
	}

	dcb.DCBlength = sizeof(DCB);
	if(!GetCommState(hCommPort, &dcb)) {
		roadmap_serial_close(hCommPort);
		roadmap_log (ROADMAP_WARNING, "roadmap_serial_open(%s,%d) -> GetCommState",
				name, baud_rate);
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
		roadmap_log (ROADMAP_WARNING, "roadmap_serial_open(%s,%d) -> SetCommState",
				name, baud_rate);
		return (HANDLE)-1;
	}

	roadmap_log (ROADMAP_WARNING, "roadmap_serial_open(%s,%d) -> ok",
			name, baud_rate);
#if 0
	{
		HANDLE	h;
		char	data[1024];
		DWORD	n;
		int	i;

#if 0
		if (! ReadFile(hCommPort, data, sizeof(data), &n, NULL))
			return (HANDLE)-1;
#else
		int count = 5;
		while (ReadFile(hCommPort, data, sizeof(data), &n, NULL) && count > 0) {
			if (n >= sizeof(data))
				n = sizeof(data)-1;
			if (n > 0) {
				data[n] = 0;
				for (i=0; i<n-1; i++)
					if (data[i] == '\r' && data[i+1] == '\n')
						roadmap_log(ROADMAP_WARNING, "CRLF @ %d", i);
					else if (data[i] == '\n' && data[i+1] == '\r')
						roadmap_log(ROADMAP_WARNING, "CRLF @ %d", i);
			}
			roadmap_log(ROADMAP_WARNING, "read -> [%s]", data);
			count--;
		}
#endif
		if (n >= sizeof(data))
			n = sizeof(data)-1;
		if (n > 0)
			data[n] = 0;
		roadmap_log(ROADMAP_WARNING, "read -> [%s]", data);
	}
#endif
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

const char **roadmap_serial_get_speeds (void)
{
	static const char *serial_speeds[] =
		{"4800", "9600", "19200", "38400", "57600", NULL};

	return serial_speeds;
}

const int *roadmap_serial_enumerate (void)
{
	return serial_ports;
}

