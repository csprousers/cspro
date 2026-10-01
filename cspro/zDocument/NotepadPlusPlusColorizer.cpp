#include "StdAfx.h"
#include "CommandLineBuilder.h"
#include <zAppO/PFF.h>
#include <zLogicO/ReservedWords.h>


void CommandLineBuilder::CreateNotepadPlusPlusColorizer()
{
    const std::string project_name = Path::GetFilenameWithoutExtension(CSProExecutables::GetModuleFilePath());
    const std::string template_file_path = Path::Combine(m_globalSettings.cspro_code_path, project_name, "userDefineLang-template.xml");
    const std::string output_file_path = Path::Combine(CSProExecutables::GetModuleDirectory(), "userDefineLang.xml");

    // read in the template file
    std::string colorizer_template = FileIO::ReadText(template_file_path);

    // replace the templated sections with logic and PFF words
    auto fill_template = [&](const char* const template_id, auto words)
    {
        const size_t template_length = colorizer_template.length();

        // create a space-separated string of the (sorted) words
        std::sort(words.begin(), words.end(), [&](const auto& word1, const auto& word2) { return ( SO::CompareNoCase(word1, word2) < 0 );});

        SO::Replace(colorizer_template, template_id, SO::CreateSingleString(words, " "));

        if( template_length == colorizer_template.length() )
            throw CSProException("Error replacing: %s", template_id);
    };

    fill_template("~~template-logic~~", Logic::ReservedWords::GetAllReservedWords());
    fill_template("~~template-pff-app-types~~", PFF::GetAppTypeWords());
    fill_template("~~template-pff-headings~~", PFF::GetHeadingWords());
    fill_template("~~template-pff-attributes~~", PFF::GetAttributeWords());

    // write out the file
    FileIO::WriteText(output_file_path, colorizer_template, false);
}
