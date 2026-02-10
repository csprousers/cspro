#pragma once

#include <zGit/GitRepository.h>
#include <zUtilO/SettingsDb.h>
#include <zUtilF/LoggingListBox.h>


class Controller
{
public:
    Controller();

    static Controller& GetInstance();

    // Returns the global settings database.
    SettingsDb& GetSettingsDb() { return m_settingsDb; }

    // Returns the full file path of the exclusions.txt file.
    std::string GetExclusionsFilePath() const;

    // Returns the full file path of the replacements.json file.
    std::string GetReplacementsFilePath() const;

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

    // Logs text to the active LoggingListBox.
    template<typename... Args>
    void LogText(Args const&... args);

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
