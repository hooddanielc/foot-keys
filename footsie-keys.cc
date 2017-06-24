#include "stdafx.h"
#include <footsie-keys.h>

#include <stdio.h>
#include <iostream>

#define _WIN32_WINNT 0x050

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
  BOOL fEatKeystroke = FALSE;

  if (nCode == HC_ACTION) {
    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;

    switch (wParam) {
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
        std::cout << "keydown " << p->vkCode << std::endl;
        break;

      case WM_KEYUP:
      case WM_SYSKEYUP:
        std::cout << "keyup " << p->vkCode << std::endl;
        break;
    }
  }

  return (fEatKeystroke ? 1 : CallNextHookEx(NULL, nCode, wParam, lParam)); q
}

int APIENTRY wWinMain(
  _In_ HINSTANCE     hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPWSTR        lpCmdLine,
  _In_ int           nCmdShow
) {
  sd::cout << "hello" << std::endl;
  // Install the low-level keyboard & mouse hooks
  HHOOK hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);

  // Keep this app running until we're told to stop
  MSG msg;
  while (!GetMessage(&msg, NULL, NULL, NULL)) {    //this while loop keeps the hook
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  UnhookWindowsHookEx(hhkLowLevelKybd);
  return(0);
}


