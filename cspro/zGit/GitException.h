#pragma once

#include <zToolsO/CSProException.h>
#include <git2/git2.h>


class GitException : public CSProException
{
public:
    template<typename... Args>
    explicit GitException(Args const&... args)
        :   CSProException(args...)
    {
    }


    explicit GitException()
        :   GitException(std::string("Git error: ").append(git_error_last()->message))
    {
    }
};
