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

#if defined(__GNUC__)
#define       __try           try
#define       __except(x)     catch(...)
#else
#define       inline  _inline
#endif

#endif /* _ROADMAP_WIN32_H_ */
