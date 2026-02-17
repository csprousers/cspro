#include "StdAfx.h"
#include "Controller.h"
#include "ControllerThreadRunningFrame.h"
#include "FileReplacer.h"
#include "LibraryManager.h"
#include "OpenSourceSyncer.h"
#include "Syncer.h"


namespace
{
    constexpr const char* ExclusionsFilename = "exclusions.txt";

    constexpr std::string_view OpenSourceCodeDirectoryKey_sv      = "open-source-code-directory";
    constexpr std::string_view OpenSourceLibrariesDirectoryKey_sv = "open-source-libraries-directory";
    constexpr std::string_view GitHubPATKey_sv                    = "github-pat";
}


Controller::Controller()
    :   m_settingsDb("OpenSourceSyncer.db"),
        m_loggingListBox(nullptr),
        m_openSourceCodeDirectory(m_settingsDb.ReadOrDefault<std::string>(OpenSourceCodeDirectoryKey_sv)),
        m_openSourceLibrariesDirectory(m_settingsDb.ReadOrDefault<std::string>(OpenSourceLibrariesDirectoryKey_sv)),
        m_githubPAT(m_settingsDb.ReadOrDefault<std::string>(GitHubPATKey_sv))
{
    const std::string this_source_directory = PortableFunctions::PathGetDirectory(__FILE__);

    m_privateRepoDirectory = MakeFullPath(this_source_directory, "..\\..\\");

    m_overridesDirectory = Path::Combine(this_source_directory, "Overrides");
}


Controller::~Controller()
{
}


Controller& Controller::GetInstance()
{
    return assert_cast<OpenSourceSyncerApp*>(AfxGetApp())->GetController();
}


std::string Controller::GetExclusionsFilePath() const
{
    return Path::Combine(m_overridesDirectory, ExclusionsFilename);
}


GitRepository& Controller::GetPrivateRepo()
{
    if( !m_privateRepo.has_value() )
    {
        LogText("Opening private repository: " + m_privateRepoDirectory);

        GitRepository repo;
        repo.OpenBare(Path::Combine(m_privateRepoDirectory, ".git"));

        m_privateRepo.emplace(std::move(repo));
    }

    return *m_privateRepo;
}


void Controller::SetOpenSourceCodeDirectory(std::string directory)
{
    if( m_openSourceCodeDirectory == directory )
        return;

    m_openSourceCodeDirectory = std::move(directory);
    m_settingsDb.Write(OpenSourceCodeDirectoryKey_sv, m_openSourceCodeDirectory);

    m_openSourceRepo.reset();
}


GitRepository& Controller::GetOpenSourceRepo()
{
    if( !m_openSourceRepo.has_value() )
    {
        if( m_openSourceCodeDirectory.empty() )
            throw CSProException("Use the Settings dialog to specify the open source code directory.");

        LogText("Opening open source repository: " + m_openSourceCodeDirectory);

        GitRepository repo;
        repo.Open(m_openSourceCodeDirectory);

        m_openSourceRepo.emplace(std::move(repo));
    }

    return *m_openSourceRepo;
}


void Controller::SetOpenSourceLibrariesDirectory(std::string directory)
{
    if( m_openSourceLibrariesDirectory == directory )
        return;

    m_openSourceLibrariesDirectory = std::move(directory);
    m_settingsDb.Write(OpenSourceLibrariesDirectoryKey_sv, m_openSourceLibrariesDirectory);

    m_openSourceLibrariesRepo.reset();
}


GitRepository& Controller::GetOpenSourceLibrariesRepo()
{
    if( !m_openSourceLibrariesRepo.has_value() )
    {
        if( m_openSourceLibrariesDirectory.empty() )
            throw CSProException("Use the Settings dialog to specify the open source libraries directory.");

        LogText("Opening open source libraries repository: " + m_openSourceLibrariesDirectory);

        GitRepository repo;
        repo.Open(m_openSourceLibrariesDirectory);

        m_openSourceLibrariesRepo.emplace(std::move(repo));
    }

    return *m_openSourceLibrariesRepo;
}


void Controller::SetGitHubPAT(std::string pat)
{
    m_githubPAT = std::move(pat);
    m_settingsDb.Write(GitHubPATKey_sv, m_githubPAT);
}


Syncer& Controller::GetSyncer()
{
    if( m_syncer == nullptr )
        m_syncer = std::make_unique<Syncer>(*this);

    return *m_syncer;
}


FileReplacer& Controller::GetFileReplacer()
{
    if( m_fileReplacer == nullptr )
        m_fileReplacer = std::make_unique<FileReplacer>(*this, m_overridesDirectory);

    return *m_fileReplacer;
}


LibraryManager& Controller::GetLibraryManager()
{
    if( m_libraryManager == nullptr )
        m_libraryManager = std::make_unique<LibraryManager>(*this);

    return *m_libraryManager;
}


bool Controller::IsOperationRunning(const CFrameWnd* const frame_wnd/* = nullptr*/) const noexcept
{
    return ( ( m_workerThread.has_value() ) &&
             ( frame_wnd == nullptr || frame_wnd == m_workerThread->frame_wnd ) );
}


void Controller::RunOperation(CFrameWnd* const frame_wnd, std::function<void(Controller& controller)> operation_callback) noexcept
{
    ASSERT(frame_wnd != nullptr && frame_wnd->IsKindOf(RUNTIME_CLASS(ControllerThreadRunningFrame)));

    if( IsOperationRunning() )
    {
        ErrorMessage::PostMessageForDisplay("An operation is currently in progress.");
        return;
    }

    CWnd* const main_wnd = AfxGetMainWnd();

    if( main_wnd->SendMessage(UWM::OpenSourceSyncer::OperationInitialize) != 1 )
        return;

    if( m_loggingListBox != nullptr )
        m_loggingListBox->Clear();

    m_workerThread = WorkerThreadData
    {
        frame_wnd,
        std::thread([this, main_wnd, operation_callback_ = std::move(operation_callback)]()
        {
            const double start_time = GetTimestamp<double>();
            const char* success_text = "successfully";

            WindowsDesktopMessage::PostObject(main_wnd, UWM::OpenSourceSyncer::UpdateStatusBar, "Running operation...");

            try
            {
                operation_callback_(*this);
            }

            catch( const CSProException& exception )
            {
                success_text = "in failure";
                LogText("\n\nError: %s", exception.what());
                ErrorMessage::PostMessageForDisplay(exception);
            }

            WindowsDesktopMessage::PostObject(main_wnd, UWM::OpenSourceSyncer::UpdateStatusBar,
                FormatText("Operation completed %s in %0.1f seconds.", success_text, GetTimestamp<double>() - start_time)
            );

            main_wnd->PostMessage(UWM::OpenSourceSyncer::OperationComplete);
        })
    };
}


void Controller::MarkOperationComplete()
{
    ASSERT(m_workerThread.has_value() && m_workerThread->frame_wnd != nullptr);

    if( m_workerThread->worker_thread.joinable() )
        m_workerThread->worker_thread.join();

    m_workerThread.reset();
}
