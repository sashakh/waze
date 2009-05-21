/*
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *   Copyright (c) 2008, 2009, Danny Backx
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
 * @brief The main function of the RoadMap application for Windows.
 * @ingroup windows
 */

/**
 * @defgroup windows windows-specific part of RoadMap, currently Windows CE
 */
#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include <winsock.h>
#include <time.h>
#ifdef UNDER_CE
#include <aygshell.h>
#include <notify.h>
#include "CEException.h"
#include "CEDevice.h"
#endif

#ifndef I_IMAGENONE
#define I_IMAGENONE (-2)
#endif

#include "../roadmap.h"
#include "../roadmap_path.h"
#include "../roadmap_start.h"
#include "../roadmap_config.h"
#include "../roadmap_history.h"
#include "../roadmap_canvas.h"
#include "../roadmap_io.h"
#include "../roadmap_main.h"
// #include "../roadmap_res.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_screen.h"
#include "../roadmap_download.h"
// #include "../roadmap_lang.h"
#include "../roadmap_dialog.h"
#include "../roadmap_gps.h"
#include "wince_input_mon.h"
#include "roadmap_wincecanvas.h"
#include "roadmap_time.h"
#include "roadmap_dialog.h"

// Menu & toolbar defines
#define MENU_ID_START	WM_USER
#define MAX_MENU_ITEMS	200
#define TOOL_ID_START	(MENU_ID_START + MAX_MENU_ITEMS + 1)
#define MAX_TOOL_ITEMS	100

struct tb_icon {
	char *name;
	int id;
};
static	RoadMapCallback menu_callbacks[MAX_MENU_ITEMS] = {0};
static	RoadMapCallback	tool_callbacks[MAX_TOOL_ITEMS] = {0};

// timer stuff
#define ROADMAP_MAX_TIMER 16
struct roadmap_main_timer {
	unsigned int id;
	RoadMapCallback callback;
};
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];

// IO stuff
#define ROADMAP_MAX_IO 16
static roadmap_main_io *RoadMapMainIo = 0;

// varibles used across this module
static RoadMapKeyInput	RoadMapMainInput = NULL;
#ifdef UNDER_CE
static HWND	RoadMapMainMenuBar = NULL;
#else
static HMENU	RoadMapMainMenuBar = NULL;
#endif
static HWND	RoadMapMainToolbar = NULL;
static BOOL	RoadMapMainFullScreen = FALSE;

// Global Variables:
HINSTANCE	g_hInst = NULL;
HWND	RoadMapMainWindow  = NULL;

// Forward declarations of functions included in this code module:
static ATOM	MyRegisterClass(HINSTANCE, LPTSTR);
static BOOL	InitInstance(HINSTANCE, LPTSTR);
static LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
static int roadmap_main_set_idle_function_helper (void *);

#define MAX_LOADSTRING 100

// class name definition
#ifdef _ROADGPS
static WCHAR szWindowClass[] = L"RoadGPSClass";
#else
static WCHAR szWindowClass[] = L"RoadMapClass";
#endif

static RoadMapConfigDescriptor RoadMapConfigMenuBar =
                        ROADMAP_CONFIG_ITEM("General", "Menu bar");

int handleException(EXCEPTION_POINTERS *exceptionPointers)
{
#ifdef UNDER_CE
	ce_exception_write((wchar_t *)L"\\roadmapCrash", exceptionPointers);
#endif
	exit(0);

	return EXCEPTION_EXECUTE_HANDLER;
}

static void CALLBACK AvoidSuspend (HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
#ifdef UNDER_CE
	ce_device_wakeup();
#endif
}

// our main function
int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
#ifdef UNDER_CE
				   LPTSTR    lpCmdLine,
#else
				   LPSTR     lpCmdLine,
#endif
				   int       nCmdShow)
{
	MSG msg;
	LPTSTR cmd_line;
	
#ifdef UNDER_CE
	cmd_line = lpCmdLine;
#else
	cmd_line = L"";
#endif

	// Perform application initialization:
	if (!InitInstance(hInstance, cmd_line)) {
		return FALSE;
	}

	/*
	 * Figure out how big the screen is, pass it on
	 */
	int width, height;
	height = GetSystemMetrics(SM_CYSCREEN);
	width = GetSystemMetrics(SM_CXSCREEN);
	roadmap_dialog_set_resolution (height, width);

	ShowWindow(RoadMapMainWindow, nCmdShow);
	UpdateWindow(RoadMapMainWindow);
	
	HACCEL hAccelTable;
	hAccelTable = 0;
	
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		roadmap_main_set_idle_function_helper (NULL);
	}

	
	WSACleanup();
	return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS wc;
	
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
#ifdef _ROADGPS
	wc.hIcon         = 0;
