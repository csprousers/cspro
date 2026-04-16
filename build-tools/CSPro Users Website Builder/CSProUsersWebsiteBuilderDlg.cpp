#include "StdAfx.h"
#include "CSProUsersWebsiteBuilderDlg.h"
#include "Builder.h"
#include <zToolsO/UWM.h>
#include <zUtilO/DataExchange.h>
#include <zUtilO/UWMRanges.h>
#include <zUtilO/WindowHelpers.h>


namespace
{
    constexpr std::string_view HelpsDirectoryKey_sv                     = "helps";
    constexpr std::string_view MobileWorkshopDirectoryKey_sv            = "mobile-workshop";
    constexpr std::string_view RubyDirectoryKey_sv                      = "ruby";
    constexpr std::string_view CSProUsersInputDirectoryKey_sv           = "input-directory";
    constexpr std::string_view CSProUsersOutputDirectoryKey_sv          = "output-directory";
    constexpr std::string_view CSProUsersOutputClearExclusions_sv       = "output-clear-exclusions";
    constexpr std::string_view CSProUsersFilesRepositoryDirectoryKey_sv = "files-repository";

}


BEGIN_MESSAGE_MAP(CSProUsersWebsiteBuilderDlg, ResizableDlgEx)
    ON_COMMAND_RANGE(IDC_BUILD_SITE, IDC_BUILD_SITE, OnBuildTask)
    ON_COMMAND_RANGE(IDC_UPDATE_BLOG, IDC_UPDATE_BLOG, OnBuildTask)
    ON_COMMAND_RANGE(IDC_UPDATE_HELPS, IDC_UPDATE_HELPS, OnBuildTask)
    ON_COMMAND_RANGE(IDC_UPDATE_MOBILE_WORKSHOP, IDC_UPDATE_MOBILE_WORKSHOP, OnBuildTask)
    ON_COMMAND_RANGE(IDC_UPDATE_GOOGLE_PLAY_PRIVACY_POLICY, IDC_UPDATE_GOOGLE_PLAY_PRIVACY_POLICY, OnBuildTask)
    ON_COMMAND(IDC_CLEAR_OUTPUTS, OnClearOutputs)
    ON_COMMAND_RANGE(IDC_CREATE_WEBSITE_UPDATERS, IDC_CREATE_WEBSITE_UPDATERS, OnBuildTask)
    ON_MESSAGE(UWM::Ranges::ExeStart, OnBuildTaskComplete)
    ON_MESSAGE(UWM::ToolsO::DisplayErrorMessage, OnDisplayErrorMessage)
END_MESSAGE_MAP()


CSProUsersWebsiteBuilderDlg::CSProUsersWebsiteBuilderDlg(CWnd* const pParent/* = nullptr*/)
    :   ResizableDlgEx(IDD_BUILDER, pParent),
        m_settingsDb("CSProUsersWebsiteBuilder.db"),
        m_inputs{ MakeFullPath(PortableFunctions::PathGetDirectory(__FILE__), "..\\.."),
                  m_settingsDb.ReadOrDefault<std::string>(HelpsDirectoryKey_sv),
                  m_settingsDb.ReadOrDefault<std::string>(MobileWorkshopDirectoryKey_sv),
                  m_settingsDb.ReadOrDefault<std::string>(RubyDirectoryKey_sv),
                  m_settingsDb.ReadOrDefault<std::string>(CSProUsersInputDirectoryKey_sv),
                  m_settingsDb.ReadOrDefault<std::string>(CSProUsersOutputDirectoryKey_sv),
                  m_settingsDb.ReadOrDefault<std::string>(CSProUsersOutputClearExclusions_sv),
                  m_settingsDb.ReadOrDefault<std::string>(CSProUsersFilesRepositoryDirectoryKey_sv) }
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

    DDX_Text(pDX, IDC_DIRECTORY_CSPRO, m_inputs.cspro_root, true);
    DDX_Text(pDX, IDC_DIRECTORY_HELPS, m_inputs.helps, true);
    DDX_Text(pDX, IDC_DIRECTORY_MOBILE_WORKSHOP, m_inputs.mobile_workshop, true);
    DDX_Text(pDX, IDC_DIRECTORY_RUBY, m_inputs.ruby, true);
    DDX_Text(pDX, IDC_DIRECTORY_CSPRO_USERS_INPUTS, m_inputs.csprousers_input, true);
    DDX_Text(pDX, IDC_DIRECTORY_CSPRO_USERS_OUTPUTS, m_inputs.csprousers_output, true);
    DDX_Control(pDX, IDC_CLEAR_EXCLUSIONS, m_clearOutputsExclusionsLogicCtrl);
    DDX_Control(pDX, IDC_CLEAR_OUTPUTS, m_clearOutputsButton);
    DDX_Text(pDX, IDC_DIRECTORY_CSPRO_USERS_REPOSITORY, m_inputs.csprousers_files_repository);
    DDX_Text(pDX, IDC_LAST_PROCESSED_COMMIT, m_lastCommitProcessed, true);
    DDX_Control(pDX, IDC_LOG, m_loggingListBox);
}


