#include "StdAfx.h"
#include "GitBase.h"


Git::Base::Base()
{
    if( git_libgit2_init() < 0 )
        throw CSProException("Could not initialize libgit2.");
}


Git::Base::~Base()
{
    git_libgit2_shutdown();
}


void Git::Base::ThrowGitException()
{
    throw CSProException("Error interacting with libgit2: %s", git_error_last()->message);
}
