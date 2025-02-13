#pragma once

#include <zUtilF/zUtilF.h>


/// <summary>
/// Custom draw combo box that shows list of colors
/// </summary>
class CLASS_DECL_ZUTILF ColorPickerComboBox : public CComboBox
{
    DECLARE_DYNAMIC(ColorPickerComboBox)

public:
    ColorPickerComboBox();

    void AddColor(LPCTSTR name, COLORREF color);
    int FindColor(COLORREF color) const;
    std::optional<COLORREF> GetSelColor() const;

protected:

    DECLARE_MESSAGE_MAP()
public:
    virtual void DrawItem(LPDRAWITEMSTRUCT lpdis) override;
    void PreSubclassWindow() override;
};
