// footkey.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "footkey.h"
#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <vector>

#define MAX_LOADSTRING 100
#define _WIN32_WINNT 0x050

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// our main window
HWND main_hWnd;

bool isPressed = false;

bool a_is_down = false;
bool s_is_down = false;
bool d_is_down = false;
bool w_is_down = false;

bool a_is_down_change = false;
bool s_is_down_change = false;
bool d_is_down_change = false;
bool w_is_down_change = false;

int a_ticker = 0;
int s_ticker = 0;
int d_ticker = 0;
int w_ticker = 0;

DWORD last_a_up = 0;
DWORD last_s_up = 0;
DWORD last_d_up = 0;
DWORD last_w_up = 0;

bool needs_draw = false;

unsigned getbits(unsigned value, unsigned offset, unsigned n) {
  const unsigned max_n = CHAR_BIT * sizeof(unsigned);
  if (offset >= max_n)
    return 0; /* value is padded with infinite zeros on the left */
  value >>= offset; /* drop offset bits */
  if (n >= max_n)
    return value; /* all  bits requested */
  const unsigned mask = (1u << n) - 1; /* n '1's */
  return value & mask;
}

std::map<DWORD, DWORD> key_down_state;
std::map<DWORD, DWORD> key_up_state;
std::map<DWORD, bool> last_key_state;

// 0x41 // a
// 0x53 // s
// 0x44 // d
// 0x57 // w
// 0x60 // num pad 0
// 0x61 // num pad 1
// 0xA2 // Left Control

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	BOOL fEatKeystroke = FALSE;

	if (nCode == HC_ACTION) {
		PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT) lParam;

		switch (wParam) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
            key_down_state[p->vkCode] = p->time;
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
            key_up_state[p->vkCode] = p->time;
            break;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL LoadBitmapFromBMPFile(
    LPCWSTR szFileName,
    HBITMAP *phBitmap,
    HPALETTE *phPalette
) {
   BITMAP bm;

   *phBitmap = NULL;
   *phPalette = NULL;

   // Use LoadImage() to get the image loaded into a DIBSection
   *phBitmap = (HBITMAP) LoadImage(
        NULL,
        szFileName,
        IMAGE_BITMAP,
        0,
        0,
        LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE
    );

    DWORD dLastError = GetLastError();

    if (*phBitmap == NULL) {
        return FALSE;
    }

    // Get the color depth of the DIBSection
    GetObject(*phBitmap, sizeof(BITMAP), &bm);

    // If the DIBSection is 256 color or less, it has a color table
    if ((bm.bmBitsPixel * bm.bmPlanes) <= 8) {
        HDC           hMemDC;
        HBITMAP       hOldBitmap;
        RGBQUAD       rgb[256];
        LPLOGPALETTE  pLogPal;
        WORD          i;

        // Create a memory DC and select the DIBSection into it
        hMemDC = CreateCompatibleDC( NULL );
        hOldBitmap = (HBITMAP)SelectObject( hMemDC, *phBitmap );
        // Get the DIBSection's color table
        GetDIBColorTable( hMemDC, 0, 256, rgb );
        // Create a palette from the color tabl
        pLogPal = (LOGPALETTE *)malloc( sizeof(LOGPALETTE) + (256*sizeof(PALETTEENTRY)) );
        pLogPal->palVersion = 0x300;
        pLogPal->palNumEntries = 256;

        for (i=0; i<256; i++) {
            pLogPal->palPalEntry[i].peRed = rgb[i].rgbRed;
            pLogPal->palPalEntry[i].peGreen = rgb[i].rgbGreen;
            pLogPal->palPalEntry[i].peBlue = rgb[i].rgbBlue;
            pLogPal->palPalEntry[i].peFlags = 0;
        }

       *phPalette = CreatePalette(pLogPal);
       // Clean up
       free(pLogPal);
       SelectObject(hMemDC, hOldBitmap);
       DeleteDC(hMemDC);
    } else { // It has no color table, so use a halftone palette
        HDC hRefDC;
        hRefDC = GetDC(NULL);
        *phPalette = CreateHalftonePalette( hRefDC );
        ReleaseDC(NULL, hRefDC);
    }

    return TRUE;
}

std::wstring ExePath() {
    wchar_t buffer[MAX_PATH]; 
    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::wstring ws(buffer);
    std::string str(ws.begin(), ws.end());
    std::string::size_type pos = str.find_last_of( "\\/" );
    str = str.substr(0, pos);
    return std::wstring(str.begin(), str.end());
}

