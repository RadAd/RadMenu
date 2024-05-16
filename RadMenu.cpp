#include "Rad/Window.h"
#include "Rad/Dialog.h"
#include "Rad/Windowxx.h"
#include "Rad/Log.h"
#include "Rad/Format.h"
//#include <tchar.h>
//#include <strsafe.h>
#include "resource.h"

#include <Shlwapi.h>

#include "EditPlus.h"
#include "ListBoxPlus.h"
#include "StrUtils.h"
#include "resource.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

Theme g_Theme;

#define UM_ADDITEM  (WM_USER + 127)

extern HINSTANCE g_hInstance;
extern HWND g_hWndDlg;

template <class R, class F, typename... Ps>
R CallWinApi(F f, Ps... args)
{
    R r = {};
    if (!f(args..., &r))
        throw WinError({ GetLastError(), nullptr, TEXT("CallWinApi") });
    return r;
}

#define IDC_EDIT                     100
#define IDC_LIST                     101

const int Border = 10;

LRESULT CALLBACK BuddyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        const UINT vk = (UINT) (wParam);
        const int cRepeat = (int) (short) LOWORD(lParam);
        UINT flags = (UINT) HIWORD(lParam);
        if ((GetKeyState(VK_SHIFT) & 0x8000) == 0)
        {
            switch (vk)
            {
            case VK_UP:         case VK_DOWN:
            case VK_HOME:       case VK_END:
            case VK_NEXT:       case VK_PRIOR:
            {
                HWND hWndBuddy = GetWindow(hWnd, GW_HWNDNEXT);
                SendMessage(hWndBuddy, uMsg, wParam, lParam);
                return TRUE;
            }
            }
        }
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

inline std::tstring GetName(const std::tstring& line)
{
    LPCTSTR filename = PathFindFileName(line.c_str());
    if (filename)
    {
        LPCTSTR fileext = PathFindExtension(filename);
        if (fileext)
            return std::tstring(filename, fileext - filename);
        else
            return filename;
    }
    else
        return {};
}

enum class DisplayMode { LINE, FNAME };

struct Options
{
    int icon_mode = -1;
    DisplayMode dm = DisplayMode::LINE;
    LPCTSTR file = nullptr;
    LPCTSTR elements = nullptr;
    bool sort = false;
    bool blur = true;

    void ParseCommandLine(const int argc, const LPCTSTR* argv);
};

class RootWindow : public Window
{
    friend WindowManager<RootWindow>;
public:
    static ATOM Register() { return WindowManager<RootWindow>::Register(); }
    static RootWindow* Create(const Options& options) { return WindowManager<RootWindow>::Create(reinterpret_cast<LPVOID>(const_cast<Options*>(&options))); }

protected:
    static void GetCreateWindow(CREATESTRUCT& cs);
    static void GetWndClass(WNDCLASS& wc);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    BOOL OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnSetFocus(HWND hwndOldFocus);
    void OnSize(UINT state, int cx, int cy);
    void OnActivate(UINT state, HWND hWndActDeact, BOOL fMinimized);
    UINT OnNCHitTest(int x, int y);
    void OnCommand(int id, HWND hWndCtl, UINT codeNotify);
    HBRUSH OnCtlColor(HDC hdc, HWND hWndChild, int type);
    void OnDrawItem(const DRAWITEMSTRUCT* lpDrawItem);

    static LPCTSTR ClassName() { return TEXT("RadMenu"); }

    static void LoadItemsFomFileThread(std::wistream* is, const DisplayMode dm, HWND hWnd)
    {
        std::tstring line;
        while (std::getline(*is, line))
            if (!line.empty())
            {
                std::tstring name;
                if (dm == DisplayMode::FNAME)
                    name = GetName(line);
                const Item i({ std::make_shared<std::tstring>(line), std::make_shared<std::tstring>(name) });
                SendMessage(hWnd, UM_ADDITEM, 0, UINT_PTR(&i));
            }
        if (is != &std::wcin)
            delete is;
    }

    void AddItem(const std::tstring& line, const DisplayMode dm)
    {
        std::tstring name;
        if (dm == DisplayMode::FNAME)
            name = GetName(line);
        m_items.push_back({ std::make_shared<std::tstring>(line), std::make_shared<std::tstring>(name) });
    }

    void LoadItemsFomFile(std::wistream* is, const DisplayMode dm)
    {
#if 0
        std::tstring line;
        while (std::getline(*is, line))
            if (!line.empty())
                AddItem(line, dm);
        if (is != &std::wcin)
            delete is;
#else
        std::thread t(LoadItemsFomFileThread, is, dm, *this);
        t.detach();
#endif
    }

