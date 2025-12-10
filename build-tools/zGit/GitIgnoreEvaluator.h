#pragma once

#include <zGit/zGit.h>
#include <zGit/GitBase.h>

namespace Git { class IgnoreEvaluator; }


// --------------------------------------------------------------------------
// Git::IgnoreEvaluator
//
// This class creates a temporary Git repository which is then used to
// evaluate whether paths should be included based on a set of gitignore
// rules.
// --------------------------------------------------------------------------

class ZGIT_API Git::IgnoreEvaluator : public Git::Base
{
public:
    IgnoreEvaluator();
    ~IgnoreEvaluator();

    void AddRules(cs::string_sz rules);
    void AddRulesFromFile(const std::string& file_path);

    void ClearRules();

    bool Include(std::string path);
    bool Ignore(std::string path) { return !Include(std::move(path)); }

private:
    void AddDefaultRules();

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
