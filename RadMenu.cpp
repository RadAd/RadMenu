#include "Rad/Window.h"
#include "Rad/Dialog.h"
#include "Rad/Windowxx.h"
#include "Rad/Log.h"
#include "Rad/Format.h"
#include "Rad/RadTextFile.h"
#include <tchar.h>
//#include <strsafe.h>
#include "resource.h"

#include <Shlwapi.h>

#include "EditPlus.h"
#include "ListBoxPlus.h"
#include "StrUtils.h"
//#include "resource.h"

#include <string>
#include <vector>
#include <fstream>
#include <thread>

// TODO
// Second line of text - maybe in a different color or font size
// Tooltips
// Option to write bom on output

#ifdef UNICODE
#define CP_INTERNAL CP_UTF16_LE
#else
#define CP_INTERNAL CP_ACP
#endif

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

std::tstring CreateProcess(_In_z_ LPCTSTR strCmdLine, _In_ UINT CodePage, _In_ DWORD dwFlags)
{
    STARTUPINFO            siStartupInfo = { sizeof(siStartupInfo) };
    siStartupInfo.dwFlags = STARTF_USESTDHANDLES;

    SECURITY_ATTRIBUTES sa = { sizeof(sa) };
    sa.bInheritHandle = TRUE;

    HANDLE hReadPipe = NULL;
    CHECK_LE(CreatePipe(&hReadPipe, &siStartupInfo.hStdOutput, &sa, sizeof(sa)));
    CHECK_LE(SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0));
    siStartupInfo.hStdError = siStartupInfo.hStdOutput;

    PROCESS_INFORMATION    piProcessInfo = {};
    if (CreateProcess(NULL, const_cast<LPTSTR>(strCmdLine), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siStartupInfo, &piProcessInfo) == FALSE)
    {
        CHECK_LE(CloseHandle(siStartupInfo.hStdOutput));
        CHECK_LE(CloseHandle(hReadPipe));
        return Format(TEXT("ERROR: Could not create new process (%d)."), GetLastError());
    }
    else
    {
        CHECK_LE(CloseHandle(siStartupInfo.hStdOutput));

        std::tstring result;
        RadITextFile file(RadIFile(hReadPipe), CodePage);
        std::tstring line;
        while (file.ReadLine(line, CP_INTERNAL))
        {
            result += line;
            result += TEXT('\n');
        }

        WaitForSingleObject(piProcessInfo.hProcess, INFINITE);

        CHECK_LE(CloseHandle(piProcessInfo.hThread));
        CHECK_LE(CloseHandle(piProcessInfo.hProcess));

        return result;
    }
}

#define IDC_EDIT                     100
#define IDC_LIST                     101
#define IDC_PREVIEW                  102

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
    UINT code_page = CP_UTF8;
    DisplayMode dm = DisplayMode::LINE;
    LPCTSTR file = nullptr;
    LPCTSTR elements = nullptr;
    std::vector<std::tstring> cols;
    std::vector<std::tstring> out_cols;
    int header = 0;
    LPCTSTR preview_cmd = nullptr;
    WCHAR sep = TEXT(',');
    bool sort = false;
    bool blur = true;

    bool ParseCommandLine(const int argc, const LPCTSTR* argv);
    bool NeedPreview() const { return preview_cmd != nullptr; }
};

class RootWindow : public Window
{
    friend WindowManager<RootWindow>;
    struct Class
    {
        static LPCTSTR ClassName() { return TEXT("RadMenu"); }
        static void GetWndClass(WNDCLASS& wc)
        {
            //MainClass::GetWndClass(wc);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = g_Theme.brWindow;
            wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON1));
        }
        static void GetCreateWindow(CREATESTRUCT& cs)
        {
            //MainClass::GetCreateWindow(cs);
            cs.style = WS_POPUP | WS_BORDER | WS_VISIBLE;
            cs.dwExStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
            cs.dwExStyle |= WS_EX_CONTROLPARENT;
            cs.x = 100;
            cs.y = 100;
            cs.cx = 500;
            cs.cy = 500;
        }
    };

