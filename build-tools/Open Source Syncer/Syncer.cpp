#include "StdAfx.h"
#include "Syncer.h"
#include "FileReplacer.h"
#include "LibraryManager.h"
#include "ReleaseTag.h"
#include <zGit/GitMerge.h>


namespace
{
    constexpr const char* BuildFilename   = "BUILD.md";
    constexpr const char* HistoryFilename = "HISTORY.md";

    constexpr std::string_view LibrariesTagInBuildFile_sv = "%LIBRARY_TAG%";

    // the commit is the first commit after the v7.5.0 tag
    constexpr std::string_view HistoryLogEarliestVersion_sv = "7.6.0";
    constexpr const char* HistoryLogEarliestCommitSHA       = "95d126ccaab8872d1d914f64d16824a5f2f19ccd";
}


Syncer::Syncer(Controller& controller) noexcept
    :   m_controller(controller)
{
}


Syncer::~Syncer()
{
}


bool Syncer::IsFileExcluded(const std::string& cs_file_path)
{
    if( !m_exclusionEvaluator.has_value() )
    {
        const std::string exclusions_file_path = m_controller.GetExclusionsFilePath();

        m_controller.LogText("Reading excluded files based on gitignore rules from: " + exclusions_file_path);

        m_exclusionEvaluator.emplace();
        m_exclusionEvaluator->AddRulesFromFile(exclusions_file_path);
    }

    return m_exclusionEvaluator->Ignore(cs_file_path);
}


struct Syncer::PullRequest
{
    std::string sha;
    int64_t commit_time;
    std::string branch_name;
    std::string message;
};


struct Syncer::GroupedPullRequests
{
    GitCommit cs_grouped_up_to_commit;
    std::map<std::string, std::vector<PullRequest>> pull_requests; // tag name -> PullRequest
    std::vector<PullRequest> newer_than_tags_pull_requests;
};


const char* Syncer::GetHistoryFilename() noexcept
{
    return HistoryFilename;
}


