#pragma once

#include <zGit/zGit.h>
#include <zGit/GitRepository.h>


// --------------------------------------------------------------------------
// GitIgnoreEvaluator
//
// This class creates a temporary Git repository which is then used to
// evaluate whether paths should be included based on a set of gitignore
// rules.
// --------------------------------------------------------------------------

class ZGIT_API GitIgnoreEvaluator : private GitRepository
{
public:
    GitIgnoreEvaluator();
    ~GitIgnoreEvaluator();

    void AddRules(cs::string_sz rules);
    void AddRulesFromFile(const std::string& file_path);

    void ClearRules();

    template<typename T>
    bool Include(T&& path) const;

    template<typename T>
    bool Ignore(T&& path) const;

private:
    void AddDefaultRules();

private:
    std::string m_tempRepoDirectory;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
bool GitIgnoreEvaluator::Include(T&& path) const
{
    return !GitRepository::IsPathIgnored(std::forward<T>(path));
}


template<typename T>
bool GitIgnoreEvaluator::Ignore(T&& path) const
{
    return GitRepository::IsPathIgnored(std::forward<T>(path));
}