private:
    ~RootWindow()
    {
        for (auto& t : m_threads)
            t.join();
    }

public:
    static ATOM Register() { return ::Register<Class>(); }
    static RootWindow* Create(const Options& options)
    {
        return WindowManager<RootWindow>::Create(NULL, TEXT("Rad Menu"), reinterpret_cast<LPVOID>(const_cast<Options*>(&options)));
    }

protected:
    void GetCreateWindow(CREATESTRUCT& cs) override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    BOOL OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnSetFocus(HWND hwndOldFocus);
    void OnSize(UINT state, int cx, int cy);
    void OnEnterSizeMove();
    void OnExitSizeMove();
    void OnActivate(UINT state, HWND hWndActDeact, BOOL fMinimized);
    UINT OnNCHitTest(int x, int y);
    void OnCommand(int id, HWND hWndCtl, UINT codeNotify);
    HBRUSH OnCtlColor(HDC hdc, HWND hWndChild, int type);
    void OnDrawItem(const DRAWITEMSTRUCT* lpDrawItem);

    struct AddItemData;
    struct ProcessLineData
    {
        HWND hWnd;
        AddItemData* paid;
    };
    static void ProcessLine(std::wstring_view linev, const ProcessLineData* pldata)
    {
        if (!linev.empty() && linev.back() == L'\n')
            linev.remove_suffix(1);
        if (!linev.empty() && linev.back() == L'\r')
            linev.remove_suffix(1);
        std::wstring line(linev);
        AddItemData* paid = pldata->paid;

        if (paid->header > 0)
        {
            --paid->header;
            int header = paid->header;
            const std::vector<std::tstring> headernames = split_unquote(line, paid->sep);

            // TODO Show header somewhere

            assert(paid->cols_map.size() == paid->cols.size());
            for (int i = 0; i < paid->cols.size(); ++i)
            {
                auto it = std::find(headernames.begin(), headernames.end(), paid->cols[i]);
                if (it != headernames.end())
                {
                    const int c = static_cast<int>(std::distance(headernames.begin(), it));
                    paid->cols_map[i] = c;
                }
            }

            assert(paid->out_cols_map.size() == paid->out_cols.size());
            for (int i = 0; i < paid->out_cols.size(); ++i)
            {
                auto it = std::find(headernames.begin(), headernames.end(), paid->out_cols[i]);
                if (it != headernames.end())
                {
                    const int c = static_cast<int>(std::distance(headernames.begin(), it));
                    paid->out_cols_map[i] = c;
                }
            }
        }
        else if (!line.empty())
            SendMessage(pldata->hWnd, UM_ADDITEM, WPARAM(paid), LPARAM(line.c_str()));
    }

    static void LoadItemsFromFileThread(HANDLE h, AddItemData* paid, HWND hWnd)
    {
        ProcessLineData pldata({ hWnd, paid });
        RadITextFile itextfile(RadIFile(h), paid->CodePage);
        std::wstring line;
        while (itextfile.ReadLine(line, CP_INTERNAL))
            ProcessLine(line, &pldata);
        delete paid;
    }

    void LoadItemsFomFile(const HANDLE hFile, const Options& options)
    {
        std::vector<int> cols_map;
        for (const std::tstring& s : options.cols)
            cols_map.push_back(_tstoi(s.c_str()) - 1);
        std::vector<int> out_cols_map;
        for (const std::tstring& s : options.out_cols)
            out_cols_map.push_back(_tstoi(s.c_str()) - 1);

        std::thread t(LoadItemsFromFileThread, hFile, new AddItemData({ options.dm, options.code_page, options.sep, options.header, options.cols, cols_map, options.out_cols, out_cols_map, options.preview_cmd }), HWND(*this));
        m_threads.push_back(std::move(t));
    }

    UINT m_code_page = CP_ACP;
    HWND m_hEdit = NULL;
    ListBoxOwnerDrawnFixed m_ListBox;
    HWND m_hPreview = NULL;
    struct Item
    {
        std::tstring line;
        std::tstring name;
        std::tstring preview_cmd;
        int iIcon = -1;
    };
    std::vector<std::unique_ptr<Item>> m_items;
    std::vector<std::thread> m_threads;

    std::vector<std::tstring> GetSearchTerms() const;
    std::tstring GetSelectedText() const;
    static bool Matches(const std::vector<std::tstring>& search, const std::tstring& text);
    void FillList();
    void AddItemToList(const Item& i, const size_t j, const std::vector<std::tstring>& search);

    struct AddItemData
    {
        DisplayMode dm;
        UINT CodePage;
        WCHAR sep;
        int header;
        std::vector<std::tstring> cols;
        std::vector<int> cols_map;
        std::vector<std::tstring> out_cols;
        std::vector<int> out_cols_map;
        LPCTSTR preview_cmd = nullptr;
    };
    Item& AddItem(const LPCTSTR line, const AddItemData& aid)
    {
        const std::vector<std::tstring> a = !aid.cols_map.empty() || !aid.out_cols_map.empty() || aid.preview_cmd ? split_unquote(line, aid.sep) : std::vector<std::tstring>();

        std::tstring name;
        if (!aid.cols_map.empty())
        {
            for (const int c : aid.cols_map)
            {
                if (!name.empty())
                    name += TEXT("\t");
                if (c >= 0 && a.size() > c)
                    name += a[c];
                else if (name.empty())
                    name += TEXT(" ");
            }
        }
        else if (aid.dm == DisplayMode::FNAME) // TODO How combine FNAME with aid.cols
            name = GetName(line);

        std::tstring line_out;
        if (!aid.out_cols_map.empty())
        {
            for (const int c : aid.out_cols_map)
            {
                if (!line_out.empty())
                    line_out += aid.sep;
                if (c >= 0 && a.size() > c)
                    line_out += a[c];
                else if (line_out.empty())
                    line_out += TEXT(" ");
            }
        }
        else
            line_out = line;

        std::tstring preview_cmd;
        if (aid.preview_cmd)
        {
            preview_cmd = aid.preview_cmd;
            for (int i = 0; i < a.size(); ++i)
            {
                auto fn = Format(TEXT("$%d"), i + 1);
                FindAndReplace(preview_cmd, fn, a[i]);
            }
        }

        m_items.push_back(std::unique_ptr<Item>(new Item({ line_out, name, preview_cmd })));
        return *m_items.back().get();
    }
};

