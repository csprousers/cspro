#include "StdAfx.h"
#include "CSProUsersWebsiteBuilderDlg.h"
#include "Builder.h"
#include <zToolsO/UWM.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/UWMRanges.h>
#include <zUtilO/WindowHelpers.h>


namespace
{
    static constexpr std::string_view HelpsDirectoryKey_sv            = "helps";
    static constexpr std::string_view MobileWorkshopDirectoryKey_sv   = "mobile-workshop";
    static constexpr std::string_view CSProUsersInputDirectoryKey_sv  = "input-directory";
    static constexpr std::string_view CSProUsersOutputDirectoryKey_sv = "output-directory";
}


BEGIN_MESSAGE_MAP(CSProUsersWebsiteBuilderDlg, ResizableDlg)
    ON_COMMAND_RANGE(IDC_UPDATE_HELPS, IDC_UPDATE_HELPS, OnBuildTask)
    ON_COMMAND_RANGE(IDC_UPDATE_MOBILE_WORKSHOP, IDC_UPDATE_MOBILE_WORKSHOP, OnBuildTask)
    ON_COMMAND_RANGE(IDC_UPDATE_GOOGLE_PLAY_PRIVACY_POLICY, IDC_UPDATE_GOOGLE_PLAY_PRIVACY_POLICY, OnBuildTask)
    ON_MESSAGE(UWM::Ranges::ExeStart, OnBuildTaskComplete)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
END_MESSAGE_MAP()


CSProUsersWebsiteBuilderDlg::CSProUsersWebsiteBuilderDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlg(IDD_BUILDER, pParent),
        m_settingsDb("CSProUsersWebsiteBuilder.db"),
        m_directories{ MakeFullPath(CSProExecutables::GetApplicationDirectory(), "..\\..\\.."),
                       m_settingsDb.ReadOrDefault<std::string>(HelpsDirectoryKey_sv),
                       m_settingsDb.ReadOrDefault<std::string>(MobileWorkshopDirectoryKey_sv),
                       m_settingsDb.ReadOrDefault<std::string>(CSProUsersInputDirectoryKey_sv),
                       m_settingsDb.ReadOrDefault<std::string>(CSProUsersOutputDirectoryKey_sv) }
{
    SerializeDialogSize("CSProUsersWebsiteBuilderDlg");
}


CSProUsersWebsiteBuilderDlg::~CSProUsersWebsiteBuilderDlg()
{
    ASSERT(m_buildThread == nullptr);
}


void CSProUsersWebsiteBuilderDlg::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_DIRECTORY_CSPRO, m_directories.cspro_root);
    DDX_Text(pDX, IDC_DIRECTORY_HELPS, m_directories.helps);
    DDX_Text(pDX, IDC_DIRECTORY_MOBILE_WORKSHOP, m_directories.mobile_workshop);
    DDX_Text(pDX, IDC_DIRECTORY_CSPRO_USERS_INPUTS, m_directories.csprousers_input);
    DDX_Text(pDX, IDC_DIRECTORY_CSPRO_USERS_OUTPUTS, m_directories.csprousers_output);
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


void CSProUsersWebsiteBuilderDlg::OnBuildTask(const UINT nID)
{
    UpdateData(TRUE);

    try
    {
        if( m_buildThread != nullptr )
            throw CSProException("You must wait until the current build task has completed.");

        // validate the directories, and if successful, save them for future runs of this program
        if( !PortableFunctions::FileIsDirectory(m_directories.cspro_root) )
            throw CSProException("Specify a valid CSPro directory.");

        if( nID == IDC_UPDATE_HELPS && !PortableFunctions::FileIsDirectory(m_directories.helps) )
            throw CSProException("Specify a valid helps directory.");

        if( nID == IDC_UPDATE_MOBILE_WORKSHOP && !PortableFunctions::FileIsDirectory(m_directories.mobile_workshop) )
            throw CSProException("Specify a valid mobile workshop directory.");

        if( !PortableFunctions::FileIsDirectory(m_directories.csprousers_input) )
            throw CSProException("Specify a valid CSPro Users (sources) directory.");

        if( !PortableFunctions::FileIsDirectory(m_directories.csprousers_output) )
            throw CSProException("Specify a valid CSPro Users (built website) directory.");

        m_settingsDb.Write<std::string>(HelpsDirectoryKey_sv, m_directories.helps);
        m_settingsDb.Write<std::string>(MobileWorkshopDirectoryKey_sv, m_directories.mobile_workshop);
        m_settingsDb.Write<std::string>(CSProUsersInputDirectoryKey_sv, m_directories.csprousers_input);
        m_settingsDb.Write<std::string>(CSProUsersOutputDirectoryKey_sv, m_directories.csprousers_output);

        m_buildThread = std::make_unique<std::thread>(
            [ builder = std::make_unique<Builder>(m_directories, m_loggingListBox),
              nID,
              this ]()
            {
                try
                {
                    m_loggingListBox.Clear();
                    m_loggingListBox.AddText(FormatText("Task started at %s\n", DateTime::LocalDateTimeString(DateTime::Now()).c_str()));

                    switch( nID )
                    {
                        case IDC_UPDATE_HELPS:
                            builder->UpdateHelps();
                            break;

                        case IDC_UPDATE_MOBILE_WORKSHOP:
                            builder->UpdateMobileWorkshop();
                            break;

                        case IDC_UPDATE_GOOGLE_PLAY_PRIVACY_POLICY:
                            builder->UpdateGooglePlayPrivacyPolicy();
                            break;

                        default:
                            throw ProgrammingErrorException();
                    }

                    m_loggingListBox.AddText("\nTask completed successfully at " + DateTime::LocalDateTimeString(DateTime::Now()));
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


LRESULT CSProUsersWebsiteBuilderDlg::OnDisplayErrorMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    ErrorMessage::DisplayPostedMessages();
    return 0;
}
