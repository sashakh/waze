#ifndef _ROADMAP_WIN32_H_
#define _ROADMAP_WIN32_H_

#include <windows.h>
#define snprintf _snprintf
#define strdup _strdup
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define vsnprintf _vsnprintf

LPWSTR ConvertToUNICODE(LPCSTR string);
char* ConvertToANSI(const LPWSTR s, UINT nCodePage);

#endif /* _ROADMAP_WIN32_H_ */