void RootWindow::GetCreateWindow(CREATESTRUCT& cs)
{
    Window::GetCreateWindow(cs);
    const Options& options = *reinterpret_cast<Options*>(cs.lpCreateParams);
    cs.cx = options.NeedPreview() ? 1000 : 500;
}

void ShowUsage()
{
    MessageBox(NULL,
        TEXT("RadMenu <options>\n")
        TEXT("Where <options> are:\n")
        TEXT("  /e <elements>\t- list of options\n")
        TEXT("  /f <filename>\t- list of options\n")
        TEXT("  /cp <code_page>\t- code page of input file (default is 65001)\n")
        TEXT("  /cols <col,>\t- list of columns\n")
        TEXT("  /out-cols <col,>\t- list of output columns\n")
        TEXT("  /sep <char>\t- column separator\n")
        TEXT("  /header <num>\t- number of header lines\n")
        TEXT("  /is\t\t- use small icons\n")
        TEXT("  /il\t\t- use large icons\n")
        TEXT("  /dm <mode>\t- display mode\n")
        TEXT("  Where <mode> is one of:\n")
        TEXT("    fname\t\t- display file name\n")
        TEXT("  /preview-cmd <cmd>\t- command to execute and show output in preview window\n")
        TEXT("  /sort\t\t- sort items\n")
        TEXT("  /noblur\t\t- remove blur effect"),
        TEXT("Rad Menu"), MB_OK | MB_ICONINFORMATION);
}

