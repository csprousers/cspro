#pragma once

#include <zToolsO/Utf8Convert.h>
#include <zUtilO/Interapp.h>


namespace ActionInvoker
{
    namespace AccessToken
    {
        // charting/frequency-view
        constexpr std::wstring_view Charting_FrequencyView_sv = _T("a125e77b3ecf5068916e0322afce5eb4");

        // questionnaire-view/index.html
        constexpr std::wstring_view QuestionnaireView_Index_sv = _T("ba595f180197aec52c4a39ffb9d12233");


        // access tokens for internal files are calculated by:
        //   - removing the html directory portion of the path,
        //   - turning the slashes into forward slashes,
        //   - calculating a lowercase MD5 of the lowercase version of this text
        inline std::wstring CreateAccessTokenForHtmlDirectoryFile(const std::wstring& path)
        {
            const std::wstring& html_directory = Html::GetDirectory();

            if( !SO::StartsWithNoCase(path, html_directory) )
                throw ProgrammingErrorException();

            // don't include the initial slash in the path
            ASSERT(PortableFunctions::IsPathCharacter(path[html_directory.length()]));
            std::wstring path_for_md5 = PortableFunctions::PathToForwardSlash(path.substr(html_directory.length() + 1));
            ASSERT(!path_for_md5.empty());

            SO::MakeLower(path_for_md5);

            std::wstring md5 = PortableFunctions::StringMd5(UTF8Convert::WideToUTF8(path_for_md5));
            ASSERT(md5 == SO::ToLower(md5));

            return md5;
        }
    }
}
