#include "StdAfx.h"
#include "Builder.h"
#include <zToolsO/FileIO.h>
#include <zUtilO/TemporaryFile.h>


Builder::Builder(LoggingListBox& logging_list_box, std::string cspro_root_directory,
                 std::string cspro_users_input_directory, std::string cspro_users_output_directory)
    :   m_loggingListBox(logging_list_box),
        m_csproRootDirectory(std::move(cspro_root_directory)),
        m_csproUsersInputDirectory(std::move(cspro_users_input_directory)),
        m_csproUsersOutputDirectory(std::move(cspro_users_output_directory))
{
    ASSERT(PortableFunctions::FileIsDirectory(m_csproRootDirectory) &&
           PortableFunctions::FileIsDirectory(m_csproUsersInputDirectory) &&
           PortableFunctions::FileIsDirectory(m_csproUsersOutputDirectory));
}


void Builder::UpdateGooglePlayPrivacyPolicy()
{
    m_loggingListBox.AddText("Creating the Google Play privacy policy...");

    const std::string gcl_exe = Path::Combine(m_csproRootDirectory, "build-tools\\Licenses\\Generate Combined License\\Debug\\Generate Combined License.exe");

    if( !PortableFunctions::FileIsRegular(gcl_exe) )
        throw CSProException("The Generate Combined License program must exist at: " + gcl_exe);

    const std::string privacy_path_directory = Path::Combine(m_csproUsersInputDirectory, "privacy");
    const std::string privacy_path_template_file_path = Path::Combine(privacy_path_directory, "privacy-policy-template.html");
    const std::string privacy_path_output_file_path = Path::Combine(privacy_path_directory, "privacy-policy.html");

    // create the license as a temporary file
    TemporaryFile privacy_path_output_temporary_file;
    const int64_t privacy_path_output_initial_size = PortableFunctions::FileSize(privacy_path_output_temporary_file.GetPath());

    const std::string command = EscapeCommandLineArgument(gcl_exe)
                                .append(" ").append(EscapeCommandLineArgument(privacy_path_template_file_path))
                                .append(" ").append(EscapeCommandLineArgument(privacy_path_output_temporary_file.GetPath()));
    int return_code;

    if( !RunProgram(TC::ToWide(command), &return_code, SW_SHOWNA, true, true) ||
        privacy_path_output_initial_size == PortableFunctions::FileSize(privacy_path_output_temporary_file.GetPath()) )
    {
        throw CSProException("Error running the Generate Combined License program.");
    }

    // copy the license to its destination
    PortableFunctions::FileCopyWithExceptions(privacy_path_output_temporary_file.GetPath(), privacy_path_output_file_path, FileOverwriteFlag::Always);
}