bool Options::ParseCommandLine(const int argc, const LPCTSTR* argv)
{
    bool ret = true;
    for (int argn = 1; argn < argc; ++argn)
    {
        LPCTSTR arg = argv[argn];
        if (lstrcmpi(arg, TEXT("/f")) == 0)
            file = argv[++argn];
        else if (lstrcmpi(arg, TEXT("/cp")) == 0)
            code_page = _tstoi(argv[++argn]);
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
        else if (lstrcmpi(arg, TEXT("/cols")) == 0)
            // TODO Allow for formatting column, ie right align, max width
            cols = split(argv[++argn], TEXT(','));
        else if (lstrcmpi(arg, TEXT("/out-cols")) == 0)
            out_cols = split(argv[++argn], TEXT(','));
        else if (lstrcmpi(arg, TEXT("/header")) == 0)
            header = _tstoi(argv[++argn]);
        else if (lstrcmpi(arg, TEXT("/sep")) == 0)
        {
            LPCTSTR s = argv[++argn];
            if (_tcsncmp(s, TEXT("\\x"), 2) == 0)
                sep = WCHAR(_tcstol(s + 2, nullptr, 16));
            else
                sep = s[0];
        }
        else if (lstrcmpi(arg, TEXT("/preview-cmd")) == 0)
            preview_cmd = argv[++argn];
        else if (lstrcmpi(arg, TEXT("/sort")) == 0)
            sort = true;
        else if (lstrcmpi(arg, TEXT("/noblur")) == 0)
            blur = false;
        else if (lstrcmpi(arg, TEXT("/?")) == 0)
        {
            ShowUsage();
            ret = false;
        }
        else
        {
            MessageBox(NULL, Format(TEXT("Unknown argument: %s"), arg).c_str(), TEXT("Rad Menu"), MB_OK | MB_ICONERROR);
            ret = false;
        }
    }
    return ret;
}

BOOL RootWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct)
{
    const Options& options = *reinterpret_cast<Options*>(lpCreateStruct->lpCreateParams);

    if (options.blur)
        SetWindowBlur(*this, true);

    m_code_page = options.code_page;

    const RECT rcClient = CallWinApi<RECT>(GetClientRect, HWND(*this));

    LOGFONT lf;
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, 0, &lf, 0);
    const HFONT hFont = CreateFontIndirect(&lf);
    const SIZE TextSize = GetFontSize(*this, hFont, TEXT("Mg"), 2);

    RECT rc = { Border, Border, Width(rcClient) - Border, Border + TextSize.cy + Border };

    m_hEdit = Edit_Create(*this, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_LEFT, rc, IDC_EDIT);
    if (m_hEdit)
    {
        SendMessage(m_hEdit, WM_SETFONT, (WPARAM)hFont, 0);
        CHECK_LE(SetWindowSubclass(m_hEdit, BuddyProc, 0, 0));
        Edit_SetCueBannerTextFocused(m_hEdit, TEXT("Search"), TRUE);
    }

    if (options.NeedPreview())
        rc.right = rc.left + Width(rc)/2;
    rc.top = rc.bottom + Border;
    rc.bottom = rcClient.bottom - Border;
    if (options.icon_mode != -1)
    {
        m_ListBox.SetIconMode(options.icon_mode);
        const SIZE szIcon = m_ListBox.GetIconSize();
        const HIMAGELIST hImgListOld = m_ListBox.SetImageList(ImageList_Create(szIcon.cx, szIcon.cy, ILC_COLOR32, 0, 100));
        if (hImgListOld)
            CHECK_LE(ImageList_Destroy(hImgListOld));
    }
    m_ListBox.Create(*this, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | LBS_USETABSTOPS | LBS_NOTIFY | (options.sort ? LBS_SORT : 0), rc, IDC_LIST);
    SendMessage(m_ListBox, WM_SETFONT, (WPARAM)hFont, 0);

    if (options.NeedPreview())
    {
        const LONG w = Width(rc);
        rc.left = rc.right + Border;
        rc.right = rc.left + w - Border;
        m_hPreview = Edit_Create(*this, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_LEFT | ES_READONLY | ES_MULTILINE, rc, IDC_PREVIEW);
        if (m_hPreview)
            SendMessage(m_hPreview, WM_SETFONT, (WPARAM)hFont, 0);
    }

    if (options.file != nullptr)
    {
        HANDLE hFile = CreateFile(options.file, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile && hFile != INVALID_HANDLE_VALUE)
            LoadItemsFomFile(hFile, options);
        else
            MessageBox(*this, Format(TEXT("Error opening file: %s"), options.file).c_str(), TEXT("Rad Menu"), MB_OK | MB_ICONERROR);
    }
    if (options.elements != nullptr)
    {
        const std::vector<std::tstring> a = split_unquote(options.elements, TEXT(','));
        for (const auto& s : a)
            if (!s.empty())
            {
                auto ss = s;
                m_items.push_back(std::unique_ptr<Item>(new Item({ ss, ss })));
            }
    }
    const HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdIn)
        LoadItemsFomFile(hStdIn, options);

    FillList();

    return TRUE;
}

