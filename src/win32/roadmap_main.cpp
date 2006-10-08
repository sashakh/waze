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
#include <notify.h>
#include "CEException.h"
#include "CEDevice.h"
#endif

#ifndef I_IMAGENONE
#define I_IMAGENONE (-2)
#endif

#define MIN_SYNC_TIME 1*3600

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
#include "../roadmap_messagebox.h"
#include "../roadmap_screen.h"
#include "../roadmap_download.h"
#include "../roadmap_lang.h"
#include "../roadmap_dialog.h"
#include "../roadmap_gps.h"
#include "wince_input_mon.h"
#include "win32_serial.h"
#include "roadmap_wincecanvas.h"
#include "../editor/editor_main.h"
#include "../editor/export/editor_sync.h"
}


// Menu & toolbar defines

struct tb_icon {
	char *name;
	int id;
};
static	RoadMapCallback menu_callbacks[MAX_MENU_ITEMS] = {0};
static	RoadMapCallback tool_callbacks[MAX_TOOL_ITEMS] = {0};

// timer stuff
#define ROADMAP_MAX_TIMER 16
struct roadmap_main_timer {
	unsigned int id;
	RoadMapCallback callback;
};
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];

// IO stuff
#define ROADMAP_MAX_IO 16
static roadmap_main_io *RoadMapMainIo[ROADMAP_MAX_IO] = {0};

// varibles used across this module
static RoadMapKeyInput	RoadMapMainInput = NULL;
#ifdef UNDER_CE
static HWND				   RoadMapMainMenuBar = NULL;
#else
static HMENU			   RoadMapMainMenuBar = NULL;
#endif
static HMENU			   RoadMapCurrentSubMenu = NULL;
static HWND				   RoadMapMainToolbar = NULL;
static bool				   RoadMapMainFullScreen = false;
static HANDLE           VirtualSerialHandle = 0;
static const char *RoadMapMainVirtualSerial;
static bool             RoadMapMainSync = false;


// Global Variables:
extern "C" HINSTANCE	g_hInst = NULL;
extern "C" HWND			RoadMapMainWindow  = NULL;

// Forward declarations of functions included in this code module:
static ATOM				   MyRegisterClass(HINSTANCE, LPTSTR);
static BOOL				   InitInstance(HINSTANCE, LPTSTR);
static LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

#define MAX_LOADSTRING 100

// class name definition
#ifdef _ROADGPS
static WCHAR szWindowClass[] = L"RoadGPSClass";
static WCHAR szOtherWindowClass[] = L"RoadMapClass";
#else
static WCHAR szWindowClass[] = L"RoadMapClass";
static WCHAR szOtherWindowClass[] = L"RoadGPSClass";
#endif

static RoadMapConfigDescriptor RoadMapConfigGPSVirtual =
                        ROADMAP_CONFIG_ITEM("GPS", "Virtual");

static RoadMapConfigDescriptor RoadMapConfigMenuBar =
                        ROADMAP_CONFIG_ITEM("General", "Menu bar");

#ifdef UNDER_CE
static RoadMapConfigDescriptor RoadMapConfigAutoSync =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Auto sync");
static RoadMapConfigDescriptor RoadMapConfigLastSync =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Last sync");
#endif

static RoadMapConfigDescriptor RoadMapConfigFirstTime =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "First time");

static RoadMapConfigDescriptor RoadMapConfigUser =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "User Name");

static RoadMapConfigDescriptor RoadMapConfigPassword =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Password");

static void first_time_wizard (void);

static HANDLE g_hMutexAppRunning;

static BOOL AppInstanceExists()
{
   BOOL bAppRunning = FALSE;

   g_hMutexAppRunning = CreateMutex( NULL, FALSE, 
                                     L"Global\\FreeMap RoadMap");

   if (( g_hMutexAppRunning != NULL ) && 
         ( GetLastError() == ERROR_ALREADY_EXISTS))
   {

      CloseHandle( g_hMutexAppRunning );
      g_hMutexAppRunning = NULL;
   }

   return ( g_hMutexAppRunning == NULL );
}


#ifndef _ROADGPS
#ifdef UNDER_CE
static int roadmap_main_should_sync (void) {

   roadmap_config_declare
      ("session", &RoadMapConfigLastSync, "0");

   if (roadmap_config_match(&RoadMapConfigAutoSync, "No")) {
      return 0;
   } else {
      unsigned int last_sync_time =
         roadmap_config_get_integer(&RoadMapConfigLastSync);

      if (last_sync_time && 
         ((last_sync_time + MIN_SYNC_TIME) > time(NULL))) {

         return 0;
      }

      roadmap_config_set_integer (&RoadMapConfigLastSync, time(NULL));
      roadmap_config_save (0);
   }

   return 1;
}
#endif

