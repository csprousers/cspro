#include "StdAfx.h"
#include "GitInitializer.h"


GitInitializer::GitInitializer()
{
    if( git_libgit2_init() < 0 )
        throw CSProException("Could not initialize libgit2.");
}


GitInitializer::~GitInitializer()
{
    git_libgit2_shutdown();
}
