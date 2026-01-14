#pragma once

#include <external/libgit2/include/git2.h>


[[noreturn]] inline void ThrowGitException()
{
    throw CSProException("Error interacting with libgit2: %s", git_error_last()->message);
}