std::string Syncer::CreateHistoryLog(const GitCommit& cs_latest_commit)
{
    GitRepository& private_repo = m_controller.GetPrivateRepo();

    m_controller.LogText("Creating history log up to the commit on: " +
                         cs_latest_commit.GetCommitter().GetWhen().GetLocalDateTimeString());

    if( m_releaseTags.empty() )
    {
        m_releaseTags = ReleaseTag::Populate(private_repo, HistoryLogEarliestVersion_sv, true);
        ASSERT(!m_releaseTags.empty());
    }

    std::unique_ptr<GroupedPullRequests> grouped_pull_requests;

    GitCommit oldest_commit_to_process = private_repo.LookupCommit(HistoryLogEarliestCommitSHA);

    // if the history has already been created for an earlier commit, we only need to walk up to that commit
    if( m_lastHistoryLogCreationGroupedPullRequests != nullptr &&
        private_repo.IsCommitDescendantOf(cs_latest_commit, m_lastHistoryLogCreationGroupedPullRequests->cs_grouped_up_to_commit) )
    {
        oldest_commit_to_process = m_lastHistoryLogCreationGroupedPullRequests->cs_grouped_up_to_commit;
        grouped_pull_requests = std::move(m_lastHistoryLogCreationGroupedPullRequests);
        grouped_pull_requests->cs_grouped_up_to_commit = cs_latest_commit;
    }

    else
    {
        grouped_pull_requests.reset(new GroupedPullRequests { cs_latest_commit });
    }

    const std::regex commit_message_regex(R"(^Merge pull request.+CSProDevelopment\/(\S+).*)");
    std::smatch matches;

    GitRevisionWalker walker(private_repo);

    walker.ReverseWalk(cs_latest_commit, oldest_commit_to_process,
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
            std::vector<PullRequest>* pull_requests = &grouped_pull_requests->newer_than_tags_pull_requests;

            for( const ReleaseTag& release_tag : m_releaseTags )
            {
                if( release_tag.commit == commit ||
                    private_repo.IsCommitDescendantOf(release_tag.commit, commit) )
                {
                    pull_requests = &grouped_pull_requests->pull_requests[release_tag.version];
                    break;
                }
            }

            std::string pull_request_branch_name = matches.str(1);
            std::string pull_request_message = matches.suffix().str();
            SO::MakeTrim(pull_request_message);

            pull_requests->emplace_back(
                PullRequest
                {
                    commit.GetObjectId().GetHexHash(),
                    commit.GetAuthor().GetWhen().GetTimestamp(),
                    std::move(pull_request_branch_name),
                    std::move(pull_request_message)
                });
        });

    std::string history =
        "## Overview\n\n"
        "Most CSPro development occurs on a [private repository](https://github.com/CSProDevelopment/cspro), "
        "though the commits are mirrored to this [public repository](https://github.com/csprousers/cspro). "
        "This document lists information about each pull request merged into the private repository, starting "
        "with pull requests for CSPro 7.6.\n"
        ;

    auto write_pull_requests = [&](const std::vector<PullRequest>& pull_requests)
    {
        history.append(
            "\n**Merged pull requests**:\n\n"
            "| Date | Branch | Pull Request Message |\n"
            "| --- | --- | --- |\n"
        );

        constexpr const char* NonBreakingHyphen = "&#8209;";
        static const std::string date_formatter = FormatText("%%Y%s%%m%s%%d", NonBreakingHyphen, NonBreakingHyphen);

        for( auto pull_request_itr = pull_requests.crbegin(); pull_request_itr != pull_requests.crend(); ++pull_request_itr )
        {
            auto escape_for_table = [&](std::string text)
            {
                return SO::Replace(text, "|", "&#124;");
            };

            history.append(FormatText(
                "| %s | [%s](https://github.com/CSProDevelopment/cspro/commit/%s) | %s |\n",
                DateTime::LocalDateTimeString(pull_request_itr->commit_time, date_formatter).c_str(),
                escape_for_table(pull_request_itr->branch_name).c_str(),
                pull_request_itr->sha.c_str(),
                escape_for_table(pull_request_itr->message).c_str()
            ));
        }
    };

    if( !grouped_pull_requests->newer_than_tags_pull_requests.empty() )
    {
        history.append("\n\n## CSPro (current development)\n");

        write_pull_requests(grouped_pull_requests->newer_than_tags_pull_requests);
    }

    for( auto tag_commits_itr = m_releaseTags.crbegin(); tag_commits_itr != m_releaseTags.crend(); ++tag_commits_itr )
    {
        history.append(FormatText("\n\n## CSPro %s\n", tag_commits_itr->version.c_str()));

        std::string url = FormatText("https://csprousers.org/downloads/cspro/cspro%s.exe", tag_commits_itr->version.c_str());
        history.append(FormatText("\n**Installer**: [%s](%s)\n", url.c_str(), url.c_str())); // X64_TODO add link to 64-bit installer

        url = FormatText("https://csprousers.org/downloads/cspro/cspro%s-release-notes.txt", tag_commits_itr->version.c_str());
        history.append(FormatText("\n**Release notes**: [%s](%s)\n", url.c_str(), url.c_str()));

        const std::vector<PullRequest>& pull_requests = grouped_pull_requests->pull_requests[tag_commits_itr->version];

        if( !pull_requests.empty() )
            write_pull_requests(pull_requests);
    }

    m_lastHistoryLogCreationGroupedPullRequests = std::move(grouped_pull_requests);

    return history;
}


void Syncer::UpdateBuildDetails(GitIndex& os_index, GitTree& cs_tree, const std::string& libraries_tag)
{
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();

    const GitBlob cs_build_blob = cs_tree.GetEntryByPath(BuildFilename).GetObject().GetBlob();
    std::string build_details = cs_build_blob.as<std::string>();

    SO::Replace(build_details, LibrariesTagInBuildFile_sv, libraries_tag);

    const GitObjectId os_new_build_blob_oid = open_source_repo.CreateBlob(build_details);
    os_index.AddEntry(os_new_build_blob_oid, BuildFilename, GIT_FILEMODE_BLOB);
}


