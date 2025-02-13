#include "StdAfx.h"
#include "MappingOptionsDlg.h"
#include <zDictO/DictionaryIterator.h>


IMPLEMENT_DYNAMIC(MappingOptionsDlg, CDialog)

BEGIN_MESSAGE_MAP(MappingOptionsDlg, CDialog)
    ON_BN_CLICKED(IDC_ENABLE_MAPPING_CASE_LIST, OnBnClickedEnableMappingCaseList)
END_MESSAGE_MAP()


MappingOptionsDlg::MappingOptionsDlg(AppMappingOptions& mapping_options, const CDataDict& dictionary, CWnd* pParent/* =nullptr*/)
    :   CDialog(IDD, pParent),
        m_mappingOptions(mapping_options),
        m_dictionary(dictionary),
        m_enabled(m_mappingOptions.IsDefined())
{
}


void MappingOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_ENABLE_MAPPING_CASE_LIST, m_enabled);
    DDX_Control(pDX, IDC_COMBO_LATITUDE, m_comboLatitude);
    DDX_CBStringExact(pDX, IDC_COMBO_LATITUDE, m_mappingOptions.latitude_item);
    DDX_Control(pDX, IDC_COMBO_LONGITUDE, m_comboLongitude);
    DDX_CBStringExact(pDX, IDC_COMBO_LONGITUDE, m_mappingOptions.longitude_item);
}


BOOL MappingOptionsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    DictionaryIterator::Foreach<CDictItem>(m_dictionary,
        [&](const CDictItem& dict_item)
        {
            if( IsNumeric(dict_item) && dict_item.GetDecimal() > 0 )
            {
                m_comboLatitude.AddString(dict_item.GetName());
                m_comboLongitude.AddString(dict_item.GetName());
            }
        });


    // For update now that combo strings are filled so that
    // previous lat/long selections will be shown
    UpdateData(FALSE);

    UpdateControlsEnabled();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void MappingOptionsDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( m_enabled )
        {
            if( !m_mappingOptions.IsDefined() )
                throw CSProException(_T("Please choose the dictionary items for latitude and longitude."));

            if( m_mappingOptions.latitude_item == m_mappingOptions.longitude_item )
                throw CSProException(_T("The latitude and longitude dictionary items must be different."));
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return;
    }

    // clear any selections if not enabled
    if( !m_enabled )
    {
        m_mappingOptions.latitude_item.clear();
        m_mappingOptions.longitude_item.clear();
    }

    CDialog::OnOK();
}


void MappingOptionsDlg::OnBnClickedEnableMappingCaseList()
{
    UpdateData(TRUE);
    UpdateControlsEnabled();
}


void MappingOptionsDlg::UpdateControlsEnabled()
{
    m_comboLatitude.EnableWindow(m_enabled);
    m_comboLongitude.EnableWindow(m_enabled);
}
