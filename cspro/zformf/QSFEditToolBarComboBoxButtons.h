#pragma once


// --------------------------------------------------------------------------
// QSFEditToolBarComboBoxButton
//
// Specialized tool bar combo box that is not linked to other toolbar
// combo boxes with same command ID.
//
// When the selected item or text in a CMFCToolBarComboBoxButton changes,
// the default implementation makes the same update to all
// CMFCToolBarComboBoxButtons that share the same command ID.
//
// This class reimplements the NotifyCommand method and removes the
// mirroring functionality.
// --------------------------------------------------------------------------

class QSFEditToolBarComboBoxButton : public CMFCToolBarComboBoxButton
{
    DECLARE_SERIAL(QSFEditToolBarComboBoxButton)

public:
    using CMFCToolBarComboBoxButton::CMFCToolBarComboBoxButton;

    BOOL NotifyCommand(int iNotifyCode) override;

    void Invalidate();
};



// --------------------------------------------------------------------------
// QSFEditToolBarStyledComboBoxButton
// 
// CMFCToolBarButton that uses styled combo box.
// --------------------------------------------------------------------------

class QSFEditToolBarStyledComboBoxButton : public QSFEditToolBarComboBoxButton
{
    DECLARE_SERIAL(QSFEditToolBarStyledComboBoxButton)

public:
    using QSFEditToolBarComboBoxButton::QSFEditToolBarComboBoxButton;

    void AddItem(LPCTSTR lpszItem, LOGFONT font, COLORREF color = GetSysColor(COLOR_WINDOWTEXT));

    void CopyFrom(const CMFCToolBarButton& s) override;

protected:
    CComboBox* CreateCombo(CWnd* pWndParent, const CRect& rect) override;

private:
    struct Item
    {
        CString text;
        LOGFONT logfont;
        COLORREF color;
    };

    std::vector<Item> m_items;
};



// --------------------------------------------------------------------------
// QSFEditNumericSortToolBarComboBoxButton
// --------------------------------------------------------------------------

class QSFEditNumericSortToolBarComboBoxButton: public QSFEditToolBarComboBoxButton
{
    DECLARE_SERIAL(QSFEditNumericSortToolBarComboBoxButton)

public:
    using QSFEditToolBarComboBoxButton::QSFEditToolBarComboBoxButton;

    int Compare(LPCTSTR lpszItem1, LPCTSTR lpszItem2) override;
};