bool Syncer::CompareRepositories(const GitCommit& cs_commit, const GitCommit& os_commit, const bool verbose)
{
    GitRepository& private_repo = m_controller.GetPrivateRepo();
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();
    FileReplacer& file_replacer = m_controller.GetFileReplacer();

    m_controller.LogText("Comparing: private (%s) <-> open source (%s)",
                         cs_commit.GetObjectId().GetHexHash().c_str(),
                         os_commit.GetObjectId().GetHexHash().c_str());

    const GitIndex cs_index = cs_commit.GetTree().GetIndex();
    const GitIndex os_index = os_commit.GetTree().GetIndex();

    if( verbose )
    {
        m_controller.LogText("Files: private (%zu) <-> open source (%zu):",
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
                m_controller.LogText("Missing file (expected exclusion): " + cs_path);
            }

            continue;
        }

        // compare the contents of the files
        if( cs_file_oid == os_lookup->second )
        {
            if( verbose )
                m_controller.LogText("Same file (by OID): " + cs_path);
        }

        // when not identical, see if this is a replacement file
        else if( file_replacer.HasReplacement(cs_path) )
        {
            if( verbose )
                m_controller.LogText("Different file (expected replacement): " + cs_path);
        }

        else if( cs_path == BuildFilename )
        {
            if( verbose )
                m_controller.LogText("Different file (expected updated): " + cs_path);
        }

        // otherwise compare as text with normalized line endings
        else
        {
            const std::string cs_text = SO::ToNewlineLF(private_repo.LookupBlob(cs_file_oid).as<std::string>());
            const std::string os_text = open_source_repo.LookupBlob(os_lookup->second).as<std::string>();

            if( cs_text != os_text )
            {
                fatal_errors.emplace_back(u8"⚠ Different file: " + cs_path);
            }

            else if( verbose )
            {
                m_controller.LogText("Same file (by normalized text comparison): " + cs_path);
            }
        }

        os_files.erase(os_lookup);
    }

    // report on any unexpected files in the open source repository,
    // first removing any files only in the open source directory
    for( const char* const os_path : { HistoryFilename } )
    {
        const auto& os_lookup = os_files.find(os_path);

        if( os_lookup != os_files.cend() )
        {
            if( verbose )
                m_controller.LogText("Missing file (expected addition): " + os_lookup->first);

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
        m_controller.LogText("\nThere are no unexpected differences between the repositories.");
        return true;
    }

    // report on the fatal errors
    m_controller.LogText(u8"\n⚠ There are %zu fatal errors!\n", fatal_errors.size());

    for( const std::string& fatal_error : fatal_errors )
        m_controller.LogText(fatal_error);

    return false;
}


std::tuple<GitCommit, GitCommit> Syncer::FindUnsyncedMergeCommits(const GitBranch& os_merge_branch)
{
    GitRepository& private_repo = m_controller.GetPrivateRepo();
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();

    const GitBranch cs_branch = private_repo.LookupBranch(os_merge_branch.GetName());
    const GitCommit cs_current_commit = private_repo.LookupCommit(cs_branch.GetTarget());

    const GitCommit os_last_merge_commit = open_source_repo.LookupCommit(os_merge_branch);

    // find the newest merge commit in the private repository
    std::optional<GitCommit> cs_newest_merge_commit;

    GitRevisionWalker walker(private_repo);

    walker.Walk(cs_current_commit,
        [&](GitCommit cs_commit)
        {
            if( cs_commit.GetParentCount() == 2 )
            {
                cs_newest_merge_commit = std::move(cs_commit);
                return false;
            }

            return true;
        });

    if( !cs_newest_merge_commit.has_value() )
        throw ProgrammingErrorException();

    if( cs_newest_merge_commit->Equals(os_last_merge_commit) )
        throw CSProException("There are no unsynced merge commits.");

    // find the merge commit in the private repository that matches the last synced merge commit
    std::optional<GitCommit> cs_oldest_merge_commit;

    walker.Walk(*cs_newest_merge_commit,
        [&](GitCommit cs_commit)
        {
            if( cs_commit.Equals(os_last_merge_commit) )
            {
                cs_oldest_merge_commit = std::move(cs_commit);
                return false;
            }

            return true;
        });

    if( !cs_oldest_merge_commit.has_value() )
    {
        throw CSProException("Could not find a merge commit in the private repository matching: " +
                             os_last_merge_commit.GetMessage());
    }

    return std::make_tuple(std::move(*cs_oldest_merge_commit), std::move(*cs_newest_merge_commit));
}


