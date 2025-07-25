#pragma once
#include "WindowsPlus.h"
#include <Windowsx.h>
#include "Rad/MessageHandler.h"

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
        CHECK_LE(m_hWnd != NULL);
    }

    operator bool() const { return m_hWnd != NULL; }

    operator HWND() const { return m_hWnd; }

    void ResetContent() { ListBox_ResetContent(m_hWnd); }
    int AddString(LPCTSTR lpsz) { return ListBox_AddString(m_hWnd, lpsz); }
    int AddItemData(LPARAM data) { return ListBox_AddItemData(m_hWnd, data); }
    int GetCount() const { return ListBox_GetCount(m_hWnd); }
    int GetCurSel() const { return ListBox_GetCurSel(m_hWnd); }
    LPARAM GetItemData(int i) const { return ListBox_GetItemData(m_hWnd, i); }
    int GetItemRect(int i, LPRECT pRect) const { return ListBox_GetItemRect(m_hWnd, i, pRect); }
    int GetText(int i, LPTSTR lpszBuffer) const { return ListBox_GetText(m_hWnd, i, lpszBuffer); }
    int SetCurSel(int i) { return ListBox_SetCurSel(m_hWnd, i); }
    int SetItemData(int i, LPARAM data) { return ListBox_SetItemData(m_hWnd, i, data); }
    int SetTopIndex(int i) const { return ListBox_SetTopIndex(m_hWnd, i); }
    int FindStringExact(int i, LPCTSTR lpsz) const { return ListBox_FindStringExact(m_hWnd, i, lpsz); }

    int GetVisibleCount() const
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

class ListBoxOwnerDrawnFixed : public ListBox, public MessageChain
{
private:
    struct ItemData
    {
        LPCTSTR pStr;
        LPARAM lParam;
        int iIcon;
    };

    ItemData* GetInternalItemData(int i)
    {
        const LPARAM lParam = ListBox::GetItemData(i);
        ItemData* pData = lParam == CB_ERR ? nullptr : reinterpret_cast<ItemData*>(lParam);
        if (pData == nullptr)
        {
            pData = new ItemData({});
            pData->iIcon = -1;
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

    bool UseTabStops() const
    {
        const DWORD dwStyle = GetWindowStyle(*this);
        return dwStyle & LBS_USETABSTOPS;
    }

public:
    void Create(_In_ HWND hParent, _In_ DWORD dwStyle, _In_ RECT rPos, _In_ int nID)
    {
        m_nID = nID;
        ListBox::Create(hParent, dwStyle | LBS_OWNERDRAWFIXED, rPos, nID);
    }

    int GetIconMode() const
    {
        return m_IconMode;
    }

    void SetIconMode(int icon_mode)
    {
        m_IconMode = icon_mode;
    }

    SIZE GetIconSize() const
    {
        return { GetSystemMetrics(m_IconMode == ICON_SMALL ? SM_CXSMICON : SM_CXICON), GetSystemMetrics(m_IconMode == ICON_SMALL ? SM_CYSMICON : SM_CYICON) };
    }

    HIMAGELIST SetImageList(HIMAGELIST hNewImageList)
    {
        HIMAGELIST hOldImageList = m_hImageList;
        m_hImageList = hNewImageList;
        return hOldImageList;
    }

    HIMAGELIST GetImageList() const
    {
        return m_hImageList;
    }

    int AddString(LPCTSTR lpsz)
    {
        if (HasString())
            return ListBox::AddString(lpsz);
        else
        {
            ItemData* pData = new ItemData({});
            pData->iIcon = -1;
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

    int GetItemIconIndex(int i) const
    {
        const ItemData* pData = GetInternalItemData(i);
        return pData->iIcon;
    }

    void SetItemIconIndex(int i, int iIcon)
    {
        ItemData* pData = GetInternalItemData(i);
        pData->iIcon = iIcon;
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
            if (m_IconMode >= 0)
                lpMeasureItem->itemHeight = GetIconSize().cy + 4;
            else
            {
                HFONT hFont = (HFONT) SendMessage(*this, WM_GETFONT, 0, 0);
                const SIZE TextSize = GetFontSize(*this, hFont, TEXT("Mg"), 2);
                lpMeasureItem->itemHeight = TextSize.cy + 4;
            }
        }
    }

    void OnDrawItem(const DRAWITEMSTRUCT* lpDrawItem)
    {
        if (lpDrawItem->hwndItem == *this)
        {
            if (lpDrawItem->itemID == -1) // Empty item
                return;

            const SIZE szIcon = GetIconSize();

            /*const*/ ItemData* pData = reinterpret_cast<ItemData*>(lpDrawItem->itemData);

            const COLORREF clrForeground = SetTextColor(lpDrawItem->hDC,
                lpDrawItem->itemState & ODS_SELECTED ? g_Theme.clrHighlightText : g_Theme.clrWindowText);

            const COLORREF clrBackground = SetBkColor(lpDrawItem->hDC,
                lpDrawItem->itemState & ODS_SELECTED ? g_Theme.clrHighlight : g_Theme.clrWindow);

            const HBRUSH hBrBackgournd = lpDrawItem->itemState & ODS_SELECTED ? g_Theme.brHighlight : g_Theme.brWindow;
            FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, hBrBackgournd);

            RECT rc = lpDrawItem->rcItem;
            if (m_IconMode >= 0)
                rc.left += szIcon.cx + 4;
            const UINT format = DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS | DT_SINGLELINE | DT_NOPREFIX | DT_EXPANDTABS;
            if (!pData or HasString())
            {
                TCHAR text[1024];
                const int cch = GetText(lpDrawItem->itemID, text);
                DrawText(lpDrawItem->hDC, text, cch, &rc, format);
            }
            else
                DrawText(lpDrawItem->hDC, pData->pStr, -1, &rc, format);

            SetTextColor(lpDrawItem->hDC, clrForeground);
            SetBkColor(lpDrawItem->hDC, clrBackground);

            if (m_hImageList != NULL and pData and pData->iIcon >= 0)
            {
                ImageList_DrawEx(m_hImageList, pData->iIcon, lpDrawItem->hDC,
                    lpDrawItem->rcItem.left + 2, lpDrawItem->rcItem.top + 2, szIcon.cx, szIcon.cy,
                    CLR_DEFAULT, CLR_DEFAULT, ILD_NORMAL);
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

    int OnCompareItem(const COMPAREITEMSTRUCT* lpCompareItem)
    {
        if (lpCompareItem->hwndItem != *this || HasString())
        {
            SetHandled(false);
            return 0;
        }
        else
        {
            LPCTSTR pStr1 = lpCompareItem->itemID1 == -1 ? reinterpret_cast<LPCTSTR>(lpCompareItem->itemData1) : reinterpret_cast<ItemData*>(lpCompareItem->itemData1)->pStr;
            LPCTSTR pStr2 = reinterpret_cast<ItemData*>(lpCompareItem->itemData2)->pStr;
            return lstrcmpi(pStr1, pStr2);
        }
    }

protected:
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        LRESULT ret = 0;
        switch (uMsg)
        {
            HANDLE_MSG(WM_MEASUREITEM, OnMeasureItem);
            HANDLE_MSG(WM_DRAWITEM, OnDrawItem);
            HANDLE_MSG(WM_DELETEITEM, OnDeleteItem);
            HANDLE_MSG(WM_COMPAREITEM, OnCompareItem);
        }
        return ret;
    }

private:
    int m_nID{ 0 };
    int m_IconMode{ -1 };
    HIMAGELIST m_hImageList{ NULL };
};
