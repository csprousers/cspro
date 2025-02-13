#pragma once

#include <zUtilF/zUtilF.h>


class CLASS_DECL_ZUTILF StyledComboBox : public CComboBox
{
    DECLARE_DYNAMIC(StyledComboBox)

public:
    StyledComboBox();

    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID) override;

    // Set font and color for an item in list
    void SetStyle(int index, LOGFONT font, COLORREF color = GetSysColor(COLOR_WINDOWTEXT));

    // Call after adding items to update the size of the dropdown
    // area to fit the items
    void UpdateDropDownSize();

    // Get size of an item in dropdown (max of all item sizes)
    CSize GetItemSize();

    // Overrides for owner draw
    void DrawItem(LPDRAWITEMSTRUCT lpdis) override;
    void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) override;
    void DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct) override;

protected:
    DECLARE_MESSAGE_MAP()

private:

    struct ItemData {
        LOGFONT m_font;
        COLORREF m_color;
    };

    CSize CalculateSize(int item_index);
};
