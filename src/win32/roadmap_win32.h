#ifndef _ROADMAP_WIN32_H_
#define _ROADMAP_WIN32_H_

#include <windows.h>

#if defined(__GNUC__)
#define __try try
#define __except(x) catch (...)
#else
#define inline _inline
#endif

#define MENU_ID_START	WM_USER
#define MAX_MENU_ITEMS	100
#define TOOL_ID_START	(MENU_ID_START + MAX_MENU_ITEMS + 1)
#define MAX_TOOL_ITEMS	100
#define WM_USER_DUMMY   (WM_USER + TOOL_ID_START + MAX_TOOL_ITEMS + 1)
#define WM_USER_READ    (WM_USER_DUMMY + 1)
#define WM_USER_SYNC    (WM_USER_READ + 1)

#define snprintf _snprintf
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define vsnprintf _vsnprintf
#define trunc(X) (X)

LPWSTR ConvertToWideChar(LPCSTR string, UINT nCodePage);
char* ConvertToMultiByte(const LPCWSTR s, UINT nCodePage);
const char *roadmap_main_get_virtual_serial (void);

time_t timegm(struct tm *_tm);

#endif /* _ROADMAP_WIN32_H_ */
