#include "StdAfx.h"
#include "QSFEditToolBarStyledComboBoxButton.h"
#include <zUtilF/StyledComboBox.h>

IMPLEMENT_SERIAL(QSFEditToolBarStyledComboBoxButton, QSFEditToolBarComboBoxButton, 1)

QSFEditToolBarStyledComboBoxButton::QSFEditToolBarStyledComboBoxButton()
{
}

QSFEditToolBarStyledComboBoxButton::QSFEditToolBarStyledComboBoxButton(UINT uiID, int iImage, DWORD dwStyle, int iWidth)
    : QSFEditToolBarComboBoxButton(uiID, iImage, dwStyle, iWidth)
{
}

void QSFEditToolBarStyledComboBoxButton::AddItem(LPCTSTR lpszItem, LOGFONT font, COLORREF color)
{
    m_items.emplace_back(Item{lpszItem, std::move(font), color});
    if (m_pWndCombo->GetSafeHwnd() != NULL) {
        auto combo = DYNAMIC_DOWNCAST(StyledComboBox, m_pWndCombo);
        int index = combo->AddString(lpszItem);
        combo->SetStyle(index, font, color);
    }
    m_lstItems.AddTail(lpszItem);
    m_lstItemData.AddTail((DWORD_PTR) 0);
}

void QSFEditToolBarStyledComboBoxButton::CopyFrom(const CMFCToolBarButton& s)
{
    QSFEditToolBarComboBoxButton::CopyFrom(s);
    const QSFEditToolBarStyledComboBoxButton& src = (const QSFEditToolBarStyledComboBoxButton&)s;
    m_items = src.m_items;
}

CComboBox* QSFEditToolBarStyledComboBoxButton::CreateCombo(CWnd* pWndParent, const CRect& rect)
{
    StyledComboBox* combo = new StyledComboBox;
    if (!combo->Create(m_dwStyle, rect, pWndParent, m_nID))
    {
        delete combo;
        return NULL;
    }

    for (const Item& item : m_items) {
        int index = combo->AddString(item.text);
        combo->SetStyle(index, item.logfont, item.color);
    }

    combo->UpdateDropDownSize();
    int drop_down_height = combo->GetItemSize().cy * combo->GetCount() + 100; // xtra space for edit too - Windows shrinks to fit items if too big
    SetDropDownHeight(drop_down_height);
    return combo;
}
