#ifndef __WINCE_SERIAL_
#define __WINCE_SERIAL_

#include <windows.h>
#include "../roadmap_main.h"

#define WM_USER_READ WM_USER+1

typedef struct roadmap_main_io {
   RoadMapIO *io;
   RoadMapInput callback;
   int is_valid;
   DWORD	thread;
} roadmap_main_io;

DWORD WINAPI SerialMonThread(LPVOID lpParam);
DWORD WINAPI SocketMonThread(LPVOID lpParam);
DWORD WINAPI FileMonThread(LPVOID lpParam);

void roadmap_main_power_monitor_start(void);
void roadmap_main_power_monitor_stop(void);

typedef void (*roadmap_power_callback) (void);
void roadmap_power_register(roadmap_power_callback func);
#endif //__WINCE_SERIAL_