#else
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ROADMAP));
#endif
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;
	
	return RegisterClass(&wc);
}

/**
 * @brief do the normal startup for PocketPC : check whether we're already running, quit if we are
 * and let the old process take over.
 * @param hInstance useless parameter on Windows CE
 * @param lpCmdLine command line passed in, also rather useless on CE
 * @return TRUE(0) if application should continue to start up
 */
BOOL InitInstance(HINSTANCE hInstance, LPTSTR lpCmdLine)
{
	HWND	hWnd;
	WSADATA wsaData;
	DWORD	e;
	
	g_hInst = hInstance; // Store instance handle in our global variable
	
#ifdef UNDER_CE
	SHInitExtraControls();
#endif
	
	SetLastError(0);
	/*
	 * If it is already running, then focus on the window, and exit.
	 * In addition to the usual stuff, also cope with strange returns from FindWindow.
	 */
	hWnd = FindWindow(szWindowClass, NULL);	
	e = GetLastError();
	if (hWnd && e != ERROR_CLASS_DOES_NOT_EXIST)
	{
		/*
		 * Set focus to foremost child window
		 * The "| 0x01" is used to bring any owned windows to the
		 * foreground and activate them.
		 */
		if (SetForegroundWindow((HWND)((ULONG) hWnd | 0x01)) != 0) {
			/*
			 * This is the normal case if another process from this application
			 * is still running. We've succeeded in popping it up so it should
			 * be visible, so this new process should quit.
			 */
			return 0;
		}
	} 
	
	if (!MyRegisterClass(hInstance, szWindowClass))
	{
		return FALSE;
	}
	
	if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0)
	{
		roadmap_log (ROADMAP_FATAL, "Can't initialize network");
	}
	
	if (lpCmdLine) {
		char	*argv[10];
		char	*args, *p, *st;
		int	len;
		int	argc = 0;

		argv[argc++] = "wroadmap.exe";

		len = wcslen(lpCmdLine);
		args = malloc(len+1);
		wcstombs(args, lpCmdLine, len);
		args[len] = '\0';

		for (p=args, st=NULL; *p; p++) {
			if (st != NULL && (*p == ' ' || *p == '\t')) {
				*p = '\0';
				argv[argc++] = st;
				st = NULL;
			} else if ((st == NULL) && ! (*p == ' ' || *p == '\t'))
				st = p;
		}
		if (st)
			argv[argc++] = st;
		
		roadmap_log (ROADMAP_WARNING, "Yow %d args", argc);
		roadmap_option(argc, argv, 0, NULL);
		roadmap_start(argc, argv);
	} else {
		char *args[1] = {0};
		roadmap_start(0, args);
	}

#ifndef _ROADGPS
	/* RoadmapEditor has stuff for first time handling here */
#endif

	return TRUE;
}


static int roadmap_main_char_key_pressed(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	char *key = NULL;
	char regular_key[2];
	
#ifdef UNDER_CE
	switch (wParam)
	{
	case VK_APP1:	key = (char *)"Button-App1";	   break;
	case VK_APP2:	key = (char *)"Button-App2";	   break;
	case VK_APP3:	key = (char *)"Button-App3";	   break;
	case VK_APP4:	key = (char *)"Button-App4";	   break;
   }
#endif

	if (key == NULL && (wParam > 0 && wParam < 128)) {
		regular_key[0] = wParam;
		regular_key[1] = 0;
		key = regular_key;
	}

        if (key != NULL) {
	        (*RoadMapMainInput) (key);
	        return 1;
        }
	
	return 0;
}


