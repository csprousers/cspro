#pragma once

#include "GitBase.h"
#include "GitObjects.h"
#include <zUtilF/LoggingListBox.h>


class Creator : public Git::Base
{
public:
    Creator();
    ~Creator();

    std::vector<Git::Tag> GetTags() const;

    void CreateRelease(LoggingListBox& logging_list_box, cs::string_sz commit_string, const std::string& output_directory);

private:
    template<typename git_oidT>
    static std::string ObjectIdToString(const git_oidT* oid);

    template<typename git_treeT>
    void PopulateRepoPaths(const git_treeT* tree, const std::string& base_path);

    void PruneRepoPaths();

    void PrepareOutputDirectory();

    void CopyFilesToOutputDirectory();

    void CopyReplacementFiles();

    template<typename git_treeT>
    void CreateSqliteWithoutSEE(const git_treeT* tree);

    struct TagCommits;
    std::vector<TagCommits> GetReleaseTags(std::string_view earliest_tag_sv);

    template<typename git_oidT>
    void CreateHistoryLog(const git_oidT* oid);

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
