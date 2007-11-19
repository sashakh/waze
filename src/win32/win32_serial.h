#ifndef __WIN32_SERIAL_
#define __WIN32_SERIAL_

#include <windows.h>

typedef struct Win32SerialConn {
   
   HANDLE handle;
   char name[20];
   char mode[10];
   int baud_rate;
   char data[10240];
   int data_count;
   int ref_count;
   int valid;
   
} Win32SerialConn;

#endif //__WIN32_SERIAL_
