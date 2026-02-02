#pragma once

#include <zGit/zGit.h>


// --------------------------------------------------------------------------
// GitMerge
//
// Wraps Git merging functionality.
// --------------------------------------------------------------------------

class ZGIT_API GitMerge
{
public:
    struct Result
    {
        std::string text;
        bool automergeable; // if false, "the output contains conflict markers"
    };

    // Merges two sets of text, "using the given common ancestor as the baseline."
    // The differences between "ancestor" and "ours" are applied on "theirs."
    static Result Merge(std::string_view ancestor_sv, std::string_view ours_sv, std::string_view theirs_sv);
};
