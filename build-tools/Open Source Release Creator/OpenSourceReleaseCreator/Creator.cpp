#include "StdAfx.h"
#include "Creator.h"
#include <zUtilO/CSProExecutables.h>
#include <external/libgit2/include/git2.h>


struct Creator::Data
{
    git_repository* repo;
};


Creator::Creator()
    :   m_data(std::make_unique<Data>())
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
