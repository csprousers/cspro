#include "StdAfx.h"
#include "GitRepository.h"


GitRepository::GitRepository() noexcept
    :   m_repo(nullptr)
{
}


GitRepository::GitRepository(GitRepository&& rhs) noexcept
    :   m_repoDirectory(std::move(rhs.m_repoDirectory)),
        m_repo(rhs.m_repo)
{
    rhs.m_repo = nullptr;
}


GitRepository::~GitRepository() noexcept
{
    if( m_repo != nullptr )
        git_repository_free(m_repo);
}


void GitRepository::EnsureRepositoryIsOpen() const
{
    if( m_repo == nullptr )
        throw CSProException("No Git repository is open.");
}


void GitRepository::Open(std::string repo_directory, const bool create, const bool bare)
{
    if( m_repo != nullptr )
        throw CSProException("A Git repository is already open: %s", m_repoDirectory.c_str());

    const int result = create ? git_repository_init(&m_repo, repo_directory.c_str(), bare) :
                       bare   ? git_repository_open_bare(&m_repo, repo_directory.c_str()) :
                                git_repository_open(&m_repo, repo_directory.c_str());

    if( result != 0 )
    {
        if( !create && !PortableFunctions::FileIsDirectory(repo_directory) )
            throw FileIO::Exception::DirectoryNotFound(repo_directory);

        ThrowGitException();
    }

    m_repoDirectory = std::move(repo_directory);
}


void GitRepository::Close() noexcept
{
    if( m_repo == nullptr )
        return;

    git_repository_free(m_repo);

    m_repoDirectory.clear();
    m_repo = nullptr;
}


std::string GitRepository::GetWorkingDirectory() const noexcept
{
    if( m_repo != nullptr )
    {
        const char* const directory = git_repository_workdir(m_repo);

        if( directory != nullptr )
        {
            std::string directory_str = directory;
            ASSERT(!directory_str.empty() && Path::IsSlashChar(directory_str.back()));
            return Path::MakeToNativeSlash(directory_str);
        }
    }

    return std::string();
}


GitBranch GitRepository::GetCurrentBranch() const
{
    EnsureRepositoryIsOpen();

    git_reference* branch_ref;

    if( git_repository_head(&branch_ref, m_repo) != 0 )
        ThrowGitException();

    return GitBranch(*branch_ref);
}


GitBranch GitRepository::LookupBranch(const cs::string_sz branch_name) const
{
    EnsureRepositoryIsOpen();

    git_reference* branch_ref;

    if( git_branch_lookup(&branch_ref, m_repo, branch_name.c_str(), GIT_BRANCH_ALL) != 0 )
        throw CSProException("The branch was not found in the repository: %s", branch_name.c_str());

    return GitBranch(*branch_ref);
}


GitBranch GitRepository::CreateBranch(const cs::string_sz branch_name, const GitCommit& commit) const
{
    EnsureRepositoryIsOpen();

    git_reference* branch_ref;

    if( git_branch_create(&branch_ref, m_repo, branch_name.c_str(), commit, 0) != 0 )
        ThrowGitException();

    return GitBranch(*branch_ref);
}


void GitRepository::ForeachLocalBranch(const std::function<bool(GitBranch)>& callback_function) const
{
    EnsureRepositoryIsOpen();

    git_branch_iterator* branch_iterator;

    if( git_branch_iterator_new(&branch_iterator, m_repo, GIT_BRANCH_LOCAL) != 0 )
        ThrowGitException();

    try
    {
        git_reference* branch_ref;
        git_branch_t branch_type;
        int next_result;

        while( ( next_result = git_branch_next(&branch_ref, &branch_type, branch_iterator) ) == 0 &&
               callback_function(GitBranch(*branch_ref)) )
        {
        }

        if( next_result != GIT_ITEROVER )
            ThrowGitException();

        git_branch_iterator_free(branch_iterator);
    }

    catch(...)
    {
        git_branch_iterator_free(branch_iterator);
        throw;
    }
}


