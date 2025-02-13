#include "StdAfx.h"
#include "SelectApplicationDlg.h"
#include <zToolsO/WinSettings.h>
#include <zUtilO/Filedlg.h>
#include <zUtilO/imsaDlg.H>
#include <zUtilO/WindowHelpers.h>


BEGIN_MESSAGE_MAP(SelectApplicationDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_BN_CLICKED(IDC_SELECT_APPLICATION_FILENAME, OnSelectApplicationFilename)
END_MESSAGE_MAP()


SelectApplicationDlg::SelectApplicationDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_CSBATCH_DIALOG, pParent),
        m_hIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME))
{
}


void SelectApplicationDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_APPLICATION_FILENAME, m_applicationFilename);
}


BOOL SelectApplicationDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    WindowHelpers::SetDialogSystemIcon(*this, m_hIcon);
    WindowHelpers::AddDialogAboutMenuItem(*this, IDS_ABOUTBOX, IDM_ABOUTBOX);

    return TRUE;
}


void SelectApplicationDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if( ( nID & 0xFFF0 ) == IDM_ABOUTBOX )
    {
        CIMSAAboutDlg about_dlg;
        about_dlg.m_hIcon = m_hIcon;
        about_dlg.m_csModuleName = _T("CSBatch");
        about_dlg.DoModal();
    }

    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}


void SelectApplicationDlg::OnSelectApplicationFilename()
{
    const TCHAR* FileFilter = _T("Batch Application Files (*.bch)|*.bch|PFF Files (*.pff)|*.pff||");
    CIMSAFileDialog file_dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, FileFilter, this, CFD_NO_DIR, FALSE);

    UpdateData(TRUE);

    CString initial_directory;

    // if the file exists, start in its directory
    if( PortableFunctions::FileIsRegular(m_applicationFilename) )
    {
        initial_directory = PortableFunctions::PathGetDirectory<CString>(m_applicationFilename);
    }

    // otherwise use the last application directory
    else if( CString directory = WinSettings::Read<CString>(WinSettings::Type::LastApplicationDirectory); !directory.IsEmpty() )
    {
        initial_directory = directory;
    }

    file_dlg.m_ofn.lpstrInitialDir = initial_directory;

    if( file_dlg.DoModal() != IDOK )
        return;

    m_applicationFilename = file_dlg.GetPathName();

    WinSettings::Write(WinSettings::Type::LastApplicationDirectory, PortableFunctions::PathGetDirectory<CString>(m_applicationFilename));

    UpdateData(FALSE);
}
