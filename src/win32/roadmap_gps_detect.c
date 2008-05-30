/* roadmap_gps_detect.c - GPS auto detection - win32 only
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_math.h"
#include "roadmap_string.h"
#include "roadmap_object.h"
#include "roadmap_config.h"
#include "roadmap_messagebox.h"

#include "roadmap_net.h"
#include "roadmap_file.h"
#include "roadmap_serial.h"
#include "roadmap_state.h"
#include "roadmap_nmea.h"
#include "roadmap_gpsd2.h"
#include "roadmap_vii.h"
#include "roadmap_driver.h"

#include "roadmap_dialog.h"
#include "roadmap_main.h"
#include "roadmap_lang.h"

#include "roadmap_gps.h"

static RoadMapConfigDescriptor RoadMapConfigGPSBaudRate =
                        ROADMAP_CONFIG_ITEM("GPS", "Baud Rate");

static char **speeds;

static int detect_one_speed(HANDLE h, int SpeedIndex)
{
	COMMTIMEOUTS	ct;
	DCB		dcb;
	char		data[1024];
	DWORD		n;
	int		i, count;
	int		baud = atoi(speeds[SpeedIndex]);

	roadmap_log (ROADMAP_WARNING, "Try speed %d", baud);

	ct.ReadIntervalTimeout = MAXDWORD;
	ct.ReadTotalTimeoutMultiplier = 0;
	ct.ReadTotalTimeoutConstant = 0;
	ct.WriteTotalTimeoutMultiplier = 10;
	ct.WriteTotalTimeoutConstant = 1000;

	if (!SetCommTimeouts(h, &ct)) {
		roadmap_log(ROADMAP_WARNING, "SetCommTimeouts failure");
		return -1;
	}

	dcb.DCBlength = sizeof(DCB);
	if (!GetCommState(h, &dcb)) {
		roadmap_log(ROADMAP_WARNING, "GetCommState failure");
		return -1;
	}

	dcb.fBinary		= TRUE;
	dcb.BaudRate		= baud;
	dcb.fOutxCtsFlow	= TRUE;
	dcb.fRtsControl		= RTS_CONTROL_DISABLE;
	dcb.fDtrControl		= DTR_CONTROL_DISABLE;
	dcb.fOutxDsrFlow	= FALSE;
	dcb.fOutX		= FALSE;
	dcb.fInX		= FALSE;
	dcb.ByteSize		= 8;
	dcb.Parity		= NOPARITY;
	dcb.StopBits		= ONESTOPBIT;

	if (!SetCommState(h, &dcb)) {
		roadmap_log(ROADMAP_WARNING, "SetCommState failure");
		return -1;
	}

	count = 5;
	while (ReadFile(h, data, sizeof(data), &n, NULL) && count > 0) {
		if (n >= sizeof(data))
			n = sizeof(data)-1;
		if (n > 0) {
			data[n] = 0;
//			roadmap_log(ROADMAP_WARNING, "read -> [%s]", data);
			for (i=0; i<n-1; i++)
				if (data[i] == '\r' && data[i+1] == '\n') {
					roadmap_log(ROADMAP_WARNING, "Yes !");
					return 1;
				}
		}
		count--;
	}
	return -1;
}

static int detect_one_port(HANDLE h)
{
	int	i;

	for (i=0; speeds[i]; i++) {
		roadmap_dialog_set_data ("GPS Receiver Auto Detect", "Speed",
				speeds[i]);
		if (detect_one_speed(h, i) == 1) {
			return i;
		}
	}

	/* Nothing found */
	return -1;
}

/* A thread function */
static DWORD WINAPI detect_simple(void *p)
{
	wchar_t	devname[8];	/* COMx: */
	char	devn[8];
	HANDLE	h;
	int	devnum;
	int	SpeedIndex = -1;
	char	Prompt[100];

	roadmap_log (ROADMAP_WARNING, "detect_simple()");
	speeds = roadmap_serial_get_speeds ();

	for (devnum=0; devnum <= 9; devnum++) {
		sprintf(devn, "COM%d:", devnum);
		wsprintf(devname, L"COM%d:", devnum);

		roadmap_dialog_set_data ("GPS Receiver Auto Detect", "Port", devn);

		h = CreateFile (devname, GENERIC_READ | GENERIC_WRITE,
				0, NULL, OPEN_EXISTING, 0, NULL);
		if (h == INVALID_HANDLE_VALUE) {
			roadmap_log (ROADMAP_WARNING, "Port %s -> invalid", devn);
			continue;
		}

		if ((SpeedIndex = detect_one_port(h)) >= 0) {
			CloseHandle(h);

			snprintf (Prompt, sizeof(Prompt),
					"%s\nPort: COM%d:\nSpeed: %s",
					roadmap_lang_get ("Found GPS Receiver."),
					devnum, speeds[SpeedIndex]);
			roadmap_messagebox(roadmap_lang_get ("Info"), Prompt);
			roadmap_dialog_hide ("Detect GPS receiver");

			roadmap_config_set (&RoadMapConfigGPSSource, devn);
			roadmap_config_set (&RoadMapConfigGPSBaudRate, speeds[SpeedIndex]);

			roadmap_gps_open();

			return 0;
		}

		CloseHandle(h);
	}

	roadmap_dialog_hide ("Detect GPS receiver");
	return 0;
}

static void detect_detach(void)
{
	HANDLE	t;

	t = CreateThread(NULL, 0, detect_simple, NULL, 0, NULL);
}

void roadmap_gps_detect_receiver (void)
{
	if (RoadMapGpsReception != 0) {
		roadmap_messagebox(roadmap_lang_get ("Info"),
		roadmap_lang_get ("GPS already connected!"));
	} else {
		if (roadmap_dialog_activate ("Detect GPS receiver", NULL)) {
			roadmap_dialog_new_label  ("GPS Receiver Auto Detect", "Port");
			roadmap_dialog_new_label  ("GPS Receiver Auto Detect", "Speed");
			roadmap_dialog_new_label  ("GPS Receiver Auto Detect", "Status");

			roadmap_dialog_complete (0);
		}

		roadmap_dialog_set_data ("GPS Receiver Auto Detect", "Status",
			roadmap_lang_get ("Running, please wait..."));
		detect_detach();
	}
}
