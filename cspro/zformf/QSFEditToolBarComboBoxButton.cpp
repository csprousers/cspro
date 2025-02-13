#include "StdAfx.h"
#include "QSFEditToolBarComboBoxButton.h"

IMPLEMENT_SERIAL(QSFEditToolBarComboBoxButton, CMFCToolBarComboBoxButton, 1)

QSFEditToolBarComboBoxButton::QSFEditToolBarComboBoxButton()
{
}

QSFEditToolBarComboBoxButton::QSFEditToolBarComboBoxButton(UINT uiID, int iImage, DWORD dwStyle, int iWidth):
    CMFCToolBarComboBoxButton(uiID, iImage, dwStyle, iWidth)
{
}

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
    }
    return TRUE;
    }

    return FALSE;
}

void QSFEditToolBarComboBoxButton::Invalidate()
{
    m_pWndCombo->GetParent()->InvalidateRect(m_rectCombo);
}
