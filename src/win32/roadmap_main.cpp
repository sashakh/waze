/* roadmap_main.c - The main function of the RoadMap application.
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
 * SYNOPSYS:
 *
 *   roadmap_main.h
 */

#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include <winsock.h>
#include <time.h>
#ifdef UNDER_CE
#include <aygshell.h>
// #include <notify.h>
#endif

#ifndef I_IMAGENONE
#define I_IMAGENONE (-2)
#endif

extern "C" {
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
#include "win32_serial.h"
#include "roadmap_wincecanvas.h"
}

#ifndef _T
#define _T(x) L ## x
#endif


// Menu & toolbar defines
#define MENU_ID_START	WM_USER
#define MAX_MENU_ITEMS	100
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
extern "C" { static roadmap_main_io *RoadMapMainIo[ROADMAP_MAX_IO] = {0}; }

// varibles used across this module
static RoadMapKeyInput	RoadMapMainInput = NULL;
#ifdef UNDER_CE
static HWND				   RoadMapMainMenuBar = NULL;
#else
static HMENU			   RoadMapMainMenuBar = NULL;
#endif
static HMENU			RoadMapCurrentSubMenu = NULL;
static HWND				RoadMapMainToolbar = NULL;
static bool				   RoadMapMainFullScreen = false;

// Global Variables:
extern "C" { HINSTANCE	g_hInst = NULL; }
extern "C" { HWND			RoadMapMainWindow  = NULL; }

// Forward declarations of functions included in this code module:
static ATOM				MyRegisterClass(HINSTANCE, LPTSTR);
static BOOL				   InitInstance(HINSTANCE, LPTSTR);
static LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

#define MAX_LOADSTRING 100

// class name definition
#ifdef _ROADGPS
static WCHAR szWindowClass[] = L"RoadGPSClass";
#else
static WCHAR szWindowClass[] = L"RoadMapClass";
#endif

static RoadMapConfigDescriptor RoadMapConfigMenuBar =
                        ROADMAP_CONFIG_ITEM("General", "Menu bar");

// our main function
#ifdef UNDER_CE
int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPTSTR    lpCmdLine,
				   int       nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPSTR     lpCmdLine,
				   int       nCmdShow)
