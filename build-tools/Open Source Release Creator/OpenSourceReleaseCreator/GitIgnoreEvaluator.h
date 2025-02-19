#pragma once

#include "GitBase.h"

namespace Git { class IgnoreEvaluator; }


// --------------------------------------------------------------------------
// Git::IgnoreEvaluator
//
// This class creates a temporary Git repository which is then used to
// evaluate whether paths should be included based on a set of gitignore
// rules.
// --------------------------------------------------------------------------

class Git::IgnoreEvaluator : public Git::Base
{
public:
    IgnoreEvaluator();
    ~IgnoreEvaluator();

    void AddRules(cs::string_sz rules);
    void AddRulesFromFile(const std::string& file_path);

    bool Include(std::string path);
    bool Ignore(std::string path) { return !Include(std::move(path)); }

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
