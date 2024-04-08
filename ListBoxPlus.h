#pragma once
#include "WindowsPlus.h"
#include <Windowsx.h>

struct Theme
{
    COLORREF clrWindow = GetSysColor(COLOR_WINDOW);
    COLORREF clrHighlight = GetSysColor(COLOR_HIGHLIGHT);
    COLORREF clrWindowText = GetSysColor(COLOR_WINDOWTEXT);
    COLORREF clrHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    COLORREF clrGrayText = GetSysColor(COLOR_GRAYTEXT);
    HBRUSH   brWindow = NULL;
    HBRUSH   brHighlight = NULL;
};

extern Theme g_Theme;

inline void InitTheme()
{
    g_Theme.brWindow = CreateSolidBrush(g_Theme.clrWindow);
    g_Theme.brHighlight = CreateSolidBrush(g_Theme.clrHighlight);
}

class ListBox
{
public:
    void Create(_In_ HWND hParent, _In_ DWORD dwStyle, _In_ RECT rPos, _In_ int nID)
    {
        const HINSTANCE hInstance = NULL;
        m_hWnd = CreateWindow(WC_LISTBOX, nullptr, dwStyle, rPos.left, rPos.top, Width(rPos), Height(rPos), hParent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(nID)), hInstance, NULL);
        CHECK_LE(m_hWnd);
    }

    operator bool() const { return m_hWnd != NULL; }

    operator HWND() const { return m_hWnd; }

    void ResetContent() { ListBox_ResetContent(m_hWnd); }
    int AddString(LPCTSTR lpsz) { return ListBox_AddString(m_hWnd, lpsz); }
    int AddItemData(LPARAM data) { return ListBox_AddItemData(m_hWnd, data); }
    int GetCount() const { return ListBox_GetCount(m_hWnd); }
    int GetCurSel() const { return ListBox_GetCurSel(m_hWnd); }
    LPARAM GetItemData(int i) const { return ListBox_GetItemData(m_hWnd, i); }
    int GetItemRect(int i, LPRECT pRect) { return ListBox_GetItemRect(m_hWnd, i, pRect); }
    int GetText(int i, LPTSTR lpszBuffer) const { return ListBox_GetText(m_hWnd, i, lpszBuffer); }
    int SetCurSel(int i) { return ListBox_SetCurSel(m_hWnd, i); }
    int SetItemData(int i, LPARAM data) { return ListBox_SetItemData(m_hWnd, i, data); }
    int SetTopIndex(int i) { return ListBox_SetTopIndex(m_hWnd, i); }


    int GetVisibleCount()
    {
        RECT rcClient;
        GetClientRect(*this, &rcClient);

        RECT rcItem;
        GetItemRect(0, &rcItem);

        return Height(rcClient) / Height(rcItem);
    }
private:
    HWND m_hWnd{ NULL };
};

class ListBoxOwnerDrawnFixed : public ListBox
{
private:
    struct ItemData
    {
        LPCTSTR pStr;
        LPARAM lParam;
        HICON hIcon;
        TCHAR strRight[MAX_PATH];
        BOOL bGray;
    };

    ItemData* GetInternalItemData(int i)
    {
        const LPARAM lParam = ListBox::GetItemData(i);
        ItemData* pData = lParam == CB_ERR ? nullptr : reinterpret_cast<ItemData*>(lParam);
        if (pData == nullptr)
        {
            pData = new ItemData({});
            ListBox::SetItemData(i, reinterpret_cast<LPARAM>(pData));
        }
        return pData;
    }

    const ItemData* GetInternalItemData(int i) const
    {
        const LPARAM lParam = ListBox::GetItemData(i);
        ItemData* pData = lParam == CB_ERR ? nullptr : reinterpret_cast<ItemData*>(lParam);
        return pData;
    }

    bool HasString() const
    {
        const DWORD dwStyle = GetWindowStyle(*this);
        return dwStyle & LBS_HASSTRINGS;
    }

public:
    void Create(_In_ HWND hParent, _In_ DWORD dwStyle, _In_ RECT rPos, _In_ int nID)
    {
        m_nID = nID;
        ListBox::Create(hParent, dwStyle | LBS_OWNERDRAWFIXED, rPos, nID);
    }

    int GetIcon() const
    {
        return m_Icon;
    }

    void SetIcon(int icon)
    {
        m_Icon = icon;
    }

    int AddString(LPCTSTR lpsz)
    {
        if (HasString())
            return ListBox::AddString(lpsz);
        else
        {
            ItemData* pData = new ItemData({});
            pData->pStr = lpsz;
            return ListBox::AddItemData(LPARAM(pData));
        }
    }

    int GetText(int i, LPTSTR lpszBuffer) const
    {
        if (HasString())
            return ListBox::GetText(i, lpszBuffer);
        else
        {
            if (i < 0 or i >= GetCount())
                return LB_ERR;
            const ItemData* pData = GetInternalItemData(i);
            lstrcpy(lpszBuffer, pData->pStr);
            return lstrlen(pData->pStr);
        }
    }

    LPARAM GetItemData(int i) const
    {
        const ItemData* pData = GetInternalItemData(i);
        return pData ? pData->lParam : 0;
    }

    void SetItemData(int i, LPARAM data)
    {
        ItemData* pData = GetInternalItemData(i);
        pData->lParam = data;
    }

    HICON GetItemIcon(int i) const
    {
        const ItemData* pData = GetInternalItemData(i);
        return pData->hIcon;
    }