    HWND m_hEdit = NULL;
    ListBoxOwnerDrawnFixed m_ListBox;
    struct Item
    {
        std::shared_ptr<std::tstring> line;
        std::shared_ptr<std::tstring> name;
        HICON hIcon;
    };
    std::vector<Item> m_items;

    std::vector<std::tstring> GetSearchTerms() const;
    std::tstring GetSelectedText() const;
    static bool Matches(const std::vector<std::tstring>& search, const std::tstring& text);
    void FillList();
    void AddItem(const Item& i);
};

void RootWindow::GetCreateWindow(CREATESTRUCT& cs)
{
    Window::GetCreateWindow(cs);
    cs.lpszName = TEXT("Rad Menu");
    cs.style = WS_POPUP | WS_BORDER | WS_VISIBLE;
    cs.dwExStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
    cs.dwExStyle |= WS_EX_CONTROLPARENT;
    cs.x = 100;
    cs.y = 100;
    cs.cx = 500;
    cs.cy = 500;
}

void RootWindow::GetWndClass(WNDCLASS& wc)
{
    Window::GetWndClass(wc);
    wc.hbrBackground = g_Theme.brWindow;
    wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
}

void Options::ParseCommandLine(const int argc, const LPCTSTR* argv)
{
    for (int argn = 1; argn < argc; ++argn)
    {
        LPCTSTR arg = argv[argn];
        if (lstrcmpi(arg, TEXT("/f")) == 0)
            file = argv[++argn];
        else if (lstrcmpi(arg, TEXT("/is")) == 0)
            icon_mode = ICON_SMALL;
        else if (lstrcmpi(arg, TEXT("/il")) == 0)
            icon_mode = ICON_BIG;
        else if (lstrcmpi(arg, TEXT("/dm")) == 0)
        {
            LPCTSTR mode = argv[++argn];
            if (lstrcmpi(mode, TEXT("fname")) == 0)
                dm = DisplayMode::FNAME;
        }
        else if (lstrcmpi(arg, TEXT("/e")) == 0)
            elements = argv[++argn];
        else if (lstrcmpi(arg, TEXT("/sort")) == 0)
            sort = true;
        else if (lstrcmpi(arg, TEXT("/noblur")) == 0)
            blur = false;
        else if (lstrcmpi(arg, TEXT("/?")) == 0)
            MessageBox(NULL,
                TEXT("RadMenu <options>\n")
                TEXT("Where <options> are:\n")
                TEXT("  /is\t\t- use small icons\n")
                TEXT("  /il\t\t- use large icons\n")
                TEXT("  /dm <mode>\t- display mode\n")
                TEXT("  Where <mode> is one of:\n")
                TEXT("    fname\t\t- display file name\n")
                TEXT("  /e <elements>\t- list of options\n")
                TEXT("  /sort\t\t- sort items\n")
                TEXT("  /noblur\t\t- remove blur effect"),
                TEXT("Rad Menu"), MB_OK | MB_ICONERROR);
        else
            MessageBox(NULL, Format(TEXT("Unknown argument: %s"), arg).c_str(), TEXT("Rad Menu"), MB_OK | MB_ICONERROR);
    }
}

BOOL RootWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct)
{
    const Options& options = *reinterpret_cast<Options*>(lpCreateStruct->lpCreateParams);

    SetWindowBlur(*this);

    const RECT rcClient = CallWinApi<RECT>(GetClientRect, HWND(*this));

    const HANDLE hFont = GetStockObject(DEFAULT_GUI_FONT);
    const SIZE TextSize = GetFontSize(*this, hFont, TEXT("Mg"), 2);

    RECT rc = { Border, Border, Width(rcClient) - Border, Border + TextSize.cy + Border };

    m_hEdit = Edit_Create(*this, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_LEFT, rc, IDC_EDIT);
    SetWindowSubclass(m_hEdit, BuddyProc, 0, 0);
    Edit_SetCueBannerTextFocused(m_hEdit, TEXT("Search"), TRUE);

    rc.top = rc.bottom + Border;
    rc.bottom = rcClient.bottom - Border;
    if (options.icon_mode != -1)
        m_ListBox.SetIconMode(options.icon_mode);
    m_ListBox.Create(*this, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | LBS_NOTIFY | (options.sort ? LBS_SORT : 0), rc, IDC_LIST);

    if (options.file != nullptr)
    {
        auto f = std::make_unique<std::wifstream>(options.file);
        if (*f)
            LoadItemsFomFile(f.release(), options.dm);
        else
            MessageBox(*this, Format(TEXT("Error opening file: %s"), options.file).c_str(), TEXT("Rad Menu"), MB_OK | MB_ICONERROR);
    }
    if (options.elements != nullptr)
    {
        const std::vector<std::tstring> a = split_unquote(options.elements, TEXT(','));
        for (const auto& s : a)
            if (!s.empty())
            {
                auto ss = std::make_shared<std::tstring>(s);
                m_items.push_back({ ss, ss });
            }
    }
    LoadItemsFomFile(&std::wcin, options.dm);

    FillList();

    return TRUE;
}

