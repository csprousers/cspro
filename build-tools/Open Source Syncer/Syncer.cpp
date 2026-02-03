#include "StdAfx.h"
#include "Syncer.h"
#include <zToolsO/CaseInsensitiveComparer.h>
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/File.h>
#include <zJson/JsonSpecFile.h>
#include <zNetwork/CurlHttpConnection.h>
#include <zGit/GitBlob.h>
#include <zGit/GitBranch.h>
#include <zGit/GitDiff.h>
#include <zGit/GitIndex.h>
#include <zGit/GitMerge.h>
#include <zGit/GitRevisionWalker.h>
#include <Update SQLite/SQLiteSourceUpdater.h>


Syncer::Syncer(SettingsDb& settings_db, LoggingListBox& logging_list_box)
    :   m_settingsDb(settings_db),
        m_loggingListBox(logging_list_box)
{
    const std::string this_source_directory = PortableFunctions::PathGetDirectory(__FILE__);

    m_overridesDirectory = Path::Combine(this_source_directory, "Overrides");

    std::string git_directory = MakeFullPath(this_source_directory, "..\\..\\.git");
    m_privateRepo.OpenBare(std::move(git_directory));
}


void Syncer::SetOpenSourceDirectory(const std::string& open_source_directory)
{
    if( m_openSourceDirectory == open_source_directory )
        return;

    m_openSourceRepo.Close();

    m_loggingListBox.AddText("Opening open source repository: " + open_source_directory);

    m_openSourceRepo.Open(open_source_directory);
    m_openSourceDirectory = open_source_directory;

    ASSERT(Path::RemoveTrailingSlash(m_openSourceRepo.GetWorkingDirectory()) == Path::RemoveTrailingSlash(open_source_directory));

    m_repoPaths.clear();
    m_repoBlobObjects.clear();
}


std::vector<GitTag> Syncer::GetTags() const
{
    std::vector<GitTag> tags = m_privateRepo.GetTags();

    // sort by name
    std::sort(tags.begin(), tags.end(),
              [&](const GitTag& tag1, const GitTag& tag2) { return ( tag1.GetName() < tag2.GetName() ); });

    return tags;
}


std::tuple<GitCommit, GitTree> Syncer::LookupCommitAndGetTree(const cs::string_sz commit_string)
{
    GitCommit commit = m_privateRepo.LookupCommit(commit_string);

    const GitSignature& author = commit.GetAuthor();
    m_loggingListBox.AddText(std::string("    Author: ").append(author.GetName()));
    m_loggingListBox.AddText(std::string("    Date: ").append(author.GetWhen().GetLocalDateTimeString()));

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

    m_loggingListBox.AddText(std::move(message_text));

    // get the list of the files that are part of this release
    GitTree tree = commit.GetTree();

    PopulateRepoPaths(tree, "");

    return { std::move(commit), std::move(tree) };
}


void Syncer::CreateRelease(const cs::string_sz commit_string)
{
    ASSERT(!m_openSourceDirectory.empty());

    m_loggingListBox.AddText("Creating open source release from commit: %s", commit_string.c_str());

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

    m_loggingListBox.AddText(SharableString());
    m_loggingListBox.AddText("Successfully created the open source release.");
}


void Syncer::ValidateRelease(const cs::string_sz commit_string)
{
    m_loggingListBox.AddText("Generating the file list for validation from commit: %s", commit_string.c_str());

    auto [commit, tree] = LookupCommitAndGetTree(commit_string);

    PruneRepoPaths();

    EnsureRepositoriesMatch(false);
}