void GitRepository::ResetBranchMixed(const GitCommit& commit) const
{
    EnsureRepositoryIsOpen();

    const git_object* const target = reinterpret_cast<const git_object*>(static_cast<const git_commit*>(commit));

    if( git_reset(m_repo, target, GIT_RESET_MIXED, nullptr) != 0 )
        ThrowGitException();
}


GitIndex GitRepository::GetIndex() const
{
    EnsureRepositoryIsOpen();

    git_index* index;

    if( git_repository_index(&index, m_repo) != 0 )
        ThrowGitException();

    return GitIndex(*index);
}


GitIndex GitRepository::GetUpdatedIndex() const
{
    GitIndex index = GetIndex();

    if( git_index_read(index, false) != 0 )
        ThrowGitException();

    return index;
}


GitTree GitRepository::WriteTree(GitIndex& index) const
{
    EnsureRepositoryIsOpen();

    git_oid tree_oid;
    git_tree* tree;

    if( git_index_write_tree(&tree_oid, index) != 0 ||
        git_tree_lookup(&tree, m_repo, &tree_oid) != 0 )
    {
        ThrowGitException();
    }

    return GitTree(*tree);
}


unsigned int GitRepository::GetStatusByPath(const cs::string_sz path) const
{
    EnsureRepositoryIsOpen();

    unsigned int status_flags;

    if( git_status_file(&status_flags, m_repo, path.c_str()) != 0 )
        ThrowGitException();

    return status_flags;
}


void GitRepository::ForeachStatusInIndex(const std::function<void(std::string path, unsigned int status_flags)>& callback_function) const
{
    const GitIndex index = GetIndex();
    const size_t count = index.GetEntryCount();
    unsigned int status_flags;

    for( size_t i = 0; i < count; ++i )
    {
        std::string path = index.GetPathByIndex(i);

        if( git_status_file(&status_flags, m_repo, path.c_str()) != 0 )
            ThrowGitException();

        callback_function(std::move(path), status_flags);
    }
}


void GitRepository::ForeachStatusInWorkingDirectory(const std::function<void(std::string path, unsigned int status_flags)>& callback_function) const
{
    EnsureRepositoryIsOpen();

    git_status_options status_options = GIT_STATUS_OPTIONS_INIT;
    status_options.show = GIT_STATUS_SHOW_WORKDIR_ONLY;
    status_options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;

    git_status_list* status_list;

    if( git_status_list_new(&status_list, m_repo, &status_options) != 0 )
        ThrowGitException();

    const size_t count = git_status_list_entrycount(status_list);

    for( size_t i = 0; i < count; ++i )
    {
        const git_status_entry* const status_entry = git_status_byindex(status_list, i);

        if( status_entry == nullptr )
        {
            git_status_list_free(status_list);
            ThrowGitException();
        }

        const char* const path = ( status_entry->head_to_index != nullptr ) ? status_entry->head_to_index->new_file.path :
                                                                              status_entry->index_to_workdir->new_file.path;
        callback_function(path, status_entry->status);
    }

    git_status_list_free(status_list);
}


auto GitRepository::GetDiffOptions()
{
    git_diff_options diff_opts = GIT_DIFF_OPTIONS_INIT;

    diff_opts.flags |= GIT_DIFF_INCLUDE_UNTRACKED |
                       GIT_DIFF_RECURSE_UNTRACKED_DIRS |
                       GIT_DIFF_SKIP_BINARY_CHECK |
                       GIT_DIFF_FORCE_BINARY;

    return diff_opts;
}