#endif
{
	MSG msg;
	LPTSTR cmd_line = L"";
	
#ifdef UNDER_CE
	cmd_line = lpCmdLine;
#endif

#warning
#warning code ifdefed for arm-wince-mingw32ce toolchain
#if LATER
	__try 
#endif
        {
	    
	    // Perform application initialization:
	    if (!InitInstance(hInstance, cmd_line))
	    {
		    return FALSE;
	    }
        }
		
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


BOOL InitInstance(HINSTANCE hInstance, LPTSTR lpCmdLine)
{
	HWND hWnd;
	WSADATA wsaData;
	
	g_hInst = hInstance; // Store instance handle in our global variable
	
#ifdef UNDER_CE
#warning
#warning code ifdefed for arm-wince-mingw32ce toolchain
#if LATER
	SHInitExtraControls();
#endif
#endif
	
	//If it is already running, then focus on the window, and exit
	hWnd = FindWindow(szWindowClass, NULL);	
	if (hWnd) 
	{
		// set focus to foremost child window
		// The "| 0x00000001" is used to bring any owned windows to the
		// foreground and activate them.
		SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
		return 0;
	} 
	
	if (!MyRegisterClass(hInstance, szWindowClass))
	{
		return FALSE;
	}
	
	if(WSAStartup(MAKEWORD(1,1), &wsaData) != 0)
	{
		roadmap_log (ROADMAP_FATAL, "Can't initialize network");
	}
	
	char *args[1] = {0};

	roadmap_start(0, args);
	
	return TRUE;
}


static int roadmap_main_char_key_pressed(HWND hWnd, WPARAM wParam,
										 LPARAM lParam)
{
	char *key = NULL;
	char regular_key[2];
	
#ifdef UNDER_CE
#warning
#warning code ifdefed for arm-wince-mingw32ce toolchain
#if LATER
	switch (wParam)
	{
	case VK_APP1:	key = "Button-App1";	   break;
	case VK_APP2:	key = "Button-App2";	   break;
	case VK_APP3:	key = "Button-App3";	   break;
	case VK_APP4:	key = "Button-App4";	   break;
   }
#endif
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
	case VK_LEFT:	key = "Button-Left";	   break;
	case VK_RIGHT:	key = "Button-Right";	break;
	case VK_UP:		key = "Button-Up";	   break;
	case VK_DOWN:	key = "Button-Down";	   break;
#ifdef UNDER_CE // mingw32ce doesn't provide
#warning
#warning code ifdefed for arm-wince-mingw32ce toolchain
#if LATER
	case VK_APP1:	key = "Button-App1";	   break;
	case VK_APP2:	key = "Button-App2";	   break;
	case VK_APP3:	key = "Button-App3";	   break;
	case VK_APP4:	key = "Button-App4";	   break;
#endif
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
	
#ifdef UNDER_CE
	static SHACTIVATEINFO s_sai;
#endif
	
#warning
#warning code ifdefed for arm-wince-mingw32ce toolchain
#if LATER
   __try 
#endif
   {
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
				(*callback)();
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
				(*callback)();
			} else {
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
		}
		break;
		
	case WM_CREATE:
		bool create_menu;
#ifdef _ROADGPS
      create_menu = true;
#else
      create_menu = roadmap_config_match (&RoadMapConfigMenuBar, "Yes") != 0;
#endif

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
#warning
#warning code ifdefed for arm-wince-mingw32ce toolchain
#if LATER
		SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
#endif
		break;
#endif
		
	case WM_USER_READ:
		{
			roadmap_main_io *context = (roadmap_main_io *) wParam;
			 if (!context->is_valid) break;

			 if (lParam != 1) {
			    Win32SerialConn *conn = (Win32SerialConn *) lParam;
			 
			    if (!ROADMAP_SERIAL_IS_VALID (conn)) {
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
    }
#warning
#warning code ifdefed for arm-wince-mingw32ce toolchain
#if LATER
    __except (handleException(GetExceptionInformation())) {}
#endif
	return 0;
}


static struct tb_icon RoadMapIcons[] = {
	{"destination", IDB_RM_DESTINATION},
	{"location", IDB_RM_LOCATION},
	{"gps", IDB_RM_GPS},
	{"hold", IDB_RM_HOLD},
	{"counterclockwise", IDB_RM_COUNTERCLOCKWISE},
	{"clockwise", IDB_RM_CLOCKWISE},
	{"zoomin", IDB_RM_ZOOMIN},
	{"zoomout", IDB_RM_ZOOMOUT},
	{"zoom1", IDB_RM_ZOOM1},
	{"full", IDB_RM_FULL},
	{"record", IDB_RM_RECORD},
	{"stop", IDB_RM_STOP},
	{"quit", IDB_RM_QUIT},
	{"down", IDB_RM_DOWN},
	{"up", IDB_RM_UP},
	{"right", IDB_RM_RIGHT},
	{"left", IDB_RM_LEFT},
	{NULL, 0}
};


static int roadmap_main_toolbar_icon (const char *icon)
{
	if (icon == NULL) return 0;
	
	tb_icon *itr = RoadMapIcons;
	
	while (itr->name != NULL) {
		if (!strcasecmp(icon, itr->name)) {
			return itr->id;
		}
		itr++;
	}
	
	return -1;
}


extern "C" {
	
	void roadmap_main_toggle_full_screen (void)
	{
		// TODO: implement
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
	}

	
	void roadmap_main_new (const char *title, int width, int height)
	{
		LPWSTR szTitle = ConvertToUNICODE(title);

      		roadmap_config_declare_enumeration
         		("preferences", &RoadMapConfigMenuBar, NULL, "No", "Yes", NULL);
#ifndef UNDER_CE
		style |= WS_OVERLAPPEDWINDOW;
#endif
		RoadMapMainWindow = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
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
	
	void roadmap_main_title(char *fmt, ...) {
		/* unimplemented */
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
		if (RoadMapMainMenuBar == NULL) return;
		
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
			AppendMenu(RoadMapCurrentSubMenu, MF_STRING,
				menu_id + MENU_ID_START, label_unicode);
			free(label_unicode);
			
			if (menu_id == MAX_MENU_ITEMS) {
				roadmap_log (ROADMAP_FATAL, "Too many menu items!");
				return;
			}
			menu_callbacks[menu_id] = callback;
			menu_id++;
		} else {
			AppendMenu((HMENU)menu, MF_SEPARATOR, 0, _T(""));
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
		
#ifdef XXX_UNDER_CE  // should use this with CE, but mingw32ce doesn't provide
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
		
#ifdef XXX_UNDER_CE  // should use this with CE, but mingw32ce doesn't provide
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
      		roadmap_main_toggle_full_screen ();
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
		
		for (i = 0; i < ROADMAP_MAX_IO; ++i) {
			if (RoadMapMainIo[i]->io == NULL) {
				RoadMapMainIo[i]->io = (RoadMapIO*)malloc(sizeof(RoadMapIO));
				*(RoadMapMainIo[i]->io) = *io;
				RoadMapMainIo[i]->callback = callback;
            			RoadMapMainIo[i]->is_valid = 1;
				break;
			}
		}
		
		if (i == ROADMAP_MAX_IO) {
			roadmap_log (ROADMAP_FATAL, "Too many set input calls");
			return;
		}
		
		HANDLE monitor_thread = NULL;
		switch (io->subsystem) {
		case ROADMAP_IO_SERIAL:
         		// io->os.serial->ref_count++;
			monitor_thread = CreateThread(NULL, 0,
				SerialMonThread, (void*)&RoadMapMainIo[i], 0, NULL);
			break;
		case ROADMAP_IO_NET:
			monitor_thread = CreateThread(NULL, 0,
				SocketMonThread, (void*)&RoadMapMainIo[i], 0, NULL);
			break;
		case ROADMAP_IO_FILE:
			monitor_thread = CreateThread(NULL, 0,
				FileMonThread, (void*)&RoadMapMainIo[i], 0, NULL);
			break;
		}
		
		if(monitor_thread == NULL)
		{
			roadmap_log (ROADMAP_FATAL, "Can't create monitor thread");
			roadmap_io_close(io);
			return;
		}
		else {
			CloseHandle(monitor_thread);
		}
	}
	
	
	void roadmap_main_remove_input (RoadMapIO *io)
	{
		int i;
				
		for (i = 0; i < ROADMAP_MAX_IO; ++i) {
			 if (RoadMapMainIo[i] && RoadMapMainIo[i]->io == io) {

				if (RoadMapMainIo[i]->is_valid) {
				   RoadMapMainIo[i]->is_valid = 0;
				} else {
				   free (RoadMapMainIo[i]);
				}
				RoadMapMainIo[i] = NULL;
				break;

			}
		}
	}
	
	
	static void roadmap_main_timeout (HWND hwnd, UINT uMsg, UINT_PTR idEvent,
		DWORD dwTime) 
	{	
		struct roadmap_main_timer *timer = RoadMapMainPeriodicTimer +
			(idEvent - 1);
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
				KillTimer(RoadMapMainWindow,
					RoadMapMainPeriodicTimer[index].id);
				
				return;
			}
		}
		
		roadmap_log (ROADMAP_ERROR, "timer 0x%08x not found", callback);
	}
	
	
	void roadmap_main_set_status (const char *text) {}
	
	void roadmap_main_flush (void)
        {
	      // HWND w = GetFocus();
	      MSG msg;

	      while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	      {
		 TranslateMessage(&msg);
		 DispatchMessage(&msg);
	      }

	      //UpdateWindow(w);
        }

       int roadmap_main_flush_synchronous (int deadline)
       {
	  return 1; /* Nothing to do. */
       }
	
	
	void roadmap_main_exit (void)
	{
		roadmap_start_exit ();
		SendMessage(RoadMapMainWindow, WM_CLOSE, 0, 0);
	}
	
       void roadmap_main_set_cursor (int cursor)
       {
	  switch (cursor) {

	  case ROADMAP_CURSOR_NORMAL:
	     SetCursor(NULL);
	     break;

	  case ROADMAP_CURSOR_WAIT:
	     SetCursor(LoadCursor(NULL, IDC_WAIT));
	     break;
	  }
       }
} // extern "C"

