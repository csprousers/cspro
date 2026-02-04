#include "StdAfx.h"
#include "Syncer.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/Hash.h>
#include <zJson/JsonSpecFile.h>
#include <zNetwork/CurlHttpConnection.h>
#include <zGit/GitBlob.h>
#include <zGit/GitDiff.h>
#include <zGit/GitIndex.h>
#include <zGit/GitMerge.h>
#include <zGit/GitRevisionWalker.h>
#include <zGit/GitTag.h>
#include <Update SQLite/SQLiteSourceUpdater.h>
#include <regex>


CREATE_JSON_KEY(replacementPath)
CREATE_JSON_KEY(replacementRoutine)
CREATE_JSON_KEY(repoPath)


namespace
{
    constexpr const char* ExclusionsFilename   = "exclusions.txt";
    constexpr const char* ReplacementsFilename = "replacements.json";

    constexpr std::string_view LibrariesCommitMessageIdentifier_sv = "Libraries ID:";
    constexpr std::string_view LibrariesSettingsKeyPrefix_sv       = "Libraries-";
    constexpr size_t LibrariesHashLength                           = 32;
}


Syncer::Syncer(SettingsDb& settings_db, LoggingListBox& logging_list_box)
    :   m_settingsDb(settings_db),
        m_loggingListBox(logging_list_box)
{
    const std::string this_source_directory = PortableFunctions::PathGetDirectory(__FILE__);

    m_privateRepoDirectory = MakeFullPath(this_source_directory, "..\\..\\");

    m_overridesDirectory = Path::Combine(this_source_directory, "Overrides");

    m_privateRepo.OpenBare(Path::Combine(m_privateRepoDirectory, ".git"));
}


Syncer::~Syncer()
{
}


void Syncer::SetOpenSourceDirectory(const std::string& open_source_directory)
{
    if( Path::RemoveTrailingSlash(m_openSourceRepo.GetWorkingDirectory()) == Path::RemoveTrailingSlash(open_source_directory) )
        return;

    m_openSourceRepo.Close();

    m_loggingListBox.AddText("Opening open source repository: " + open_source_directory);

    m_openSourceRepo.Open(open_source_directory);
}


bool Syncer::IsFileExcluded(const std::string& cs_file_path)
{
    if( !m_exclusionEvaluator.has_value() )
    {
        const std::string exclusions_file_path = Path::Combine(m_overridesDirectory, ExclusionsFilename);

        m_loggingListBox.AddText("Reading excluded files based on gitignore rules from: " + exclusions_file_path);

        m_exclusionEvaluator.emplace();
        m_exclusionEvaluator->AddRulesFromFile(exclusions_file_path);
    }

    return m_exclusionEvaluator->Ignore(cs_file_path);
}


