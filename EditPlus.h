#pragma once
#include "WindowsPlus.h"
#include <Windowsx.h>
#include <CommCtrl.h>

inline HWND Edit_Create(HWND hParent, DWORD dwStyle, RECT rc, int id)
{
    HWND hWnd = CreateWindow(
        WC_EDIT,
        TEXT(""),
        dwStyle,
        rc.left, rc.top, Width(rc), Height(rc),
        hParent,
        (HMENU) (INT_PTR) id,
        NULL,
        NULL);
    CHECK_LE(hWnd);
    return hWnd;
}

inline HWND ComboBox_Create(HWND hParent, DWORD dwStyle, RECT rc, int id)
{
    HWND hWnd = CreateWindow(
        WC_COMBOBOX,
        TEXT(""),
        dwStyle,
        rc.left, rc.top, Width(rc), Height(rc) + 200,
        hParent,
        (HMENU) (INT_PTR) id,
        NULL,
        NULL);
    CHECK_LE(hWnd);
    return hWnd;
}
