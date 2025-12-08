#include "StdAfx.h"
#include "SecurityOptionsDlg.h"


namespace
{
    struct MinuteOption { const wchar_t* text; int minutes; };

    constexpr MinuteOption MinuteOptions[] =
    {
        { L"Never",     0 },
        { L"One Hour",  60 },
        { L"One Day",   60 * 24 },
        { L"One Week",  60 * 24 * 7 },
        { L"One Month", 60 * 24 * 30 },
        { L"One Year",  60 * 24 * 365 },
        { L"Forever",   INT_MAX },
        { L"Custom",    0 },
    };

    constexpr size_t MinuteNeverIndex = 0;
    constexpr size_t MinuteForeverIndex = 6;
    constexpr size_t MinuteCustomIndex = 7;
}


BEGIN_MESSAGE_MAP(SecurityOptionsDlg, CDialog)
    ON_CBN_SELENDOK(IDC_COMBO_PASSWORD_CACHE_MINUTES, OnMinutesComboChange)
    ON_EN_CHANGE(IDC_EDIT_PASSWORD_CACHE_MINUTES, OnMinutesTextChange)
END_MESSAGE_MAP()


SecurityOptionsDlg::SecurityOptionsDlg(const CDataDict& dictionary, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_SECURITY_OPTIONS, pParent),
        m_allowDataManagerModifications(dictionary.GetAllowDataManagerModifications()),
        m_allowExport(dictionary.GetAllowExport()),
        m_cachedPasswordMinutes(dictionary.GetCachedPasswordMinutes()),
        m_minutesText(MinutesToText(m_cachedPasswordMinutes))
{
}


void SecurityOptionsDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_CHECK_ALLOW_DATA_MANAGER_MODIFICATIONS, m_allowDataManagerModifications);
    DDX_Check(pDX, IDC_CHECK_ALLOW_EXPORTS, m_allowExport);
    DDX_Text(pDX, IDC_EDIT_PASSWORD_CACHE_MINUTES, m_minutesText);
    DDX_Control(pDX, IDC_COMBO_PASSWORD_CACHE_MINUTES, m_minutesCombo);
}


BOOL SecurityOptionsDlg::OnInitDialog()
{
    __super::OnInitDialog();

    for( size_t i = 0; i < _countof(MinuteOptions); i++ )
        m_minutesCombo.AddString(MinuteOptions[i].text);

    m_minutesCombo.SetCurSel(MinutesToComboBoxIndex(m_cachedPasswordMinutes));

    return TRUE;
}


void SecurityOptionsDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( m_minutesCombo.GetCurSel() != MinuteCustomIndex )
        {
            m_cachedPasswordMinutes = MinuteOptions[m_minutesCombo.GetCurSel()].minutes;
        }

        else
        {
            m_cachedPasswordMinutes = -1;

            if( CIMSAString::IsNumeric(m_minutesText) )
                m_cachedPasswordMinutes = std::stoi(m_minutesText);

            if( m_cachedPasswordMinutes < 0 )
                throw CSProException("The minutes value must be a non-negative number.");
        }

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
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

    const int current_index = m_minutesCombo.GetCurSel();
    int new_index = current_index;

    if( SO::IsBlank(m_minutesText) )
    {
        if( current_index != MinuteNeverIndex && current_index != MinuteForeverIndex )
            new_index = MinuteNeverIndex;
    }

    else
    {
        new_index = CIMSAString::IsNumeric(m_minutesText) ? MinutesToComboBoxIndex(std::stoi(m_minutesText)) :
                                                            MinuteCustomIndex;
    }

    if( current_index != new_index )
        m_minutesCombo.SetCurSel(new_index);
}


std::string SecurityOptionsDlg::MinutesToText(const int minutes)
{
    return ( minutes <= 0 || minutes == INT_MAX ) ? std::string() :
                                                    IntToString(minutes);
}


int SecurityOptionsDlg::MinutesToComboBoxIndex(const int minutes)
{
    for( int i = 0; i < _countof(MinuteOptions) - 1; ++i )
    {
        if( minutes == MinuteOptions[i].minutes )
            return i;
    }

    return MinuteCustomIndex;
}