void RootWindow::OnDestroy()
{
    for (const auto& sp : m_items)
        DestroyIcon(sp.hIcon);
    PostQuitMessage(0);
}

void RootWindow::OnSetFocus(const HWND hwndOldFocus)
{
    if (m_hEdit)
        SetFocus(m_hEdit);
}

void RootWindow::OnSize(UINT state, int cx, int cy)
{
    RECT rc;
    GetWindowRect(m_hEdit, &rc);
    ScreenToClient(*this, &rc);
    rc.right = cx - Border;
    SetWindowPos(m_hEdit, NULL, rc.left, rc.top, Width(rc), Height(rc), SWP_NOOWNERZORDER | SWP_NOZORDER);
    InvalidateRect(m_hEdit, nullptr, FALSE);;

    rc.top = rc.bottom + Border;
    rc.bottom = cy - Border;
    SetWindowPos(m_ListBox, NULL, rc.left, rc.top, Width(rc), Height(rc), SWP_NOOWNERZORDER | SWP_NOZORDER);
    InvalidateRect(m_ListBox, nullptr, FALSE);;
}

void RootWindow::OnActivate(UINT state, HWND hWndActDeact, BOOL fMinimized)
{
#ifndef _DEBUG
    if (state == WA_INACTIVE)
        SendMessage(*this, WM_CLOSE, 0, 0);
#endif
}

UINT RootWindow::OnNCHitTest(int x, int y)
{
    POINT pt = { x, y };
    ScreenToClient(*this, &pt);

    RECT rc;
    GetClientRect(*this, &rc);
    InflateRect(&rc, -5, -5);

    if (pt.x > rc.right and pt.x <= rc.top)
        return HTTOPRIGHT;
    else if (pt.x <= rc.left and pt.x <= rc.top)
        return HTTOPLEFT;
    else if (pt.x > rc.right and pt.y > rc.bottom)
        return HTBOTTOMRIGHT;
    else if (pt.x <= rc.left and pt.y > rc.bottom)
        return HTBOTTOMLEFT;
    else if (pt.x > rc.right)
        return HTRIGHT;
    else if (pt.x <= rc.left)
        return HTLEFT;
    else if (pt.y > rc.bottom)
        return HTBOTTOM;
    else if (pt.x <= rc.top)
        return HTTOP;
    else
        return HTCAPTION;
}

void RootWindow::OnCommand(int id, HWND hWndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDC_EDIT:
        switch (codeNotify)
        {
        case EN_CHANGE:
            FillList();
            break;
        }
        break;

    case IDC_LIST:
        switch (codeNotify)
        {
        case LBN_DBLCLK:
            SendMessage(*this, WM_COMMAND, IDOK, 0);
            break;
        }
        break;

    case IDCANCEL:
        if (Edit_GetTextLength(m_hEdit) > 0)
            Edit_SetText(m_hEdit, TEXT(""));
        else
            SendMessage(*this, WM_CLOSE, 0, 0);
        break;

    case IDOK:
    {
        const int sel = m_ListBox.GetCurSel();
        if (sel >= 0)
        {
            const int j = (int) m_ListBox.GetItemData(sel);
            std::wcout << m_items[j].line << std::endl;
            SendMessage(*this, WM_CLOSE, 0, 0);
        }
        break;
    }
    }
}

HBRUSH RootWindow::OnCtlColor(HDC hDC, HWND hWndChild, int type)
{
    SetTextColor(hDC, g_Theme.clrWindowText);
    SetBkColor(hDC, g_Theme.clrWindow);
    return g_Theme.brWindow;
}

void RootWindow::OnDrawItem(const DRAWITEMSTRUCT* lpDrawItem)
{
    if (lpDrawItem->hwndItem == m_ListBox and m_ListBox.GetIconMode() >= 0 and lpDrawItem->itemID >= 0)
    {
        const int j = (int) m_ListBox.GetItemData(lpDrawItem->itemID);
        Item& item = m_items[j];
        if (m_ListBox.GetItemIcon(lpDrawItem->itemID) == NULL)
        {
            if (item.hIcon == NULL)
            {
                item.hIcon = GetIconWithoutShortcutOverlay(item.line->c_str(), m_ListBox.GetIconMode() == ICON_BIG);
                if (!item.hIcon)
                    item.hIcon = (HICON) INVALID_HANDLE_VALUE;
            }
            if (item.hIcon != INVALID_HANDLE_VALUE)
                m_ListBox.SetItemIcon(lpDrawItem->itemID, item.hIcon);
        }
    }
    SetHandled(false);
}

