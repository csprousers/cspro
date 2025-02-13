#pragma once

#include <zformf/QSFEditToolBarComboBoxButton.h>

/// <summary>
/// MFCToolBarButton that uses styled combo box
/// </summary>
class QSFEditToolBarStyledComboBoxButton : public QSFEditToolBarComboBoxButton
{
    DECLARE_SERIAL(QSFEditToolBarStyledComboBoxButton)

public:
    QSFEditToolBarStyledComboBoxButton();
    QSFEditToolBarStyledComboBoxButton(UINT uiID, int iImage, DWORD dwStyle = CBS_DROPDOWNLIST, int iWidth = 0);

    void AddItem(LPCTSTR lpszItem, LOGFONT font, COLORREF color = GetSysColor(COLOR_WINDOWTEXT));

    void CopyFrom(const CMFCToolBarButton& s) override;

protected:
    CComboBox* CreateCombo(CWnd* pWndParent, const CRect& rect) override;

    struct Item {
        CString text;
        LOGFONT logfont;
        COLORREF color;
    };
    std::vector<Item> m_items;
};