BOOL CSProUsersWebsiteBuilderDlg::OnInitDialog()
{
    __super::OnInitDialog();

    WindowHelpers::RemoveDialogSystemIcon(*this);

    m_clearOutputsExclusionsLogicCtrl.ReplaceCEdit(this, false, false, SCLEX_NULL);
    m_clearOutputsExclusionsLogicCtrl.SetText(m_inputs.csprousers_output_clear_exclusions);

    // set up the clear output button's menu
    m_clearOutputsMenu.LoadMenu(IDR_CLEAR_OUTPUTS);
    CMenu* const clear_outputs_menu = m_clearOutputsMenu.GetSubMenu(0);
    m_clearOutputsButton.m_hMenu = clear_outputs_menu->GetSafeHmenu();

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
    m_inputs.csprousers_output_clear_exclusions = m_clearOutputsExclusionsLogicCtrl.GetText();

    try
    {
        if( m_buildThread != nullptr )
            throw CSProException("You must wait until the current build task has completed.");

        // validate the directories, and if successful, save them for future runs of this program
        if( !PortableFunctions::FileIsDirectory(m_inputs.cspro_root) )
            throw CSProException("Specify a valid CSPro directory.");

        if( nID == IDC_UPDATE_HELPS && !PortableFunctions::FileIsDirectory(m_inputs.helps) )
            throw CSProException("Specify a valid helps directory.");

        if( nID == IDC_UPDATE_MOBILE_WORKSHOP && !PortableFunctions::FileIsDirectory(m_inputs.mobile_workshop) )
            throw CSProException("Specify a valid mobile workshop directory.");

        if( nID == IDC_BUILD_SITE && !PortableFunctions::FileIsDirectory(m_inputs.ruby) )
            throw CSProException("Specify a valid Ruby directory.");

        if( !PortableFunctions::FileIsDirectory(m_inputs.csprousers_input) )
            throw CSProException("Specify a valid CSPro Users (sources) directory.");

        if( !PortableFunctions::FileIsDirectory(m_inputs.csprousers_output) )
            throw CSProException("Specify a valid CSPro Users (built website) directory.");

        if( nID == IDC_CREATE_WEBSITE_UPDATERS )
        {
            if( !PortableFunctions::FileIsDirectory(m_inputs.csprousers_files_repository) )
                throw CSProException("Specify a valid CSPro Users (files repository) directory.");

            if( m_lastCommitProcessed.empty() )
                throw CSProException("Specify the last processed commit.");
        }

        m_settingsDb.Write<std::string>(HelpsDirectoryKey_sv, m_inputs.helps);
        m_settingsDb.Write<std::string>(MobileWorkshopDirectoryKey_sv, m_inputs.mobile_workshop);
        m_settingsDb.Write<std::string>(RubyDirectoryKey_sv, m_inputs.ruby);
        m_settingsDb.Write<std::string>(CSProUsersInputDirectoryKey_sv, m_inputs.csprousers_input);
        m_settingsDb.Write<std::string>(CSProUsersOutputDirectoryKey_sv, m_inputs.csprousers_output);
        m_settingsDb.Write<std::string>(CSProUsersOutputClearExclusions_sv, m_inputs.csprousers_output_clear_exclusions);
        m_settingsDb.Write<std::string>(CSProUsersFilesRepositoryDirectoryKey_sv, m_inputs.csprousers_files_repository);

        m_buildThread = std::make_unique<std::thread>(
            [ builder = std::make_unique<Builder>(m_inputs, m_loggingListBox),
              nID,
              this ]()
            {
                try
                {
                    m_loggingListBox.Clear();
                    m_loggingListBox.AddText("Task started at %s\n", DateTime::LocalDateTimeString(DateTime::Now()).c_str());

                    switch( nID )
                    {
                        case IDC_BUILD_SITE:
                            builder->BuildSite();
                            break;

                        case IDC_UPDATE_BLOG:
                            builder->UpdateBlog();
                            break;

                        case IDC_UPDATE_HELPS:
                            builder->UpdateHelps();
                            break;

                        case IDC_UPDATE_MOBILE_WORKSHOP:
                            builder->UpdateMobileWorkshop();
                            break;

                        case IDC_UPDATE_GOOGLE_PLAY_PRIVACY_POLICY:
                            builder->UpdateGooglePlayPrivacyPolicy();
                            break;

                        case IDC_CLEAR_ALL:
                        case IDC_CLEAR_SITE:
                        case IDC_CLEAR_HELPS:
                        case IDC_CLEAR_MOBILE_WORKSHOP:
                            builder->ClearOutputs(nID);
                            break;

                        case IDC_CREATE_WEBSITE_UPDATERS:
                            builder->CreateWebsiteUpdaters(m_lastCommitProcessed);
                            break;

                        default:
                            throw ProgrammingErrorException();
                    }

                    m_loggingListBox.AddText("\nTask completed successfully at " + DateTime::LocalDateTimeString(DateTime::Now()));
                }

                catch( const CSProException& exception )
                {
                    m_loggingListBox.AddText("\nTask ended in error: %s", exception.what());
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


void CSProUsersWebsiteBuilderDlg::OnClearOutputs()
{
    UINT nID = m_clearOutputsButton.m_nMenuResult;

    if( nID == 0 )
    {
        if( AfxMessageBox(L"Are you sure that you want to clear all outputs?",
                          MB_ICONQUESTION | MB_YESNO) != IDYES )
        {
            return;
        }

        nID = IDC_CLEAR_ALL;
    }

    OnBuildTask(nID);
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
