#pragma once

#include <zGit/zGit.h>


// --------------------------------------------------------------------------
// GitInitializer
//
// This class initializes libgit2 in the constructor and shuts it down in the
// destructor.
// --------------------------------------------------------------------------

class ZGIT_API GitInitializer
{
public:
    GitInitializer();
    ~GitInitializer();
};