void RootWindow::OnDestroy()
{
    ImageList_Destroy(m_ListBox.SetImageList(NULL));
    PostQuitMessage(0);
}

void RootWindow::OnSetFocus(const HWND hwndOldFocus)
{
    if (m_hEdit)
        SetFocus(m_hEdit);
}

void RootWindow::OnSize(UINT state, int cx, int cy)
{
    RECT rc = CallWinApi<RECT>(GetWindowRect, m_hEdit);
    CHECK_LE(ScreenToClient(*this, &rc));
    rc.right = cx - Border;
    CHECK_LE(SetWindowPos(m_hEdit, NULL, rc.left, rc.top, Width(rc), Height(rc), SWP_NOOWNERZORDER | SWP_NOZORDER));
    CHECK_LE(InvalidateRect(m_hEdit, nullptr, FALSE));

    if (m_hPreview)
        rc.right = rc.left + Width(rc) / 2;
    rc.top = rc.bottom + Border;
    rc.bottom = cy - Border;
    CHECK_LE(SetWindowPos(m_ListBox, NULL, rc.left, rc.top, Width(rc), Height(rc), SWP_NOOWNERZORDER | SWP_NOZORDER));
    CHECK_LE(InvalidateRect(m_ListBox, nullptr, FALSE));

    if (m_hPreview)
    {
        const LONG w = Width(rc);
        rc.left = rc.right + Border;
        rc.right = rc.left + w - Border;
        CHECK_LE(SetWindowPos(m_hPreview, NULL, rc.left, rc.top, Width(rc), Height(rc), SWP_NOOWNERZORDER | SWP_NOZORDER));
        CHECK_LE(InvalidateRect(m_hPreview, nullptr, FALSE));
    }
}

void RootWindow::OnEnterSizeMove()
{
    SetWindowBlur(*this, false);
}

