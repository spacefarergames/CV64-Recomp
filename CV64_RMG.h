#pragma once

#include "resource.h"
#include <Windows.h>

// Dark mode support for dialogs
void EnableDarkModeForDialog(HWND hDlg);

// Enable dark mode for title bar/borders only (no dialog content theming)
void EnableDarkModeTitleBarOnly(HWND hDlg);

// Handle dark mode WM_CTLCOLOR* messages in dialogs
// Call this in dialog procedures for WM_CTLCOLORDLG, WM_CTLCOLORSTATIC, etc.
// Returns non-NULL brush if handled, NULL to use default
HBRUSH HandleDarkModeCtlColor(HWND hDlg, UINT message, HDC hdc, HWND hCtl);