std::wstring get_key_img_path(const std::wstring &str) {
    std::wstring p = ExePath() + std::wstring(L"\\") + str;
    return p;
}


class bitmap_t {
public:
    BITMAP bm;
    HBITMAP hBitmap, hOldBitmap;
    HPALETTE hPalette, hOldPalette;
    HDC hdc, hMemDC;
    bool loaded;
    int x = 0;
    int y = 0;

    bitmap_t(const std::wstring &file_name, HDC hdc_) : hBitmap(NULL) {
        hdc = hdc_;
        LoadBitmapFromBMPFile(file_name.c_str(), &hBitmap, &hPalette);
        GetObject(hBitmap, sizeof(BITMAP), &bm);
    }

    ~bitmap_t() {
        SelectObject(hMemDC, hOldBitmap);
        DeleteObject(hBitmap);
        SelectPalette(hdc, hOldPalette, FALSE);
        DeleteObject(hPalette);
    }

    void draw() {
        hMemDC = CreateCompatibleDC(hdc);
        hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
        hOldPalette = SelectPalette(hdc, hPalette, FALSE);
        RealizePalette(hdc);

        BitBlt(
            hdc,
            x,
            y,
            bm.bmWidth,
            bm.bmHeight,
            hMemDC,
            0,
            0,
            SRCCOPY
        );
    }

    auto width() {
        return bm.bmWidth;
    }

    auto height() {
        return bm.bmHeight;
    }
};

const int KEY_TIMER = 0;

// static const DWORD registered_keys_arr[] = {
//     0x41, // a
//     0x53, // s
//     0x44, // d
//     0x57, // w
//     0x60, // num pad 0
//     0x61, // num pad 1
//     0xA2 // Left Control
// };

std::vector<DWORD> registered_keys = {
    0x41, // a
    0x53, // s
    0x44, // d
    0x57, // w
    0x60, // num pad 0
    0x61, // num pad 1
    0xA3, // Right Control
    0x25, // left arrow
    0x26, // up arrow
    0x27, // rigt arrow
    0x28  // down arrow
};

std::map<char, DWORD> key_name_to_code = {
    { 'a', 0x25 },
    { 's', 0x28 },
    { 'd', 0x27 },
    { 'w', 0x26 },
    { '0', 0x60 },
    { '1', 0x61 },
    { 'c', 0xA3 }
};

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow
) {
    isPressed = false;
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_FOOTKEY, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FOOTKEY));
    HHOOK hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    MSG msg;

    SetTimer(
        main_hWnd, 
        KEY_TIMER, 
        30, 
        (TIMERPROC) NULL
    );

    // Main message loop:
    while (true) {
        if (!GetMessage(&msg, nullptr, 0, 0)) {
            break;
        }

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnhookWindowsHookEx(hhkLowLevelKybd);
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FOOTKEY));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    // wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_FOOTKEY);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

