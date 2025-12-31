#include "StdAfx.h"
#include "PropertiesDlg.h"
#include "DiffTool.h"
#include <zUtilO/FileDlg.h>


BEGIN_MESSAGE_MAP(PropertiesDlg, ResizableDlg)
    ON_BN_CLICKED(IDC_DIFF_TOOL_SELECT, OnDiffToolSelect)
    ON_BN_CLICKED(IDC_DIFF_TOOL_TEST, OnDiffToolTest)
END_MESSAGE_MAP()


PropertiesDlg::PropertiesDlg(SettingsDb& global_settings_db, CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_PROPERTIES, pParent),
        m_globalSettingsDb(global_settings_db),
        m_diffToolCommand(m_globalSettingsDb.ReadOrDefault<std::string>(DiffTool::CommandKey_sv)),
        m_diffToolArgument(m_globalSettingsDb.ReadOrDefault<std::string>(DiffTool::ArgumentKey_sv))
{
    SerializeDialogSize("Stygitan-PropertiesDlg");
}


void PropertiesDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_DIFF_TOOL_COMMAND, m_diffToolCommand, true);
    DDX_Text(pDX, IDC_DIFF_TOOL_ARGUMENTS, m_diffToolArgument, true);
}


void PropertiesDlg::OnOK()
{
    UpdateData(TRUE);

    try
    {
        if( !m_diffToolCommand.empty() &&
            !PortableFunctions::FileIsRegular(m_diffToolCommand) )
        {
            throw FileIO::Exception::FileNotFound(m_diffToolCommand);
        }

        m_globalSettingsDb.Write(DiffTool::CommandKey_sv, m_diffToolCommand);
        m_globalSettingsDb.Write(DiffTool::ArgumentKey_sv, m_diffToolArgument);

        __super::OnOK();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void PropertiesDlg::OnDiffToolSelect()
{
    UpdateData(TRUE);

    OpenFileDlg open_file_dlg(0, nullptr, m_diffToolCommand, L"Executables (*.exe)|*.exe|All Files (*.*)|*.*||", this);
    open_file_dlg.SetTitle(L"Select the Diff Tool Executable");

    if( open_file_dlg.DoModal() != IDOK )
        return;

    m_diffToolCommand = open_file_dlg.GetFilePath();

    UpdateData(FALSE);
}


void PropertiesDlg::OnDiffToolTest()
{
    UpdateData(TRUE);

    try
    {
        if( m_diffToolTextFiles == nullptr )
        {
            m_diffToolTextFiles = std::make_unique<std::tuple<TemporaryFile, TemporaryFile>>();
            FileIO::WriteText(std::get<0>(*m_diffToolTextFiles).GetPath(), "Old\nfile\ncontent", false);
            FileIO::WriteText(std::get<1>(*m_diffToolTextFiles).GetPath(), "New\nfile\ncontent", false);
        }

        DiffTool::Launch(std::get<0>(*m_diffToolTextFiles).GetPath(), std::get<1>(*m_diffToolTextFiles).GetPath(),
                         m_diffToolCommand, m_diffToolArgument);
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
