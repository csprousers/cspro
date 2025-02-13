#include "StdAfx.h"
#include "SecurityOptionsDlg.h"


namespace
{
    struct MinuteOption { const TCHAR* text; int minutes; };

    static MinuteOption MinuteOptions[] =
    {
        { _T("Never"),     0 },
        { _T("One Hour"),  60 },
        { _T("One Day"),   60 * 24 },
        { _T("One Week"),  60 * 24 * 7 },
        { _T("One Month"), 60 * 24 * 30 },
        { _T("One Year"),  60 * 24 * 365 },
        { _T("Forever"),   INT_MAX },
        { _T("Custom"),    0 },
    };

    const size_t MinuteNeverIndex = 0;
    const size_t MinuteForeverIndex = 6;
    const size_t MinuteCustomIndex = 7;

    CString MinutesToText(int minutes)
    {
        if( minutes <= 0 || minutes == INT_MAX )
            return _T("");

        return IntToString(minutes);
    }

    int MinutesToComboBoxIndex(int minutes)
    {
        for( size_t i = 0; i < _countof(MinuteOptions) - 1; i++ )
        {
            if( minutes == MinuteOptions[i].minutes )
                return i;
        }

        return MinuteCustomIndex;
    }
}


IMPLEMENT_DYNAMIC(SecurityOptionsDlg, CDialog)

BEGIN_MESSAGE_MAP(SecurityOptionsDlg, CDialog)
    ON_CBN_SELENDOK(IDC_COMBO_PASSWORD_CACHE_MINUTES, OnMinutesComboChange)
    ON_EN_CHANGE(IDC_EDIT_PASSWORD_CACHE_MINUTES, OnMinutesTextChange)
END_MESSAGE_MAP()


SecurityOptionsDlg::SecurityOptionsDlg(const CDataDict& dictionary, CWnd* pParent /*=NULL*/)
    :   CDialog(IDD_SECURITY_OPTIONS, pParent)
{
    m_allowDataViewerModifications = dictionary.GetAllowDataViewerModifications();
    m_allowExport = dictionary.GetAllowExport();
    m_cachedPasswordMinutes = dictionary.GetCachedPasswordMinutes();
    m_minutesText = MinutesToText(m_cachedPasswordMinutes);
}


void SecurityOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_CHECK_ALLOW_DATA_VIEWER_MODIFICATIONS, m_allowDataViewerModifications);
    DDX_Check(pDX, IDC_CHECK_ALLOW_EXPORTS, m_allowExport);
    DDX_Control(pDX, IDC_COMBO_PASSWORD_CACHE_MINUTES, m_minutesCombo);
    DDX_Text(pDX, IDC_EDIT_PASSWORD_CACHE_MINUTES, m_minutesText);
}


BOOL SecurityOptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    for( size_t i = 0; i < _countof(MinuteOptions); i++ )
        m_minutesCombo.AddString(MinuteOptions[i].text);

    m_minutesCombo.SetCurSel(MinutesToComboBoxIndex(m_cachedPasswordMinutes));

    return TRUE;
}


void SecurityOptionsDlg::OnOK()
{
    UpdateData(TRUE);

    if( m_minutesCombo.GetCurSel() != MinuteCustomIndex )
        m_cachedPasswordMinutes = MinuteOptions[m_minutesCombo.GetCurSel()].minutes;

    else
    {
        m_cachedPasswordMinutes = -1;

        if( m_minutesText.IsNumeric() )
            m_cachedPasswordMinutes = _ttoi(m_minutesText);

        if( m_cachedPasswordMinutes < 0 )
        {
            AfxMessageBox(_T("The minutes value must be a non-negative number"));
            return;
        }
    }

    CDialog::OnOK();
}


void SecurityOptionsDlg::OnMinutesComboChange()
{
    if( m_minutesCombo.GetCurSel() != MinuteCustomIndex )
    {
        m_minutesText = MinutesToText(MinuteOptions[m_minutesCombo.GetCurSel()].minutes);
        UpdateData(FALSE);
    }
}


void SecurityOptionsDlg::OnMinutesTextChange()
{
    UpdateData(TRUE);

    int current_index = m_minutesCombo.GetCurSel();
    int new_index = current_index;

    if( SO::IsBlank(m_minutesText) )
    {
        if( current_index != MinuteNeverIndex && current_index != MinuteForeverIndex )
            new_index = MinuteNeverIndex;
    }

    else
    {
        new_index = m_minutesText.IsNumeric() ? MinutesToComboBoxIndex(_ttoi(m_minutesText))
                                              : MinuteCustomIndex;
    }

    if( current_index != new_index )
        m_minutesCombo.SetCurSel(new_index);
}