static void roadmap_main_start_sync (void) {

   struct hostent *h;
   int i;

   if (RoadMapMainSync) return;

   RoadMapMainSync = true;

   Sleep(1000);

   for (i=0; i<5; i++) {
      if ((h = gethostbyname ("ppp_peer")) != NULL) {
         export_sync ();
         break;
      }
      Sleep(1000);
   }

   RoadMapMainSync = false;
}
#endif

static void setup_virtual_serial (void) {

#ifdef UNDER_CE
   DWORD index;
   DWORD resp;
	HKEY key;

   const char *virtual_port = roadmap_config_get (&RoadMapConfigGPSVirtual);

   if (strlen(virtual_port) < 5) return;
   if (strncmp(virtual_port, "COM", 3)) return;

   index = atoi (virtual_port + 3);

   if ((index < 0) || (index > 9)) return;

#ifdef _ROADGPS
   if (FindWindow(szOtherWindowClass, NULL) != NULL) {
      /* RoadMap or RoadGPS is already running */
      RoadMapMainVirtualSerial = virtual_port;
      return;
   }
#endif

	RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\RoadMap",
		0, NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &key, &resp);
	RegSetValueEx(key, L"Dll", 0, REG_SZ, (unsigned char*)L"ComSplit.dll", 26);
	RegSetValueEx(key, L"Prefix", 0, REG_SZ, (unsigned char*)L"COM", 8);
	RegSetValueEx(key, L"Index", 0, REG_DWORD, (unsigned char*)&index, sizeof(DWORD));

	RegCloseKey(key);
	
	//res = RegisterDevice(L"COM", 4, L"ComSplit.dll", 0);
	VirtualSerialHandle = ActivateDevice(L"Drivers\\RoadMap", NULL);

   if (VirtualSerialHandle == 0) {
      roadmap_messagebox ("Virtual comm Error!", "Can't setup virtual serial port.");
   }
#endif
}


const char *roadmap_main_get_virtual_serial (void) {
   return RoadMapMainVirtualSerial;
}

int handleException(EXCEPTION_POINTERS *exceptionPointers) {

#ifdef UNDER_CE
        CEException::writeException(TEXT("\\roadmapCrash"), exceptionPointers);
#endif
        exit(0);

        return EXCEPTION_EXECUTE_HANDLER;
}


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

#ifdef WIN32_PROFILE
   //SuspendCAPAll();
#endif

   __try 
   {
      // Perform application initialization:
	   if (!InitInstance(hInstance, cmd_line))
	   {
		   return FALSE;
	   }

      setup_virtual_serial ();

   }
   __except (handleException(GetExceptionInformation())) {}

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

#ifdef UNDER_CE
   if (VirtualSerialHandle != 0) {
      DeactivateDevice (VirtualSerialHandle);
   }
#endif

      
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
   BOOL do_sync = false;
	
	g_hInst = hInstance; // Store instance handle in our global variable
	
#ifdef UNDER_CE
	SHInitExtraControls();

   if (!wcscmp(lpCmdLine, APP_RUN_AT_RS232_DETECT) ||
       !wcscmp(lpCmdLine, APP_RUN_AFTER_SYNC)) {
      do_sync = true;
   }

#endif
   
   BOOL other_instance = AppInstanceExists ();

	//If it is already running, then focus on the window, and exit
	hWnd = FindWindow(szWindowClass, NULL);	
	if (hWnd) 
	{
      if (do_sync) {
         PostMessage(hWnd, WM_USER_SYNC, 0, 0);
         return 0;
      } else {
		   // set focus to foremost child window
		   // The "| 0x00000001" is used to bring any owned windows to the
		   // foreground and activate them.
		   SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
		   return 0;
      }
	} 
	
   if (other_instance) return 0;

	if (!MyRegisterClass(hInstance, szWindowClass))
	{
		return FALSE;
	}
	
	if(WSAStartup(MAKEWORD(1,1), &wsaData) != 0)
	{
		roadmap_log (ROADMAP_FATAL, "Can't initialize network");
	}
	
	char *args[1] = {0};

