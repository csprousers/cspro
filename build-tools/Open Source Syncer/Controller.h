#pragma once

#include <zGit/GitRepository.h>
#include <zUtilO/SettingsDb.h>
#include <zUtilF/LoggingListBox.h>

class FileReplacer;
class LibraryManager;
class Syncer;


class Controller
{
public:
    Controller();
    ~Controller();

    static Controller& GetInstance();

    // Returns the global settings database.
    SettingsDb& GetSettingsDb() { return m_settingsDb; }

    // Logs text to the active LoggingListBox.
    template<typename... Args>
    void LogText(Args const&... args);

    // Sets the active LoggingListBox.
    void SetLoggingListBox(LoggingListBox* logging_list_box) { m_loggingListBox = logging_list_box; }

    // Returns the full file path of the exclusions.txt file.
    std::string GetExclusionsFilePath() const;

    // Returns the full file path of a file in the Templates directory.
    static std::string GetTemplatesFilePath(const char* filename);

    // Gets the directory of the private repository.
    const std::string& GetPrivateRepoDirectory() const { return m_privateRepoDirectory; }

    // Opens the private repository if not open, throwing exceptions on error.
    GitRepository& GetPrivateRepo();

    // Gets or sets the directory of the open source repository.
    // If the directory changes, the repository is closed (if applicable).
    const std::string& GetOpenSourceCodeDirectory() const { return m_openSourceCodeDirectory; }
    void SetOpenSourceCodeDirectory(std::string directory);

    // Opens the open source repository if not open, throwing exceptions on error.
    GitRepository& GetOpenSourceRepo();

    // Gets or sets the directory of the open source libraries repository.
    // If the directory changes, the repository is closed (if applicable).
    const std::string& GetOpenSourceLibrariesDirectory() const { return m_openSourceLibrariesDirectory; }
    void SetOpenSourceLibrariesDirectory(std::string directory);

    // Opens the open source libraries repository if not open, throwing exceptions on error.
    GitRepository& GetOpenSourceLibrariesRepo();

    // Gets or sets the GitHub personal access token (PAT) used when performing
    // operations that require authentication (e.g., creating releases).
    const std::string& GetGitHubPAT() const { return m_githubPAT; }
    void SetGitHubPAT(std::string pat);

    // Creates a signature using the committer's email but the name "CSPro Bot".
    static GitSignature GetCSProBotSignature(GitRepository& repo);

    // Returns of instance of the Syncer, for synchronizing the private and open source repositories.
    Syncer& GetSyncer();

    // Returns an instance of the FileReplacer, used to replace sensitive files.
    FileReplacer& GetFileReplacer();

    // Returns an instance of the LibraryManager, for managing prebuilt
    // external libraries and other binary file dependencies.
    LibraryManager& GetLibraryManager();

    // Returns true if an operation is running in a worker thread.
    // When providing a CFrameWnd argument, the method returns true if the worker thread belongs to the frame.
    bool IsOperationRunning(const CFrameWnd* frame_wnd = nullptr) const noexcept;

    // Runs an operation in a worker thread.
    // Only one operation can be run at any time.
    void RunOperation(CFrameWnd* frame_wnd, bool show_log, std::function<void(Controller& controller)> operation_callback) noexcept;
    void RunOperation(CFrameWnd* frame_wnd, std::function<void(Controller& controller)> operation_callback) noexcept;

    // Marks the operation in progress as complete.
    void MarkOperationComplete();

private:
    SettingsDb m_settingsDb;
    LoggingListBox* m_loggingListBox;

    std::string m_overridesDirectory;

    std::string m_privateRepoDirectory;
    std::optional<GitRepository> m_privateRepo;

    std::string m_openSourceCodeDirectory;
    std::optional<GitRepository> m_openSourceRepo;

    std::string m_openSourceLibrariesDirectory;
    std::optional<GitRepository> m_openSourceLibrariesRepo;

    std::string m_githubPAT;

    std::unique_ptr<Syncer> m_syncer;
    std::unique_ptr<FileReplacer> m_fileReplacer;
    std::unique_ptr<LibraryManager> m_libraryManager;

    struct WorkerThreadData
    {
        CFrameWnd* frame_wnd;
        std::thread worker_thread;
    };

    std::optional<WorkerThreadData> m_workerThread;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename... Args>
void Controller::LogText(Args const&... args)
{
    if( m_loggingListBox != nullptr )
        m_loggingListBox->AddText(args...);
}
