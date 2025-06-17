//***************************************************************************
//  File name: TwoFileDialog.cpp
//
//  Description:
//       CSSort two file dialog implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Nov 00   bmd     Created for CSPro 2.1
//
//***************************************************************************

#include "StdAfx.h"
#include "TwoFileDialog.h"
#include <zUtilO/PathHelpers.h>
#include <zBridgeO/DataFileDlg.h>


BEGIN_MESSAGE_MAP(CTwoFileDialog, CDialog)
    ON_EN_CHANGE(IDC_IN_FILE_NAME, OnChangeConnectionString)
    ON_EN_KILLFOCUS(IDC_IN_FILE_NAME, OnKillFocusInputConnectionString)
    ON_BN_CLICKED(IDC_IN_FILE_BROWSE, OnInputBrowse)
    ON_EN_CHANGE(IDC_OUT_FILE_NAME, OnChangeConnectionString)
    ON_BN_CLICKED(IDC_OUT_FILE_BROWSE, OnOutputBrowse)
END_MESSAGE_MAP()


CTwoFileDialog::CTwoFileDialog(PFF& pff, std::string dictionary_file_path, CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_TWOFILE, pParent),
        m_pff(pff),
        m_dictionaryFilePath(std::move(dictionary_file_path)),
        m_inputConnectionString(m_pff.GetSingleInputDataConnectionString()),
        m_outputConnectionString(m_pff.GetSingleOutputDataConnectionString())
{
}


void CTwoFileDialog::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_IN_FILE_NAME, m_inputConnectionString);
    DDX_Text(pDX, IDC_OUT_FILE_NAME, m_outputConnectionString);

    GetDlgItem(IDOK)->EnableWindow(( m_inputConnectionString.IsDefined() &&
                                     m_outputConnectionString.IsDefined() ));
}


bool CTwoFileDialog::SuggestOutputConnectionString()
{
    if( m_inputConnectionString.HasFilePath() && !m_outputConnectionString.IsDefined() )
    {
        m_outputConnectionString = PathHelpers::AppendToConnectionStringFilename(m_inputConnectionString, "_sorted");
        return true;
    }

    return false;
}


void CTwoFileDialog::OnChangeConnectionString()
{
    UpdateData(TRUE);
}


void CTwoFileDialog::OnKillFocusInputConnectionString()
{
    UpdateData(TRUE);

    if( SuggestOutputConnectionString() )
    {
        UpdateData(FALSE);
        GotoDlgCtrl(static_cast<CEdit*>(GetDlgItem(IDC_IN_FILE_NAME)));
    }
}


void CTwoFileDialog::OnInputBrowse()
{
    UpdateData(TRUE);

    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, m_inputConnectionString);
    data_file_dlg.SetDictionaryFilePath(m_dictionaryFilePath);

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_inputConnectionString = data_file_dlg.GetConnectionString();
    SuggestOutputConnectionString();

    UpdateData(FALSE);
    GotoDlgCtrl(static_cast<CEdit*>(GetDlgItem(IDC_OUT_FILE_NAME)));
}


void CTwoFileDialog::OnOutputBrowse()
{
    UpdateData(TRUE);

    DataFileDlg data_file_dlg(DataFileDlg::Type::CreateNew, false, m_outputConnectionString);
    data_file_dlg.SetDictionaryFilePath(m_dictionaryFilePath)
                 .SuggestMatchingDataRepositoryType(m_inputConnectionString)
                 .WarnIfDifferentDataRepositoryType();

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_outputConnectionString = data_file_dlg.GetConnectionString();

    UpdateData(FALSE);
    GotoDlgCtrl(static_cast<CEdit*>(GetDlgItem(IDC_OUT_FILE_NAME)));
}


void CTwoFileDialog::OnOK()
{
    UpdateData(TRUE);

    if( m_inputConnectionString.SharesResource(m_outputConnectionString) )
    {
        AfxMessageBox(L"The output data source cannot be the same as the input file data source.", MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    m_pff.SetSingleInputDataConnectionString(std::move(m_inputConnectionString));
    m_pff.SetSingleOutputDataConnectionString(std::move(m_outputConnectionString));

    __super::OnOK();
}