template<typename T/* = bool*/>
T Syncer::HasFileReplacement(const std::string& cs_file_path)
{
    if( m_fileReplacements.empty() )
    {
        const std::string replacements_file_path = Path::Combine(m_overridesDirectory, ReplacementsFilename);

        m_loggingListBox.AddText("Reading replacement files specified in: " + replacements_file_path);

        const std::unique_ptr<JsonSpecFile::Reader> json_reader = JsonSpecFile::CreateReader(replacements_file_path);

        for( const JsonNode& replacement_json_node : json_reader->GetArray() )
        {
            const bool is_file_path = replacement_json_node.Contains(JK::replacementPath);

            m_fileReplacements.emplace(
                replacement_json_node.Get<std::string>(JK::repoPath),
                FileReplacement
                {
                    is_file_path,
                    is_file_path ? replacement_json_node.GetAbsolutePath(JK::replacementPath) :
                                   replacement_json_node.Get<std::string>(JK::replacementRoutine)
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


std::string Syncer::GetFileReplacement(const git_diff_file& new_file)
{
    const auto& lookup = HasFileReplacement<std::map<std::string, FileReplacement>::iterator>(new_file.path);

    if( lookup == m_fileReplacements.cend() )
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


std::string Syncer::CreateSqliteWithoutSEE(const git_diff_file& new_file)
{
    const std::string filename = Path::GetFilename(new_file.path);
    const bool is_header = ( filename == "sqlite3.h" );
    ASSERT(is_header || filename == "sqlite3.c");

    m_loggingListBox.AddText("Creating the non-SEE version of SQLite for: " + filename);

    // find the version of SQLite currently in use
    const GitBlob cs_header_blob = m_privateRepo.LookupBlob(new_file.id);

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

    m_loggingListBox.AddText("Found SQLite version: " + version);

    // use a cached version when possible
    const std::string cache_key = FormatText("SQLite-%s-%s", version.c_str(), filename.c_str());
    std::string public_sqlite = m_settingsDb.ReadOrDefault(cache_key, SO::Empty_string);

    if( !public_sqlite.empty() )
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

        // download the non-SEE version
        const std::string url = FormatText("https://raw.githubusercontent.com/%s/%s/%s",
                                           AmalgamationRepository,
                                           commit_sha.c_str(),
                                           filename.c_str());

        const HttpRequest request = HttpRequestBuilder(url).build();
        HttpResponse response = connection.Request(request);

        if( response.http_status != HttpResponse::Status_200_OK )
            throw CSProException("Error accessing: " + url);

        public_sqlite = response.body.ToString();

        if( public_sqlite.find(full_version_line) == std::string::npos )
            throw CSProException("The SQLite amalgamation version header does not match: " + full_version_line);

        // OS_TODO change to V3 after merging 2025-03-28
        const SQLiteSourceUpdater::DllVersion sqlite_version = SQLiteSourceUpdater::DllVersion::V2;

        SQLiteSourceUpdater::Update(public_sqlite, is_header, SQLiteSourceUpdater::SQLiteVersion::Public, sqlite_version);

        // cache this result
        m_settingsDb.Write(cache_key, public_sqlite);
    }

    ASSERT(!public_sqlite.empty());

    return public_sqlite;
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


void Syncer::PopulateReleaseTags(const std::string_view earliest_tag_sv)
{
    ASSERT(m_releaseTags.empty());

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
            auto lookup = std::find_if(m_releaseTags.begin(), m_releaseTags.end(),
                                       [&](const TagCommits& tc) { return ( tag_name == tc.tag_name ); });

            if( lookup == m_releaseTags.end() )
            {
                m_releaseTags.emplace_back(TagCommits { std::move(tag_name), std::move(commit) });
            }

            else if( lookup->commit.GetAuthor().GetWhen().GetTimestamp() < commit.GetAuthor().GetWhen().GetTimestamp() )
            {
                lookup->commit = std::move(commit);
            }

            return keep_processing;
        });

    // sort by name
    std::sort(m_releaseTags.begin(), m_releaseTags.end(),
              [&](const TagCommits& tc1, const TagCommits& tc2) { return ( tc1.tag_name < tc2.tag_name ); });

    if( m_releaseTags.empty() )
        throw ProgrammingErrorException();
}


std::string Syncer::CreateHistoryLog(const GitCommit& os_latest_commit)
{
    constexpr std::string_view EarliestTag_sv = "7.6.0";

    // the first commit in dev after the v7.5.0 tag
    constexpr const char* EarliestCommitSHA = "95d126ccaab8872d1d914f64d16824a5f2f19ccd";

    m_loggingListBox.AddText("Creating history log up to the commit on: " +
                             os_latest_commit.GetCommitter().GetWhen().GetLocalDateTimeString());

    if( m_releaseTags.empty() )
        PopulateReleaseTags(EarliestTag_sv);

    std::vector<TagCommits::PullRequest> newer_than_tags_pull_requests;

    const GitCommit oldest_commit_to_process = m_privateRepo.LookupCommit(EarliestCommitSHA);

    const std::regex commit_message_regex(R"(^Merge pull request.+CSProDevelopment\/(\S+).*)");
    std::smatch matches;

    GitRevisionWalker walker(m_privateRepo);

    walker.Walk(os_latest_commit, oldest_commit_to_process,
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

            for( TagCommits& tc : m_releaseTags )
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

    std::string history =
        "## Overview\n\n"
        "Because most CSPro development occurs on a [private repository](https://github.com/CSProDevelopment/cspro), "
        "the history of this public repository does not reveal much about CSPro development. Because of this, "
        "this document lists information about each pull request merged into the private repository.\n"
        ;

    auto write_pull_requests = [&](const std::vector<TagCommits::PullRequest>& pull_requests)
    {
        history.append(
            "\n**Merged pull requests**:\n\n"
            "| Date | Branch | Pull Request Message |\n"
            "| --- | --- | --- |\n"
        );

        constexpr const char* NonBreakingHyphen = "&#8209;";
        static const std::string date_formatter = FormatText("%%Y%s%%m%s%%d", NonBreakingHyphen, NonBreakingHyphen);

        for( const TagCommits::PullRequest& pull_request : pull_requests )
        {
            auto escape_for_table = [&](std::string text)
            {
                return SO::Replace(text, "|", "&#124;");
            };

            history.append(FormatText(
                "| %s | [%s](https://github.com/CSProDevelopment/cspro/commit/%s) | %s |\n",
                DateTime::LocalDateTimeString(pull_request.commit_time, date_formatter).c_str(),
                escape_for_table(pull_request.branch_name).c_str(),
                pull_request.sha.c_str(),
                escape_for_table(pull_request.message).c_str()
            ));
        }
    };

    if( !newer_than_tags_pull_requests.empty() )
    {
        history.append("\n\n## CSPro (current development)\n");

        write_pull_requests(newer_than_tags_pull_requests);
    }

    for( auto tag_commits_itr = m_releaseTags.crbegin(); tag_commits_itr != m_releaseTags.crend(); ++tag_commits_itr )
    {
        history.append(FormatText("\n\n## CSPro %s\n",tag_commits_itr->tag_name.c_str()));

        std::string url = FormatText("https://csprousers.org/downloads/cspro/cspro%s.exe", tag_commits_itr->tag_name.c_str());
        history.append(FormatText("\n**Installer**: [%s](%s)\n", url.c_str(), url.c_str())); // X64_TODO add link to 64-bit installer

        url = FormatText("https://csprousers.org/downloads/cspro/cspro%s-release-notes.txt", tag_commits_itr->tag_name.c_str());
        history.append(FormatText("\n**Release notes**: [%s](%s)\n", url.c_str(), url.c_str()));

        if( !tag_commits_itr->pull_requests.empty() )
            write_pull_requests(tag_commits_itr->pull_requests);
    }

    return history;
}


bool Syncer::CompareRepositories(const GitCommit& cs_commit, const GitCommit& os_commit, const bool verbose)
{
    m_loggingListBox.AddText("Comparing: private (%s) <-> open source (%s)",
                             cs_commit.GetObjectId().GetHexHash().c_str(),
                             os_commit.GetObjectId().GetHexHash().c_str());

    const GitIndex cs_index = cs_commit.GetTree().GetIndex();
    const GitIndex os_index = os_commit.GetTree().GetIndex();

    if( verbose )
    {
        m_loggingListBox.AddText("Files: private (%zu) <-> open source (%zu):",
                                 cs_index.GetEntryCount(), os_index.GetEntryCount());
    }

    std::vector<std::string> fatal_errors;

    const std::map<std::string, GitObjectId> cs_files = cs_index.GetPathObjectIdMap();
    std::map<std::string, GitObjectId> os_files = os_index.GetPathObjectIdMap();

    // iterate over the files in the private repository
    for( const auto& [cs_path, cs_file_oid] : cs_files )
    {
        const auto& os_lookup = os_files.find(cs_path);

        // if the private file is not in the open source repository, make sure that it is excluded
        if( os_lookup == os_files.cend() )
        {
            if( !IsFileExcluded(cs_path) )
            {
                fatal_errors.emplace_back(u8"⚠ Missing file: " + cs_path);
            }

            else if( verbose )
            {
                m_loggingListBox.AddText("Missing file (expected exclusion): " + cs_path);
            }

            continue;
        }

        // compare the contents of the files
        if( cs_file_oid == os_lookup->second )
        {
            if( verbose )
                m_loggingListBox.AddText("Same file (by OID): " + cs_path);
        }

        // when not identical, see if this is a replacement file
        else if( HasFileReplacement(cs_path) )
        {
            if( verbose )
                m_loggingListBox.AddText("Different file (expected replacement): " + cs_path);
        }

        // otherwise compare as text with normalized line endings
        else
        {
            const std::string cs_text = SO::ToNewlineLF(m_privateRepo.LookupBlob(cs_file_oid).as<std::string>());
            const std::string os_text = m_openSourceRepo.LookupBlob(os_lookup->second).as<std::string>();

            if( cs_text != os_text )
            {
                fatal_errors.emplace_back(u8"⚠ Different file: " + cs_path);
            }

            else if( verbose )
            {
                m_loggingListBox.AddText("Same file (by normalized text comparison): " + cs_path);
            }
        }

        os_files.erase(os_lookup);
    }

    // report on any unexpected files in the open source repository,
    // first removing any files only in the open source directory
    for( const char* const os_path : { "HISTORY.md" } )
    {
        const auto& os_lookup = os_files.find(os_path);

        if( os_lookup != os_files.cend() )
        {
            if( verbose )
                m_loggingListBox.AddText("Missing file (expected addition): " + os_lookup->first);

            os_files.erase(os_lookup);
        }

        else
        {
            fatal_errors.emplace_back(u8"⚠ Missing file (open source repository): ").append(os_path);
        }
    }

    for( const auto& [os_path, os_file_oid] : os_files )
        fatal_errors.emplace_back(u8"⚠ Unexpected file: " + os_path);

    if( fatal_errors.empty() )
    {
        m_loggingListBox.AddText("\nThere are no unexpected differences between the repositories.");
        return true;
    }

    // report on the fatal errors
    m_loggingListBox.AddText(u8"\n⚠ There are %zu fatal errors!\n", fatal_errors.size());

    for( const std::string& fatal_error : fatal_errors )
        m_loggingListBox.AddText(fatal_error);

    return false;
}


void Syncer::MirrorFeatureBranches(const GitBranch& os_merge_branch,
                                   const GitCommit& cs_oldest_merge_commit, const GitCommit& cs_newest_merge_commit)
{
    // make sure that there are no pending open source changes
    if( m_openSourceRepo.HasChanges() )
        throw CSProException("You cannot run the sync if there are changes in the open source directory.");

    GitCommit os_last_merged_commit = m_openSourceRepo.LookupCommit(os_merge_branch.GetTarget());

    GitCommit cs_old_merge_commit = cs_oldest_merge_commit;

    while( true )
    {
        GitCommit cs_new_merge_commit = cs_newest_merge_commit;  // OS_TODO calculate next merge commit

        os_last_merged_commit = MirrorFeatureBranch(
            os_merge_branch,
            os_last_merged_commit,
            cs_old_merge_commit,
            cs_new_merge_commit
        );

        if( cs_new_merge_commit == cs_newest_merge_commit )
            break;

        cs_old_merge_commit = cs_new_merge_commit;
    }

    // when complete, checkout the HEAD so that the working directory matches the index
    m_openSourceRepo.CheckoutHead(GIT_CHECKOUT_FORCE);
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

    // update HISTORY.md
    const std::string history = CreateHistoryLog(cs_new_merge_commit);
    const GitObjectId os_history_blob_oid = m_openSourceRepo.CreateBlob(history.data(), history.size());
    os_index.AddEntry(os_history_blob_oid, "HISTORY.md", GIT_FILEMODE_BLOB);

    // commit this merge commit with the updated history
    os_new_tree = m_openSourceRepo.WriteTree(os_index);

    GitCommit os_new_merge_commit = CreateMirroredCommit(
        cs_new_merge_commit,
        os_new_tree,
        os_start_commit,
        &os_feature_branch_final_commit
    );

    // delete the temporary feature branch
    os_temp_branch.Refresh(m_openSourceRepo);
    os_temp_branch.Delete();

    // make sure that the repositories match
    if( !CompareRepositories(cs_new_merge_commit, os_new_merge_commit, false) )
        throw CSProException("The repositories do not following the creation of the merge commit.");

    return os_new_merge_commit;
}


GitCommit Syncer::MirrorFeatureBranchCommits(const GitCommit& cs_old_merge_commit, const GitCommit& cs_new_merge_commit,
                                             const GitCommit& os_start_commit)
{
    GitCommit cs_parent_commit = cs_old_merge_commit;
    GitTree cs_parent_tree = cs_old_merge_commit.GetTree();

    GitCommit os_parent_commit = os_start_commit;

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

            if( cs_parent_commit != cs_commit.GetParent(0) )
                throw ProgrammingErrorException();

            GitTree cs_commit_tree = cs_commit.GetTree();
            GitIndex os_index = m_openSourceRepo.GetIndex();

            MirrorCommit(os_index, cs_parent_tree, cs_commit_tree);

            // commit these changes
            GitTree os_new_tree = m_openSourceRepo.WriteTree(os_index);

            os_parent_commit = CreateMirroredCommit(
                cs_commit,
                os_new_tree,
                os_parent_commit,
                nullptr
            );

            cs_parent_commit = std::move(cs_commit);
            cs_parent_tree = std::move(cs_commit_tree);
        });

    return os_parent_commit;
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
    const std::string replacement_text = GetFileReplacement(diff_delta.new_file);

    if( !replacement_text.empty() )
    {
        m_loggingListBox.AddText("Using override for %s: %s", file_type, path.c_str());

        if( is_binary || diff_delta.status != GIT_DELTA_MODIFIED )
            throw CSProException("Replacement files should be text with the status 'modified': " + path);

        MirrorFileAddEntry(os_index, diff_delta.new_file, replacement_text.data(), replacement_text.size());

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


struct Syncer::BuiltLibrary
{
    std::string file_path;
    std::string repo_path;
    std::optional<GitObjectId> cs_blob_oid;
    std::string md5;
    std::shared_ptr<BinaryBlock> file_data;
};


void Syncer::PopulateBuiltLibraries(const GitCommit& cs_commit)
{
    constexpr std::tuple<std::string_view, std::string_view> LibraryDirectoryAndWildcard_sv[] =
    {
        { "build-tools/Installer Inputs/Webview2",        "MicrosoftEdgeWebview2Setup.exe" },
        { "cspro/external",                               "*.dll;*.lib" },
        { "cspro/CSEntryDroid/app/libs",                  "*.jar" },
        { "cspro/CSEntryDroid/app/src/main/jni/external", "*.a" }
    };

    // when possible, we will reuse previously examined data
    const std::vector<BuiltLibrary> previously_built_libraries = std::move(m_builtLibraries);
    ASSERT(m_builtLibraries.empty());

    // to determine the prebuilt libraries needed at this point,
    // we will look at the directories where external libraries are located,
    // and then use the version in the repository (when available), or the version on the disk (when not)
    const GitIndex cs_index = cs_commit.GetTree().GetIndex();

    DirectoryLister directory_lister(true);
    ASSERT(m_privateRepoDirectory.back() == Path::NativeSlashChar);

    for( const auto& [directory_sv, wildcard_sv] : LibraryDirectoryAndWildcard_sv )
    {
        const std::string full_directory = Path::Combine(m_privateRepoDirectory, directory_sv);
        directory_lister.SetNameFilter(wildcard_sv);

        for( std::string& file_path : directory_lister.GetPaths(full_directory) )
        {
            std::string repo_path = Path::ToForwardSlash(file_path.substr(m_privateRepoDirectory.length()));
            std::optional<GitObjectId> cs_blob_oid;

            try
            {
                // an exception is thrown if this library in not in repository
                cs_blob_oid = cs_index.GetObjectIdByPath(repo_path);
            }
            catch(...) { }

            // see if we can reuse information about this library
            const auto& lookup = std::find_if(
                previously_built_libraries.cbegin(), previously_built_libraries.cend(),
                [&](const BuiltLibrary& built_library)
                {
                    return ( repo_path == built_library.repo_path &&
                             cs_blob_oid == built_library.cs_blob_oid );
                }
            );

            if( lookup != previously_built_libraries.cend() )
            {
                ASSERT(file_path == lookup->file_path);
                m_builtLibraries.emplace_back(*lookup);
            }

            else
            {
                m_builtLibraries.emplace_back(
                    BuiltLibrary
                    {
                        std::move(file_path),
                        std::move(repo_path),
                        std::move(cs_blob_oid)
                    }
                );
            }
        }
    }
}


std::string Syncer::CalculateBuiltLibrariesCacheKey(const bool local_version)
{
    ASSERT(!m_builtLibraries.empty());

    std::string cache_key_inputs;

    for( BuiltLibrary& built_library : m_builtLibraries )
    {
        cache_key_inputs.append(built_library.repo_path);

        // for files in the repository, the cache key will be the OID hex hash
        // for the local version and the MD5 for the actual version
        if( built_library.cs_blob_oid.has_value() )
        {
            if( local_version )
            {
                cache_key_inputs.append(built_library.cs_blob_oid->GetHexHash());
            }

            else
            {
                if( built_library.md5.empty() )
                {
                    ASSERT(built_library.file_data == nullptr);
                    const GitBlob cs_blob = m_privateRepo.LookupBlob(*built_library.cs_blob_oid);
                    built_library.file_data = std::make_unique<BinaryBlock>(cs_blob.data(), cs_blob.size());
                    built_library.md5 = PortableFunctions::BinaryMd5(*built_library.file_data);
                }

                cache_key_inputs.append(built_library.md5);
            }
        }

        // for files on disk, the cache key will be the file size and modified time
        // for the local version and the MD5 for the actual version
        else
        {
            if( local_version )
            {
                const std::tuple<int64_t, int64_t> file_size_and_modified_time = PortableFunctions::FileSizeAndModifiedTime(built_library.file_path);
                cache_key_inputs.append(IntToString(std::get<0>(file_size_and_modified_time)));
                cache_key_inputs.append(IntToString(std::get<1>(file_size_and_modified_time)));
            }

            else
            {
                if( built_library.md5.empty() )
                {
                    ASSERT(built_library.file_data == nullptr);
                    built_library.file_data = std::make_shared<BinaryBlock>(FileIO::ReadBinary(built_library.file_path));
                    built_library.md5 = PortableFunctions::BinaryMd5(*built_library.file_data);
                }

                cache_key_inputs.append(built_library.md5);
            }
        }
    }

    return Hash::Hash(cache_key_inputs, LibrariesHashLength);
}


void Syncer::RefreshLibraryTags(GitRepository& library_repo)
{
    m_loggingListBox.AddText("Reading tags from the built libraries repository.");

    library_repo.ForeachTag(
        [&](const GitTag tag)
        {
            m_loggingListBox.AddText("Library tag: " + tag.GetDisplayName());

            // look up the commit message and see if it contains the library ID
            const GitCommit commit = library_repo.LookupCommit(tag);
            const std::string& commit_message = commit.GetMessage();

            const size_t id_pos = commit_message.find(LibrariesCommitMessageIdentifier_sv);

            if( id_pos == std::string::npos )
            {
                m_loggingListBox.AddText(u8"⚠ Library ID not found!");
            }

            else
            {
                const std::string library_id(SO::Trim(
                    std::string_view(commit_message).substr(id_pos + LibrariesCommitMessageIdentifier_sv.length())
                ));

                if( library_id.length() != LibrariesHashLength )
                    throw CSProException("The library ID was not valid: %s", library_id.c_str());

                m_loggingListBox.AddText("Library ID updated: " + library_id);

                const std::string cache_key = SO::Concatenate(LibrariesSettingsKeyPrefix_sv, library_id);
                m_settingsDb.Write(cache_key, tag.GetDisplayName());
            }

            return true;
        });
}
