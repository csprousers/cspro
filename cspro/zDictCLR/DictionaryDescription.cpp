#include "Stdafx.h"
#include "DictionaryDescription.h"
#include <zToolsO/PortableFunctions.h>
#include <zAppO/Application.h>


CSPro::Dictionary::DictionaryDescription::DictionaryDescription(System::String^ path, const bool is_input_dictionary)
    :   m_path(path),
        m_isInputDictionary(is_input_dictionary)
{
}


System::String^ CSPro::Dictionary::DictionaryDescription::Path::get()
{
    return m_path;
}


bool CSPro::Dictionary::DictionaryDescription::IsInputDictionary::get()
{
    return m_isInputDictionary;
}


System::String^ CSPro::Dictionary::DictionaryDescription::ToString()
{
    // as ToString is only used by CSDeploy, append the syncable name when set
    if( m_displayText == nullptr )
    {
        const std::string file_path = clr_helpers::to_string(m_path);
        std::string syncable_name;

        try
        {
             syncable_name = CDataDict::InstantiateAndOpen(file_path, true)->GetSyncableName(false);
        }
        catch(...) { }

        m_displayText = clr_helpers::to_SystemString(SO::CreateParentheticalExpression(PortableFunctions::PathGetFilename(file_path),
                                                                                       syncable_name));
    }

    return m_displayText;
}


System::Collections::Generic::List<CSPro::Dictionary::DictionaryDescription^>^ CSPro::Dictionary::DictionaryDescription::GetFromApplication(System::String^ application_filename)
{
    try
    {
        Application application;
        application.Open(clr_helpers::to_wstring(application_filename), true, false);

        auto dictionary_descriptions = gcnew System::Collections::Generic::List<CSPro::Dictionary::DictionaryDescription^>();

        for( const ::DictionaryDescription& dictionary_description : application.GetDictionaryDescriptions() )
        {
            if( dictionary_description.GetDictionaryType() == DictionaryType::Working )
                continue;

            dictionary_descriptions->Add(gcnew DictionaryDescription(clr_helpers::to_SystemString(dictionary_description.GetDictionaryFilePath()),
                                                                     ( dictionary_description.GetDictionaryType() == DictionaryType::Input )));
        }

        return dictionary_descriptions;
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(clr_helpers::to_SystemString(exception.what()));
    }
}
