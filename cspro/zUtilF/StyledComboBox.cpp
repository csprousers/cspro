#include "stdafx.h"
#include "StyledComboBox.h"

IMPLEMENT_DYNAMIC(StyledComboBox, CComboBox)

BEGIN_MESSAGE_MAP(StyledComboBox, CComboBox)
END_MESSAGE_MAP()

StyledComboBox::StyledComboBox()
{
}

BOOL StyledComboBox::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
    dwStyle |= CBS_OWNERDRAWFIXED | CBS_HASSTRINGS;
    return CComboBox::Create(dwStyle, rect, pParentWnd, nID);
}

void StyledComboBox::SetStyle(int index, LOGFONT font, COLORREF color)
{
    SetItemDataPtr(index, new ItemData{ std::move(font), color });
}

void StyledComboBox::DrawItem(LPDRAWITEMSTRUCT lpdis)
{
    ASSERT(lpdis->CtlType == ODT_COMBOBOX);

    if (lpdis->itemID == -1) // Empty item)
        return;

    CDC* dc = CDC::FromHandle(lpdis->hDC);
    ASSERT_VALID(dc);

    int old_dc_index = dc->SaveDC();

    CFont* old_font = nullptr;
    const ItemData* item_data = reinterpret_cast<ItemData*>(GetItemDataPtr(lpdis->itemID));
    if (item_data) {
        CFont font;
        font.CreateFontIndirect(&item_data->m_font);
        old_font = dc->SelectObject(&font);
    }

    if (lpdis->itemState & ODS_FOCUS)
        dc->DrawFocusRect(&lpdis->rcItem);

    COLORREF clrForeground;
    COLORREF clrBackground;
    if (lpdis->itemState & ODS_DISABLED)
    {
        clrForeground = GetSysColor(COLOR_GRAYTEXT);
        clrBackground = GetSysColor(COLOR_BTNFACE);
    }
    else if (lpdis->itemState & ODS_SELECTED)
    {
        clrForeground = GetSysColor(COLOR_HIGHLIGHTTEXT);
        clrBackground = GetSysColor(COLOR_HIGHLIGHT);
    }
    else {
        clrForeground = item_data ? item_data->m_color : GetSysColor(COLOR_WINDOWTEXT);
        clrBackground = GetSysColor(COLOR_WINDOW);
    }

    CBrush brushFill;
    brushFill.CreateSolidBrush(clrBackground);
    dc->SetTextColor(clrForeground);

    dc->SetBkMode(TRANSPARENT);
    dc->FillRect(&lpdis->rcItem, &brushFill);

    CString text;
    GetLBText(lpdis->itemID, text);

    dc->DrawText(text, &lpdis->rcItem, DT_SINGLELINE | DT_VCENTER);

    if (old_font)
        dc->SelectObject(old_font);

    dc->RestoreDC(old_dc_index);
}

void StyledComboBox::MeasureItem(LPMEASUREITEMSTRUCT)
{
    // Not implemented - sizing is taken care of by UpdateDropDownSize
}

void StyledComboBox::DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct)
{
    if (lpDeleteItemStruct->itemData)
        delete reinterpret_cast<ItemData*>(lpDeleteItemStruct->itemData);
}

void StyledComboBox::UpdateDropDownSize()
{
    CSize max_item_size = GetItemSize();
    SetDroppedWidth(max_item_size.cx + GetSystemMetrics(SM_CXVSCROLL));
    SetItemHeight(0, max_item_size.cy);
}

CSize StyledComboBox::GetItemSize()
{
    LONG max_height = 0;
    LONG max_width = 0;
    for (int i = 0; i < GetCount(); ++i) {
        CSize item_size = CalculateSize(i);
        max_width = std::max(max_width, item_size.cx);
        max_height = std::max(max_height, item_size.cy);
    }
    return CSize(max_width, max_height);
}

CSize StyledComboBox::CalculateSize(int item_index)
{
    CString item_text;
    GetLBText(item_index, item_text);
    const ItemData* item_data = reinterpret_cast<ItemData*>(GetItemDataPtr(item_index));

    CClientDC dc(this);

    CFont* old_font = nullptr;
    if (item_data) {
        CFont font;
        font.CreateFontIndirect(&item_data->m_font);
        old_font = dc.SelectObject(&font);
    }

    CSize size = dc.GetTextExtent(item_text);

    if (old_font)
        dc.SelectObject(old_font);

    size.cy += 6;
    return size;
}
