#include "StdAfx.h"
#include "CommandLineBuilder.h"
#include <zAppO/PFF.h>
#include <zLogicO/ReservedWords.h>


void CommandLineBuilder::CreateNotepadPlusPlusColorizer()
{
    const std::wstring project_name = PortableFunctions::PathGetFilenameWithoutExtension(CSProExecutables::GetModuleFilename());
    const std::wstring template_filename = PortableFunctions::PathAppendToPath(PortableFunctions::PathAppendToPath(m_globalSettings.cspro_code_path,
                                                                                                                   project_name),
                                                                                                                   _T("userDefineLang-template.xml"));
    const std::wstring output_filename = PortableFunctions::PathAppendToPath(CSProExecutables::GetModuleDirectory(), _T("userDefineLang.xml"));
    
    // read in the template file
    std::wstring colorizer_template = FileIO::ReadText<std::wstring>(template_filename);

    // replace the templated sections with logic and PFF words
    auto fill_template = [&](const TCHAR* template_id, auto words)
    {
        size_t template_length = colorizer_template.length();

        // create a space-separated string of the (sorted) words
        std::sort(words.begin(), words.end(), [&](const auto& word1, const auto& word2) { return ( SO::CompareNoCase(word1, word2) < 0 );});

        SO::Replace(colorizer_template, template_id, SO::CreateSingleString(words, _T(" ")));

        if( template_length == colorizer_template.length() )
            throw CSProException(_T("Error replacing: %s"), template_id);
    };

    fill_template(_T("~~template-logic~~"), Logic::ReservedWords::GetAllReservedWords());
    fill_template(_T("~~template-pff-app-types~~"), PFF::GetAppTypeWords());
    fill_template(_T("~~template-pff-headings~~"), PFF::GetHeadingWords());
    fill_template(_T("~~template-pff-attributes~~"), PFF::GetAttributeWords());

    // write out the file
    FileIO::WriteText(output_filename, colorizer_template, false);
}