static int roadmap_main_vkey_pressed(HWND w, WPARAM wParam, LPARAM lParam)
{
	char *key = NULL;
	
	switch (wParam)
	{
	case VK_LEFT:	key = (char *)"Button-Left";	break;
	case VK_RIGHT:	key = (char *)"Button-Right";	break;
	case VK_UP:	key = (char *)"Button-Up";	break;
	case VK_DOWN:	key = (char *)"Button-Down";	break;
#ifdef UNDER_CE
	case VK_APP1:	key = (char *)"Button-App1";	break;
	case VK_APP2:	key = (char *)"Button-App2";	break;
	case VK_APP3:	key = (char *)"Button-App3";	break;
	case VK_APP4:	key = (char *)"Button-App4";	break;
#endif
#if 0
		/* These binding are for the iPAQ buttons: */
	case 0x1008ff1a: key = "Button-Menu";           break;
	case 0x1008ff20: key = "Button-Calendar";       break;
	case 0xaf9:      key = "Button-Contact";        break;
	case 0xff67:     key = "Button-Start";          break;
#endif
		/* Regular keyboard keys: */
	}
	
	if (key != NULL && RoadMapMainInput != NULL) {
		(*RoadMapMainInput) (key);
		return 1;
	}
	
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	BOOL create_menu;

#ifdef UNDER_CE
	static SHACTIVATEINFO s_sai;
#endif
	
	switch (message) 
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		{
			if ((wmId >= MENU_ID_START) &&
				(wmId < (MENU_ID_START + MAX_MENU_ITEMS))) {
				RoadMapCallback callback =
					menu_callbacks[wmId - MENU_ID_START];
				if (callback == NULL) {
					roadmap_log (ROADMAP_ERROR,
						"Can't find callback for menu item:%d",
						wmId);
					break;
				}
				roadmap_start_do_callback(callback);
			} else if ((wmId >= TOOL_ID_START) &&
				(wmId < (TOOL_ID_START + MAX_TOOL_ITEMS))) {
				RoadMapCallback callback =
					tool_callbacks[wmId - TOOL_ID_START];
				if (callback == NULL) {
					roadmap_log (ROADMAP_ERROR,
						"Can't find callback for tool item:%d",
						wmId);
					break;
				}
				roadmap_start_do_callback(callback);
			} else {
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
		
	case WM_CREATE:
#ifdef _ROADGPS
      create_menu = TRUE;
#else
      create_menu = roadmap_config_match (&RoadMapConfigMenuBar, "Yes") != 0;
#endif
      create_menu = TRUE;

#ifdef UNDER_CE
		SHMENUBARINFO mbi;
		
		memset(&mbi, 0, sizeof(SHMENUBARINFO));
		mbi.cbSize     = sizeof(SHMENUBARINFO);
		mbi.hwndParent = hWnd;
		mbi.dwFlags	   = 0;//SHCMBF_EMPTYBAR;
		mbi.nToolBarId = IDM_MENU;
		mbi.hInstRes   = g_hInst;
		
		// This is really stupid but I can't find another way.
		// I create a menubar as defined in our resource file and then I
		// delete everything in it, to get an empty menu.
		// If I try to just create an empty menu, it does't work! (can't
		// add items to it).
		if (!create_menu || !SHCreateMenuBar(&mbi)) 
		{
			RoadMapMainMenuBar = NULL;
		}
		else
		{
			RoadMapMainMenuBar = mbi.hwndMB;
			while (SendMessage(RoadMapMainMenuBar, (UINT) TB_BUTTONCOUNT,
				(WPARAM) 0, (LPARAM)0) > 0) {
				SendMessage(RoadMapMainMenuBar, (UINT) TB_DELETEBUTTON, 0, 0);
			}
		}
#else
		if (create_menu) {
		      RoadMapMainMenuBar = CreateMenu ();
		      SetMenu(hWnd, RoadMapMainMenuBar);
		}
#endif
		
#ifdef UNDER_CE                
		// Initialize the shell activate info structure
		memset(&s_sai, 0, sizeof (s_sai));
		s_sai.cbSize = sizeof (s_sai);

		ce_device_init();
		SetTimer(NULL, 0, 50000, AvoidSuspend);

		/* Start something (a thread?) to monitor power state changes */
		roadmap_main_power_monitor_start();
#endif
		break;
		
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		roadmap_canvas_refresh();
		EndPaint(hWnd, &ps);
		break;
		
	case WM_SIZE:
		roadmap_canvas_new(RoadMapMainWindow, RoadMapMainToolbar);
		break;

	case WM_DESTROY:
#ifdef UNDER_CE
		roadmap_main_power_monitor_stop();
		ce_device_end();
#endif
		if (RoadMapMainMenuBar != NULL) {
#ifdef UNDER_CE			
			DestroyWindow(RoadMapMainMenuBar);
#else
			DestroyMenu(RoadMapMainMenuBar);
#endif
		}
	
#ifdef UNDER_CE
		if (RoadMapMainToolbar != NULL) {
			CommandBar_Destroy(RoadMapMainToolbar);
		}
#endif
		PostQuitMessage(0);
		break;
		
	case WM_LBUTTONDOWN:
		{
			POINT point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			if (RoadMapMainToolbar != NULL) {
				point.y -= 26;
			}
			roadmap_canvas_button_pressed(1, &point);
		}
		break;

	case WM_LBUTTONUP:
		{
			POINT point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			if (RoadMapMainToolbar != NULL) {
				point.y -= 26;
			}
			roadmap_canvas_button_released(1, &point);
		}
		break;

	case WM_MBUTTONDOWN:
		{
			POINT point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			if (RoadMapMainToolbar != NULL) {
				point.y -= 26;
			}
			roadmap_canvas_button_pressed(2, &point);
		}
		break;

	case WM_MBUTTONUP:
		{
			POINT point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			if (RoadMapMainToolbar != NULL) {
				point.y -= 26;
			}
			roadmap_canvas_button_released(2, &point);
		}
		break;

	case WM_RBUTTONDOWN:
		{
			POINT point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			if (RoadMapMainToolbar != NULL) {
				point.y -= 26;
			}
			roadmap_canvas_button_pressed(3, &point);
		}
		break;

	case WM_RBUTTONUP:
		{
			POINT point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			if (RoadMapMainToolbar != NULL) {
				point.y -= 26;
			}
			roadmap_canvas_button_released(3, &point);
		}
		break;

	case WM_MOUSEMOVE:
		{
			POINT point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			if (RoadMapMainToolbar != NULL) {
				point.y -= 26;
			}
			roadmap_canvas_mouse_move(&point);
		}
		break;

	case WM_MOUSEWHEEL:
		{
			POINT point;
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			if (RoadMapMainToolbar != NULL) {
				point.y -= 26;
			}
			roadmap_canvas_mouse_scroll(HIWORD(lParam), &point);
		}
		break;
				
	case WM_KEYDOWN:
		if (roadmap_main_vkey_pressed(hWnd, wParam, lParam)) return 0;
		else return DefWindowProc(hWnd, message, wParam, lParam);
		break;
		
	case WM_CHAR:
		if (roadmap_main_char_key_pressed(hWnd, wParam, lParam)) return 0;
		else return DefWindowProc(hWnd, message, wParam, lParam);
		break;
		
#ifdef UNDER_CE
	case WM_ACTIVATE:
      		if ((LOWORD(wParam)!=WA_INACTIVE) && RoadMapMainFullScreen) {
        		SHFullScreen (RoadMapMainWindow,
				SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON);
         		MoveWindow(RoadMapMainWindow, 0, 0,
            			GetSystemMetrics(SM_CXSCREEN),
				GetSystemMetrics(SM_CYSCREEN),
            			TRUE);
      		} else {
			// Notify shell of our activate message
			SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
      		}
		break;
      
	case WM_SETTINGCHANGE:
#if 0
		// Ehud's version has this commented out
		// SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
#endif
		break;
	case WM_KILLFOCUS:
		//roadmap_screen_freeze ();
		break;
		
	case WM_SETFOCUS:
		//roadmap_screen_unfreeze ();
		break;
#endif
		
	case WM_USER_READ:
		{
			roadmap_main_io *context = (roadmap_main_io *) wParam;
			 if (!context->is_valid) break;

			 if (lParam != 1) {
			    if (!ROADMAP_SERIAL_IS_VALID (lParam)) {
			       /* An old input which was removed */
			       break;
			    }
			 }
			(*context->callback) (context->io);
			break;
		}
		
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
    return 0;
}


static struct tb_icon RoadMapIcons[] = {
	{(char *)"destination", IDB_RM_DESTINATION},
	{(char *)"location", IDB_RM_LOCATION},
	{(char *)"gps", IDB_RM_GPS},
	{(char *)"hold", IDB_RM_HOLD},
	{(char *)"counterclockwise", IDB_RM_COUNTERCLOCKWISE},
	{(char *)"clockwise", IDB_RM_CLOCKWISE},
	{(char *)"zoomin", IDB_RM_ZOOMIN},
	{(char *)"zoomout", IDB_RM_ZOOMOUT},
	{(char *)"zoom1", IDB_RM_ZOOM1},
	{(char *)"full", IDB_RM_FULL},
	{(char *)"record", IDB_RM_RECORD},
	{(char *)"stop", IDB_RM_STOP},
	{(char *)"quit", IDB_RM_QUIT},
	{(char *)"down", IDB_RM_DOWN},
	{(char *)"up", IDB_RM_UP},
	{(char *)"right", IDB_RM_RIGHT},
	{(char *)"left", IDB_RM_LEFT},
	{NULL, 0}
};


static int roadmap_main_toolbar_icon (const char *icon)
{
	struct tb_icon *itr;

	if (icon == NULL)
		return 0;
	
	itr = RoadMapIcons;
	
	while (itr->name != NULL) {
		if (!strcasecmp(icon, itr->name)) {
			return itr->id;
		}
		itr++;
	}
	
	return -1;
}

void roadmap_main_toggle_full_screen (void)
{
#if 1
	// TODO: implement
	return;
#else
#ifdef UNDER_CE
      RECT rc;
      int menu_height = 0;

      if (RoadMapMainMenuBar != NULL) {
	 menu_height = 26;
      }

      //get window size
      GetWindowRect(RoadMapMainWindow, &rc);

      if (!RoadMapMainFullScreen) {
	 SHFullScreen(RoadMapMainWindow, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON);
	 if (RoadMapMainMenuBar != NULL) ShowWindow(RoadMapMainMenuBar, SW_HIDE);

	 MoveWindow(RoadMapMainWindow,
		      rc.left, 
		      rc.top-26,
		      rc.right, 
		      rc.bottom+menu_height,
		      TRUE);

	 RoadMapMainFullScreen = true;

      } else {
	
	 SHFullScreen(RoadMapMainWindow, SHFS_SHOWTASKBAR);
	 if (RoadMapMainMenuBar != NULL) ShowWindow(RoadMapMainMenuBar, SW_SHOW);
	 MoveWindow(RoadMapMainWindow, 
	    rc.left, 
	    rc.top+26,
	    rc.right, 
	    rc.bottom- 26 - menu_height,
	    TRUE);

	 RoadMapMainFullScreen = false;

      }
#endif
#endif
}


void roadmap_main_new (const char *title, int width, int height)
{
	LPWSTR szTitle = ConvertToUNICODE(title);
	DWORD style = WS_VISIBLE;

	roadmap_config_declare_enumeration
		("preferences", &RoadMapConfigMenuBar, "No", "Yes", NULL);
#ifndef UNDER_CE
	style |= WS_OVERLAPPEDWINDOW;
#endif
	RoadMapMainWindow = CreateWindow(szWindowClass, szTitle, style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
		NULL, g_hInst, NULL);
	
	free(szTitle);
	if (!RoadMapMainWindow)
	{
		roadmap_log (ROADMAP_FATAL, "Can't create main window");
		return;
	}
	
#ifdef UNDER_CE
	if (RoadMapMainMenuBar)
	{
		RECT rc;
		RECT rcMenuBar;
		
		GetWindowRect(RoadMapMainWindow, &rc);
		GetWindowRect(RoadMapMainMenuBar, &rcMenuBar);
		rc.bottom -= (rcMenuBar.bottom - rcMenuBar.top);
		
		MoveWindow(RoadMapMainWindow, rc.left, rc.top, rc.right-rc.left,
			rc.bottom-rc.top, FALSE);
	}
#endif
}

void roadmap_main_title(char *fmt, ...)
{
	va_list	ap;
	int	n, sz;
	char	*str;
	wchar_t	*ws;

	sz = 200;
	str = (char *)malloc(sz);

#if 1	/* Concatenate "RoadMap" with the parameter passed" */
#define	RoadMapMainTitle	"RoadMap"
	n = snprintf(str, sz, "%s", RoadMapMainTitle);
	va_start(ap, fmt);
	vsnprintf(&str[n], sz - n, fmt, ap);
	va_end(ap);
#else	/* Just show the path */
	va_start(ap, fmt);
	vsnprintf(str, sz, fmt, ap);
	va_end(ap);
#endif

	ws = ConvertToUNICODE(str);
	SetWindowText(RoadMapMainWindow, ws);

	free(str);
	free(ws);
}

void roadmap_main_set_keyboard (RoadMapKeyInput callback)
{
	RoadMapMainInput = callback;
}

RoadMapMenu roadmap_main_new_menu (const char *title)
{
    return (RoadMapMenu)CreatePopupMenu();
}


void roadmap_main_add_menu (RoadMapMenu menu, const char *label)
{
	static int menu_id = 0;
	if (RoadMapMainMenuBar == NULL) {
		roadmap_log (ROADMAP_WARNING, "roadmap_main_add_menu NULL");
		return;
	}
	
	LPWSTR label_unicode = ConvertToUNICODE(label);
#ifdef UNDER_CE
	TBBUTTON button;
	SendMessage(RoadMapMainMenuBar, TB_BUTTONSTRUCTSIZE, sizeof(button),
		0);
	
	memset(&button, 0, sizeof(button));
	button.iBitmap = I_IMAGENONE;
	button.idCommand = menu_id++;
	button.iString = (int)label_unicode;
	button.fsState = TBSTATE_ENABLED;
	// I'm not really sure what this magic number represents(but it works)
	button.fsStyle = 152;
	
	button.dwData = (UINT)menu;
	
	SendMessage(RoadMapMainMenuBar, (UINT) TB_ADDBUTTONS, (WPARAM) 1,
		(LPARAM) (LPTBBUTTON) &button);
#else
	DWORD res = AppendMenu(RoadMapMainMenuBar, MF_POPUP, (UINT)menu, label_unicode);
	SetMenu (RoadMapMainWindow, RoadMapMainMenuBar);
#endif
	free(label_unicode);
}


void roadmap_main_add_menu_item (RoadMapMenu menu, 
	const char *label, const char *tip, RoadMapCallback callback)
{
	static int menu_id = 0;
	
	if (label != NULL) {
		LPWSTR label_unicode = ConvertToUNICODE(label);
		AppendMenu((HMENU)menu, MF_STRING,
			menu_id + MENU_ID_START, label_unicode);
		free(label_unicode);
		
		if (menu_id == MAX_MENU_ITEMS) {
			roadmap_log (ROADMAP_FATAL, "Too many menu items!");
			return;
		}
		menu_callbacks[menu_id] = callback;
		menu_id++;
	} else {
		AppendMenu((HMENU)menu, MF_SEPARATOR, 0, (wchar_t *)L"");
	}
	
	if (tip != NULL) {
		//TODO: does wince support it?
	}
}

void roadmap_main_popup_menu (RoadMapMenu menu, 
			  const RoadMapGuiPoint *position) {

    TrackPopupMenuEx( (HMENU)menu, 0,
	position->x, position->y, RoadMapMainWindow, NULL);
}

void roadmap_main_add_separator  (RoadMapMenu menu)
{
	roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
}

void roadmap_main_add_toolbar (const char *orientation) {
	if (RoadMapMainToolbar == NULL) {
 // TBD: consider the orientation.
#ifdef UNDER_CE
		RoadMapMainToolbar = CommandBar_Create (g_hInst,
			RoadMapMainWindow, 1);
#else
		INITCOMMONCONTROLSEX InitCtrlEx;

		InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
		InitCtrlEx.dwICC  = ICC_BAR_CLASSES;
		InitCommonControlsEx(&InitCtrlEx);

		RoadMapMainToolbar = CreateWindowEx (
			0,
			TOOLBARCLASSNAME,     // Class name
			L"toolbar",  		  // Window name
			WS_CHILD|WS_VISIBLE,  // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			RoadMapMainWindow,    // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you

		SendMessage(RoadMapMainToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

#endif
		// LONG style = GetWindowLong (RoadMapMainToolbar, GWL_STYLE);
	}
}

void roadmap_main_add_tool (const char *label,
	const char *icon,
	const char *tip,
	RoadMapCallback callback)
{
	static int tool_id = 0;
	
	if (RoadMapMainToolbar == NULL) {
	    roadmap_main_add_toolbar (0);
	}
	
	int icon_res_id = roadmap_main_toolbar_icon(icon);
	if (icon_res_id == -1) return;
	
	int icon_tb_id = 0;
	int res;
#ifdef UNDER_CE
	icon_tb_id = CommandBar_AddBitmap(RoadMapMainToolbar, g_hInst,
		icon_res_id, 1, 16, 16);
#else
	TBADDBITMAP tbb;

	tbb.hInst = NULL;
	tbb.nID   = (DWORD)LoadBitmap(g_hInst, MAKEINTRESOURCE(icon_res_id));

	icon_tb_id = SendMessage(RoadMapMainToolbar,
			      (UINT) TB_ADDBITMAP,
				  (WPARAM) 1,
				  (LPARAM) &tbb);
#endif
	
	TBBUTTON but;
	but.iBitmap = icon_tb_id;
	but.idCommand = TOOL_ID_START + tool_id;
	but.fsState = TBSTATE_ENABLED;
	but.fsStyle = 0;
	but.dwData = 0;
	but.iString = 0;
	
#if defined(UNDER_CE) && defined(CommandBar_AddButtons)
	res = CommandBar_AddButtons(RoadMapMainToolbar, 1, &but);
#else
	res = SendMessage(RoadMapMainToolbar,
			      (UINT) TB_ADDBUTTONS,
				  (WPARAM) 1,
				  (LPARAM) &but);
#endif
	if (res != FALSE) {
		tool_callbacks[tool_id] = callback;
		tool_id++;
	}
}


void roadmap_main_add_tool_space (void)
{
	if (RoadMapMainToolbar == NULL) {
		roadmap_log (ROADMAP_FATAL,
			"Invalid toolbar space: no toolbar yet");
	}
	
	TBBUTTON but;
	but.iBitmap = -1;
	but.idCommand = 1;
	but.fsState = 0;
	but.fsStyle = TBSTYLE_SEP;
	but.dwData = 0;
	but.iString = 0;
	
#ifdef UNDER_CE
	CommandBar_AddButtons(RoadMapMainToolbar, 1, &but);
#else
	SendMessage(RoadMapMainToolbar,
		       (UINT) TB_ADDBUTTONS,
			   (WPARAM) 1,
			   (LPARAM) &but);
#endif
	
}


void roadmap_main_add_canvas (void)
{
#ifndef _ROADGPS
//      		roadmap_main_toggle_full_screen ();
#endif

	roadmap_canvas_new(RoadMapMainWindow, RoadMapMainToolbar);
}


void roadmap_main_add_status (void)
{
	//TODO: do we need this?
}


void roadmap_main_show (void)
{
	if (RoadMapMainWindow != NULL) {
		roadmap_canvas_new(RoadMapMainWindow, RoadMapMainToolbar);
	}
}

void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback)
{
	int i;
	
	if (RoadMapMainIo == 0)
		RoadMapMainIo = (roadmap_main_io*)calloc(ROADMAP_MAX_IO,
				sizeof(roadmap_main_io));

	for (i = 0; i < ROADMAP_MAX_IO; ++i) {
		if (RoadMapMainIo[i].io == NULL) {
			RoadMapMainIo[i].io = (RoadMapIO*)malloc(sizeof(RoadMapIO));
			*(RoadMapMainIo[i].io) = *io;
			RoadMapMainIo[i].callback = callback;
			RoadMapMainIo[i].thread = NULL;
			RoadMapMainIo[i].is_valid = 1;
			break;
		}
	}

	if (i == ROADMAP_MAX_IO) {
		roadmap_log (ROADMAP_FATAL, "Too many set input calls");
		return;
	}

	HANDLE monitor_thread = NULL;
	roadmap_main_io *p = &RoadMapMainIo[i];
	DWORD		tid;

	switch (io->subsystem) {
	case ROADMAP_IO_SERIAL:
		monitor_thread = CreateThread(NULL, 0,
			SerialMonThread, (void*)p, 0, &tid);
		break;
	case ROADMAP_IO_NET:
		monitor_thread = CreateThread(NULL, 0,
			SocketMonThread, (void*)&RoadMapMainIo[i], 0, &tid);
		break;
	case ROADMAP_IO_FILE:
		monitor_thread = CreateThread(NULL, 0,
			FileMonThread, (void*)&RoadMapMainIo[i], 0, &tid);
		break;
	}

	p->thread = tid;
	
	if (monitor_thread == NULL)
	{
		roadmap_log (ROADMAP_FATAL, "Can't create monitor thread");
		roadmap_io_close(io);
		return;
	} else {
		CloseHandle(monitor_thread);
	}
}


void roadmap_main_remove_input (RoadMapIO *io)
{
	int i;
			
	for (i = 0; i < ROADMAP_MAX_IO; ++i) {
		 if (RoadMapMainIo[i].io == io) {
			if (RoadMapMainIo[i].is_valid) {
			   RoadMapMainIo[i].is_valid = 0;
			} else {
//				   free (RoadMapMainIo[i]);
			}
			RoadMapMainIo[i].io = NULL;

			if (RoadMapMainIo[i].thread) {
				TerminateThread(RoadMapMainIo[i].thread, -1);
				CloseHandle(RoadMapMainIo[i].thread);
				RoadMapMainIo[i].thread = NULL;
			}
			break;

		}
	}
}


static void roadmap_main_timeout (HWND hwnd, UINT uMsg, UINT_PTR idEvent,
	DWORD dwTime) 
{	
	struct roadmap_main_timer *timer;

	/*
	 * Protect against the occasional invalid call.
	 * On my system, this happens with hwnd == 0.
	 */
	if (idEvent > ROADMAP_MAX_TIMER || hwnd == 0)
		return;
	timer = RoadMapMainPeriodicTimer + (idEvent - 1);
	RoadMapCallback callback = (RoadMapCallback) timer->callback;
	
	if (callback != NULL) {
		(*callback) ();
	}
}


void roadmap_main_set_periodic (int interval, RoadMapCallback callback)
{
	int index;
	struct roadmap_main_timer *timer = NULL;
	
	for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {
		
		if (RoadMapMainPeriodicTimer[index].callback == callback) {
			return;
		}
		if (timer == NULL) {
			if (RoadMapMainPeriodicTimer[index].callback == NULL) {
				timer = RoadMapMainPeriodicTimer + index;
				timer->id = index + 1;
			}
		}
	}
	
	if (timer == NULL) {
		roadmap_log (ROADMAP_FATAL, "Timer table saturated");
	}
	
	timer->callback = callback;
	SetTimer(RoadMapMainWindow, timer->id, interval,
		roadmap_main_timeout);
}


void roadmap_main_remove_periodic (RoadMapCallback callback)
{
	int index;

	for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {
		
		if (RoadMapMainPeriodicTimer[index].callback == callback) {
			
			RoadMapMainPeriodicTimer[index].callback = NULL;
			if (KillTimer(RoadMapMainWindow,
				RoadMapMainPeriodicTimer[index].id) == 0) {
				DWORD e = GetLastError();
				roadmap_log (ROADMAP_ERROR, "KillTimer failed (%d)", e);
			}
			
			return;
		}
	}
	
	roadmap_log (ROADMAP_DEBUG, "timer 0x%08x not found", callback);
}


void roadmap_main_set_status (const char *text) {}

int roadmap_main_flush (void)
{
      // HWND w = GetFocus();
      MSG msg;
      int hadevent = 0;

      while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
	 hadevent = 1;
	 TranslateMessage(&msg);
	 DispatchMessage(&msg);
      }

      //UpdateWindow(w);
      return hadevent;
}

int roadmap_main_flush_synchronous (int deadline)
{
  return 1; /* Nothing to do. */
}


void roadmap_main_exit (void)
{
	roadmap_start_exit ();
	/* This is an easy and clean way to terminate threads too */
	ExitProcess(0);
}

static unsigned long roadmap_main_busy_start;

void roadmap_main_set_cursor (RoadMapCursor newcursor)
{
  static RoadMapCursor lastcursor;

  roadmap_main_busy_start = 0;

  if (newcursor == ROADMAP_CURSOR_WAIT_WITH_DELAY) {
     roadmap_main_busy_start = roadmap_time_get_millis();
     return;
  }

  if (newcursor == lastcursor)
     return;

  lastcursor = newcursor;

  switch (newcursor) {

  case ROADMAP_CURSOR_NORMAL:
     SetCursor(NULL);
     break;

  case ROADMAP_CURSOR_WAIT:
     SetCursor(LoadCursor(NULL, IDC_WAIT));
     break;

  case ROADMAP_CURSOR_CROSS:
     SetCursor(LoadCursor(NULL, IDC_CROSS));
     break;

  case ROADMAP_CURSOR_WAIT_WITH_DELAY:
     /* ?? FIX ME */
     break;
  }
}


void roadmap_main_busy_check(void)
{
  if (roadmap_main_busy_start == 0)
     return;

  if (roadmap_time_get_millis() - roadmap_main_busy_start > 1000) {
     roadmap_main_set_cursor (ROADMAP_CURSOR_WAIT);
  }
}

static RoadMapCallback idle_callback;

static int roadmap_main_set_idle_function_helper (void *data)
{
    if (idle_callback) idle_callback();
    return 0;
}

void roadmap_main_set_idle_function (RoadMapCallback callback)
{
   idle_callback = callback;
}

void roadmap_main_remove_idle_function (void)
{
	idle_callback = 0;
}