void GitRepository::ForeachDifferenceInWorkingDirectory(const GitCommit& commit, const std::function<bool(std::string path, unsigned int diff_flag)>& callback_function) const
{
    EnsureRepositoryIsOpen();

    GitTree tree = commit.GetTree();

    git_diff_options diff_opts = GetDiffOptions();
    git_diff* diff_tree_to_index;

    if( git_diff_tree_to_index(&diff_tree_to_index, m_repo, tree, nullptr, &diff_opts) != 0 )
        ThrowGitException();

    git_diff* diff_index_to_workdir;

    if( git_diff_index_to_workdir(&diff_index_to_workdir, m_repo, nullptr, &diff_opts) != 0 )
    {
        git_diff_free(diff_tree_to_index);
        ThrowGitException();
    }

    const bool merge_successful = ( git_diff_merge(diff_tree_to_index, diff_index_to_workdir) == 0 );

    if( merge_successful )
    {
        struct CB
        {
            static int func(const git_diff_delta* const delta, float /*progress*/, void* const payload)
            {
                ASSERT(delta != nullptr && delta->new_file.path != nullptr && payload != nullptr);
                const std::function<bool(std::string, unsigned int)>& callback_function = *reinterpret_cast<const std::function<bool(std::string, unsigned int)>*>(payload);
                return !callback_function(delta->new_file.path, delta->status);
            }
        };

        git_diff_foreach(diff_tree_to_index, CB::func, nullptr, nullptr, nullptr,
                         const_cast<std::function<bool(std::string, unsigned int)>*>(&callback_function));
    }

    git_diff_free(diff_index_to_workdir);
    git_diff_free(diff_tree_to_index);

    if( !merge_successful )
        ThrowGitException();
}


size_t GitRepository::GetDifferenceDeltasCount(GitTree& tree1, GitTree& tree2) const
{
    EnsureRepositoryIsOpen();

    git_diff_options diff_opts = GetDiffOptions();
    git_diff* diff;

    if( git_diff_tree_to_tree(&diff, m_repo, tree1, tree2, &diff_opts) )
        ThrowGitException();

    const size_t count = git_diff_num_deltas(diff);

    git_diff_free(diff);

    return count;
}


template<typename GitObjectT>
GitObject GitRepository::LookupObject(const GitObjectId& oid, const GitObjectT type) const
{
    EnsureRepositoryIsOpen();

    git_object* object;

    if( git_object_lookup(&object, m_repo, oid, type) != 0 )
        throw CSProException("The object was not found in the repository: %s", oid.GetHexHash().c_str());

    return GitObject(*object);
}


GitObject GitRepository::LookupObject(const GitObjectId& oid, const GitObjectType type) const
{
    return LookupObject(oid, static_cast<git_object_t>(type));
}


GitObject GitRepository::LookupObject(const GitObjectId& oid) const
{
    return LookupObject(oid, GIT_OBJECT_ANY);
}


GitCommit GitRepository::LookupCommit(const GitObjectId& oid) const
{
    EnsureRepositoryIsOpen();

    git_commit* commit;

    if( git_commit_lookup(&commit, m_repo, oid) == 0 )
        return GitCommit(*commit);

    throw CSProException("The commit was not found in the repository: %s", oid.GetHexHash().c_str());
}


GitCommit GitRepository::LookupCommit(const cs::string_sz hex_hash) const
{
    EnsureRepositoryIsOpen();

    std::optional<GitObjectId> oid;
    git_object* object;

    if( git_revparse_single(&object, m_repo, hex_hash.c_str()) == 0 )
    {
        if( git_object_type(object) == git_object_t::GIT_OBJECT_COMMIT )
            oid.emplace(*object);

        git_object_free(object);
    }

    // if the commit was not found, an exception will be thrown by GitObjectId's constructor
    // if the hex hash is invalid, or will be thrown by LookupCommit
    if( !oid.has_value() )
        oid.emplace(hex_hash);

    return LookupCommit(*oid);
}


GitCommit GitRepository::LookupCommit(const GitBranch& branch) const
{
    return LookupCommit(branch.GetTarget());
}


GitCommit GitRepository::LookupCommit(const GitTag& tag) const
{
    const GitObject object = LookupObject(tag);
    const GitObjectType type = object.GetType();
    git_commit* commit;

    if( type == GitObjectType::Commit )
    {
        if( git_object_peel(reinterpret_cast<git_object**>(&commit), object, GIT_OBJECT_COMMIT) != 0 )
            ThrowGitException();
    }

    else if( type == GitObjectType::Tag )
    {
        if( git_commit_lookup(&commit, m_repo, git_tag_target_id(reinterpret_cast<const git_tag*>(static_cast<const git_object*>(object)))) != 0 )
            ThrowGitException();
    }

    else
    {
        throw ProgrammingErrorException();
    }

    return GitCommit(*commit);
}


