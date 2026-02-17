#include "StdAfx.h"
#include "FileReplacer.h"
#include "GitHubConnection.h"
#include <zJson/JsonSpecFile.h>
#include <Update SQLite/SQLiteSourceUpdater.h>


CREATE_JSON_KEY(replacementPath)
CREATE_JSON_KEY(replacementRoutine)
CREATE_JSON_KEY(repoPath)


namespace
{
    constexpr const char* ReplacementsFilename = "replacements.json";
}


struct FileReplacer::Replacement
{
    bool is_file_path;
    std::string file_path_or_routine;
};


FileReplacer::FileReplacer(Controller& controller, const std::string& overrides_directory)
    :   m_controller(controller)
{
    const std::string replacements_file_path = Path::Combine(overrides_directory, ReplacementsFilename);

    m_controller.LogText("Reading replacement files specified in: " + replacements_file_path);

    const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(replacements_file_path);

    for( const JsonNode& replacement_json_node : json_reader->GetArray() )
    {
        const bool is_file_path = replacement_json_node.Contains(JK::replacementPath);

        m_replacements.emplace(
            replacement_json_node.Get<std::string>(JK::repoPath),
            Replacement
            {
                is_file_path,
                is_file_path ? replacement_json_node.GetAbsolutePath(JK::replacementPath) :
                               replacement_json_node.Get<std::string>(JK::replacementRoutine)
            }
        );
    }
}


FileReplacer::~FileReplacer()
{
}


bool FileReplacer::HasReplacement(const std::string& cs_file_path)
{
    return HasReplacementWorker<bool>(cs_file_path);
}


template<typename T>
T FileReplacer::HasReplacementWorker(const std::string& cs_file_path)
{
    auto lookup = m_replacements.find(cs_file_path);

    if constexpr(std::is_same_v<T, bool>)
    {
        return ( lookup != m_replacements.cend() );
    }

    else
    {
        return lookup;
    }
}


std::string FileReplacer::GetReplacement(const git_diff_file& new_file)
{
    const auto& lookup = HasReplacementWorker<std::map<std::string, Replacement>::iterator>(new_file.path);

    if( lookup == m_replacements.cend() )
    {
        return std::string();
    }

    else if( lookup->second.is_file_path )
    {
        return SO::ToNewlineLF(FileIO::ReadText(lookup->second.file_path_or_routine));
    }

    else if( lookup->second.file_path_or_routine == "SqliteWithoutSEE" )
    {
        return CreateSqliteWithoutSEE(new_file);
    }

    else
    {
        throw ProgrammingErrorException();
    }
}


std::string FileReplacer::CreateSqliteWithoutSEE(const git_diff_file& new_file)
{
    GitRepository& private_repo = m_controller.GetPrivateRepo();

    const std::string filename = Path::GetFilename(new_file.path);
    const bool is_header = ( filename == "sqlite3.h" );
    ASSERT(is_header || filename == "sqlite3.c");

    m_controller.LogText("Creating the non-SEE version of SQLite for: " + filename);

    // find the version of SQLite currently in use
    const GitBlob cs_header_blob = private_repo.LookupBlob(new_file.id);

    constexpr std::string_view VersionPrefix_sv = "#define SQLITE_VERSION";
    std::string version;
    std::string full_version_line;

    SO::ForeachLine(cs_header_blob.as<std::string_view>(), false,
        [&](std::string_view line_sv)
        {
            if( SO::StartsWith(line_sv, VersionPrefix_sv) )
            {
                full_version_line = line_sv;

                line_sv.remove_prefix(VersionPrefix_sv.length());
                SO::MakeTrim(line_sv);
                SO::MakeTrim(line_sv, '\"');
                version = line_sv;

                return false;
            }

            return true;
        });

    if( version.empty() )
        throw CSProException("Could not find the version in: " + filename);

    m_controller.LogText("Found SQLite version: " + version);

    // use a cached version when possible
    const std::string cache_key = FormatText("SQLite-%s-%s", version.c_str(), filename.c_str());
    std::string public_sqlite = m_controller.GetSettingsDb().ReadOrDefault(cache_key, SO::Empty_string);

    if( !public_sqlite.empty() )
    {
        m_controller.LogText("Using a cached version of the SQLite amalgamation files.");
    }

    // if not created, download the non-SEE SQLite amalgamation from: https://github.com/rhuijben/sqlite-amalgamation/
    else
    {
        constexpr const char* AmalgamationRepository = "rhuijben/sqlite-amalgamation";

        GitHubConnection gh_connection;

        // find this commit with this version
        std::string commit_sha;

        for( int commit_page = 1; commit_sha.empty(); ++commit_page )
        {
            const std::string url = FormatText(
                "https://api.github.com/repos/%s/commits?page=%d",
                AmalgamationRepository,
                commit_page
            );

            const JsonNode json_node = gh_connection.Request<JsonNode>(url);
            const JsonNodeArray commits_json_node_array = json_node.GetArray();

            if( commits_json_node_array.empty() )
                break;

            for( const JsonNode& commit_json_node : commits_json_node_array )
            {
                const std::string commit_message = commit_json_node.Get("commit")
                                                                   .Get<std::string>("message");

                if( commit_message.find(version) != std::string::npos )
                {
                    if( !commit_sha.empty() )
                        throw CSProException("Multiple SQLite amalgamations have a commit message containing: " + version);

                    commit_sha = commit_json_node.Get<std::string>("sha");
                }
            }
        }

        if( commit_sha.empty() )
            throw CSProException("No SQLite amalgamation has a commit message containing: " + version);

        m_controller.LogText("Downloading SQLite files from %s commit SHA: %s", AmalgamationRepository, commit_sha.c_str());

        // download the non-SEE version
        const std::string url = FormatText(
            "https://raw.githubusercontent.com/%s/%s/%s",
            AmalgamationRepository,
            commit_sha.c_str(),
            filename.c_str()
        );

        public_sqlite = gh_connection.Request<std::string>(url);

        if( public_sqlite.find(full_version_line) == std::string::npos )
            throw CSProException("The SQLite amalgamation version header does not match: " + full_version_line);

        SQLiteSourceUpdater::Update(public_sqlite, is_header, SQLiteSourceUpdater::Version::Public);

        // cache this result
        m_controller.GetSettingsDb().Write(cache_key, public_sqlite);
    }

    ASSERT(!public_sqlite.empty());

    return public_sqlite;
}
