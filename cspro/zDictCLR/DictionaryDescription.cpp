#include "Stdafx.h"
#include "DictionaryDescription.h"
#include <zAppO/Application.h>


System::String^ CSPro::Dictionary::DictionaryDescription::ToString()
{
    return System::IO::Path::GetFileName(Path);
}


System::Collections::Generic::List<CSPro::Dictionary::DictionaryDescription^>^ CSPro::Dictionary::DictionaryDescription::GetFromApplication(System::String^ application_filename)
{
    try
    {
        Application application;
        application.Open(ToWS(application_filename), true, false);

        auto dictionary_descriptions = gcnew System::Collections::Generic::List<CSPro::Dictionary::DictionaryDescription^>();

        for( const ::DictionaryDescription& dictionary_description : application.GetDictionaryDescriptions() )
        {
            auto dictionary_description_clr = gcnew CSPro::Dictionary::DictionaryDescription;

            dictionary_description_clr->Path = gcnew System::String(dictionary_description.GetDictionaryFilename().c_str());
            dictionary_description_clr->Type = gcnew System::String(::ToString(dictionary_description.GetDictionaryType()));

            dictionary_descriptions->Add(dictionary_description_clr);
        }

        return dictionary_descriptions;
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}