// Get the horizontal and vertical screen sizes in pixel
void GetDesktopResolution(int& horizontal, int& vertical)
{
   RECT desktop;
   // Get a handle to the desktop window
   const HWND hDesktop = GetDesktopWindow();
   // Get the size of screen to the variable desktop
   GetWindowRect(hDesktop, &desktop);
   // The top left corner will have coordinates (0,0)
   // and the bottom right corner will have coordinates
   // (horizontal, vertical)
   horizontal = desktop.right;
   vertical = desktop.bottom;
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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance; // Store instance handle in our global variable

    // bitmap_t a_key(get_key_img_path(L"a_down.bmp"), NULL);
    // bitmap_t s_key(get_key_img_path(L"s_down.bmp"), NULL);
    // bitmap_t d_key(get_key_img_path(L"d_down.bmp"), NULL);
    // bitmap_t w_key(get_key_img_path(L"w_down.bmp"), NULL);
    // bitmap_t c_key(get_key_img_path(L"c_down.bmp"), NULL);
    // bitmap_t n0_key(get_key_img_path(L"0_down.bmp"), NULL);
    // bitmap_t n1_key(get_key_img_path(L"1_down.bmp"), NULL);

    main_hWnd = CreateWindow(
        szWindowClass,
        0,
        WS_VISIBLE,
        5, // X pos
        5, // Y pos
        500, // width
        400, // height
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!main_hWnd) {
        return FALSE;
    }

    SetWindowLong(main_hWnd, GWL_STYLE, 0);
    SetWindowLong(main_hWnd, GWL_EXSTYLE, GetWindowLong(main_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(main_hWnd, RGB(255,0,0), 0, LWA_COLORKEY);
    HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
    SetClassLongPtr(main_hWnd, GCLP_HBRBACKGROUND, (LONG)brush);
    SetWindowPos(main_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ShowWindow(main_hWnd, nCmdShow);
    UpdateWindow(main_hWnd);

   return TRUE;
}

// std::stringstream ss;
// ss << "UP TIME " << up_diff << std::endl;
// ss << "DOWN TIME " << down_diff << std::endl;
// ss << "DIFF TIME " << down_diff - up_diff << std::endl;
// std::string str(ss.str());
// std::wstring out(str.begin(), str.end());
// OutputDebugString(out.c_str());

bool is_key_down(DWORD key_code) {
    if (key_up_state.count(key_code) > 0) {
        int up_diff = (GetMessageTime() - key_up_state[key_code]);
        int down_diff = (GetMessageTime() - key_down_state[key_code]);
        return down_diff - up_diff < 5;
    } else if (key_down_state.count(key_code) > 0) {
        return true;
    }

    return false;
}

bool key_state_changed(DWORD key_code) {
    bool is_down = is_key_down(key_code);

    if (last_key_state[key_code] != is_down) {
        last_key_state[key_code] = is_down;
        return true;
    }

    return false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_NCHITTEST:
        {
            LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
            if (hit == HTCLIENT) hit = HTCAPTION;
            return hit;
        }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId) {
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
    case WM_TIMER:
        switch (wParam) {
            case KEY_TIMER:
                bool needs_draw = false;

                for (std::vector<DWORD>::iterator it = registered_keys.begin(); it != registered_keys.end(); ++it) {
                    if (key_state_changed((*it))) {
                        needs_draw = true;
                    }
                }

                if (needs_draw) {
                    RedrawWindow(
                        main_hWnd,
                        0,
                        NULL,
                        RDW_ERASE | RDW_FRAME | RDW_INTERNALPAINT | RDW_INVALIDATE
                    );
                }

                return 0; 
        } 
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            bitmap_t a_key(is_key_down(key_name_to_code['a']) ? get_key_img_path(L"a_down.bmp") : get_key_img_path(L"a_up.bmp"), hdc);
            bitmap_t s_key(is_key_down(key_name_to_code['s']) ? get_key_img_path(L"s_down.bmp") : get_key_img_path(L"s_up.bmp"), hdc);
            bitmap_t d_key(is_key_down(key_name_to_code['d']) ? get_key_img_path(L"d_down.bmp") : get_key_img_path(L"d_up.bmp"), hdc);
            bitmap_t w_key(is_key_down(key_name_to_code['w']) ? get_key_img_path(L"w_down.bmp") : get_key_img_path(L"w_up.bmp"), hdc);
            bitmap_t n0_key(is_key_down(key_name_to_code['0']) ? get_key_img_path(L"0_down.bmp") : get_key_img_path(L"0_up.bmp"), hdc);
            bitmap_t n1_key(is_key_down(key_name_to_code['1']) ? get_key_img_path(L"1_down.bmp") : get_key_img_path(L"1_up.bmp"), hdc);
            bitmap_t ctrl_key(is_key_down(key_name_to_code['c']) ? get_key_img_path(L"ctrl_down.bmp") : get_key_img_path(L"ctrl_up.bmp"), hdc);

            a_key.x = ctrl_key.width();
            a_key.y = w_key.height();

            s_key.x = a_key.x + a_key.width();
            s_key.y = w_key.height();

            d_key.x = s_key.x + s_key.width();
            d_key.y = w_key.height();

            w_key.x = a_key.x + a_key.width();
            w_key.y = 0;

            ctrl_key.x = 0;
            ctrl_key.y = a_key.height() + w_key.height();

            n0_key.x = (n1_key.x + n1_key.width() + n0_key.width()) / 2;
            n0_key.y = a_key.height() * 3;

            n1_key.x = (d_key.x + d_key.width());
            n1_key.y = (d_key.y + d_key.height());

            a_key.draw();
            s_key.draw();
            d_key.draw();
            w_key.draw();
            ctrl_key.draw();
            n0_key.draw();
            n1_key.draw();

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
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
