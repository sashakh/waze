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
#include <aygshell.h>
#include "resource.h"
#include <Winsock.h>

#ifdef WIN32_PROFILE
#include <C:\Program Files\Windows CE Tools\Common\Platman\sdk\wce500\include\cecap.h>
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
#include "../roadmap_serial.h"
#include "wince_input_mon.h"
#include "win32_serial.h"
#include "roadmap_wincecanvas.h"
}


// Menu & toolbar defines

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
static roadmap_main_io RoadMapMainIo[ROADMAP_MAX_IO] = {0};

// varibles used across this module
static RoadMapKeyInput	RoadMapMainInput = NULL;
static HWND				RoadMapMainMenuBar = NULL;
static HMENU			RoadMapCurrentSubMenu = NULL;
static HWND				RoadMapMainToolbar = NULL;

// Global Variables:
extern "C" HINSTANCE	g_hInst = NULL;
extern "C" HWND			RoadMapMainWindow  = NULL;

// Forward declarations of functions included in this code module:
static ATOM				MyRegisterClass(HINSTANCE, LPTSTR);
static BOOL				InitInstance(HINSTANCE);
static LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

#define MAX_LOADSTRING 100

// class name definition
#ifdef _ROADGPS
static TCHAR szWindowClass[] = _T("RoadGPSClass");
#else
static TCHAR szWindowClass[] = _T("RoadMapClass");
#endif

