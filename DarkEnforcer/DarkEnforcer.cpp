#include "framework.h"
#include "DarkEnforcer.h"
#include <shellapi.h>
#include <commctrl.h>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"" \
    "type='win32' " \
    "name='Microsoft.Windows.Common-Controls' " \
    "version='6.0.0.0' " \
    "processorArchitecture='*' "  \
    "publicKeyToken='6595b64144ccf1df' " \
    "language='*'\"")





enum IMMERSIVE_HC_CACHE_MODE
{
    IHCM_USE_CACHED_VALUE,
    IHCM_REFRESH
};

// 1903 18362
enum PreferredAppMode
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

enum WINDOWCOMPOSITIONATTRIB
{
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

using fnAllowDarkModeForWindow = bool (WINAPI*)(HWND hWnd, bool allow); // ordinal 133
using fnSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);


fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;








#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK       EnumWindowsProc(HWND hWnd, LPARAM lParam);

NOTIFYICONDATA nid;
HHOOK keyboardHook = NULL;
HWND g_hwnd = NULL;

// Callback function for EnumWindows to apply dark mode to all windows
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
    if (_SetWindowCompositionAttribute != nullptr) {
        _SetWindowCompositionAttribute(hWnd, (WINDOWCOMPOSITIONATTRIBDATA*)lParam);
    }
    return TRUE; // Continue enumeration
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYUP) {
        KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *)lParam;
        
        // Check for Alt+B combination
        if ((pkbhs->vkCode == 'B') && 
            (GetAsyncKeyState(VK_MENU) & 0x8000)) {
            
            // Exit the application
            PostMessage(g_hwnd, WM_CLOSE, 0, 0);
            return 1; // Block the key event
        }
    }
    
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DARKENFORCER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Create a hidden window to handle messages
    nCmdShow = SW_HIDE; // Start hidden

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    // Add icon to system tray
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = g_hwnd;
    nid.uID = 1001;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DARKENFORCER));
    wcscpy_s(nid.szTip, L"Dark Enforcer - Right-click to exit");

    Shell_NotifyIcon(NIM_ADD, &nid);

    // Install low-level keyboard hook for Alt+B
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DARKENFORCER));

    // Load the DLL and set up hooks
    HINSTANCE hinstDLL = LoadLibrary(_T("DarkDll.dll"));
    HOOKPROC DarkHookProc = (HOOKPROC)GetProcAddress(hinstDLL, "DarkHookProc");
    HOOKPROC DarkHookProcRet = (HOOKPROC)GetProcAddress(hinstDLL, "DarkHookProcRet");
    HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC, DarkHookProc, hinstDLL, 0);
    HHOOK hookRet = SetWindowsHookEx(WH_CALLWNDPROCRET, DarkHookProcRet, hinstDLL, 0);

    _SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

    BOOL dark = TRUE;
    WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
    
    // Apply dark mode initially to all existing windows
    EnumWindows(EnumWindowsProc, (LPARAM)&data);

    MSG msg;

    // Continue updating windows in a timer-based approach
    SetTimer(g_hwnd, 1, 33, NULL); // Timer ID 1, fires every 33ms

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Cleanup
    KillTimer(g_hwnd, 1);
    Shell_NotifyIcon(NIM_DELETE, &nid);
    UnhookWindowsHookEx(keyboardHook);
    UnhookWindowsHookEx(hookRet);
    UnhookWindowsHookEx(hook);

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DARKENFORCER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DARKENFORCER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // Store the window handle globally
   g_hwnd = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        HWND btn = CreateWindow(L"Button", L"My button", WS_CHILD | WS_TABSTOP | WS_VISIBLE, 11, 11, 111, 55, hWnd, (HMENU)13, hInst, 0);
    }
        break;
    case WM_TIMER:
        {
            // Handle periodic update of dark mode for all windows
            if (wParam == 1) // Timer ID 1
            {
                fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
                
                BOOL dark = TRUE;
                WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
                EnumWindows(EnumWindowsProc, (LPARAM)&data);
            }
        }
        break;
    case WM_USER+1: // System tray notification
        {
            if (lParam == WM_RBUTTONUP) // Right click
            {
                POINT pt;
                GetCursorPos(&pt);
                
                HMENU hMenu = CreatePopupMenu();
                AppendMenu(hMenu, MF_STRING, 1001, L"Exit");
                
                // Make the window track mouse messages to dismiss the menu
                SetForegroundWindow(hWnd);
                
                UINT clicked = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
                
                if (clicked == 1001) // Exit selected
                {
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                }
                
                DestroyMenu(hMenu);
            }
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSTATIC:
    {/*
        {
            COLORREF darkTextColor = RGB(0, 255, 255);
            COLORREF darkBkColor = RGB(255, 1, 1);
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, darkTextColor);
            SetBkColor(hdc, darkBkColor);
            static HBRUSH hbrBkgnd = 0;
            if (!hbrBkgnd)
                hbrBkgnd = (HBRUSH)GetStockObject(BLACK_BRUSH); //hbrBkgnd = CreateSolidBrush(darkBkColor);
            //UpdateWindow(msg->hwnd);
            return reinterpret_cast<INT_PTR>(hbrBkgnd);
        }*/
    }
    break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