std::vector<GitCommit> Syncer::GetOrderedMergeCommits(const GitCommit& oldest_merge_commit,
                                                      const GitCommit& newest_merge_commit)
{
    std::vector<GitCommit> merge_commits { newest_merge_commit };

    while( true )
    {
        const GitCommit& current_merge_commit = merge_commits.front();

        if( current_merge_commit == oldest_merge_commit )
            break;

        if( current_merge_commit.GetParentCount() != 2 )
        {
            throw CSProException("Update this tool to support merge commits with more than two parents for commit: " +
                                 current_merge_commit.GetObjectId().GetHexHash());
        }

        merge_commits.insert(merge_commits.begin(), current_merge_commit.GetParent(0));
    };

    return merge_commits;
}


std::vector<GitCommit> Syncer::GetOrderedCommits(const GitCommit& oldest_commit, const GitCommit& newest_commit)
{
    std::vector<GitCommit> commits { newest_commit };

    while( true )
    {
        const GitCommit& current_commit = commits.front();

        if( current_commit == oldest_commit )
            break;

        if( current_commit.GetParentCount() != 1 )
        {
            throw CSProException("Update this tool to support mirroring commits with more than one parent for commit: " +
                                 current_commit.GetObjectId().GetHexHash());
        }

        commits.insert(commits.begin(), current_commit.GetParent(0));
    };

    return commits;
}


void Syncer::MirrorFeatureBranches(const GitBranch& os_merge_branch,
                                   const GitCommit& cs_oldest_merge_commit, const GitCommit& cs_newest_merge_commit)
{
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();

    // make sure that there are no pending open source changes
    if( open_source_repo.HasChanges() )
        throw CSProException("You cannot run the sync if there are changes in the open source directory.");

    // multiple feature branches may be mirrored
    if( cs_newest_merge_commit == cs_oldest_merge_commit )
        throw CSProException("The oldest and newest merge commits are identical.");

    const std::vector<GitCommit> cs_merge_commits = GetOrderedMergeCommits(cs_oldest_merge_commit, cs_newest_merge_commit);
    ASSERT(cs_merge_commits.front() == cs_oldest_merge_commit && cs_merge_commits.back() == cs_newest_merge_commit);

    auto cs_merge_commit_old_itr = cs_merge_commits.cbegin();
    auto cs_merge_commit_new_itr = cs_merge_commit_old_itr + 1;

    do
    {
        MirrorFeatureBranch(
            os_merge_branch,
            *cs_merge_commit_old_itr,
            *cs_merge_commit_new_itr
        );

        cs_merge_commit_old_itr = cs_merge_commit_new_itr++;

    } while( cs_merge_commit_new_itr != cs_merge_commits.cend() );

    // when complete, checkout the HEAD so that the working directory matches the index
    open_source_repo.CheckoutHead(GIT_CHECKOUT_FORCE);
}


GitCommit Syncer::CreateMirroredCommit(const GitCommit& cs_commit, const GitTree& os_written_tree,
                                       const GitCommit& os_parent_commit1, const GitCommit* const os_parent_commit2)
{
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();

    const GitObjectId os_commit_oid = open_source_repo.CreateCommit(
        cs_commit.GetAuthor(),
        cs_commit.GetCommitter(),
        cs_commit.GetMessage(),
        os_written_tree,
        os_parent_commit1,
        os_parent_commit2
    );

    m_controller.LogText("\nMirrored commit %s: %s\n\n",
                         os_commit_oid.GetHexHash().c_str(),
                         cs_commit.GetMessage().c_str());

    return open_source_repo.LookupCommit(os_commit_oid);
}


