#include "StdAfx.h"
#include "Creator.h"
#include <zToolsO/CaseInsensitiveComparer.h>
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/File.h>
#include <zJson/JsonSpecFile.h>
#include <zNetwork/CurlHttpConnection.h>
#include <zGit/GitIgnoreEvaluator.h>
#include <zGit/GitRevisionWalker.h>
#include <Update SQLite/SQLiteSourceUpdater.h>
#include <external/libgit2/include/git2/status.h>


Creator::Creator(SettingsDb& settings_db)
    :   m_settingsDb(settings_db),
        m_loggingListBox(nullptr)
{
    const std::string this_source_directory = PortableFunctions::PathGetDirectory(__FILE__);

    m_overridesDirectory = Path::Combine(this_source_directory, "Overrides");

    std::string git_directory = MakeFullPath(this_source_directory, "..\\..\\.git");
    m_repo.OpenBare(std::move(git_directory));
}


std::vector<GitTag> Creator::GetTags() const
{
    std::vector<GitTag> tags = m_repo.GetTags();

    // sort by name
    std::sort(tags.begin(), tags.end(),
              [&](const GitTag& tag1, const GitTag& tag2) { return ( tag1.GetName() < tag2.GetName() ); });

    return tags;
}


void Creator::Initialize(LoggingListBox& logging_list_box, const std::string& open_source_directory)
{
    m_loggingListBox = &logging_list_box;

    if( m_openSourceDirectory != open_source_directory )
    {
        m_openSourceDirectory = open_source_directory;
        m_repoPaths.clear();
        m_repoBlobObjects.clear();
    }

    m_loggingListBox->Clear();
}


std::tuple<GitCommit, GitTree> Creator::LookupCommitAndGetTree(const cs::string_sz commit_string)
{
    GitCommit commit = m_repo.LookupCommit(commit_string);

    const GitSignature& author = commit.GetAuthor();
    m_loggingListBox->AddText(std::string("    Author: ").append(author.GetName()));
    m_loggingListBox->AddText(std::string("    Date: ").append(author.GetWhen().GetLocalDateTimeString()));

    // properly space multiline messages
    std::string message_text = "    Message: ";
    const size_t message_indentation_length = message_text.length();

    SO::ForeachLine(commit.GetMessage(), false,
        [&](const std::string_view line_sv)
        {
            if( message_indentation_length != message_text.length() )
            {
                message_text.push_back('\n');
                message_text.append(message_indentation_length, ' ');
            }

            message_text.append(line_sv);
        });

    m_loggingListBox->AddText(std::move(message_text));

    // get the list of the files that are part of this release
    GitTree tree = commit.GetTree();

    PopulateRepoPaths(tree, "");

    return { std::move(commit), std::move(tree) };
}


void Creator::CreateRelease(const cs::string_sz commit_string)
{
    ASSERT(!m_openSourceDirectory.empty());

    m_loggingListBox->AddText("Creating open source release from commit: %s", commit_string.c_str());

    auto [commit, tree] = LookupCommitAndGetTree(commit_string);

    // remove files that should not be part of the open source release
    PruneRepoPaths();

    // remove all existing non-Git files from the output directory...
    PrepareOutputDirectory();

    // ...and then copy the current release files
    CopyFilesToOutputDirectory();

    // copy dummy files for some sensitive files
    CopyReplacementFiles();

    // create a version of SQLite without the SQLite Encryption Extension (SEE)
    CreateSqliteWithoutSEE(tree);

    // create a log showing the history of pull requests
    CreateHistoryLog(commit);

    // ensure that the files in the repositories are identical
    EnsureRepositoriesMatch(true);

    m_loggingListBox->AddText(SharableString());
    m_loggingListBox->AddText("Successfully created the open source release.");
}


void Creator::ValidateRelease(const cs::string_sz commit_string)
{
    m_loggingListBox->AddText("Generating the file list for validation from commit: %s", commit_string.c_str());

    auto [commit, tree] = LookupCommitAndGetTree(commit_string);

    PruneRepoPaths();

    EnsureRepositoriesMatch(false);
}


