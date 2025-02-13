#include "StdAfx.h"
#include "FilteredExtensionProcessor.h"


std::unique_ptr<FilteredExtensionProcessor> CreateListingFilteredExtensionProcessor()
{
    auto filtered_extension_processor = std::make_unique<FilteredExtensionProcessor>();

    filtered_extension_processor->AddType(_T("Text"),       { FileExtensions::Listing                                              }, true);
    filtered_extension_processor->AddType(_T("HTML"),       { FileExtensions::HTML,          FileExtensions::HTM                   });
    filtered_extension_processor->AddType(_T("Excel"),      { FileExtensions::Excel                                                });
    filtered_extension_processor->AddType(_T("CSV"),        { FileExtensions::CSV                                                  });
    filtered_extension_processor->AddType(_T("JSON"),       { FileExtensions::Json });
    filtered_extension_processor->AddType(_T("CSPro Data"), { FileExtensions::Data::CSProDB, FileExtensions::Data::TextDataDefault });

    filtered_extension_processor->SetWinSettingsType(WinSettings::Type::ListingFilenameExtension);

    return filtered_extension_processor;
}


std::unique_ptr<FilteredExtensionProcessor> CreateFrequencyFilteredExtensionProcessor()
{
    auto filtered_extension_processor = std::make_unique<FilteredExtensionProcessor>();

    filtered_extension_processor->AddType(_T("Text"),  { FileExtensions::Listing                   }, true);
    filtered_extension_processor->AddType(_T("Table"), { FileExtensions::Table                     });
    filtered_extension_processor->AddType(_T("HTML"),  { FileExtensions::HTML, FileExtensions::HTM });
    filtered_extension_processor->AddType(_T("Excel"), { FileExtensions::Excel                     });
    filtered_extension_processor->AddType(_T("JSON"),  { FileExtensions::Json,                     });

    filtered_extension_processor->SetWinSettingsType(WinSettings::Type::FrequencyFilenameExtension);

    return filtered_extension_processor;
}


void FilteredExtensionProcessor::AddType(const CString& type, std::vector<CString> extensions, bool set_as_type_for_unknown_extension/* =false*/)
{
    if( set_as_type_for_unknown_extension )
        m_unknownExtensionIndex = m_typesAndExtensions.size();

    m_typesAndExtensions.emplace_back(type, std::move(extensions));
}


std::vector<const TCHAR*> FilteredExtensionProcessor::GetTypes() const
{
    std::vector<const TCHAR*> types;

    for( const auto& [type, extensions] : m_typesAndExtensions )
        types.emplace_back(type);

    return types;
}


CString FilteredExtensionProcessor::GetTypeFromFilename(wstring_view filename) const
{
    std::wstring extension = PortableFunctions::PathGetFileExtension(filename);

    for( const auto& [type, extensions] : m_typesAndExtensions )
    {
        for( const CString& this_extension : extensions )
        {
            if( SO::EqualsNoCase(extension, this_extension) )
                return type;
        }
    }

    if( m_unknownExtensionIndex.has_value() )
        return std::get<0>(m_typesAndExtensions[*m_unknownExtensionIndex]);

    throw ProgrammingErrorException();
}


CString FilteredExtensionProcessor::GetFilenameFromType(wstring_view type_text, const CString& filename) const
{
    // don't modify if this is the right type
    if( type_text == GetTypeFromFilename(filename) )
        return filename;

    // otherwise get the right extension
    for( const auto& [type, extensions] : m_typesAndExtensions )
    {
        if( type_text == type )
        {
            return FormatText(_T("%s%s.%s"), PortableFunctions::PathGetDirectory(filename).c_str(),
                                             PortableFunctions::PathGetFilenameWithoutExtension(filename).c_str(),
                                             extensions.front().GetString());
        }
    }

    throw ProgrammingErrorException();
}


void FilteredExtensionProcessor::SetWinSettingsType(WinSettings::Type type)
{
    m_winSettingsType = type;
}


void FilteredExtensionProcessor::UpdateWinSettings(wstring_view filename) const
{
    if( m_winSettingsType.has_value() )
        WinSettings::Write(*m_winSettingsType, PortableFunctions::PathGetFileExtension<CString>(filename));
}