GitCommit Syncer::MirrorFeatureBranch(const GitBranch& os_merge_branch,
                                      const GitCommit& cs_old_merge_commit, const GitCommit& cs_new_merge_commit)
{
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();

    if( cs_old_merge_commit.GetParentCount() != 2 || cs_new_merge_commit.GetParentCount() != 2 )
    {
        throw CSProException("Update this tool to support mirroring between non-merge commits: %s -> %s",
                             cs_old_merge_commit.GetObjectId().GetHexHash().c_str(),
                             cs_new_merge_commit.GetObjectId().GetHexHash().c_str());
    }

    // make sure that the libraries for this feature branch have been created
    LibraryManager& library_manager = m_controller.GetLibraryManager();
    const std::string libraries_tag = library_manager.GetTagForBuiltLibraries(cs_new_merge_commit);

    // create and checkout a temporary open source branch for this work
    const GitCommit os_start_commit = open_source_repo.LookupCommit(os_merge_branch);

    const std::string os_temp_branch_name = SO::Concatenate(
        IntToString(GetTimestamp()),
        "-",
        os_start_commit.GetObjectId().GetHexHash()
    );

    GitBranch os_temp_branch = open_source_repo.CreateBranch(os_temp_branch_name, os_start_commit);
    open_source_repo.CheckoutBranch(os_temp_branch);

    // mirror the feature branch
    const GitCommit os_feature_branch_final_commit = MirrorFeatureBranchCommits(
        cs_old_merge_commit,
        cs_new_merge_commit,
        os_start_commit
    );

    // switch back to the destination branch
    open_source_repo.SetHead(os_merge_branch);
    open_source_repo.ResetHead(GitRepository::ResetType::Hard, os_start_commit);

    // mirror the merge commit
    GitIndex os_index = open_source_repo.GetIndex();
    GitTree cs_old_merge_tree = cs_old_merge_commit.GetTree();
    GitTree cs_new_merge_tree = cs_new_merge_commit.GetTree();

    MirrorCommit(os_index, cs_old_merge_tree, cs_new_merge_tree);

    GitTree os_new_tree = open_source_repo.WriteTree(os_index);

    // make sure that the feature branch matches the merge commit
    GitTree os_feature_branch_tree = os_feature_branch_final_commit.GetTree();

    const GitDiff merge_diff = open_source_repo.GetDifference(os_feature_branch_tree, os_new_tree);

    if( merge_diff.GetNumberDeltas() != 0 )
    {
        merge_diff.ForeachDifference(
            [&](const std::string path, unsigned int /*diff_flag*/)
            {
                m_controller.LogText(u8"⚠ Differences exist in: " + path);
                return true;
            });

        throw CSProException("Differences exist between the feature branch and merge commit: %zu",
                             merge_diff.GetNumberDeltas());
    }

    // update HISTORY.md
    const std::string history = CreateHistoryLog(cs_new_merge_commit);
    const GitObjectId os_history_blob_oid = open_source_repo.CreateBlob(history);
    os_index.AddEntry(os_history_blob_oid, HistoryFilename, GIT_FILEMODE_BLOB);

    // update BUILD.md with information about the external libraries used
    UpdateBuildDetails(os_index, cs_new_merge_tree, libraries_tag);

    // commit this merge commit with the updated history and build details
    os_new_tree = open_source_repo.WriteTree(os_index);

    GitCommit os_new_merge_commit = CreateMirroredCommit(
        cs_new_merge_commit,
        os_new_tree,
        os_start_commit,
        &os_feature_branch_final_commit
    );

    // delete the temporary feature branch
    os_temp_branch.Delete();

    // make sure that the repositories match
    if( !CompareRepositories(cs_new_merge_commit, os_new_merge_commit, false) )
        throw CSProException("The repositories do not match following the creation of the merge commit.");

    return os_new_merge_commit;
}


