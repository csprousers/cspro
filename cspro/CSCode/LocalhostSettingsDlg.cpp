#include "StdAfx.h"
#include "LocalhostSettingsDlg.h"
#include <zHtml/LocalhostSettings.h>


BEGIN_MESSAGE_MAP(LocalhostSettingsDlg, CDialog)
    ON_COMMAND(IDC_LOCALHOST_CREATE_MAPPING, OnCreateMapping)
END_MESSAGE_MAP()


LocalhostSettingsDlg::LocalhostSettingsDlg(CWnd* pParent/* = nullptr*/)
    :   CDialog(IDD_LOCALHOST, pParent),
        m_logicalDrives(GetLogicalDrivesVector())
{
    m_initialSettings.start_automatically = ( WinSettings::Read<DWORD>(WinSettings::Type::LocalhostStartAutomatically, 0) != 0 );

    std::optional<int> preferred_port = LocalhostSettings::GetPreferredPort();

    if( preferred_port.has_value() )
        m_initialSettings.preferred_port = std::to_wstring(*preferred_port);

    m_initialSettings.automatically_mapped_drives = WinSettings::Read<std::wstring>(WinSettings::Type::LocalhostAutomaticallyMappedDrives);

    m_settings = m_initialSettings;    
}


void LocalhostSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Check(pDX, IDC_LOCALHOST_START_AUTOMATICALLY, m_settings.start_automatically);
    DDX_Text(pDX, IDC_LOCALHOST_PREFERRED_PORT, m_settings.preferred_port, true);
    DDX_Control(pDX, IDC_LOCALHOST_AUTOMATICALLY_MAPPED_DRIVES, m_automaticallyMappedDrivesCtrl);
}


BOOL LocalhostSettingsDlg::OnInitDialog()
{
    __super::OnInitDialog();

    // add the drives and make sure the vertical height appears correctly
    CDC* pDC = m_automaticallyMappedDrivesCtrl.GetDC();

    std::vector<std::wstring> drives_to_select = LocalhostSettings::GetDrivesToAutomaticallyMap(m_initialSettings.automatically_mapped_drives);

    for( const std::wstring& drive : m_logicalDrives )
    {
        int index = m_automaticallyMappedDrivesCtrl.AddString(drive.c_str());
        m_automaticallyMappedDrivesCtrl.SetItemHeight(index, pDC->GetTextExtent(drive.c_str()).cy);

        if( std::find(drives_to_select.cbegin(), drives_to_select.cend(), drive) != drives_to_select.cend() )
            m_automaticallyMappedDrivesCtrl.SetCheck(index, BST_CHECKED);
    }

    m_automaticallyMappedDrivesCtrl.ReleaseDC(pDC);

    return TRUE;
}


void LocalhostSettingsDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        // validate the port
        std::optional<int> preferred_port;

        if( !m_settings.preferred_port.empty() )
        {
            try
            {
                preferred_port = std::stoi(m_settings.preferred_port);
            }
            catch(...) { }

            if( !preferred_port.has_value() || *preferred_port < LocalhostSettings::MinPort || *preferred_port > LocalhostSettings::MaxPort )
                throw CSProException(_T("The preferred port must be between %d-%d"), LocalhostSettings::MinPort, LocalhostSettings::MaxPort);
        }

        // add the automatic drive mappings
        m_settings.automatically_mapped_drives.clear();

        ASSERT(m_logicalDrives.size() == static_cast<size_t>(m_automaticallyMappedDrivesCtrl.GetCount()));

        for( size_t i = 0; i < m_logicalDrives.size(); ++i )
        {
            if( m_automaticallyMappedDrivesCtrl.GetCheck(static_cast<int>(i)) == BST_CHECKED )
                SO::AppendWithSeparator(m_settings.automatically_mapped_drives, m_logicalDrives[i], LocalhostSettings::AutomaticallyMappedDrivesSeparator);
        }

        // potentially save the changes
        if( m_initialSettings.start_automatically != m_settings.start_automatically ||
            m_initialSettings.preferred_port != m_settings.preferred_port ||
            m_initialSettings.automatically_mapped_drives != m_settings.automatically_mapped_drives )
        {
            WinSettings::Write<DWORD>(WinSettings::Type::LocalhostStartAutomatically, m_settings.start_automatically ? 1 : 0);
            WinSettings::Write<DWORD>(WinSettings::Type::LocalhostPreferredPort, preferred_port.has_value() ? *preferred_port : -1);

            // this check is so that the drives text isn't serialized if empty (the typical case)
            if( m_initialSettings.automatically_mapped_drives != m_settings.automatically_mapped_drives )
                WinSettings::Write(WinSettings::Type::LocalhostAutomaticallyMappedDrives, m_settings.automatically_mapped_drives);

            AfxMessageBox(_T("The changes will not take effect until you restart this program."));
        }

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void LocalhostSettingsDlg::OnCreateMapping()
{
    CIMSAFileDialog file_dlg(TRUE, nullptr, nullptr, OFN_HIDEREADONLY,
                             _T("All Files (*.*)|*.*|||"), nullptr, CFD_NO_DIR);

    if( file_dlg.DoModal() != IDOK )
        return;

    SharedHtmlLocalFileServer& file_server = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer();

    std::wstring url = file_server.GetFilenameUrl(file_dlg.GetPathName());

    WinClipboard::PutText(this, url);

    AfxMessageBox(_T("The mapping has been copied to the clipboard:\n\n") + url);
}
