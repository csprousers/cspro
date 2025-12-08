#pragma once

namespace Git { class Base; }


// --------------------------------------------------------------------------
// Git::Base
//
// This class initializes libgit2 in the constructor and shuts it down in the
// destructor.
//
// It also provides a method for throwing libgit2 errors as exceptions.
// --------------------------------------------------------------------------

class Git::Base
{
protected:
    Base();

public:
    ~Base();

protected:
    [[noreturn]] static void ThrowGitException();
};