void RootWindow::OnExitSizeMove()
{
    // TODO Do not reenable if disabled in options
    SetWindowBlur(*this, true);
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
    CHECK_LE(ScreenToClient(*this, &pt));

    RECT rc = CallWinApi<RECT>(GetClientRect, HWND(*this));
    CHECK_LE(InflateRect(&rc, -5, -5));

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
        case LBN_SELCHANGE:
            if (m_hPreview)
            {
                const int sel = m_ListBox.GetCurSel();
                if (sel >= 0)
                {
                    const int j = (int)m_ListBox.GetItemData(sel);
                    const std::tstring preview_output = CreateProcess(m_items[j]->preview_cmd.c_str(), m_code_page, 0);

                    CHECK_LE(SetWindowText(m_hPreview, preview_output.c_str()));
                }
                else
                    CHECK_LE(SetWindowText(m_hPreview, nullptr));
            }
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
            const HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hStdOut)
            {
                RadOTextFile out(hStdOut, m_code_page);
                if ((GetKeyState(VK_CONTROL) & 0x8000))
                    out.Write(TEXT("!"), CP_INTERNAL);
                if ((GetKeyState(VK_SHIFT) & 0x8000))
                    out.Write(TEXT("+"), CP_INTERNAL);
                const int j = (int) m_ListBox.GetItemData(sel);
                out.WriteLine(m_items[j]->line, CP_INTERNAL);
            }
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
        Item& item = *m_items[j].get();
        if (m_ListBox.GetItemIconIndex(lpDrawItem->itemID) == -1)
        {
            if (item.iIcon == -1)
            {
                HICON hIcon = GetIconWithoutShortcutOverlay(item.line.c_str(), m_ListBox.GetIconMode() == ICON_BIG);
                if (!hIcon)
                    item.iIcon = -2;
                else
                {
                    HIMAGELIST hImageList = m_ListBox.GetImageList();
                    item.iIcon = ImageList_AddIcon(hImageList, hIcon);
                    DestroyIcon(hIcon);
                }
            }
            m_ListBox.SetItemIconIndex(lpDrawItem->itemID, item.iIcon);
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
    size_t j = 0;
    for (const auto& sp : m_items)
        AddItemToList(*sp.get(), j++, search);
    const int sel = m_ListBox.FindStringExact(-1, seltext.c_str());
    m_ListBox.SetCurSel(sel >= 0 ? sel : 0);
    SendMessage(*this, WM_COMMAND, MAKEWPARAM(IDC_LIST, LBN_SELCHANGE), m_ListBox);
    SetWindowRedraw(m_ListBox, TRUE);
}

void RootWindow::AddItemToList(const Item& sp, const size_t j, const std::vector<std::tstring>& search)
{
    const std::tstring& text = sp.name.empty() ? sp.line : sp.name;
    if (Matches(search, text))
    {
        SendMessage(m_ListBox, WM_SETREDRAW, FALSE, 0);
        const int n = m_ListBox.AddString(text.c_str());
        m_ListBox.SetItemData(n, j);
        m_ListBox.SetItemIconIndex(n, sp.iIcon);
        SendMessage(m_ListBox, WM_SETREDRAW, TRUE, 0);
    }
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
        HANDLE_MSG(WM_ENTERSIZEMOVE, OnEnterSizeMove);
        HANDLE_MSG(WM_EXITSIZEMOVE, OnExitSizeMove);
        HANDLE_MSG(WM_ACTIVATE, OnActivate);
        HANDLE_MSG(WM_NCHITTEST, OnNCHitTest);
        HANDLE_MSG(WM_COMMAND, OnCommand);
        HANDLE_MSG(WM_CTLCOLOREDIT, OnCtlColor);
        HANDLE_MSG(WM_CTLCOLORSTATIC, OnCtlColor);
        HANDLE_MSG(WM_CTLCOLORLISTBOX, OnCtlColor);
        HANDLE_MSG(WM_DRAWITEM, OnDrawItem);
    case UM_ADDITEM:
    {
        SetHandled(true);
        const AddItemData* paid = (AddItemData*)(wParam);
        const LPCTSTR line = reinterpret_cast<LPCTSTR>(lParam);
        const size_t j = m_items.size();
        const std::vector<std::tstring> search = GetSearchTerms();
        AddItemToList(AddItem(line, *paid), j, search);
        break;
    }
    }

    if (!IsHandled())
    {
        bool bHandled = false;
        ret = m_ListBox.ProcessMessage(*this, uMsg, wParam, lParam, bHandled);
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
    if (!options.ParseCommandLine(__argc, __wargv))
        return false;

    if (options.elements == nullptr && options.file == nullptr && GetStdHandle(STD_INPUT_HANDLE) == NULL)
    {
        ShowUsage();
        return false;
    }

    InitTheme();

    CHECK_LE_RET(RootWindow::Register(), false);

    RootWindow* prw = RootWindow::Create(options);
    CHECK_LE_RET(prw != nullptr, false);

    RadLogInitWnd(*prw, "RadMenu", L"RadMenu");
    g_hWndDlg = *prw;
    CHECK_LE(ShowWindow(*prw, nShowCmd));

    return true;
}
