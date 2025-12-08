#include "StdAfx.h"
#include "GitIgnoreEvaluator.h"


struct Git::IgnoreEvaluator::Data
{
    std::string repo_directory;
    git_repository* repo;
};


Git::IgnoreEvaluator::IgnoreEvaluator()
    :   m_data(std::make_unique<Data>())
{
    m_data->repo_directory = GetUniqueTempFilePath("Git-IgnoreEvaluator");

    if( git_repository_init(&m_data->repo, m_data->repo_directory.c_str(), true) < 0 )
        ThrowGitException();
}


Git::IgnoreEvaluator::~IgnoreEvaluator()
{
    git_repository_free(m_data->repo);

    if( !PortableFunctions::DirectoryDelete(m_data->repo_directory, true) )
        ErrorMessage::Display("Git::IgnoreEvaluator: Error deleting: " + m_data->repo_directory);
}


void Git::IgnoreEvaluator::AddRules(const cs::string_sz rules)
{
    if( git_ignore_add_rule(m_data->repo, rules.c_str()) != 0 )
        ThrowGitException();
}


void Git::IgnoreEvaluator::AddRulesFromFile(const std::string& file_path)
{
    AddRules(FileIO::ReadText(file_path));
}


bool Git::IgnoreEvaluator::Include(std::string path)
{
    Path::MakeToForwardSlash(path);

    int ignored;

    if( git_ignore_path_is_ignored(&ignored, m_data->repo, path.c_str()) < 0 )
        ThrowGitException();

    return ( ignored == 0 );
}
