#pragma once


class Builder
{
public:
    Builder(LoggingListBox& logging_list_box, std::string cspro_root_directory,
            std::string cspro_users_input_directory, std::string cspro_users_output_directory);

    void UpdateGooglePlayPrivacyPolicy();

private:
    LoggingListBox& m_loggingListBox;
    std::string m_csproRootDirectory;
    std::string m_csproUsersInputDirectory;
    std::string m_csproUsersOutputDirectory;
};