void Syncer::GenerateFileList(const cs::string_sz commit_string)
{
    m_loggingListBox.AddText("Generating the file list from commit: %s", commit_string.c_str());

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


void Syncer::PopulateRepoPaths(const GitTree& tree, const std::string& base_path)
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


bool Syncer::IsFileExcluded(const std::string& cs_file_path)
{
    if( !m_exclusionEvaluator.has_value() )
    {
        const std::string exclusions_file_path = Path::Combine(m_overridesDirectory, "exclusions.txt");

        m_loggingListBox.AddText("Reading excluded files based on gitignore rules from: " + exclusions_file_path);

        m_exclusionEvaluator.emplace();
        m_exclusionEvaluator->AddRulesFromFile(exclusions_file_path);
    }

    return m_exclusionEvaluator->Ignore(cs_file_path);
}


void Syncer::PruneRepoPaths()
{
    m_loggingListBox.AddText(SharableString());
    m_loggingListBox.AddText("Pruning files based on gitignore rules");

    std::vector<std::string>& repo_paths = m_repoPaths;
    const size_t initial_file_count = repo_paths.size();

    for( size_t i = repo_paths.size() - 1; i < repo_paths.size(); --i )
    {
        if( IsFileExcluded(repo_paths[i]) )
            repo_paths.erase(repo_paths.begin() + i);
    }

    m_loggingListBox.AddText("Pruned files from %d to %d.", static_cast<int>(initial_file_count),
                                                            static_cast<int>(repo_paths.size()));
}


void Syncer::PrepareOutputDirectory()
{
    // move all non-Git files to a temporary directory, which will then be recycled
    const std::string temp_directory = GetUniqueTempFilePath("CSPro-Open-Source-Old-Files");

    m_loggingListBox.AddText(SharableString());
    m_loggingListBox.AddText("Moving existing open source files to: %s", temp_directory.c_str());

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

    m_loggingListBox.AddText("Recycling: %s", temp_directory.c_str());

    if( SHFileOperation(&info) != 0 )
        throw CSProException("Error recycling: %s", temp_directory.c_str());
}


void Syncer::CopyFilesToOutputDirectory()
{
    m_loggingListBox.AddText(SharableString());
    m_loggingListBox.AddText("Copying %d files to: %s", static_cast<int>(m_repoPaths.size()),
                                                        m_openSourceDirectory.c_str());

    uint64_t total_content_size = 0;

    constexpr double PercentReportingInterval = 5;
    const double percent_multiplier = CreatePercentMultiplier(m_repoPaths.size());
    double percent = 0;
    double next_percent_for_reporting = PercentReportingInterval;

    for( const std::string& repo_path : m_repoPaths )
    {
        const GitObject& object = m_repoBlobObjects.find(repo_path)->second;
        const GitBlob blob = object.GetBlob();

        const std::string output_file_path = Path::Combine(m_openSourceDirectory, repo_path);
        blob.WriteToDisk(output_file_path);

        total_content_size += blob.size();
        percent += percent_multiplier;

        if( percent >= next_percent_for_reporting )
        {
            m_loggingListBox.AddText("Copy percent: %d", static_cast<int>(percent));
            next_percent_for_reporting += PercentReportingInterval;
        }
    }

    m_loggingListBox.AddText("Copied bytes: " Formatter_uint64_t, total_content_size);
}


template<typename T/* = bool*/>
T Syncer::HasFileReplacement(const std::string& cs_file_path)
{
    if( m_fileReplacements.empty() )
    {
        const std::string replacements_file_path = Path::Combine(m_overridesDirectory, "replacements.json");

        m_loggingListBox.AddText("Reading replacement files specified in: " + replacements_file_path);

        const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(replacements_file_path);

        for( const JsonNode& replacement_json_node : json_reader->GetArray() )
        {
            const bool is_file_path = replacement_json_node.Contains("replacementPath");

            m_fileReplacements.emplace(
                replacement_json_node.Get<std::string>("repoPath"),
                FileReplacement
                {
                    is_file_path,
                    is_file_path ? replacement_json_node.GetAbsolutePath("replacementPath") :
                                   replacement_json_node.Get<std::string>("replacementRoutine")
                }
            );
        }
    }

    auto lookup = m_fileReplacements.find(cs_file_path);

    if constexpr(std::is_same_v<T, bool>)
    {
        return ( lookup != m_fileReplacements.cend() );
    }

    else
    {
        return lookup;
    }
}


std::unique_ptr<BinaryBlock> Syncer::GetFileReplacement(const git_diff_file& new_file)
{
    const auto& lookup = HasFileReplacement<std::map<std::string, FileReplacement>::iterator>(new_file.path);

    if( lookup == m_fileReplacements.cend() )
    {
        return nullptr;
    }

    else if( lookup->second.is_file_path )
    {
        return std::make_unique<BinaryBlock>(FileIO::ReadBinary(lookup->second.file_path_or_routine));
    }

    else
    {
        throw ProgrammingErrorException();
    }
}


void Syncer::CopyReplacementFiles()
{
    const std::string replacements_file_path = Path::Combine(m_overridesDirectory, "replacements.json");

    m_loggingListBox.AddText(SharableString());
    m_loggingListBox.AddText("Copying replacement files specified in: %s", replacements_file_path.c_str());

    const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(replacements_file_path);

    for( const JsonNode& replacement_json_node : json_reader->GetArray() )
    {
        const std::string repo_path = Path::ToNativeSlash(replacement_json_node.Get<std::string>("repoPath"));
        const std::string replacement_file_path = replacement_json_node.GetAbsolutePath("replacementPath");
        const std::string output_file_path = Path::Combine(m_openSourceDirectory, repo_path);

        m_loggingListBox.AddText("Replacing: " + repo_path);

        PortableFunctions::FileCopyWithExceptions(replacement_file_path, output_file_path, FileOverwriteFlag::Fail);
    }
}


void Syncer::CreateSqliteWithoutSEE(const GitTree& tree)
{
    m_loggingListBox.AddText(SharableString());
    m_loggingListBox.AddText("Creating the non-SEE version of SQLite...");

    const std::string sqlite_repo_path = "cspro/external/SQLite/";

    const GitTreeEntry tree_entry = tree.GetEntryByPath(Path::Combine(sqlite_repo_path, "sqlite3.h"));
    const GitBlob header = tree_entry.GetObject().GetBlob();

    // find the version of SQLite that this release uses
    constexpr std::string_view VersionPrefix_sv = "#define SQLITE_VERSION";
    std::string version;
    std::string full_version_line;

    SO::ForeachLine(header.as<std::string_view>(), false,
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
        throw CSProException("Could not find the version in sqlite3.h.");

    m_loggingListBox.AddText("Found SQLite version for this release: " + version);

    // use a cached version when possible
    const std::string cache_key_h = "SQLite-" + version + "-h";
    const std::string cache_key_c = "SQLite-" + version + "-c";
    std::string sqlite_h = m_settingsDb.ReadOrDefault(cache_key_h, SO::Empty_string);
    std::string sqlite_c = m_settingsDb.ReadOrDefault(cache_key_c, SO::Empty_string);
    ASSERT(sqlite_h.empty() == sqlite_c.empty());

    if( !sqlite_h.empty() && !sqlite_c.empty() )
    {
        m_loggingListBox.AddText("Using a cached version of the SQLite amalgamation files.");
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
            headers.Add("User-Agent: CSPro Open Source Syncer");
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

        m_loggingListBox.AddText("Downloading SQLite files from %s commit SHA: %s", AmalgamationRepository, commit_sha.c_str());

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
        m_loggingListBox.AddText("Saving '%s' (length %d) to: %s", filename, static_cast<int>(text.size()), output_file_path.c_str());
        FileIO::WriteText(output_file_path, text, false);
    };

    write("sqlite3.h", sqlite_h);
    write("sqlite3.c", sqlite_c);
}


struct Syncer::TagCommits
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


std::vector<Syncer::TagCommits> Syncer::GetReleaseTags(const std::string_view earliest_tag_sv)
{
    std::vector<TagCommits> tag_commits;
    std::regex tag_regex = std::regex(R"(^refs/tags/v(\d+\.\d+\.\d+).*$)");
    std::smatch matches;

    m_privateRepo.ForeachTag(
        [&](const GitTag tag)
        {
            constexpr bool keep_processing = true;

            if( !std::regex_search(tag.GetName(), matches, tag_regex) )
                return keep_processing;

            std::string tag_name = matches.str(1);

            if( tag_name < earliest_tag_sv )
                return keep_processing;

            GitCommit commit = m_privateRepo.LookupCommit(tag);

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


void Syncer::CreateHistoryLog(const GitCommit& latest_commit)
{
    constexpr std::string_view EarliestTag_sv = "7.6.0";

    // the first commit in dev after the v7.5.0 tag
    constexpr const char* EarliestCommitSHA = "95d126ccaab8872d1d914f64d16824a5f2f19ccd";

    const std::string history_file_path = Path::Combine(m_openSourceDirectory, "HISTORY.md");

    m_loggingListBox.AddText(SharableString());
    m_loggingListBox.AddText("Creating history log: %s", history_file_path.c_str());

    FileIO::TextFile history_file;
    history_file.OpenForTextWritingCreate(history_file_path);

    std::vector<TagCommits> tag_commits = GetReleaseTags(EarliestTag_sv);
    std::vector<TagCommits::PullRequest> newer_than_tags_pull_requests;

    if( tag_commits.empty() )
        throw ProgrammingErrorException();

    const GitCommit oldest_commit_to_process = m_privateRepo.LookupCommit(EarliestCommitSHA);

    const std::regex commit_message_regex(R"(^Merge pull request.+CSProDevelopment\/(\S+).*)");
    std::smatch matches;

    GitRevisionWalker walker(m_privateRepo);

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
                if( tc.commit == commit || m_privateRepo.IsCommitDescendantOf(tc.commit, commit) )
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


void Syncer::EnsureRepositoriesMatch(const bool add_space_before_log)
{
    if( add_space_before_log )
        m_loggingListBox.AddText(SharableString());

    m_loggingListBox.AddText("Validating open source directory: %s", m_openSourceDirectory.c_str());

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

    m_loggingListBox.AddText(SharableString());

    if( missing_repo_paths_text.empty() )
    {
        m_loggingListBox.AddText("No files are missing.");
    }

    else
    {
        m_loggingListBox.AddText("The following files are missing:");
        m_loggingListBox.AddText(missing_repo_paths_text);
    }

    std::string unexpected_repo_paths_text;

    for( const std::string& repo_path : unexpected_repo_paths )
        SO::AppendWithSeparator(unexpected_repo_paths_text, "    " + repo_path, '\n');

    m_loggingListBox.AddText(SharableString());

    if( unexpected_repo_paths_text.empty() )
    {
        m_loggingListBox.AddText("No unexpected files are present.");
    }

    else
    {
        m_loggingListBox.AddText("The following unexpected files are present:");
        m_loggingListBox.AddText(unexpected_repo_paths_text);
    }

    if( !missing_repo_paths_text.empty() || !unexpected_repo_paths_text.empty() )
        throw CSProException("There are missing or unexpected files present in the open source directory.");
}


void Syncer::StartMirror()
{
    // make sure that there are no pending open source changes
    if( m_openSourceRepo.HasChanges() )
        throw CSProException("You cannot run the sync if there are changes in the open source directory.");

    // OS_TODO need to first sync main, and then dev
    constexpr const char* branch_name = "dev";

    const GitBranch os_merge_branch = m_openSourceRepo.LookupBranch(branch_name);
    GitCommit os_last_merged_commit = m_openSourceRepo.LookupCommit(os_merge_branch.GetTarget());

    // OS_TODO calculate cs_old_merge_commit + cs_new_merge_commit
}


GitCommit Syncer::CreateMirroredCommit(const GitCommit& cs_commit, const GitTree& os_written_tree,
                                       const GitCommit& os_parent_commit1, const GitCommit* const os_parent_commit2)
{
    const GitObjectId os_commit_oid = m_openSourceRepo.CreateCommit(
        cs_commit.GetAuthor(),
        cs_commit.GetCommitter(),
        cs_commit.GetMessage(),
        os_written_tree,
        os_parent_commit1,
        os_parent_commit2
    );

    m_loggingListBox.AddText("\nMirrored commit %s: %s\n\n",
                             os_commit_oid.GetHexHash().c_str(),
                             cs_commit.GetMessage().c_str());

    return m_openSourceRepo.LookupCommit(os_commit_oid);
}


GitCommit Syncer::MirrorFeatureBranch(const GitBranch& os_merge_branch, const GitCommit& os_start_commit,
                                      const GitCommit& cs_old_merge_commit, const GitCommit& cs_new_merge_commit)
{
    if( cs_old_merge_commit.GetParentCount() != 2 || cs_new_merge_commit.GetParentCount() != 2 )
    {
        throw CSProException("Update this tool to support mirroring between non-merge commits: %s -> %s",
                             cs_old_merge_commit.GetObjectId().GetHexHash().c_str(),
                             cs_new_merge_commit.GetObjectId().GetHexHash().c_str());
    }

    // create and checkout a temporary open source branch for this work
    const std::string os_temp_branch_name = SO::Concatenate(
        IntToString(GetTimestamp()),
        "-",
        os_start_commit.GetObjectId().GetHexHash()
    );

    GitBranch os_temp_branch = m_openSourceRepo.CreateBranch(os_temp_branch_name, os_start_commit);

    m_openSourceRepo.CheckoutBranch(os_temp_branch);

    // mirror the feature branch
    const GitCommit os_feature_branch_final_commit = MirrorFeatureBranchCommits(
        cs_old_merge_commit,
        cs_new_merge_commit,
        os_start_commit
    );

    // switch back to the destination branch
    m_openSourceRepo.CheckoutBranch(os_merge_branch);

    // mirror the merge commit
    GitIndex os_index = m_openSourceRepo.GetIndex();
    GitTree cs_old_merge_tree = cs_old_merge_commit.GetTree();
    GitTree cs_new_merge_tree = cs_new_merge_commit.GetTree();

    MirrorCommit(os_index, cs_old_merge_tree, cs_new_merge_tree);

    GitTree os_new_tree = m_openSourceRepo.WriteTree(os_index);

    // make sure that the feature branch matches the merge commit
    GitTree os_feature_branch_tree = os_feature_branch_final_commit.GetTree();

    const GitDiff merge_diff = m_openSourceRepo.GetDifference(os_feature_branch_tree, os_new_tree);

    if( merge_diff.GetNumberDeltas() != 0 )
    {
        throw CSProException("Differences exist between the feature branch and merge commit: %zu",
                             merge_diff.GetNumberDeltas());
    }

    GitCommit os_new_merge_commit = CreateMirroredCommit(
        cs_new_merge_commit,
        os_new_tree,
        os_start_commit,
        &os_feature_branch_final_commit
    );

    // delete the temporary feature branch
    os_temp_branch.Refresh(m_openSourceRepo);
    os_temp_branch.Delete();

    return os_new_merge_commit;
}


GitCommit Syncer::MirrorFeatureBranchCommits(const GitCommit& cs_old_merge_commit, const GitCommit& cs_new_merge_commit,
                                             const GitCommit& os_start_commit)
{
    std::optional<GitCommit> cs_parent_commit = cs_old_merge_commit;
    std::optional<GitTree> cs_parent_tree = cs_old_merge_commit.GetTree();

    std::optional<GitCommit> os_parent_commit = os_start_commit;

    // walk the two merge commits in reverse order
    GitRevisionWalker walker(m_privateRepo);

    walker.ReverseWalk(cs_new_merge_commit, cs_old_merge_commit,
        [&](GitCommit cs_commit)
        {
            // don't process the merge commit during this walk
            if( cs_commit == cs_new_merge_commit )
                return;

            m_loggingListBox.AddText("Mirroring %s: %s\n",
                                     cs_commit.GetObjectId().GetHexHash().c_str(),
                                     cs_commit.GetMessage().c_str());

            if( cs_commit.GetParentCount() != 1 )
            {
                throw CSProException("Update this tool to handle multiple parents on feature branches for commit: " +
                                     cs_commit.GetObjectId().GetHexHash());
            }

            if( *cs_parent_commit != cs_commit.GetParent(0) )
                throw ProgrammingErrorException();

            GitTree cs_commit_tree = cs_commit.GetTree();
            GitIndex os_index = m_openSourceRepo.GetIndex();

            MirrorCommit(os_index, *cs_parent_tree, cs_commit_tree);

            // commit these changes
            GitTree os_new_tree = m_openSourceRepo.WriteTree(os_index);

            os_parent_commit = CreateMirroredCommit(
                cs_commit,
                os_new_tree,
                *os_parent_commit,
                nullptr
            );

            cs_parent_commit.emplace(std::move(cs_commit));
            cs_parent_tree.emplace(std::move(cs_commit_tree));
        });

    return std::move(*os_parent_commit);
}


void Syncer::MirrorCommit(GitIndex& os_index, GitTree& cs_parent_tree, GitTree& cs_tree)
{
    // because of line ending differences between the private and open source repositories,
    // instead of using git_apply, we process differences and manually merge text files

    // get the differences between this commit and its parent
    GitDiff cs_diff = m_privateRepo.GetDifference(cs_parent_tree, cs_tree, GIT_DIFF_NORMAL);
    cs_diff.FindSimilar();

    cs_diff.ForeachDifference(
        [&](const void* const delta)
        {
            const git_diff_delta* diff_delta = static_cast<const git_diff_delta*>(delta);
            MirrorFile(os_index, *diff_delta);
            return true;
        });
}


void Syncer::MirrorFile(GitIndex& os_index, const git_diff_delta& diff_delta)
{
    const bool is_binary = ( ( diff_delta.flags & GIT_DIFF_FLAG_BINARY ) != 0 );
    const char* const file_type = is_binary ? "binary file" : "text file";

    // do not mirror files if they are in the exclusions list
    const std::string path = diff_delta.new_file.path;

    if( IsFileExcluded(path) )
    {
        m_loggingListBox.AddText("Skipping excluded %s: %s", file_type, path.c_str());
        return;
    }

    // if the file has a replacement, use it instead
    const std::unique_ptr<const BinaryBlock> replacement_data = GetFileReplacement(diff_delta.new_file);

    if( replacement_data != nullptr )
    {
        m_loggingListBox.AddText("Using override for %s: %s", file_type, path.c_str());

        if( is_binary || diff_delta.status != GIT_DELTA_MODIFIED )
            throw CSProException("Replacement files should be text with the status 'modified': " + path);

        MirrorFileAddEntry(os_index, diff_delta.new_file, replacement_data->data(), replacement_data->size());

        return;
    }

    // otherwise mirror the file
    switch( diff_delta.status )
    {
        case GIT_DELTA_ADDED:
            m_loggingListBox.AddText("Adding %s: %s", file_type, path.c_str());
            is_binary ? MirrorFileAddBinary(os_index, diff_delta.new_file) :
                        MirrorFileAddText(os_index, diff_delta.new_file);
            break;

        case GIT_DELTA_DELETED:
            m_loggingListBox.AddText("Deleting %s: %s", file_type, path.c_str());
            MirrorFileDelete(os_index, path);
            break;

        case GIT_DELTA_MODIFIED:
            m_loggingListBox.AddText("Modifying %s: %s", file_type, path.c_str());
            is_binary ? MirrorFileModifyBinary(os_index, diff_delta.new_file) :
                        MirrorFileModifyText(os_index, diff_delta.old_file, diff_delta.new_file);
            break;

        case GIT_DELTA_RENAMED:
            m_loggingListBox.AddText("Renaming %s: %s -> %s", file_type, path.c_str(), diff_delta.old_file.path);
            is_binary ? MirrorFileRenameBinary(os_index, diff_delta.old_file, diff_delta.new_file) :
                        MirrorFileRenameText(os_index, diff_delta.old_file, diff_delta.new_file);
            break;

        default:
            throw CSProException("Unknown diff status: '%s' -> %d", path.c_str(), static_cast<int>(diff_delta.status));
    }
}


void Syncer::MirrorFileAddEntry(GitIndex& os_index, const git_diff_file& new_file, const void* const data, const size_t size)
{
    const GitObjectId os_blob_oid = m_openSourceRepo.CreateBlob(data, size);
    os_index.AddEntry(os_blob_oid, new_file.path, new_file.mode);
}


void Syncer::MirrorFileAddBinary(GitIndex& os_index, const git_diff_file& new_file)
{
    const GitBlob cs_blob = m_privateRepo.LookupBlob(new_file.id);
    MirrorFileAddEntry(os_index, new_file, cs_blob.data(), cs_blob.size());
}


void Syncer::MirrorFileAddText(GitIndex& os_index, const git_diff_file& new_file)
{
    const GitBlob cs_blob = m_privateRepo.LookupBlob(new_file.id);

    // normalize the line endings
    const std::string os_blob_text = SO::ToNewlineLF(cs_blob.as<std::string>());

    MirrorFileAddEntry(os_index, new_file, os_blob_text.data(), os_blob_text.size());
}


void Syncer::MirrorFileDelete(GitIndex& os_index, const cs::string_sz path)
{
    os_index.RemoveEntryByPath(path);
}


void Syncer::MirrorFileModifyBinary(GitIndex& os_index, const git_diff_file& new_file)
{
    MirrorFileAddBinary(os_index, new_file);
}


void Syncer::MirrorFileModifyText(GitIndex& os_index, const git_diff_file& old_file, const git_diff_file& new_file)
{
    // for a three-way merge, we will load the changed data from the private repository...
    std::string cs_text_before = m_privateRepo.LookupBlob(old_file.id).as<std::string>();
    std::string cs_text_after = m_privateRepo.LookupBlob(new_file.id).as<std::string>();

    // ...and then apply it onto the open source repository
    const GitObjectId os_text_now_oid = os_index.GetObjectIdByPath(old_file.path);
    const std::string os_text_now = m_openSourceRepo.LookupBlob(os_text_now_oid).as<std::string>();

    // normalize the line endings
    SO::MakeNewlineLF(cs_text_before);
    SO::MakeNewlineLF(cs_text_after);

    if( os_text_now.find('\r') != std::string::npos )
        throw CSProException("There should not be '\\r' characters in the open source repository.");

    // merge the files
    const GitMerge::Result merge_result = GitMerge::Merge(
        cs_text_before,
        cs_text_after,
        os_text_now
    );

    if( !merge_result.automergeable )
        throw CSProException("The file is not automergeable: %s", new_file.path);

    MirrorFileAddEntry(os_index, new_file, merge_result.text.data(), merge_result.text.size());
}


void Syncer::MirrorFileRenameBinary(GitIndex& os_index, const git_diff_file& old_file, const git_diff_file& new_file)
{
    MirrorFileDelete(os_index, old_file.path);
    MirrorFileAddBinary(os_index, new_file);
}


void Syncer::MirrorFileRenameText(GitIndex& os_index, const git_diff_file& old_file, const git_diff_file& new_file)
{
    MirrorFileDelete(os_index, old_file.path);
    MirrorFileAddText(os_index, new_file);
}