#ifdef UNDER_CE
#ifndef _ROADGPS
   if (do_sync) {
      roadmap_config_initialize ();
      roadmap_config_declare_enumeration
         ("preferences", &RoadMapConfigAutoSync, "Yes", "No", NULL);    

      if (!roadmap_main_should_sync ()) return 0;
      roadmap_lang_initialize ();
      roadmap_start_set_title (roadmap_lang_get ("RoadMap"));
      roadmap_download_initialize ();
      editor_main_initialize ();
      editor_main_set (1);
      roadmap_main_start_sync ();      
      return 0;
   }
#endif
#endif


	roadmap_start(0, args);
	
#ifndef _ROADGPS
#ifdef FREEMAP_IL

   roadmap_config_declare_enumeration
      ("session", &RoadMapConfigFirstTime, "Yes", "No", NULL);    

   if (roadmap_config_match(&RoadMapConfigFirstTime, "Yes")) {
      first_time_wizard();
   }

#endif
#endif
	return TRUE;
}


static int roadmap_main_char_key_pressed(HWND hWnd, WPARAM wParam,
										 LPARAM lParam)
{
	char *key = NULL;
	char regular_key[2];

#ifdef UNDER_CE
	switch (wParam)
	{
	case VK_APP1:	key = "Button-App1";	   break;
	case VK_APP2:	key = "Button-App2";	   break;
	case VK_APP3:	key = "Button-App3";	   break;
	case VK_APP4:	key = "Button-App4";	   break;
   }
#endif

  if ((key == NULL) && (wParam > 0 && wParam < 128)) {
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


static void CALLBACK AvoidSuspend (HWND hwnd, UINT uMsg, UINT idEvent,
                                   DWORD dwTime) {
#ifdef UNDER_CE
   CEDevice::wakeUp();
#endif
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
#ifdef UNDER_CE
	case VK_APP1:	key = "Button-App1";	   break;
	case VK_APP2:	key = "Button-App2";	   break;
	case VK_APP3:	key = "Button-App3";	   break;
	case VK_APP4:	key = "Button-App4";	   break;
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
	
   __try {
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
      CEDevice::init();

      SetTimer (NULL, 0, 50000, AvoidSuspend);
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
      CEDevice::end();
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
			roadmap_canvas_mouse_moved(&point);
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
      if ((LOWORD(wParam)!=WA_INACTIVE) &&
            RoadMapMainFullScreen) {
         SHFullScreen(RoadMapMainWindow, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON);
         MoveWindow(RoadMapMainWindow, 0, 0,
            GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
            TRUE);
      } else {
		   // Notify shell of our activate message
		   SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
      }
		break;
      
	case WM_SETTINGCHANGE:
		//SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
		break;
#endif

#ifdef UNDER_CE
#ifndef _ROADGPS
   case WM_KILLFOCUS:
      //roadmap_screen_freeze ();
      break;

   case WM_SETFOCUS:
      //roadmap_screen_unfreeze ();
      break;
#endif
#endif

#ifndef _ROADGPS
	case WM_USER_SYNC:
      roadmap_main_start_sync ();
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
__except (handleException(GetExceptionInformation())) {}
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
		LPWSTR szTitle = ConvertToWideChar(title, CP_UTF8);
		DWORD style = WS_VISIBLE;

      roadmap_config_declare_enumeration
         ("preferences", &RoadMapConfigMenuBar, "No", "Yes", NULL);
#ifdef UNDER_CE
      roadmap_config_declare_enumeration
         ("preferences", &RoadMapConfigAutoSync, "Yes", "No", NULL);
      roadmap_config_declare
         ("session", &RoadMapConfigLastSync, "0");
#else
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

#ifdef FREEMAP_IL
      editor_main_check_map ();
      editor_main_set (1);
#endif
   }
	
	
	void roadmap_main_set_keyboard
      (struct RoadMapFactoryKeyMap *bindings, RoadMapKeyInput callback)
	{

#ifdef UNDER_CE
      const struct RoadMapFactoryKeyMap *binding;

      for (binding = bindings; binding->key != NULL; ++binding) {

         if (!strncmp(binding->key, "Button-App", 10)) {

            int id = atoi(binding->key + 10);

            if ((id <= 0) || (id > 4)) continue;

            SHSetAppKeyWndAssoc(VK_APP1 + id - 1, RoadMapMainWindow);
         }
      }
#endif

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
			AppendMenu((HMENU)menu, MF_SEPARATOR, 0, L"");
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
			LONG style = GetWindowLong (RoadMapMainToolbar, GWL_STYLE);
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
		
#ifdef UNDER_CE
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
      roadmap_main_toggle_full_screen ();
#endif
      
      roadmap_canvas_new(RoadMapMainWindow, RoadMapMainToolbar);

#ifdef FREEMAP_IL
      RoadMapImage image = roadmap_canvas_load_image (roadmap_path_user(),
                           "icons\\welcome.bmp");

	  if (image) {
		  RoadMapGuiPoint pos;
		  pos.x = (roadmap_canvas_width () - roadmap_canvas_image_width(image)) / 2;
		  pos.y = (roadmap_canvas_height () - roadmap_canvas_image_height(image)) / 2;
		  roadmap_canvas_draw_image (image, &pos, 0, IMAGE_NORMAL);
		  roadmap_canvas_free_image (image);
		  roadmap_canvas_refresh ();
		  Sleep (500);

		  const char *message = roadmap_lang_get ("Loading please wait...");
		  RoadMapPen pen = roadmap_canvas_create_pen ("splash");
		  roadmap_canvas_set_foreground ("red");

		  int width;
		  int ascent;
		  int descent;

		  int current_x;

		  if (roadmap_lang_rtl ()) {
			 current_x = roadmap_canvas_width () - pos.x - 10;
		  } else {
			 current_x = pos.x + 10;
		  }

		  for (unsigned int i=0; i<strlen(message); i++) {
			 char str[3] = {0};
			 RoadMapGuiPoint position = {current_x, 300};

			 for (int j=0; j<3; j++) {
				str[j] = message[i];
				if (str[j] != -41) break;
				i++;
			 }
         
			 roadmap_canvas_get_text_extents
				(str, -1, &width, &ascent, &descent, NULL);

			 if (roadmap_lang_rtl ()) {
				current_x -= width;
				position.x = current_x;
			 } else {
				current_x += width;
			 }

			 roadmap_canvas_draw_string_angle (&position, &position, 0, str);
			 roadmap_canvas_refresh ();

			 Sleep (30);
		  }

		  Sleep (1000);
	  }
#endif
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
			if (RoadMapMainIo[i] == NULL) {
            RoadMapMainIo[i] = (roadmap_main_io *) malloc (sizeof(roadmap_main_io));
				RoadMapMainIo[i]->io = io;
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
         io->os.serial->ref_count++;
			monitor_thread = CreateThread(NULL, 0,
				SerialMonThread, (void*)RoadMapMainIo[i], 0, NULL);
			break;
		case ROADMAP_IO_NET:
			monitor_thread = CreateThread(NULL, 0,
				SocketMonThread, (void*)RoadMapMainIo[i], 0, NULL);
			break;
		case ROADMAP_IO_FILE:
			monitor_thread = CreateThread(NULL, 0,
				FileMonThread, (void*)RoadMapMainIo[i], 0, NULL);
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
	
	
	static void roadmap_main_timeout (HWND hwnd, UINT uMsg, UINT idEvent,
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
			(TIMERPROC)roadmap_main_timeout);
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
      HWND w = GetFocus();
      MSG msg;

      while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

      //UpdateWindow(w);
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

/* first time wizard */

static void wizard_close (const char *name, void *context) {

   roadmap_config_set (&RoadMapConfigUser,
      (char *)roadmap_dialog_get_data ("main", "User Name"));

   roadmap_config_set (&RoadMapConfigPassword,
      (char *)roadmap_dialog_get_data ("main", "Password"));

   roadmap_config_set (&RoadMapConfigFirstTime, "No");

   roadmap_dialog_hide (name);
}

static void wizard_detect_gps (const char *name, void *context) {
   roadmap_gps_detect_receiver ();
}

void first_time_wizard (void) {
   if (roadmap_dialog_activate ("Setup wizard", NULL)) {

      roadmap_config_declare ("preferences", &RoadMapConfigUser, "");
      roadmap_config_declare_password ("preferences", &RoadMapConfigPassword, "");
      roadmap_dialog_new_label  ("main", ".Welcome to FreeMap!");
      roadmap_dialog_new_label  ("main", ".Enter your user name if you registered one.");
      roadmap_dialog_new_label  ("main", ".Otherwise leave these fields empty.");
      roadmap_dialog_new_entry  ("main", "User Name", NULL);
      roadmap_dialog_new_password  ("main", "Password");
      roadmap_dialog_add_button ("Ok", wizard_close);
      roadmap_dialog_add_button ("Detect GPS", wizard_detect_gps);

      roadmap_dialog_complete (0);
   }

   roadmap_dialog_set_data ("main", "User Name",
         roadmap_config_get (&RoadMapConfigUser));
   roadmap_dialog_set_data ("main", "Password",
         roadmap_config_get (&RoadMapConfigPassword));

}

