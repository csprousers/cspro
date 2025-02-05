#pragma once

#include "GitObjects.h"
#include <zUtilF/LoggingListBox.h>


class Creator
{
public:
    Creator(LoggingListBox& logging_list_box);
    ~Creator();

    std::vector<Git::Tag> GetTags() const;

    void CreateRelease(cs::string_sz commit_string, const std::string& output_directory);

private:
    [[noreturn]] static void ThrowGitException();

    template<typename git_oidT>
    static std::string ObjectIdToString(const git_oidT* oid);

private:
    LoggingListBox& m_loggingListBox;

    struct Data;
    std::unique_ptr<Data> m_data;
};