std::vector<std::tstring> RootWindow::GetSearchTerms() const
{
    TCHAR text[1024];
    Edit_GetText(m_hEdit, text, ARRAYSIZE(text));
    return split_unquote(text, TEXT(' '));
}

std::tstring RootWindow::GetSelectedText() const
{
    const int sel = m_ListBox.GetCurSel();
    TCHAR buf[1024] = TEXT("");
    if (sel >= 0)
        m_ListBox.GetText(sel, buf);
    return buf;
}

bool RootWindow::Matches(const std::vector<std::tstring>& search, const std::tstring& text)
{
    bool found = true;
    for (const auto& s : search)
        if (!StrFindI(text.c_str(), s.c_str()))
        {
            found = false;
            break;
        }
    return found;
}

void RootWindow::FillList()
{
    SetWindowRedraw(m_ListBox, FALSE);
    const std::tstring seltext = GetSelectedText();
    const std::vector<std::tstring> search = GetSearchTerms();
    m_ListBox.ResetContent();
    int j = 0;
    for (const auto& sp : m_items)
    {
        const std::tstring& text = sp.name->empty() ? *sp.line : *sp.name;
        if (Matches(search, text))
        {
            const int n = m_ListBox.AddString(text.c_str());
            m_ListBox.SetItemData(n, j);
        }
        ++j;
    }
    const int sel = ListBox_FindStringExact(m_ListBox, 0, seltext.c_str());
    m_ListBox.SetCurSel(sel >= 0 ? sel : 0);
    SetWindowRedraw(m_ListBox, TRUE);
}

void RootWindow::AddItem(const Item& i)
{
    m_items.push_back(i);
    const Item& sp = m_items.back();

    //const std::tstring seltext = GetSelectedText();
    const std::vector<std::tstring> search = GetSearchTerms();

    const std::tstring& text = sp.name->empty() ? *sp.line : *sp.name;
    if (Matches(search, text))
    {
        SendMessage(m_ListBox, WM_SETREDRAW, FALSE, 0);
        const int j = static_cast<int>(m_items.size()) - 1;
        const int n = m_ListBox.AddString(text.c_str());
        m_ListBox.SetItemData(n, j);
        SendMessage(m_ListBox, WM_SETREDRAW, TRUE, 0);
    }

    //const int sel = ListBox_FindStringExact(m_ListBox, 0, seltext.c_str());
    //m_ListBox.SetCurSel(sel >= 0 ? sel : 0);
}

LRESULT RootWindow::HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    LRESULT ret = 0;
    switch (uMsg)
    {
        HANDLE_MSG(WM_CREATE, OnCreate);
        HANDLE_MSG(WM_DESTROY, OnDestroy);
        HANDLE_MSG(WM_SETFOCUS, OnSetFocus);
        HANDLE_MSG(WM_SIZE, OnSize);
        HANDLE_MSG(WM_ACTIVATE, OnActivate);
        HANDLE_MSG(WM_NCHITTEST, OnNCHitTest);
        HANDLE_MSG(WM_COMMAND, OnCommand);
        HANDLE_MSG(WM_CTLCOLOREDIT, OnCtlColor);
        HANDLE_MSG(WM_CTLCOLORLISTBOX, OnCtlColor);
        HANDLE_MSG(WM_DRAWITEM, OnDrawItem);
    case UM_ADDITEM:
    {
        SetHandled(true);
        const Item* i = reinterpret_cast<Item*>(lParam);
        AddItem(*i);
        break;
    }
    }

    if (!IsHandled())
    {
        bool bHandled = false;
        ret = m_ListBox.HandleChainMessage(uMsg, wParam, lParam, bHandled);
        if (bHandled)
            SetHandled(true);
    }
    if (!IsHandled())
        ret = Window::HandleMessage(uMsg, wParam, lParam);

    return ret;
}

bool Run(_In_ const LPCTSTR lpCmdLine, _In_ const int nShowCmd)
{
    RadLogInitWnd(NULL, "RadMenu", L"RadMenu");

    if (true)
    {
        g_Theme.clrWindow = RGB(0, 0, 0);
        g_Theme.clrHighlight = RGB(61, 61, 61);
        g_Theme.clrWindowText = RGB(250, 250, 250);
        g_Theme.clrHighlightText = g_Theme.clrWindowText;
        g_Theme.clrGrayText = RGB(128, 128, 128);
    }

    Options options;
    options.ParseCommandLine(__argc, __wargv);

    InitTheme();

    CHECK_LE_RET(RootWindow::Register(), false);

    RootWindow* prw = RootWindow::Create(options);
    CHECK_LE_RET(prw != nullptr, false);

    RadLogInitWnd(*prw, "RadMenu", L"RadMenu");
    g_hWndDlg = *prw;
    ShowWindow(*prw, nShowCmd);

    return true;
}
