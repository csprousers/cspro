#include "StdAfx.h"
#include "QSFEditToolBarComboBoxButtons.h"
#include <zUtilF/StyledComboBox.h>


// --------------------------------------------------------------------------
// QSFEditToolBarComboBoxButton
// --------------------------------------------------------------------------

IMPLEMENT_SERIAL(QSFEditToolBarComboBoxButton, CMFCToolBarComboBoxButton, 1)


BOOL QSFEditToolBarComboBoxButton::NotifyCommand(int iNotifyCode)
{
    // This is all copied from CMFCToolBarComboBoxButton::NotifyCommand with the sections
    // that update other combo boxes removed.

    if (m_pWndCombo->GetSafeHwnd() == NULL)
    {
        return FALSE;
    }

    if (m_bFlat && iNotifyCode == 0)
    {
        return TRUE;
    }

    if (m_bFlat && m_pWndCombo->GetParent() != NULL)
    {
        m_pWndCombo->GetParent()->InvalidateRect(m_rectCombo);
        m_pWndCombo->GetParent()->UpdateWindow();
    }

    switch (iNotifyCode)
    {
    case CBN_SELENDOK:
    {
        m_iSelIndex = m_pWndCombo->GetCurSel();
        if (m_iSelIndex < 0)
        {
            return FALSE;
        }

        m_pWndCombo->GetLBText(m_iSelIndex, m_strEdit);
        if (m_pWndEdit != NULL)
        {
            m_pWndEdit->SetWindowText(m_strEdit);
        }

    }

    if (m_pWndEdit != NULL)
    {
        m_pWndEdit->SetFocus();
    }

    return TRUE;

    case CBN_KILLFOCUS:
    case CBN_EDITUPDATE:
        return TRUE;

    case CBN_SETFOCUS:
        if (m_pWndEdit != NULL)
        {
            m_pWndEdit->SetFocus();
        }
        return TRUE;

    case CBN_SELCHANGE: // yurig: process selchange
        if (m_pWndEdit != NULL)
        {
            CString strEdit;
            m_pWndCombo->GetLBText(m_pWndCombo->GetCurSel(), strEdit);
            m_pWndEdit->SetWindowText(strEdit);
        }

        return TRUE;

    case CBN_EDITCHANGE:
    {
        m_pWndCombo->GetWindowText(m_strEdit);

        if (m_pWndEdit != NULL && m_pWndEdit->GetSafeHwnd() != NULL)
        {
            CString str;
            m_pWndEdit->GetWindowText(str);
            CComboBox* pBox = GetComboBox();
            if (pBox != NULL && pBox->GetSafeHwnd() != NULL)
            {
                int nCurSel = pBox->GetCurSel();
                int nNextSel = pBox->FindStringExact(nCurSel + 1, str);
                if (nNextSel == -1)
                {
                    nNextSel = pBox->FindString(nCurSel + 1, str);
                }

                if (nNextSel != -1)
                {
                    pBox->SetCurSel(nNextSel);
                }

                pBox->SetWindowText(str);
            }
        }

        return TRUE;
    }

    }

    return FALSE;
}


void QSFEditToolBarComboBoxButton::Invalidate()
{
    m_pWndCombo->GetParent()->InvalidateRect(m_rectCombo);
}



// --------------------------------------------------------------------------
// QSFEditToolBarStyledComboBoxButton
// --------------------------------------------------------------------------

IMPLEMENT_SERIAL(QSFEditToolBarStyledComboBoxButton, QSFEditToolBarComboBoxButton, 1)


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



// --------------------------------------------------------------------------
// QSFEditNumericSortToolBarComboBoxButton
// --------------------------------------------------------------------------

IMPLEMENT_SERIAL(QSFEditNumericSortToolBarComboBoxButton, QSFEditToolBarComboBoxButton, 1)


int QSFEditNumericSortToolBarComboBoxButton::Compare(LPCTSTR lpszItem1, LPCTSTR lpszItem2)
{
    return _ttoi(lpszItem1) - _ttoi(lpszItem2);
}