void Creator::GenerateFileList(const cs::string_sz commit_string)
{
    m_loggingListBox->AddText("Generating the file list from commit: %s", commit_string.c_str());

    auto [commit, tree] = LookupCommitAndGetTree(commit_string);

    const std::vector<std::string> all_repo_paths = m_repoPaths;

    PruneRepoPaths();

    const std::vector<std::string>& included_repo_paths = m_repoPaths;
    const std::string* included_repo_paths_itr = included_repo_paths.data();

    std::vector<std::string> excluded_repo_paths;

    for( const std::string& repo_path: all_repo_paths )
    {
        if( repo_path == *included_repo_paths_itr )
        {
            ++included_repo_paths_itr;
        }

        else
        {
            excluded_repo_paths.emplace_back(repo_path);
        }
    }

    ASSERT(all_repo_paths.size() == ( included_repo_paths.size() + excluded_repo_paths.size() ));

    auto write_repo_paths = [&](const char* const type, const std::vector<std::string>& repo_paths)
    {
        FileIO::TextFile text_file;
        text_file.OpenForTextWritingCreate(Path::Combine(m_overridesDirectory, FormatText("file-listing-%s.txt", type)));

        for( const std::string& repo_path : repo_paths )
            text_file.WriteLine(repo_path);
    };

    write_repo_paths("source", all_repo_paths);
    write_repo_paths("included", included_repo_paths);
    write_repo_paths("excluded", excluded_repo_paths);

    OpenContainingFolder(m_overridesDirectory);
}


void Creator::PopulateRepoPaths(const GitTree& tree, const std::string& base_path)
{
    const size_t count = tree.GetEntryCount();

    for( size_t i = 0; i < count; ++i )
    {
        const GitTreeEntry tree_entry = tree.GetEntryByIndex(i);

        std::string repo_path = Path::Combine(base_path, tree_entry.GetName());
        ASSERT(repo_path == Path::ToNativeSlash(repo_path));

        const GitObjectType entry_type = tree_entry.GetType();
        GitObject object = tree_entry.GetObject();

        // entries will be another tree...
        if( entry_type == GitObjectType::Tree )
        {
            PopulateRepoPaths(object.GetTree(), repo_path);
        }

        // ... or a file
        else if( entry_type == GitObjectType::Blob )
        {
            m_repoPaths.emplace_back(repo_path);
            m_repoBlobObjects.try_emplace(std::move(repo_path), std::move(object));
        }

        else
        {
            ASSERT(false);
        }
    }
}


void Creator::PruneRepoPaths()
{
    const std::string exclusions_file_path = Path::Combine(m_overridesDirectory, "exclusions.txt");

    m_loggingListBox->AddText(SharableString());
    m_loggingListBox->AddText("Pruning files based on gitignore rules from: %s", exclusions_file_path.c_str());

    std::vector<std::string>& repo_paths = m_repoPaths;
    const size_t initial_file_count = repo_paths.size();

    GitIgnoreEvaluator gitignore_evaluator;
    gitignore_evaluator.AddRulesFromFile(exclusions_file_path);

    for( size_t i = repo_paths.size() - 1; i < repo_paths.size(); --i )
    {
        if( gitignore_evaluator.Ignore(repo_paths[i]) )
            repo_paths.erase(repo_paths.begin() + i);
    }

    m_loggingListBox->AddText("Pruned files from %d to %d.", static_cast<int>(initial_file_count),
                                                             static_cast<int>(repo_paths.size()));
}


void Creator::PrepareOutputDirectory()
{
    // move all non-Git files to a temporary directory, which will then be recycled
    const std::string temp_directory = GetUniqueTempFilePath("CSPro-Open-Source-Old-Files");

    m_loggingListBox->AddText(SharableString());
    m_loggingListBox->AddText("Moving existing open source files to: %s", temp_directory.c_str());

    FileIO::CreateDirectories(temp_directory);

    SHFILEOPSTRUCT info = { nullptr };
    wchar_t complete_from_path[MAX_PATH];
    wchar_t complete_to_path[MAX_PATH];
    info.wFunc = FO_MOVE;
    info.fFlags = FOF_NOCONFIRMATION;
    info.pFrom = complete_from_path;
    info.pTo = complete_to_path;

    auto to_wide = [&](const std::string& path, wchar_t* const complete_path)
    {
        const int path_length = GetFullPathName(TC::ToWide(path).c_str(), MAX_PATH, complete_path, nullptr);

        if( path_length == 0 || path_length >= MAX_PATH )
            throw CSProException("GetFullPathName error: %s", path.c_str());
    };

    DirectoryLister directory_lister(false, true, true, false);

    for( const std::string& path : directory_lister.GetPaths(m_openSourceDirectory) )
    {
        if( Path::GetFilename(path) == ".git" )
            continue;

        const std::string temp_path = Path::Combine(temp_directory, Path::GetFilename(path));

        to_wide(path, complete_from_path);
        to_wide(temp_path, complete_to_path);

        if( SHFileOperation(&info) != 0 )
            throw CSProException("Error moving '%s' to '%s'.", path.c_str(), temp_path.c_str());
    }

    info.wFunc = FO_DELETE;
    info.fFlags |= FOF_ALLOWUNDO;
    to_wide(temp_directory, complete_from_path);
    info.pTo = nullptr;

    m_loggingListBox->AddText("Recycling: %s", temp_directory.c_str());

    if( SHFileOperation(&info) != 0 )
        throw CSProException("Error recycling: %s", temp_directory.c_str());
}


