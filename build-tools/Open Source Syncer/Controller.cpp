#include "StdAfx.h"
#include "Controller.h"
#include "OpenSourceSyncer.h"


namespace
{
    constexpr const char* ExclusionsFilename   = "exclusions.txt";
    constexpr const char* ReplacementsFilename = "replacements.json";

    constexpr std::string_view OpenSourceCodeDirectoryKey_sv      = "open-source-code-directory";
    constexpr std::string_view OpenSourceLibrariesDirectoryKey_sv = "open-source-libraries-directory";
}


Controller::Controller()
    :   m_settingsDb("OpenSourceSyncer.db"),
        m_loggingListBox(nullptr),
        m_openSourceCodeDirectory(m_settingsDb.ReadOrDefault<std::string>(OpenSourceCodeDirectoryKey_sv)),
        m_openSourceLibrariesDirectory(m_settingsDb.ReadOrDefault<std::string>(OpenSourceLibrariesDirectoryKey_sv))
{
    const std::string this_source_directory = PortableFunctions::PathGetDirectory(__FILE__);

    m_privateRepoDirectory = MakeFullPath(this_source_directory, "..\\..\\");

    m_overridesDirectory = Path::Combine(this_source_directory, "Overrides");
}


Controller& Controller::GetInstance()
{
    return assert_cast<OpenSourceSyncerApp*>(AfxGetApp())->GetController();
}


std::string Controller::GetExclusionsFilePath() const
{
    return Path::Combine(m_overridesDirectory, ExclusionsFilename);
}


std::string Controller::GetReplacementsFilePath() const
{
    return Path::Combine(m_overridesDirectory, ReplacementsFilename);
}


GitRepository& Controller::GetPrivateRepo()
{
    if( !m_privateRepo.has_value() )
    {
        GitRepository repo;
        repo.OpenBare(Path::Combine(m_privateRepoDirectory, ".git"));

        m_openSourceRepo.emplace(std::move(repo));
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
