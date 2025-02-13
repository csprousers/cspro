//***************************************************************************
//  File name: TwoFldlg.cpp
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
#include "TwoFldlg.h"
#include <zUtilO/PathHelpers.h>
#include <ZBRIDGEO/DataFileDlg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace
{
    CString GetSuggestedOutputFilename(const ConnectionString& connection_string)
    {
        ConnectionString suggested_connection_string = PathHelpers::AppendToConnectionStringFilename(connection_string, _T("_sorted"));
        return suggested_connection_string.IsDefined() ? WS2CS(suggested_connection_string.GetFilename()) : CString();
    }
}


/////////////////////////////////////////////////////////////////////////////
// CTwoFileDialog dialog


CTwoFileDialog::CTwoFileDialog(PFF& pff, CString dictionary_filename, CWnd* pParent /*=NULL*/)
    :   CDialog(CTwoFileDialog::IDD, pParent),
        m_pff(pff),
        m_dictionaryFilename(dictionary_filename)
{
    if( m_pff.GetSingleInputDataConnectionString().IsDefined() )
        m_csInFileName = WS2CS(m_pff.GetSingleInputDataConnectionString().ToString());

    if( m_pff.GetSingleOutputDataConnectionString().IsDefined() )
        m_csOutFileName = WS2CS(m_pff.GetSingleOutputDataConnectionString().ToString());
}


void CTwoFileDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTwoFileDialog)
    DDX_Text(pDX, IDC_IN_FILE_NAME, m_csInFileName);
    DDX_Text(pDX, IDC_OUT_FILE_NAME, m_csOutFileName);
    //}}AFX_DATA_MAP
    GetDlgItem(IDOK)->EnableWindow(!SO::IsBlank(m_csInFileName) && !SO::IsBlank(m_csOutFileName));
}


BEGIN_MESSAGE_MAP(CTwoFileDialog, CDialog)
    //{{AFX_MSG_MAP(CTwoFileDialog)
    ON_EN_CHANGE(IDC_IN_FILE_NAME, OnChangeFileName)
    ON_EN_KILLFOCUS(IDC_IN_FILE_NAME, OnKillfocusInFileName)
    ON_BN_CLICKED(IDC_IN_FILE_BROWSE, OnInFileBrowse)
    ON_BN_CLICKED(IDC_OUT_FILE_BROWSE, OnOutFileBrowse)
    ON_EN_CHANGE(IDC_OUT_FILE_NAME, OnChangeFileName)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
//
//                    CTwoFileDialog::OnChangeFileName
//
/////////////////////////////////////////////////////////////////////////////

void CTwoFileDialog::OnChangeFileName() {

    UpdateData();
}


/////////////////////////////////////////////////////////////////////////////
//
//                   CTwoFileDialog::OnKillfocusInFileName
//
/////////////////////////////////////////////////////////////////////////////

void CTwoFileDialog::OnKillfocusInFileName() {

    UpdateData();

    if( !SO::IsBlank(m_csInFileName) && SO::IsBlank(m_csOutFileName) )
    {
        ConnectionString input_connection_string(m_csInFileName);

        if( input_connection_string.IsFilenamePresent() )
        {
            m_csOutFileName = GetSuggestedOutputFilename(input_connection_string);
            UpdateData(FALSE);
            GotoDlgCtrl((CEdit*) GetDlgItem(IDC_IN_FILE_NAME));
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                      CTwoFileDialog::OnInFileBrowse
//
/////////////////////////////////////////////////////////////////////////////

void CTwoFileDialog::OnInFileBrowse() {

    UpdateData();

    DataFileDlg data_file_dlg(DataFileDlg::Type::OpenExisting, true, ConnectionString(m_csInFileName));
    data_file_dlg.SetDictionaryFilename(m_dictionaryFilename);

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_csInFileName = WS2CS(data_file_dlg.GetConnectionString().ToString());

    if( SO::IsBlank(m_csOutFileName) )
        m_csOutFileName = GetSuggestedOutputFilename(data_file_dlg.GetConnectionString());

    UpdateData(FALSE);
    GotoDlgCtrl((CEdit*) GetDlgItem(IDC_OUT_FILE_NAME));
}


/////////////////////////////////////////////////////////////////////////////
//
//                    CTwoFileDialog::OnOutFileBrowse
//
/////////////////////////////////////////////////////////////////////////////

void CTwoFileDialog::OnOutFileBrowse() {

    UpdateData();

    DataFileDlg data_file_dlg(DataFileDlg::Type::CreateNew, false, ConnectionString(m_csOutFileName));
    data_file_dlg.SetDictionaryFilename(m_dictionaryFilename)
                 .SuggestMatchingDataRepositoryType(ConnectionString(m_csInFileName))
                 .WarnIfDifferentDataRepositoryType();

    if( data_file_dlg.DoModal() != IDOK )
        return;

    m_csOutFileName = WS2CS(data_file_dlg.GetConnectionString().ToString());

    UpdateData(FALSE);
    GotoDlgCtrl((CEdit*) GetDlgItem(IDC_OUT_FILE_NAME));
}


/////////////////////////////////////////////////////////////////////////////
//
//                          CTwoFileDialog::OnOK
//
/////////////////////////////////////////////////////////////////////////////

void CTwoFileDialog::OnOK() {

    UpdateData();

    ConnectionString input_connection_string(m_csInFileName);
    ConnectionString output_connection_string(m_csOutFileName);

    if( input_connection_string.IsFilenamePresent() && input_connection_string.Equals(output_connection_string) )
    {
        AfxMessageBox(_T("The output file name cannot be the same as the input file name."), MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    m_pff.SetSingleInputDataConnectionString(input_connection_string);
    m_pff.SetSingleOutputDataConnectionString(output_connection_string);

    CDialog::OnOK();
}