    void SetItemIcon(int i, HICON hIcon)
    {
        ItemData* pData = GetInternalItemData(i);
        pData->hIcon = hIcon;
        InvalidateItem(i);
    }

    void SetItemRightString(int i, LPCTSTR s)
    {
        ItemData* pData = GetInternalItemData(i);
        lstrcpy(pData->strRight, s);
        InvalidateItem(i);
    }

    void SetItemGray(int i, BOOL b)
    {
        ItemData* pData = GetInternalItemData(i);
        pData->bGray = b;
        InvalidateItem(i);
    }

    void InvalidateItem(int i)
    {
        if (IsWindow(*this))
        {
            RECT r;
            GetItemRect(i, &r);
            InvalidateRect(*this, &r, FALSE);
        }
    }

private:
    void OnMeasureItem(MEASUREITEMSTRUCT* lpMeasureItem)
    {
        //if (lpMeasureItem->CtlType == ODT_LISTBOX and lpMeasureItem->CtlID == GetWindowLong(*this, GWL_ID))
        if (lpMeasureItem->CtlType == ODT_LISTBOX and lpMeasureItem->CtlID == m_nID)
        {
            if (m_Icon >= 0)
                lpMeasureItem->itemHeight = GetSystemMetrics(m_Icon == ICON_SMALL ? SM_CYSMICON : SM_CYICON) + 4;
        }
    }

    void OnDrawItem(const DRAWITEMSTRUCT* lpDrawItem)
    {
        if (lpDrawItem->hwndItem == *this)
        {
            if (lpDrawItem->itemID == -1) // Empty item
                return;

            const SIZE szIcon = { GetSystemMetrics(m_Icon == ICON_SMALL ? SM_CXSMICON : SM_CXICON), GetSystemMetrics(m_Icon == ICON_SMALL ? SM_CYSMICON : SM_CYICON) };

            /*const*/ ItemData* pData = reinterpret_cast<ItemData*>(lpDrawItem->itemData);

            const COLORREF clrForeground = SetTextColor(lpDrawItem->hDC,
                lpDrawItem->itemState & ODS_SELECTED ? g_Theme.clrHighlightText : (pData and pData->bGray ? g_Theme.clrGrayText : g_Theme.clrWindowText));

            const COLORREF clrBackground = SetBkColor(lpDrawItem->hDC,
                lpDrawItem->itemState & ODS_SELECTED ? g_Theme.clrHighlight : g_Theme.clrWindow);

            const HBRUSH hBrBackgournd = lpDrawItem->itemState & ODS_SELECTED ? g_Theme.brHighlight : g_Theme.brWindow;
            FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, hBrBackgournd);

            RECT rc = lpDrawItem->rcItem;
            rc.left = lpDrawItem->rcItem.right - Width(lpDrawItem->rcItem) / 4;
            if (pData)
                DrawText(lpDrawItem->hDC, pData->strRight, lstrlen(pData->strRight), &rc, DT_VCENTER | DT_RIGHT | DT_END_ELLIPSIS | DT_SINGLELINE | DT_NOPREFIX);

            rc.right = rc.left;
            rc.left = lpDrawItem->rcItem.left;
            if (m_Icon >= 0)
                rc.left += szIcon.cx + 4;
            if (!pData or HasString())
            {
                TCHAR text[1024];
                const int cch = GetText(lpDrawItem->itemID, text);
                DrawText(lpDrawItem->hDC, text, cch, &rc, DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE | DT_NOPREFIX);
            }
            else
                DrawText(lpDrawItem->hDC, pData->pStr, -1, &rc, DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE | DT_NOPREFIX);

            SetTextColor(lpDrawItem->hDC, clrForeground);
            SetBkColor(lpDrawItem->hDC, clrBackground);

            if (m_Icon >= 0 and pData and pData->hIcon)
            {
                //DrawIcon(lpDrawItem->hDC, lpDrawItem->rcItem.left + 2, lpDrawItem->rcItem.top + 2, pData->hIcon);
                DrawIconEx(lpDrawItem->hDC,
                    lpDrawItem->rcItem.left + 2, lpDrawItem->rcItem.top + 2, pData->hIcon,
                    szIcon.cx, szIcon.cy,
                    0, hBrBackgournd, DI_NORMAL);
            }

            if (lpDrawItem->itemState & ODS_FOCUS)
                DrawFocusRect(lpDrawItem->hDC, &lpDrawItem->rcItem);
        }
    }

    void OnDeleteItem(const DELETEITEMSTRUCT* lpDeleteItem)
    {
        if (lpDeleteItem->hwndItem == *this)
        {
            ItemData* pData = reinterpret_cast<ItemData*>(lpDeleteItem->itemData);
            if (pData)
            {
                //DestroyIcon(pData->hIcon);
                delete pData;
            }
        }
    }

private:
    bool m_bHandled;

    void SetHandled(bool bHandled) { m_bHandled = bHandled;  }

public:
    LRESULT HandleChainMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
    {
        m_bHandled = false;
        LRESULT ret = 0;
        switch (uMsg)
        {
            HANDLE_MSG(WM_MEASUREITEM, OnMeasureItem);
            HANDLE_MSG(WM_DRAWITEM, OnDrawItem);
            HANDLE_MSG(WM_DELETEITEM, OnDeleteItem);
        }

        return ret;
    }

private:
    int m_nID{ 0 };
    int m_Icon = -1;
};