GitCommit Syncer::MirrorFeatureBranchCommits(const GitCommit& cs_old_merge_commit, const GitCommit& cs_new_merge_commit,
                                             const GitCommit& os_start_commit)
{
    GitRepository& private_repo = m_controller.GetPrivateRepo();
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();

    GitCommit cs_parent_commit = cs_old_merge_commit;
    GitTree cs_parent_tree = cs_old_merge_commit.GetTree();

    GitCommit os_parent_commit = os_start_commit;

    // walk the two merge commits in reverse order
    GitRevisionWalker walker(private_repo);

    walker.ReverseWalk(cs_new_merge_commit, cs_old_merge_commit,
        [&](GitCommit cs_commit)
        {
            // don't process the merge commit during this walk
            if( cs_commit == cs_new_merge_commit )
                return;

            m_controller.LogText("Mirroring %s: %s\n",
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
            GitIndex os_index = open_source_repo.GetIndex();

            MirrorCommit(os_index, cs_parent_tree, cs_commit_tree);

            // commit these changes
            GitTree os_new_tree = open_source_repo.WriteTree(os_index);

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
    GitRepository& private_repo = m_controller.GetPrivateRepo();

    // get the differences between this commit and its parent
    GitDiff cs_diff = private_repo.GetDifference(cs_parent_tree, cs_tree, GIT_DIFF_NORMAL);
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
    FileReplacer& file_replacer = m_controller.GetFileReplacer();

    const bool is_binary = ( ( diff_delta.flags & GIT_DIFF_FLAG_BINARY ) != 0 );
    const char* const file_type = is_binary ? "binary file" : "text file";

    // do not mirror files if they are in the exclusions list
    const std::string path = diff_delta.new_file.path;

    if( IsFileExcluded(path) )
    {
        m_controller.LogText("Skipping excluded %s: %s", file_type, path.c_str());
        return;
    }

    // if the file has a replacement, use it instead
    const std::string replacement_text = file_replacer.GetReplacement(diff_delta.new_file);

    if( !replacement_text.empty() )
    {
        ErrorMessage::PostMessageForDisplay("Verify that the override is valid: " + path);

        m_controller.LogText("Using override for %s: %s", file_type, path.c_str());

        if( is_binary || diff_delta.status != GIT_DELTA_MODIFIED )
            throw CSProException("Replacement files should be text with the status 'modified': " + path);

        MirrorFileAddEntry(os_index, diff_delta.new_file, replacement_text.data(), replacement_text.size());

        return;
    }

    // otherwise mirror the file
    switch( diff_delta.status )
    {
        case GIT_DELTA_ADDED:
            m_controller.LogText("Adding %s: %s", file_type, path.c_str());
            is_binary ? MirrorFileAddBinary(os_index, diff_delta.new_file) :
                        MirrorFileAddText(os_index, diff_delta.new_file);
            break;

        case GIT_DELTA_DELETED:
            m_controller.LogText("Deleting %s: %s", file_type, path.c_str());
            MirrorFileDelete(os_index, path);
            break;

        case GIT_DELTA_MODIFIED:
            m_controller.LogText("Modifying %s: %s", file_type, path.c_str());
            is_binary ? MirrorFileModifyBinary(os_index, diff_delta.new_file) :
                        MirrorFileModifyText(os_index, diff_delta.old_file, diff_delta.new_file);
            break;

        case GIT_DELTA_RENAMED:
            m_controller.LogText("Renaming %s: %s -> %s", file_type, diff_delta.old_file.path, path.c_str());
            is_binary ? MirrorFileRenameBinary(os_index, diff_delta.old_file, diff_delta.new_file) :
                        MirrorFileRenameText(os_index, diff_delta.old_file, diff_delta.new_file);
            break;

        default:
            throw CSProException("Unknown diff status: '%s' -> %d", path.c_str(), static_cast<int>(diff_delta.status));
    }
}


void Syncer::MirrorFileAddEntry(GitIndex& os_index, const git_diff_file& new_file, const void* const data, const size_t size)
{
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();
    const GitObjectId os_blob_oid = open_source_repo.CreateBlob(data, size);
    os_index.AddEntry(os_blob_oid, new_file.path, new_file.mode);
}


void Syncer::MirrorFileAddBinary(GitIndex& os_index, const git_diff_file& new_file)
{
    GitRepository& private_repo = m_controller.GetPrivateRepo();
    const GitBlob cs_blob = private_repo.LookupBlob(new_file.id);
    MirrorFileAddEntry(os_index, new_file, cs_blob.data(), cs_blob.size());
}


void Syncer::MirrorFileAddText(GitIndex& os_index, const git_diff_file& new_file)
{
    GitRepository& private_repo = m_controller.GetPrivateRepo();
    const GitBlob cs_blob = private_repo.LookupBlob(new_file.id);

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
    GitRepository& private_repo = m_controller.GetPrivateRepo();
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();

    // for a three-way merge, we will load the changed data from the private repository...
    std::string cs_text_before = private_repo.LookupBlob(old_file.id).as<std::string>();
    std::string cs_text_after = private_repo.LookupBlob(new_file.id).as<std::string>();

    // ...and then apply it onto the open source repository
    const GitObjectId os_text_now_oid = os_index.GetObjectIdByPath(old_file.path);
    const std::string os_text_now = open_source_repo.LookupBlob(os_text_now_oid).as<std::string>();

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


void Syncer::MirrorCommits(const GitBranch& os_branch,
                           const GitCommit& cs_oldest_commit, const GitCommit* const cs_newest_commit)
{
    GitRepository& private_repo = m_controller.GetPrivateRepo();
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();

    if( cs_newest_commit != nullptr &&
        !private_repo.IsCommitDescendantOf(*cs_newest_commit, cs_oldest_commit) )
    {
        throw CSProException("The newest commit is not a descendant of the oldest commit: (old) %s -> (new) %s",
                             cs_oldest_commit.GetObjectId().GetHexHash().c_str(),
                             cs_newest_commit->GetObjectId().GetHexHash().c_str());
    }

    const std::vector<GitCommit> cs_commits =
        ( cs_newest_commit != nullptr ) ? GetOrderedCommits(cs_oldest_commit, *cs_newest_commit) :
                                          std::vector<GitCommit>({ cs_oldest_commit });

    GitCommit os_parent_commit = open_source_repo.LookupCommit(os_branch);

    open_source_repo.CheckoutBranch(os_branch);
    GitIndex os_index = open_source_repo.GetIndex();

    auto cs_commits_itr = cs_commits.cbegin();

    if( cs_commits_itr->GetParentCount() > 1 )
    {
        throw CSProException("Update this tool to support mirroring a commit with multiple parents: " +
                             cs_commits_itr->GetObjectId().GetHexHash());
    }

    GitTree cs_parent_tree = cs_commits_itr->GetParent(0).GetTree();

    while( true )
    {
        GitTree cs_commit_tree = cs_commits_itr->GetTree();

        MirrorCommit(os_index, cs_parent_tree, cs_commit_tree);

        GitTree os_new_tree = open_source_repo.WriteTree(os_index);

        os_parent_commit = CreateMirroredCommit(
            *cs_commits_itr,
            os_new_tree,
            os_parent_commit,
            nullptr
        );

        if( ++cs_commits_itr == cs_commits.cend() )
            break;

        cs_parent_tree = std::move(cs_commit_tree);
    }

    // when complete, checkout the HEAD so that the working directory matches the index
    open_source_repo.CheckoutHead(GIT_CHECKOUT_FORCE);
}


void Syncer::MirrorMergeCommit(const GitBranch& os_branch, const GitCommit& cs_merge_commit,
                               const GitCommit& os_parent_commit1, const GitCommit& os_parent_commit2)
{
    GitRepository& open_source_repo = m_controller.GetOpenSourceRepo();

    // make sure that the libraries used at this merge commit have been created
    LibraryManager& library_manager = m_controller.GetLibraryManager();
    const std::string libraries_tag = library_manager.GetTagForBuiltLibraries(cs_merge_commit);

    // mirror the merge commit
    open_source_repo.CheckoutBranch(os_branch);
    GitIndex os_index = open_source_repo.GetIndex();

    GitTree cs_parent_tree = cs_merge_commit.GetParent(0).GetTree();
    GitTree cs_commit_tree = cs_merge_commit.GetTree();

    MirrorCommit(os_index, cs_parent_tree, cs_commit_tree);

    // update HISTORY.md
    const std::string history = CreateHistoryLog(cs_merge_commit);
    const GitObjectId os_history_blob_oid = open_source_repo.CreateBlob(history);
    os_index.AddEntry(os_history_blob_oid, HistoryFilename, GIT_FILEMODE_BLOB);

    // update BUILD.md with information about the external libraries used
    UpdateBuildDetails(os_index, cs_commit_tree, libraries_tag);

    // commit this merge commit with the updated history and build details
    GitTree os_new_tree = open_source_repo.WriteTree(os_index);

    GitCommit os_new_merge_commit = CreateMirroredCommit(
        cs_merge_commit,
        os_new_tree,
        os_parent_commit1,
        &os_parent_commit2
    );

    // make sure that the repositories match
    if( !CompareRepositories(cs_merge_commit, os_new_merge_commit, false) )
        throw CSProException("The repositories do not match following the creation of the merge commit.");

    // when complete, checkout the HEAD so that the working directory matches the index
    open_source_repo.CheckoutHead(GIT_CHECKOUT_FORCE);
}