void Creator::CopyFilesToOutputDirectory()
{
    m_loggingListBox->AddText(SharableString());
    m_loggingListBox->AddText("Copying %d files to: %s", static_cast<int>(m_repoPaths.size()),
                                                         m_openSourceDirectory.c_str());

    uint64_t total_content_size = 0;

    constexpr double PercentReportingInterval = 5;
    const double percent_multiplier = CreatePercentMultiplier(m_repoPaths.size());
    double percent = 0;
    double next_percent_for_reporting = PercentReportingInterval;

    for( const std::string& repo_path : m_repoPaths )
    {
        const GitObject& object = m_repoBlobObjects.find(repo_path)->second;

        object.DoAsBlob(
            [&](const void* const data, const size_t size)
            {
                const std::string output_file_path = Path::Combine(m_openSourceDirectory, repo_path);
                FileIO::Write(output_file_path, data, size);
                total_content_size += size;
            });

        percent += percent_multiplier;

        if( percent >= next_percent_for_reporting )
        {
            m_loggingListBox->AddText("Copy percent: %d", static_cast<int>(percent));
            next_percent_for_reporting += PercentReportingInterval;
        }
    }

    m_loggingListBox->AddText("Copied bytes: " Formatter_uint64_t, total_content_size);
}


void Creator::CopyReplacementFiles()
{
    const std::string replacements_file_path = Path::Combine(m_overridesDirectory, "replacements.json");

    m_loggingListBox->AddText(SharableString());
    m_loggingListBox->AddText("Copying replacement files specified in: %s", replacements_file_path.c_str());

    const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(replacements_file_path);

    for( const JsonNode& replacement_json_node : json_reader->GetArray() )
    {
        const std::string repo_path = Path::ToNativeSlash(replacement_json_node.Get<std::string>("repoPath"));
        const std::string replacement_file_path = replacement_json_node.GetAbsolutePath("replacementPath");
        const std::string output_file_path = Path::Combine(m_openSourceDirectory, repo_path);

        m_loggingListBox->AddText("Replacing: " + repo_path);

        PortableFunctions::FileCopyWithExceptions(replacement_file_path, output_file_path, FileOverwriteFlag::Fail);
    }
}


