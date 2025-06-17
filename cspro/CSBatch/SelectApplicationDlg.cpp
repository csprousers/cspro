#include "StdAfx.h"
#include "SelectApplicationDlg.h"
#include <zToolsO/WinSettings.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/FileDlg.h>
#include <zUtilO/FileExtensions.h>
#include <zUtilO/ImsaDlg.h>
#include <zUtilO/WindowHelpers.h>


BEGIN_MESSAGE_MAP(SelectApplicationDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_BN_CLICKED(IDC_SELECT_APPLICATION_FILENAME, OnSelectApplicationFilePath)
END_MESSAGE_MAP()


SelectApplicationDlg::SelectApplicationDlg(CWnd* const pParent/* = nullptr*/)
    :   CDialog(IDD_CSBATCH_DIALOG, pParent),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
{
}


void SelectApplicationDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_APPLICATION_FILENAME, m_applicationFilePath);
}


BOOL SelectApplicationDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::SetDialogSystemIcon(*this, m_hIcon);
    WindowHelpers::AddDialogAboutMenuItem(*this, IDS_ABOUTBOX, IDM_ABOUTBOX);

    return TRUE;
}


void SelectApplicationDlg::OnSysCommand(const UINT nID, const LPARAM lParam)
{
    if( ( nID & 0xFFF0 ) == IDM_ABOUTBOX )
    {
        CIMSAAboutDlg about_dlg(L"CSBatch", m_hIcon);
        about_dlg.DoModal();
    }

    else
    {
        __super::OnSysCommand(nID, lParam);
    }
}


void SelectApplicationDlg::OnSelectApplicationFilePath()
{
    UpdateData(TRUE);

    OpenFileDlg open_file_dlg(0, nullptr, m_applicationFilePath,
                              L"Batch Application Files (*.bch)|*.bch|PFF Files (*.pff)|*.pff||", this);

    // if the file exists, start in its directory
    if( PortableFunctions::FileIsRegular(m_applicationFilePath) )
    {
        open_file_dlg.SetInitialDirectory(PortableFunctions::PathGetDirectory(m_applicationFilePath));
    }

    // otherwise use the last application directory
    else
    {
        open_file_dlg.SetInitialDirectory(WinSettings::Read<std::wstring>(WinSettings::Type::LastApplicationDirectory));
    }

    if( open_file_dlg.DoModal() != IDOK )
        return;

    m_applicationFilePath = open_file_dlg.GetFilePath();

    WinSettings::Write(WinSettings::Type::LastApplicationDirectory, PortableFunctions::PathGetDirectory(m_applicationFilePath));

    UpdateData(FALSE);
}
