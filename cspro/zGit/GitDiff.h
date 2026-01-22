#pragma once

#include <zGit/zGit.h>

struct git_diff;


// --------------------------------------------------------------------------
// GitDiff
//
// Wraps git_diff: the "object that contains all individual file deltas."
// --------------------------------------------------------------------------

class ZGIT_API GitDiff
{
public:
    // GitDiff assumes ownership of the git_diff object.
    GitDiff(git_diff& diff) noexcept;
    GitDiff(const GitDiff& rhs) = delete;
    GitDiff(GitDiff&& rhs) noexcept;
    ~GitDiff() noexcept;

    // Returns the non-null git_diff object that GitDiff wraps.
    operator const git_diff*() const noexcept { return m_diff; }
    operator git_diff*() noexcept             { return m_diff; }

    // Returns the number of deltas between two trees.
    size_t GetNumberDeltas() const noexcept;

    // Executes the callback function for each file in the difference.
    // The function is passed either the git_diff_delta or the path and difference code.
    // The callback function, which can throw exceptions, should return true to continue processing.
    // Difference codes are in diff.h.
    void ForeachDifference(const std::function<bool(const void* delta)>& callback_function) const;
    void ForeachDifference(const std::function<bool(std::string path, unsigned int diff_flag)>& callback_function) const;

private:
    git_diff* m_diff;
};