void Creator::CreateSqliteWithoutSEE(const GitTree& tree)
{
    m_loggingListBox->AddText(SharableString());
    m_loggingListBox->AddText("Creating the non-SEE version of SQLite...");

    const std::string sqlite_repo_path = "cspro/external/SQLite/";

    const GitTreeEntry tree_entry = tree.GetEntryByPath(Path::Combine(sqlite_repo_path, "sqlite3.h"));
    const GitObject header = tree_entry.GetObject();

    // find the version of SQLite that this release uses
    std::string version;
    std::string full_version_line;

    header.DoAsBlob(
        [&](const void* const data, const size_t size)
        {
            constexpr std::string_view VersionPrefix_sv = "#define SQLITE_VERSION";

            SO::ForeachLine(std::string_view(static_cast<const char*>(data), size), false,
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
        });

    if( version.empty() )
        throw CSProException("Could not find the version in sqlite3.h.");

    m_loggingListBox->AddText("Found SQLite version for this release: " + version);

    // use a cached version when possible
    const std::string cache_key_h = "SQLite-" + version + "-h";
    const std::string cache_key_c = "SQLite-" + version + "-c";
    std::string sqlite_h = m_settingsDb.ReadOrDefault(cache_key_h, SO::Empty_string);
    std::string sqlite_c = m_settingsDb.ReadOrDefault(cache_key_c, SO::Empty_string);
    ASSERT(sqlite_h.empty() == sqlite_c.empty());

    if( !sqlite_h.empty() && !sqlite_c.empty() )
    {
        m_loggingListBox->AddText("Using a cached version of the SQLite amalgamation files.");
    }

    // if not created, download the non-SEE SQLite amalgamation from: https://github.com/rhuijben/sqlite-amalgamation/
    else
    {
        constexpr const char* AmalgamationRepository = "rhuijben/sqlite-amalgamation";

        CurlHttpConnection connection;

        // find this commit with this version
        std::string commit_sha;

        for( int commit_page = 1; commit_sha.empty(); ++commit_page )
        {
            HeaderList headers;
            headers.Add("User-Agent: CSPro Open Source Release Creator");
            headers.Add_Accept_Json();

            std::string url = FormatText("https://api.github.com/repos/%s/commits?page=%d", AmalgamationRepository, commit_page);

            const HttpRequest request = HttpRequestBuilder(std::move(url), std::move(headers)).build();
            HttpResponse response = connection.Request(request);

            const JsonNode json_node = Json::Parse(response.body.ToString());
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

        m_loggingListBox->AddText("Downloading SQLite files from %s commit SHA: %s", AmalgamationRepository, commit_sha.c_str());

        // download the non-SEE versions
        auto process = [&](const bool is_header, std::string& sqlite_result)
        {
            const std::string url = FormatText("https://raw.githubusercontent.com/%s/%s/%s",
                                               AmalgamationRepository,
                                               commit_sha.c_str(),
                                               is_header ? "sqlite3.h" : "sqlite3.c");

            const HttpRequest request = HttpRequestBuilder(url).build();
            HttpResponse response = connection.Request(request);

            if( response.http_status != HttpResponse::Status_200_OK )
                throw CSProException("Error accessing: " + url);

            sqlite_result = response.body.ToString();

            if( is_header && sqlite_result.find(full_version_line) == std::string::npos )
                throw CSProException("The SQLite amalgamation version header does not match: " + full_version_line);
        };

        process(true, sqlite_h);
        process(false, sqlite_c);

        // OS_TODO before f2e462839685617f68a01518af2db690edc24645 is V1, after until ? is V2, then V3
        const SQLiteSourceUpdater::DllVersion sqlite_version = SQLiteSourceUpdater::DllVersion::V1;

        SQLiteSourceUpdater::Update(sqlite_h, sqlite_c, SQLiteSourceUpdater::SQLiteVersion::Public, sqlite_version);

        // cache these results
        m_settingsDb.Write(cache_key_h, sqlite_h);
        m_settingsDb.Write(cache_key_c, sqlite_c);
    }

    ASSERT(!sqlite_h.empty() && !sqlite_c.empty());

    auto write = [&](const char* const filename, const std::string& text)
    {
        const std::string output_file_path = Path::Combine(m_openSourceDirectory, Path::ToNativeSlash(sqlite_repo_path), filename);
        m_loggingListBox->AddText("Saving '%s' (length %d) to: %s", filename, static_cast<int>(text.size()), output_file_path.c_str());
        FileIO::WriteText(output_file_path, text, false);
    };

    write("sqlite3.h", sqlite_h);
    write("sqlite3.c", sqlite_c);
}


struct Creator::TagCommits
{
    std::string tag_name;
    GitCommit commit;

    struct PullRequest
    {
        std::string sha;
        int64_t commit_time;
        std::string branch_name;
        std::string message;
    };

    std::vector<PullRequest> pull_requests;
};


std::vector<Creator::TagCommits> Creator::GetReleaseTags(const std::string_view earliest_tag_sv)
{
    std::vector<TagCommits> tag_commits;
    std::regex tag_regex = std::regex(R"(^refs/tags/v(\d+\.\d+\.\d+).*$)");
    std::smatch matches;

    m_repo.ForeachTag(
        [&](const GitTag tag)
        {
            constexpr bool keep_processing = true;

            if( !std::regex_search(tag.GetName(), matches, tag_regex) )
                return keep_processing;

            std::string tag_name = matches.str(1);

            if( tag_name < earliest_tag_sv )
                return keep_processing;

            GitCommit commit = m_repo.LookupCommit(tag);

            // associate the tag with the latest commit in case of multiple tags for the same version (e.g., v7.6.1-Apr20 and v7.6.1-Apr26)
            auto lookup = std::find_if(tag_commits.begin(), tag_commits.end(),
                                       [&](const TagCommits& tc) { return ( tag_name == tc.tag_name ); });

            if( lookup == tag_commits.end() )
            {
                tag_commits.emplace_back(TagCommits { std::move(tag_name), std::move(commit) });
            }

            else if( lookup->commit.GetAuthor().GetWhen().GetTimestamp() < commit.GetAuthor().GetWhen().GetTimestamp() )
            {
                lookup->commit = std::move(commit);
            }

            return keep_processing;
        });

    // sort by name
    std::sort(tag_commits.begin(), tag_commits.end(),
              [&](const TagCommits& tc1, const TagCommits& tc2) { return ( tc1.tag_name < tc2.tag_name ); });

    return tag_commits;
}


void Creator::CreateHistoryLog(const GitCommit& latest_commit)
{
    constexpr std::string_view EarliestTag_sv = "7.6.0";
    constexpr const char* EarliestCommitSHA = "1da299ffcad0143ae4218b6a1057ef8e1ed8935d"; // the first commit after the v7.5.0 tag

    const std::string history_file_path = Path::Combine(m_openSourceDirectory, "HISTORY.md");

    m_loggingListBox->AddText(SharableString());
    m_loggingListBox->AddText("Creating history log: %s", history_file_path.c_str());

    FileIO::TextFile history_file;
    history_file.OpenForTextWritingCreate(history_file_path);

    std::vector<TagCommits> tag_commits = GetReleaseTags(EarliestTag_sv);
    std::vector<TagCommits::PullRequest> newer_than_tags_pull_requests;

    if( tag_commits.empty() )
        throw ProgrammingErrorException();

    const GitCommit oldest_commit_to_process = m_repo.LookupCommit(EarliestCommitSHA);

    const std::regex commit_message_regex(R"(^Merge pull request.+CSProDevelopment\/(\S+).*)");
    std::smatch matches;

    GitRevisionWalker walker(m_repo);

    walker.Walk(latest_commit, oldest_commit_to_process,
        [&](const GitCommit commit)
        {
            // only process commits with at least two parents (which should be the pull requests)
            // that match the pull request regular expression
            if( commit.GetParentCount() < 2 ||
                !std::regex_search(commit.GetMessage(), matches, commit_message_regex) )
            {
                return;
            }

            // determine the first tag that contains this commit
            std::vector<TagCommits::PullRequest>* pull_requests = &newer_than_tags_pull_requests;

            for( TagCommits& tc : tag_commits )
            {
                if( tc.commit == commit || m_repo.IsCommitDescendantOf(tc.commit, commit) )
                {
                    pull_requests = &tc.pull_requests;
                    break;
                }
            }

            std::string pull_request_branch_name = matches.str(1);
            std::string pull_request_message = matches.suffix().str();
            SO::MakeTrim(pull_request_message);

            pull_requests->emplace_back(
                TagCommits::PullRequest
                {
                    commit.GetObjectId().GetHexHash(),
                    commit.GetAuthor().GetWhen().GetTimestamp(),
                    std::move(pull_request_branch_name),
                    std::move(pull_request_message)
                });
        });

    history_file.WriteLine("## Overview\n");
    history_file.WriteLine("Because most CSPro development occurs on a [private repository](https://github.com/CSProDevelopment/cspro), "
                           "the history of this public repository does not reveal much about CSPro development. Because of this, "
                           "this document lists information about each pull request merged into the private repository.");

    auto write_pull_requests = [&](const std::vector<TagCommits::PullRequest>& pull_requests)
    {
        history_file.WriteLine("\n**Merged pull requests**:\n");

        history_file.WriteLine("| Date | Branch | Pull Request Message |");
        history_file.WriteLine("| --- | --- | --- |");

        constexpr const char* NonBreakingHyphen = "&#8209;";
        const std::string date_formatter = FormatText("%%Y%s%%m%s%%d", NonBreakingHyphen, NonBreakingHyphen);

        for( const TagCommits::PullRequest& pull_request : pull_requests )
        {
            auto escape_for_table = [&](std::string text)
            {
                return SO::Replace(text, "|", "&#124;");
            };

            history_file.WriteFormattedLine("| %s | [%s](https://github.com/CSProDevelopment/cspro/commit/%s) | %s |",
                                            DateTime::LocalDateTimeString(pull_request.commit_time, date_formatter).c_str(),
                                            escape_for_table(pull_request.branch_name).c_str(),
                                            pull_request.sha.c_str(),
                                            escape_for_table(pull_request.message).c_str());
        }
    };

    if( !newer_than_tags_pull_requests.empty() )
    {
        history_file.WriteLine("\n\n## CSPro (Latest Release)");

        write_pull_requests(newer_than_tags_pull_requests);
    }

    for( auto tag_commits_itr = tag_commits.crbegin(); tag_commits_itr != tag_commits.crend(); ++tag_commits_itr )
    {
        history_file.WriteFormattedLine("\n\n## CSPro %s", tag_commits_itr->tag_name.c_str());

        std::string url = FormatText("https://csprousers.org/downloads/cspro/cspro%s.exe", tag_commits_itr->tag_name.c_str());
        history_file.WriteFormattedLine("\n**Installer**: [%s](%s)", url.c_str(), url.c_str()); // X64_TODO add link to 64-bit installer

        url = FormatText("https://csprousers.org/downloads/cspro/cspro%s-release-notes.txt", tag_commits_itr->tag_name.c_str());
        history_file.WriteFormattedLine("\n**Release notes**: [%s](%s)", url.c_str(), url.c_str());

        if( !tag_commits_itr->pull_requests.empty() )
            write_pull_requests(tag_commits_itr->pull_requests);
    }
}


void Creator::EnsureRepositoriesMatch(const bool add_space_before_log)
{
    if( add_space_before_log )
        m_loggingListBox->AddText(SharableString());

    m_loggingListBox->AddText("Validating open source directory: %s", m_openSourceDirectory.c_str());

    // because gitignore rules can result in some tracked files being excluded, we check that
    // the open source directory contains the exact set of files from the input
    GitRepository open_source_repo;
    open_source_repo.Open(m_openSourceDirectory);

    std::map<std::string, bool, cs::case_insensitive_less> expected_repo_paths; // path -> found in the open source directory
    std::set<std::string, cs::case_insensitive_less> unexpected_repo_paths;

    for( const std::string& repo_path : m_repoPaths )
        expected_repo_paths.try_emplace(repo_path, false);

    for( const char* repo_path : { R"(HISTORY.md)",
                                   R"(cspro\CSEntryDroid\app\src\main\res\values\api_keys.xml)",
                                   R"(cspro\external\SQLite\sqlite3.c)",
                                   R"(cspro\external\SQLite\sqlite3.h)",
                                   R"(cspro\zToolsO\ApiKeys.h)" } )
    {
        expected_repo_paths.try_emplace(repo_path, false);
    }

    const std::function<void(std::string path, unsigned int status_flags)> process_repo_path =
        [&](std::string path, const unsigned int status_flags)
        {
            // ignore files that are deleted in the working directory
            if( ( status_flags & GIT_STATUS_WT_DELETED ) != 0 )
                return;

            Path::MakeToNativeSlash(path);

            auto lookup = expected_repo_paths.find(path);

            if( lookup != expected_repo_paths.cend() )
            {
                lookup->second = true;
            }

            else
            {
                unexpected_repo_paths.emplace(std::move(path));
            }
        };

    // process tracked files
    open_source_repo.ForeachStatusInIndex(process_repo_path);

    // process modified and untracked files
    open_source_repo.ForeachStatusInWorkingDirectory(process_repo_path);

    std::string missing_repo_paths_text;

    for( const auto& [repo_path, found] : expected_repo_paths )
    {
        if( !found )
            SO::AppendWithSeparator(missing_repo_paths_text, "    " + repo_path, '\n');
    }

    m_loggingListBox->AddText(SharableString());

    if( missing_repo_paths_text.empty() )
    {
        m_loggingListBox->AddText("No files are missing.");
    }

    else
    {
        m_loggingListBox->AddText("The following files are missing:");
        m_loggingListBox->AddText(missing_repo_paths_text);
    }

    std::string unexpected_repo_paths_text;

    for( const std::string& repo_path : unexpected_repo_paths )
        SO::AppendWithSeparator(unexpected_repo_paths_text, "    " + repo_path, '\n');

    m_loggingListBox->AddText(SharableString());

    if( unexpected_repo_paths_text.empty() )
    {
        m_loggingListBox->AddText("No unexpected files are present.");
    }

    else
    {
        m_loggingListBox->AddText("The following unexpected files are present:");
        m_loggingListBox->AddText(unexpected_repo_paths_text);
    }

    if( !missing_repo_paths_text.empty() || !unexpected_repo_paths_text.empty() )
        throw CSProException("There are missing or unexpected files present in the open source directory.");
}
