#include "StdAfx.h"
#include "ColorPickerComboBox.h"

IMPLEMENT_DYNAMIC(ColorPickerComboBox, CComboBox)

ColorPickerComboBox::ColorPickerComboBox()
{
}

void ColorPickerComboBox::AddColor(LPCTSTR name, COLORREF color)
{
    SetItemData(AddString(name), color);
}

int ColorPickerComboBox::FindColor(COLORREF color) const
{
    for (int i = 0; i < GetCount(); ++i) {
        if (((COLORREF) GetItemData(i) & 0x00FFFFFF) == (color & 0x00FFFFFF)) {
            return i;
        }
    }
    return CB_ERR;
}

std::optional<COLORREF> ColorPickerComboBox::GetSelColor() const
{
    int sel = GetCurSel();
    if (sel != CB_ERR)
        return (COLORREF) GetItemData(sel);
    else
        return {};
}

BEGIN_MESSAGE_MAP(ColorPickerComboBox, CComboBox)
END_MESSAGE_MAP()

void ColorPickerComboBox::DrawItem(LPDRAWITEMSTRUCT lpdis)
{
    if (lpdis->itemID == -1) // Empty item)
        return;

    CDC dc;
    dc.Attach(lpdis->hDC);

    // The colors depend on whether the item is selected.
    COLORREF clrForeground = dc.SetTextColor(
        GetSysColor(lpdis->itemState & ODS_SELECTED ?
            COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

    COLORREF clrBackground = dc.SetBkColor(
        GetSysColor(lpdis->itemState & ODS_SELECTED ?
            COLOR_HIGHLIGHT : COLOR_WINDOW));

    // Calculate the vertical and horizontal position.
    TEXTMETRIC tm;
    dc.GetTextMetrics(&tm);
    int y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
    int x = LOWORD(GetDialogBaseUnits()) / 4;

    CString item_text;
    GetLBText(lpdis->itemID, item_text);

    const int color_box_width = 40 ;
    dc.ExtTextOut(color_box_width + 4 * x, y,
        ETO_CLIPPED | ETO_OPAQUE, &lpdis->rcItem,
        item_text, (UINT)item_text.GetLength(), NULL);

    //  Draw the color rectangle
    CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
    dc.SelectObject(pen);
    COLORREF color = (COLORREF) lpdis->itemData;
    CBrush brush;
    brush.CreateSolidBrush(color);
    dc.SelectObject(brush);

    CRect color_box_rect(lpdis->rcItem.left + 1, lpdis->rcItem.top + 1,
        lpdis->rcItem.left + 1 + color_box_width, lpdis->rcItem.bottom - 1);
    dc.Rectangle(color_box_rect);

    // If the item has the focus, draw the focus rectangle.
    if (lpdis->itemState & ODS_FOCUS)
        dc.DrawFocusRect(&lpdis->rcItem);

    // Restore the previous colors.
    dc.SetTextColor(clrForeground);
    dc.SetBkColor(clrBackground);

    dc.Detach();
}

void ColorPickerComboBox::PreSubclassWindow()
{
    CComboBox::PreSubclassWindow();

    AddColor(L"Black", RGB(0,0,0));
    AddColor(L"Maroon", RGB(128,0,0));
    AddColor(L"Green", RGB(0, 128, 0));
    AddColor(L"Olive", RGB(128, 128, 0));
    AddColor(L"Navy", RGB(0, 0, 128));
    AddColor(L"Purple", RGB(128, 0, 128));
    AddColor(L"Teal", RGB(0, 128, 128));
    AddColor(L"Gray", RGB(128, 128, 128));
    AddColor(L"Silver", RGB(192, 192, 192));
    AddColor(L"Red", RGB(255, 0, 0));
    AddColor(L"Lime", RGB(0, 255, 0));
    AddColor(L"Yellow", RGB(255, 255, 0));
    AddColor(L"Blue", RGB(0, 0, 255));
    AddColor(L"Fuchsia", RGB(255, 0, 255));
    AddColor(L"Aqua", RGB(0, 255, 255));
    AddColor(L"White", RGB(255, 255, 255));
}

