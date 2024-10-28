#pragma once
#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <CommCtrl.h>

inline LONG Width(const RECT r)
{
    return r.right - r.left;
}

inline LONG Height(const RECT r)
{
    return r.bottom - r.top;
}

inline void SetWindowAccentDisabled(HWND hWnd)
{
    const HINSTANCE hModule = GetModuleHandle(TEXT("user32.dll"));
    if (hModule)
    {
        typedef enum _ACCENT_STATE {
            ACCENT_DISABLED,
            ACCENT_ENABLE_GRADIENT,
            ACCENT_ENABLE_TRANSPARENTGRADIENT,
            ACCENT_ENABLE_BLURBEHIND,
            ACCENT_ENABLE_ACRYLICBLURBEHIND,
            ACCENT_INVALID_STATE
        } ACCENT_STATE;
        struct ACCENTPOLICY
        {
            ACCENT_STATE nAccentState;
            DWORD nFlags;
            DWORD nColor;
            DWORD nAnimationId;
        };
        typedef enum _WINDOWCOMPOSITIONATTRIB {
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
            WCA_LAST = 23
        } WINDOWCOMPOSITIONATTRIB;
        struct WINCOMPATTRDATA
        {
            WINDOWCOMPOSITIONATTRIB nAttribute;
            PVOID pData;
            ULONG ulDataSize;
        };
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
        const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute) GetProcAddress(hModule, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute)
        {
            ACCENTPOLICY policy = { ACCENT_DISABLED, 0, 0, 0 };
            WINCOMPATTRDATA data = { WCA_ACCENT_POLICY, &policy, sizeof(ACCENTPOLICY) };
            SetWindowCompositionAttribute(hWnd, &data);
            //DwmSetWindowAttribute(hWnd, WCA_ACCENT_POLICY, &policy, sizeof(ACCENTPOLICY));
        }
        //FreeLibrary(hModule);
    }
}

inline void SetWindowBlur(HWND hWnd, bool bEnabled)
{
    const HINSTANCE hModule = GetModuleHandle(TEXT("user32.dll"));
    if (hModule)
    {
        typedef enum _ACCENT_STATE {
            ACCENT_DISABLED,
            ACCENT_ENABLE_GRADIENT,
            ACCENT_ENABLE_TRANSPARENTGRADIENT,
            ACCENT_ENABLE_BLURBEHIND,
            ACCENT_ENABLE_ACRYLICBLURBEHIND,
            ACCENT_INVALID_STATE
        } ACCENT_STATE;
        struct ACCENTPOLICY
        {
            ACCENT_STATE nAccentState;
            DWORD nFlags;
            DWORD nColor;
            DWORD nAnimationId;
        };
        typedef enum _WINDOWCOMPOSITIONATTRIB {
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
            WCA_LAST = 23
        } WINDOWCOMPOSITIONATTRIB;
        struct WINCOMPATTRDATA
        {
            WINDOWCOMPOSITIONATTRIB nAttribute;
            PVOID pData;
            ULONG ulDataSize;
        };
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
        const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute) GetProcAddress(hModule, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute)
        {
            ACCENTPOLICY policy = { bEnabled ? ACCENT_ENABLE_ACRYLICBLURBEHIND : ACCENT_DISABLED, 0, 0x80000000, 0 };
            //ACCENTPOLICY policy = { ACCENT_ENABLE_BLURBEHIND };
            WINCOMPATTRDATA data = { WCA_ACCENT_POLICY, &policy, sizeof(ACCENTPOLICY) };
            SetWindowCompositionAttribute(hWnd, &data);
            //DwmSetWindowAttribute(hWnd, WCA_ACCENT_POLICY, &policy, sizeof(ACCENTPOLICY));
        }
        //FreeLibrary(hModule);
    }
}

inline HICON GetFileIcon(LPCTSTR pStrFile, bool Large)
{
#if 0
    WORD icon = 0;
    TCHAR file[MAX_PATH];
    lstrcpy(file, pStrFile);
    return ExtractAssociatedIcon(g_hInstance, file, &icon);  // TODO Also get small icons and ones without shortcut arrow
#else
    SHFILEINFO fi = {};
    SHGetFileInfo(pStrFile, 0, &fi, sizeof(SHFILEINFO), SHGFI_ICON | (Large ? SHGFI_LARGEICON : SHGFI_SMALLICON));
    return fi.hIcon;
#endif
}

// https://devblogs.microsoft.com/oldnewthing/20110127-00/?p=11653
inline HICON GetIconWithoutShortcutOverlay(PCTSTR pszFile, bool Large)
{
    SHFILEINFO sfi;
    HIMAGELIST himl = reinterpret_cast<HIMAGELIST>(SHGetFileInfo(pszFile, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | (Large ? SHGFI_LARGEICON : SHGFI_SMALLICON)));
    if (himl)
        return ImageList_GetIcon(himl, sfi.iIcon, ILD_NORMAL);
    else
        return NULL;
}

inline SIZE GetFontSize(const HWND hWnd, const HANDLE hFont, _In_reads_(c) LPCTSTR lpString, _In_ int c)
{
    SIZE TextSize = {};
    HDC hDC = ::GetDC(hWnd);
    const HANDLE hOldFont = SelectObject(hDC, hFont);
    GetTextExtentPoint32(hDC, lpString, c, &TextSize);
    SelectObject(hDC, hOldFont);
    ReleaseDC(hWnd, hDC);
    return TextSize;
}

inline BOOL ScreenToClient(_In_ HWND hWnd, _Inout_ LPRECT lpRect)
{
    _ASSERT(::IsWindow(hWnd));
    if (!::ScreenToClient(hWnd, (LPPOINT) lpRect))
        return FALSE;
    return ::ScreenToClient(hWnd, ((LPPOINT) lpRect) + 1);
}