bool GitRepository::IsCommitDescendantOf(const GitCommit& commit, const GitCommit& ancestor) const
{
    EnsureRepositoryIsOpen();

    switch( git_graph_descendant_of(m_repo, commit, ancestor) )
    {
        case 0:  return false;
        case 1:  return true;
        default: ThrowGitException();
    }
}


GitObjectId GitRepository::CreateCommit(const GitSignature& author, const GitSignature& committer,
                                        const cs::string_sz message, const GitTree& tree,
                                        const GitCommit& parent_commit1, const GitCommit* const parent_commit2/* = nullptr*/)
{
    EnsureRepositoryIsOpen();

    const git_commit* parent_commits[2] =
    {
        static_cast<const git_commit*>(parent_commit1),
        ( parent_commit2 != nullptr ) ? static_cast<const git_commit*>(parent_commit1) : nullptr
    };

    git_oid commit_oid;

    const int result = git_commit_create(
        &commit_oid,
        m_repo,
        "HEAD",
        author,
        committer,
        nullptr, // UTF-8
        message.c_str(),
        tree,
        ( parent_commit2 != nullptr ) ? 2 : 1,
        parent_commits
    );

    if( result != 0 )
        ThrowGitException();

    return GitObjectId(commit_oid);
}


GitObjectId GitRepository::CreateCommit(const GitSignature& author_and_committer,
                                        const cs::string_sz message, const GitTree& tree,
                                        const GitCommit& parent_commit1, const GitCommit* const parent_commit2/* = nullptr*/)
{
    return CreateCommit(author_and_committer, author_and_committer, message, tree, parent_commit1, parent_commit2);
}


void GitRepository::ForeachTag(const std::function<bool(GitTag)>& callback_function) const
{
    EnsureRepositoryIsOpen();

    struct CB
    {
        static int func(const char* const name, git_oid* const oid, void* const payload)
        {
            ASSERT(name != nullptr && oid != nullptr && payload != nullptr);
            const std::function<bool(GitTag)>& callback_function = *reinterpret_cast<const std::function<bool(GitTag)>*>(payload);
            return !callback_function(GitTag(*oid, name));
        }
    };

    git_tag_foreach(m_repo, CB::func, const_cast<std::function<bool(GitTag)>*>(&callback_function));
}


std::vector<GitTag> GitRepository::GetTags() const
{
    std::vector<GitTag> tags;

    ForeachTag([&tags](GitTag tag) { tags.emplace_back(std::move(tag)); return true; });

    return tags;
}


void GitRepository::AddIgnoreRule(const cs::string_sz rules)
{
    EnsureRepositoryIsOpen();

    if( git_ignore_add_rule(m_repo, rules.c_str()) != 0 )
        ThrowGitException();
}


void GitRepository::AddIgnoreRulesFromFile(const std::string& file_path)
{
    AddIgnoreRule(FileIO::ReadText(file_path));
}


void GitRepository::ClearIgnoreRules()
{
    EnsureRepositoryIsOpen();

    if( git_ignore_clear_internal_rules(m_repo) != 0 )
        ThrowGitException();
}


bool GitRepository::IsPathIgnoredWorker(const cs::string_sz path) const
{
    EnsureRepositoryIsOpen();

    int ignored;

    if( git_ignore_path_is_ignored(&ignored, m_repo, path.c_str()) < 0 )
        ThrowGitException();

    return ( ignored == 1 );
}


bool GitRepository::IsPathIgnored(const std::string& path) const
{
    return ( path.find('\\') != std::string::npos ) ? IsPathIgnoredWorker(Path::ToForwardSlash(path)) :
                                                      IsPathIgnoredWorker(path);
}


bool GitRepository::IsPathIgnored(std::string&& path) const
{
    Path::MakeToForwardSlash(path);
    return IsPathIgnoredWorker(path.c_str());
}
