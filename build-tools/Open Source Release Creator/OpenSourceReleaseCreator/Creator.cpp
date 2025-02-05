#include "StdAfx.h"
#include "Creator.h"
#include <zUtilO/CSProExecutables.h>
#include <external/libgit2/include/git2.h>


struct Creator::Data
{
    git_repository* repo;
};


Creator::Creator(LoggingListBox& logging_list_box)
    :   m_loggingListBox(logging_list_box),
        m_data(std::make_unique<Data>())
{
    if( git_libgit2_init() < 0 )
        throw CSProException("Could not initialize libgit2.");

    const std::string git_directory = MakeFullPath(CSProExecutables::GetApplicationDirectory(), "..\\..\\..\\.git");

    if( git_repository_open_bare(&m_data->repo, git_directory.c_str()) < 0 )
        ThrowGitException();
}


Creator::~Creator()
{
    git_libgit2_shutdown();
}


void Creator::ThrowGitException()
{
    throw CSProException("Error interacting with libgit2: %s", git_error_last()->message);
}


template<typename git_oidT>
std::string Creator::ObjectIdToString(const git_oidT* const oid)
{
    constexpr size_t SHA256HexSize = 64;
    char buffer[SHA256HexSize + 1];

    return std::string(git_oid_tostr(buffer, _countof(buffer), oid),
                       SHA256HexSize);
}


std::vector<Git::Tag> Creator::GetTags() const
{
    std::vector<Git::Tag> tags;

    struct CB
    {
        static int func(const char* const name, git_oid* const oid, void* const payload)
        {
            auto tags = reinterpret_cast<std::vector<Git::Tag>*>(payload);
            tags->emplace_back(Git::Tag { ObjectIdToString(oid), name });
            return 0;
        };
    };

    git_tag_foreach(m_data->repo, CB::func, &tags);

    return tags;
}


void Creator::CreateRelease(const cs::string_sz commit_string, const std::string& output_directory)
{
    m_loggingListBox.ResetContent();
    m_loggingListBox.AddText(FormatText("Creating open source release from commit: %s", commit_string.c_str()));

    // ensure that the commit is valid
    git_oid oid;
    git_commit* commit;

    if( git_oid_fromstr(&oid, commit_string.c_str()) < 0 ||
        git_commit_lookup(&commit, m_data->repo, &oid) < 0 )
    {
        throw CSProException("The commit was not found in the repo: %s", commit_string.c_str());
    }

    const git_signature* const author = git_commit_author(commit);
    m_loggingListBox.AddText(std::string("    Author: ").append(author->name));
    m_loggingListBox.AddText(std::string("    Date: ").append(DateTime::LocalDateTimeString(author->when.time)));
    m_loggingListBox.AddText(std::string("    Message: ").append(git_commit_message(commit)));

    git_commit_free(commit);
}
