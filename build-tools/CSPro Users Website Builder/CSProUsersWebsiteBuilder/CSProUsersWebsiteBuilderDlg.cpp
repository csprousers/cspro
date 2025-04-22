#include "StdAfx.h"
#include "CSProUsersWebsiteBuilderDlg.h"
#include "Builder.h"
#include <zToolsO/UWM.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/UWMRanges.h>
#include <zUtilO/WindowHelpers.h>


namespace
{
    static constexpr std::string_view CSProUsersInputDirectoryKey_sv  = "input-directory";
    static constexpr std::string_view CSProUsersOutputDirectoryKey_sv = "output-directory";
}



BEGIN_MESSAGE_MAP(CSProUsersWebsiteBuilderDlg, ResizableDlg)
    ON_COMMAND(IDC_UPDATE_GOOGLE_PLAY_PRIVACY_POLICY, OnUpdateGooglePlayPrivacyPolicy)
    ON_MESSAGE(UWM::Ranges::ExeStart, OnBuildTaskComplete)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
END_MESSAGE_MAP()


CSProUsersWebsiteBuilderDlg::CSProUsersWebsiteBuilderDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_BUILDER, pParent),
        m_settingsDb("CSProUsersWebsiteBuilder.db"),
        m_csproRootDirectory(MakeFullPath(CSProExecutables::GetApplicationDirectory(), "..\\..\\..")),
        m_csproUsersInputDirectory(m_settingsDb.ReadOrDefault<std::string>(CSProUsersInputDirectoryKey_sv)),
        m_csproUsersOutputDirectory(m_settingsDb.ReadOrDefault<std::string>(CSProUsersOutputDirectoryKey_sv))
{
    SerializeDialogSize("CSProUsersWebsiteBuilderDlg");
}


CSProUsersWebsiteBuilderDlg::~CSProUsersWebsiteBuilderDlg()
{
    ASSERT(m_buildThread == nullptr);
}


void CSProUsersWebsiteBuilderDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_DIRECTORY_CSPRO, m_csproRootDirectory);
    DDX_Text(pDX, IDC_DIRECTORY_CSPRO_USERS_INPUTS, m_csproUsersInputDirectory);
    DDX_Text(pDX, IDC_DIRECTORY_CSPRO_USERS_OUTPUTS, m_csproUsersOutputDirectory);
    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
}


BOOL CSProUsersWebsiteBuilderDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    return TRUE;
}


void CSProUsersWebsiteBuilderDlg::OnCancel()
{
    if( m_buildThread != nullptr )
    {
        ErrorMessage::Display(L"You cannot exit while a build task is in progress.");
        return;
    }

    __super::OnCancel();
}


template<typename FP>
void CSProUsersWebsiteBuilderDlg::RunBuildTask(FP task_function)
{
    UpdateData(TRUE);

    try
    {
        if( m_buildThread != nullptr )
            throw CSProException("You must wait until the current build task has completed.");

        // validate the directories, and if successful, save them for future runs of this program
        if( !PortableFunctions::FileIsDirectory(m_csproRootDirectory) )
            throw CSProException("Specify a valid CSPro directory.");

        if( !PortableFunctions::FileIsDirectory(m_csproUsersInputDirectory) )
            throw CSProException("Specify a valid CSPro Users (sources) directory.");

        if( !PortableFunctions::FileIsDirectory(m_csproUsersOutputDirectory) )
            throw CSProException("Specify a valid CSPro Users (built website) directory.");

        m_settingsDb.Write<std::string>(CSProUsersInputDirectoryKey_sv, m_csproUsersInputDirectory);
        m_settingsDb.Write<std::string>(CSProUsersOutputDirectoryKey_sv, m_csproUsersOutputDirectory);

        m_buildThread = std::make_unique<std::thread>(
            [ builder = std::make_unique<Builder>(m_loggingListBox, m_csproRootDirectory, m_csproUsersInputDirectory, m_csproUsersOutputDirectory),
              task_function,
              this ]()
            {
                try
                {
                    m_loggingListBox.Clear();
                    m_loggingListBox.AddText(FormatText("Task started at %s\n", DateTime::LocalDateTimeString(DateTime::Now()).c_str()));

                    ((*builder).*task_function)();

                    m_loggingListBox.AddText("\nTask completed successfully.");
                }

                catch( const CSProException& exception )
                {
                    m_loggingListBox.AddText(FormatText("\nTask ended in error: %s", exception.what()));
                    ErrorMessage::PostMessageForDisplay(exception);
                }

                PostMessage(UWM::Ranges::ExeStart);
            });
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


LRESULT CSProUsersWebsiteBuilderDlg::OnBuildTaskComplete(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ASSERT(m_buildThread != nullptr);

    if( m_buildThread->joinable() )
        m_buildThread->join();

    m_buildThread.reset();

    return 1;
}


void CSProUsersWebsiteBuilderDlg::OnUpdateGooglePlayPrivacyPolicy()
{
    RunBuildTask(&Builder::UpdateGooglePlayPrivacyPolicy);
}


LRESULT CSProUsersWebsiteBuilderDlg::OnDisplayErrorMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ErrorMessage::DisplayPostedMessages();
    return 0;
}