// our main function
int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPTSTR    lpCmdLine,
				   int       nCmdShow)
{
	MSG msg;
	
	// Perform application initialization:
	if (!InitInstance(hInstance))
	{
		return FALSE;
	}

#ifdef WIN32_PROFILE
   SuspendCAPAll();
#endif

/*
   DWORD disp;
	HKEY key;

	LONG res_key = RegCreateKeyEx(HKEY_LOCAL_MACHINE, _T("RoadMap"),
		0, NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &key, &disp);
	RegSetValueEx(key, _T("Dll"), 0, REG_SZ, (unsigned char*)_T("ComSplit.dll"), 26);
	RegSetValueEx(key, _T("Prefix"), 0, REG_SZ, (unsigned char*)_T("COM"), 8);
	disp = 4;
	RegSetValueEx(key, _T("Index"), 0, REG_DWORD, (unsigned char*)&disp, sizeof(DWORD));

	RegCloseKey(key);
	
	//res = RegisterDevice(_T("COM"), 4, _T("ComSplit.dll"), 0);
	HANDLE res = ActivateDevice(_T("RoadMap"), NULL);

   res = res;
*/
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


BOOL InitInstance(HINSTANCE hInstance)
{
	HWND hWnd;
	WSADATA wsaData;
	
	g_hInst = hInstance; // Store instance handle in our global variable
	
	SHInitExtraControls();
	
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
	
	if (wParam > 0 && wParam < 128) {
		regular_key[0] = wParam;
		regular_key[1] = 0;
		key = regular_key;
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
	case VK_LEFT:	key = "J";	break;
	case VK_RIGHT:	key = "K";	break;
	case VK_UP:		key = "+";	break;
	case VK_DOWN:	key = "-";	break;
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
	
	static SHACTIVATEINFO s_sai;
	
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
		if (!SHCreateMenuBar(&mbi)) 
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
		
		// Initialize the shell activate info structure
		memset(&s_sai, 0, sizeof (s_sai));
		s_sai.cbSize = sizeof (s_sai);
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
			DestroyWindow(RoadMapMainMenuBar);
		}
		
		if (RoadMapMainToolbar != NULL) {
			CommandBar_Destroy(RoadMapMainToolbar);
		}
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
			roadmap_canvas_button_pressed(&point);
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
			roadmap_canvas_button_released(&point);
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
				
	case WM_KEYDOWN:
		if (roadmap_main_vkey_pressed(hWnd, wParam, lParam)) return 0;
		else return DefWindowProc(hWnd, message, wParam, lParam);
		break;
		
	case WM_CHAR:
		if (roadmap_main_char_key_pressed(hWnd, wParam, lParam)) return 0;
		else return DefWindowProc(hWnd, message, wParam, lParam);
		break;
		
	case WM_ACTIVATE:
		// Notify shell of our activate message
		SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
		break;
		
	case WM_SETTINGCHANGE:
		SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
		break;
		
	case WM_USER_READ:
		{
			roadmap_main_io *context = (roadmap_main_io *) wParam;

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
	if (icon == NULL) return NULL;
	
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
	}

	
	void roadmap_main_new (const char *title, int width, int height)
	{
		LPWSTR szTitle = ConvertToWideChar(title, CP_UTF8);
		RoadMapMainWindow = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
			NULL, g_hInst, NULL);
		
		free(szTitle);
		if (!RoadMapMainWindow)
		{
			roadmap_log (ROADMAP_FATAL, "Can't create main window");
			return;
		}
		
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
	}
	
	
	void roadmap_main_set_keyboard (RoadMapKeyInput callback)
	{
		RoadMapMainInput = callback;
	}


   RoadMapMenu roadmap_main_new_menu (void)
   {
      return CreatePopupMenu();
   }

   
	void roadmap_main_add_menu (RoadMapMenu menu, const char *label)
	{
		static int menu_id = 0;
		if (RoadMapMainMenuBar == NULL) return;
		
		LPWSTR label_unicode = ConvertToWideChar(label, CP_UTF8);
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
		free(label_unicode);
	}
	
	
	void roadmap_main_add_menu_item (RoadMapMenu menu, const char *label,
      const char *tip, RoadMapCallback callback)
	{
		static int menu_id = 0;
		
		if (label != NULL) {
			LPWSTR label_unicode = ConvertToWideChar(label, CP_UTF8);
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
			AppendMenu((HMENU)menu, MF_SEPARATOR, 0, _T(""));
		}
		
		if (tip != NULL) {
			//TODO: does wince support it?
		}
	}
	
   void roadmap_main_popup_menu (RoadMapMenu menu, int x, int y) {

      TrackPopupMenuEx( (HMENU)menu, 0, x, y, RoadMapMainWindow, NULL);
   }

	void roadmap_main_add_separator (RoadMapMenu menu)
	{
		roadmap_main_add_menu_item (menu, NULL, NULL, NULL);
	}
	
	void roadmap_main_add_tool (const char *label,
		const char *icon,
		const char *tip,
		RoadMapCallback callback)
	{
		static int tool_id = 0;
		
		if (RoadMapMainToolbar == NULL) {
			RoadMapMainToolbar = CommandBar_Create (g_hInst,
				RoadMapMainWindow, 1);
			LONG style = GetWindowLong (RoadMapMainToolbar, GWL_STYLE);
		}
		
		int icon_res_id = roadmap_main_toolbar_icon(icon);
		if (icon_res_id == -1) return;
		
		int icon_tb_id = CommandBar_AddBitmap(RoadMapMainToolbar, g_hInst,
			icon_res_id, 1, 16, 16);
		
		TBBUTTON but;
		but.iBitmap = icon_tb_id;
		but.idCommand = TOOL_ID_START + tool_id;
		but.fsState = TBSTATE_ENABLED;
		but.fsStyle = 0;
		but.dwData = 0;
		but.iString = 0;
		
		int res = CommandBar_AddButtons(RoadMapMainToolbar, 1, &but);
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
		
		int res = CommandBar_AddButtons(RoadMapMainToolbar, 1, &but);
		
	}
	
	
	void roadmap_main_add_canvas (void)
	{
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
			if (RoadMapMainIo[i].io == NULL) {
				RoadMapMainIo[i].io = io;
				RoadMapMainIo[i].callback = callback;
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
         io->os.serial->ref_count++;
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
         if (RoadMapMainIo[i].io == io) {

				RoadMapMainIo[i].io = NULL;
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
      UpdateWindow(GetFocus());
   }
	
	
	void roadmap_main_exit (void)
	{
		roadmap_start_exit ();
		SendMessage(RoadMapMainWindow, WM_CLOSE, 0, 0);
	}
	
} // extern "C"

