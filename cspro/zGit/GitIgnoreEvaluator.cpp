#include "StdAfx.h"
#include "GitIgnoreEvaluator.h"
#include <zUtilO/Interapp.h>


GitIgnoreEvaluator::GitIgnoreEvaluator()
    :   m_tempRepoDirectory(GetUniqueTempFilePath("Git-GitIgnoreEvaluator"))
{
    Create(m_tempRepoDirectory, true);

    AddDefaultRules();
}


GitIgnoreEvaluator::~GitIgnoreEvaluator()
{
    Close();

    if( !PortableFunctions::DirectoryDelete(m_tempRepoDirectory, true) )
        ErrorMessage::Display("GitIgnoreEvaluator: Error deleting: " + m_tempRepoDirectory);
}


void GitIgnoreEvaluator::AddRules(cs::string_sz rules)
{
    AddIgnoreRule(rules);
}


void GitIgnoreEvaluator::AddRulesFromFile(const std::string& file_path)
{
    AddIgnoreRulesFromFile(file_path);
}


void GitIgnoreEvaluator::AddDefaultRules()
{
    // by default the .git directory is ignored, so restore it
    AddRules("!.git");
}


void GitIgnoreEvaluator::ClearRules()
{
    ClearIgnoreRules();

    AddDefaultRules();
}
