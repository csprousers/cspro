#include "StdAfx.h"
#include "NotepadPlusPlusColorizer.h"
#include <zToolsO/FileIO.h>
#include <zAppO/PFF.h>
#include <zLogicO/ReservedWords.h>


std::string NotepadPlusPlusColorizerCreate(const std::string& installer_inputs_directory)
{
    // read in the template file
    const std::string template_file_path = Path::Combine(installer_inputs_directory, "userDefineLang-template.xml");
    std::string colorizer_template = FileIO::ReadText(template_file_path);

    // replace the templated sections with logic and PFF words
    auto fill_template = [&](const char* const template_id, auto words)
    {
        const size_t template_length = colorizer_template.length();

        // create a space-separated string of the (sorted) words
        std::sort(words.begin(), words.end(),
            [&](const auto& word1, const auto& word2) { return ( SO::CompareNoCase(word1, word2) < 0 );}
        );

        SO::Replace(colorizer_template, template_id, SO::CreateSingleString(words, " "));

        if( template_length == colorizer_template.length() )
            throw CSProException("Error replacing: %s", template_id);
    };

    fill_template("~~template-logic~~", Logic::ReservedWords::GetAllReservedWords());
    fill_template("~~template-pff-app-types~~", PFF::GetAppTypeWords());
    fill_template("~~template-pff-headings~~", PFF::GetHeadingWords());
    fill_template("~~template-pff-attributes~~", PFF::GetAttributeWords());

    return colorizer_template;
}


System::String^ CSPro::InstallerGenerator::NotepadPlusPlusColorizer::Create(System::String^ installer_inputs_directory)
{
    try
    {
        return clr_helpers::to_SystemString(
            NotepadPlusPlusColorizerCreate(clr_helpers::to_string(installer_inputs_directory))
        );
    }

    catch( const std::exception& exception )
    {
        throw clr_helpers::to_SystemException(exception);
    }
}
