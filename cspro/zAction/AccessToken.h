#pragma once

#include <zToolsO/Hash.h>
#include <zToolsO/Utf8.h>
#include <zUtilO/Interapp.h>


namespace ActionInvoker
{
    namespace AccessToken
    {
        // charting/frequency-view
        constexpr std::string_view Charting_FrequencyView_sv = "a125e77b3ecf5068916e0322afce5eb4";

        // questionnaire-view/index.html
        constexpr std::string_view QuestionnaireView_Index_sv = "ba595f180197aec52c4a39ffb9d12233";


        // access tokens for internal files are calculated by:
        //   - removing the html directory portion of the path,
        //   - turning the slashes into forward slashes,
        //   - calculating a lowercase MD5 of the lowercase version of this text
        inline std::string CreateAccessTokenForHtmlDirectoryFile(const std::string& file_path)
        {
            const std::string& html_directory = Html::GetDirectory();

            if( !SO::StartsWithNoCase(file_path, html_directory) )
                throw ProgrammingErrorException();

            // don't include the initial slash in the path
            ASSERT(Path::IsSlashChar(file_path[html_directory.length()]));
            std::string path_for_md5 = PortableFunctions::PathToForwardSlash(file_path.substr(html_directory.length() + 1));
            ASSERT(!path_for_md5.empty());

            SO::MakeLower(path_for_md5);

            std::string md5 = Hash::Md5::Create(path_for_md5);
            ASSERT(md5 == SO::ToLower(md5));

            return md5;
        }
    }
}
