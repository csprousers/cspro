#include "stdafx.h"
#include "PackEntry.h"
#include <zToolsO/DirectoryLister.h>
#include <zUtilO/ApplicationLoadException.h>
#include <zUtilO/Specfile.h>
#include <zAppO/Application.h>
#include <zDictO/DDClass.h>
#include <zDictO/DictionaryIterator.h>
#include <zFormO/FormFile.h>
#include <zDataO/DataRepositoryHelpers.h>


namespace
{
    inline void AddPotentiallyBlankFilename(std::vector<std::wstring>& filenames, const wstring_view filename_sv)
    {
        if( !filename_sv.empty() )
            filenames.emplace_back(filename_sv);
    }

    template<typename T>
    std::shared_ptr<T> GetNonNullExtras(std::shared_ptr<T> extras)
    {
        if( extras == nullptr )
            extras = std::make_shared<T>();

        return extras;                                       ;
    }
}


// --------------------------------------------------------------------------
//
// PackEntry
//
// --------------------------------------------------------------------------

PackEntry::PackEntry(std::wstring path, const bool entry_is_file)
    :   m_path(std::move(path))
{
    if( entry_is_file )
    {
        if( !PortableFunctions::FileIsRegular(m_path.c_str()) )
            throw FileIO::Exception::FileNotFound(m_path.c_str());
    }

    else if( !PortableFunctions::FileIsDirectory(m_path.c_str()) )
    {
        throw CSProException(_T("The directory '%s' does not exist."), PortableFunctions::PathGetFilename(m_path.c_str()));
    }
}


std::unique_ptr<PackEntry> PackEntry::Create(std::wstring path)
{
    if( PortableFunctions::FileIsDirectory(path.c_str()) )
        return std::make_unique<DirectoryPackEntry>(std::move(path));

    auto matches = [extension = PortableFunctions::PathGetFileExtension(path)](const auto& test_extension)
    {
        return SO::EqualsNoCase(extension, test_extension);
    };

    if( matches(FileExtensions::EntryApplication) ||
        matches(FileExtensions::BatchApplication) ||
        matches(FileExtensions::TabulationApplication) )
    {
        return std::make_unique<ApplicationPackEntry>(std::move(path));
    }

    else if( matches(FileExtensions::Form) ||
             matches(FileExtensions::Order) )
    {
        return std::make_unique<FormPackEntry>(std::move(path), nullptr);
    }

    else if( matches(FileExtensions::TableSpec) )
    {
        return std::make_unique<TabSpecPackEntry>(std::move(path), nullptr);
    }

    else if( matches(FileExtensions::Dictionary) )
    {
        return std::make_unique<DictionaryPackEntry>(std::move(path), nullptr);
    }

    else if( matches(FileExtensions::Pff) )
    {
        return std::make_unique<PffPackEntry>(std::move(path), nullptr);
    }

    else
    {
        return std::make_unique<PackEntry>(std::move(path), true);
    }
}


std::vector<std::tuple<std::wstring, std::wstring>> PackEntry::GetFilenamesForDisplay() const
{
    std::vector<std::wstring> filenames = GetAssociatedFilenames();
    VectorHelpers::RemoveDuplicates(filenames);

    // if the pack entry only consists of this file, then there are no additional filenames to display
    if( filenames.empty() || ( filenames.size() == 1 && SO::EqualsNoCase(filenames.front(), m_path) ) )
        return { };

    // GetRelativeFNameForDisplay requires a filename, so create a fake one for directories
    std::wstring relative_to_filename = m_path;

    if( dynamic_cast<const DirectoryPackEntry*>(this) != nullptr )
        relative_to_filename = PortableFunctions::PathAppendToPath(WS2CS(relative_to_filename), _T("a"));

    //  turn the filenames into relative paths and then sort by filename
    std::vector<std::tuple<std::wstring, std::wstring>> filenames_for_display;

    for( const std::wstring& filename : filenames )
        filenames_for_display.emplace_back(filename, GetRelativeFNameForDisplay(relative_to_filename.c_str(), filename.c_str()));

    std::sort(filenames_for_display.begin(), filenames_for_display.end(),
        [&](const auto& ffd1, const auto& ffd2)
        {
            // sort order: files in the directory (1), files in subdirectories (2), files in other directories or drives (3)
            auto get_sort_order = [](wstring_view filename_sv) -> int
            {
                return ( !filename_sv.empty() && filename_sv.front() == '.' )                                      ? 3 :
                       ( filename_sv.find_first_of(_T(":")) != std::wstring_view::npos )                           ? 3 :
                       ( filename_sv.find_first_of(PortableFunctions::PathSlashChars) != std::wstring_view::npos ) ? 2 :
                                                                                                                     1;
            };

            const int sort1 = get_sort_order(std::get<1>(ffd1));
            const int sort2 = get_sort_order(std::get<1>(ffd2));

            return ( sort1 != sort2 ) ? ( sort1 < sort2 ) :
                                        ( SO::CompareNoCase(std::get<1>(ffd1), std::get<1>(ffd2)) < 0 );
        });

    return filenames_for_display;
}



// --------------------------------------------------------------------------
//
// DirectoryPackEntry
//
// --------------------------------------------------------------------------

DirectoryPackEntry::DirectoryPackEntry(std::wstring path)
    :   PackEntry(std::move(path), false)
{
}


DirectoryPackEntryExtras* DirectoryPackEntry::GetDirectoryExtras()
{
    return &m_directoryExtras;
}


std::vector<std::wstring> DirectoryPackEntry::GetAssociatedFilenames() const
{
    const auto& filenames_lookup = m_directoryFilenames.find(m_directoryExtras.recursive);

    if( filenames_lookup != m_directoryFilenames.cend() )
        return filenames_lookup->second;

    std::vector<std::wstring>& directory_filenames = m_directoryFilenames[m_directoryExtras.recursive];

    DirectoryLister(m_directoryExtras.recursive, true, false).AddPaths(directory_filenames, m_path);

    return directory_filenames;
}



// --------------------------------------------------------------------------
//
// DictionaryPackEntry
//
// --------------------------------------------------------------------------

DictionaryPackEntry::DictionaryPackEntry(std::wstring path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras)
    :   PackEntry(std::move(path), true),
        m_dictionaryExtras(GetNonNullExtras(std::move(dictionary_extras)))
{
}


DictionaryPackEntryExtras* DictionaryPackEntry::GetDictionaryExtras()
{
    return m_dictionaryExtras.get();
}


std::vector<std::wstring> DictionaryPackEntry::GetAssociatedFilenames() const
{
    std::vector<std::wstring> filenames = PackEntry::GetAssociatedFilenames();

    if( m_dictionaryExtras->value_set_images )
    {
        if( m_valueSetImageFilenames == nullptr )
        {
            std::unique_ptr<const CDataDict> dictionary = CDataDict::InstantiateAndOpen(m_path.c_str(), true);

            m_valueSetImageFilenames = std::make_unique<std::vector<std::wstring>>();

            DictionaryIterator::Foreach<DictValue>(*dictionary,
                [&](const DictValue& dict_value)
                {
                    AddPotentiallyBlankFilename(*m_valueSetImageFilenames, dict_value.GetImageFilename());
                });
        }

        VectorHelpers::Append(filenames, *m_valueSetImageFilenames);
    }

    return filenames;
}



// --------------------------------------------------------------------------
//
// FormPackEntry
//
// --------------------------------------------------------------------------

FormPackEntry::FormPackEntry(std::wstring path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras)
    :   PackEntry(std::move(path), true)
{
    CDEFormFile form_file;

    if( !form_file.Open(m_path.c_str(), true) )
        throw ApplicationFileLoadException(m_path.c_str());

    m_dictionaryPackEntry = std::make_unique<DictionaryPackEntry>(CS2WS(form_file.GetDictionaryFilename()), std::move(dictionary_extras));
}


DictionaryPackEntryExtras* FormPackEntry::GetDictionaryExtras()
{
    return m_dictionaryPackEntry->GetDictionaryExtras();
}


std::vector<std::wstring> FormPackEntry::GetAssociatedFilenames() const
{
    return VectorHelpers::Concatenate(PackEntry::GetAssociatedFilenames(), m_dictionaryPackEntry->GetAssociatedFilenames());
}



// --------------------------------------------------------------------------
//
// TabSpecPackEntry
//
// --------------------------------------------------------------------------

TabSpecPackEntry::TabSpecPackEntry(std::wstring path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras)
    :   PackEntry(std::move(path), true)
{
    CSpecFile specfile;

    if( specfile.Open(m_path.c_str(), CFile::modeRead) )
    {
        std::vector<std::wstring> dictionary_filenames = GetFileNameArrayFromSpecFile(specfile, CSPRO_DICTS);
        specfile.Close();

        if( dictionary_filenames.size() == 1 )
        {
            m_dictionaryPackEntry = std::make_unique<DictionaryPackEntry>(std::move(dictionary_filenames.front()), std::move(dictionary_extras));
            return;
        }
    }

    throw ApplicationFileLoadException(m_path.c_str());
}


DictionaryPackEntryExtras* TabSpecPackEntry::GetDictionaryExtras()
{
    return m_dictionaryPackEntry->GetDictionaryExtras();
}


std::vector<std::wstring> TabSpecPackEntry::GetAssociatedFilenames() const
{
    return VectorHelpers::Concatenate(PackEntry::GetAssociatedFilenames(), m_dictionaryPackEntry->GetAssociatedFilenames());
}



// --------------------------------------------------------------------------
//
// PffPackEntry
//
// --------------------------------------------------------------------------

PffPackEntry::PffPackEntry(std::wstring path, std::shared_ptr<PffPackEntryExtras> pff_extras)
    :   PackEntry(std::move(path), true),
        m_pffExtras(GetNonNullExtras(std::move(pff_extras)))
{
}


PffPackEntry::~PffPackEntry()
{
}


PffPackEntryExtras* PffPackEntry::GetPffExtras()
{
    return m_pffExtras.get();
}


std::vector<std::wstring> PffPackEntry::GetAssociatedFilenames() const
{
    std::vector<std::wstring> filenames = PackEntry::GetAssociatedFilenames();

    auto get_pff = [&]() -> PFF&
    {
        if( m_pff == nullptr )
        {
            auto pff = std::make_unique<PFF>(WS2CS(m_path));

            if( !pff->LoadPifFile(true) )
                throw ApplicationFileLoadException(m_path.c_str());

            m_pff = std::move(pff);
        }

        return *m_pff;
    };

    auto add_connection_string_data = [&](std::vector<std::wstring>& destination_filenames, const ConnectionString& connection_string)
    {
        VectorHelpers::Append(destination_filenames, DataRepositoryHelpers::GetAssociatedFileList(connection_string, true));
    };


    if( m_pffExtras->input_data )
    {
        if( m_inputDataFilenames == nullptr )
        {
            auto input_data_filenames = std::make_unique<std::vector<std::wstring>>();

            for( const ConnectionString& input_connection_string : get_pff().GetInputDataConnectionStrings() )
                add_connection_string_data(*input_data_filenames, input_connection_string);

            m_inputDataFilenames = std::move(input_data_filenames);
        }

        VectorHelpers::Append(filenames, *m_inputDataFilenames);
    }


    if( m_pffExtras->external_dictionary_data )
    {
        if( m_externalDictionaryDataFilenames == nullptr )
        {
            auto external_dictionary_data_filenames = std::make_unique<std::vector<std::wstring>>();

            for( const auto& [dictionary_name, connection_string] : get_pff().GetExternalDataConnectionStrings() )
                add_connection_string_data(*external_dictionary_data_filenames, connection_string);

            m_externalDictionaryDataFilenames = std::move(external_dictionary_data_filenames);
        }

        VectorHelpers::Append(filenames, *m_externalDictionaryDataFilenames);
    }


    if( m_pffExtras->user_files )
    {
        if( m_userFilenames == nullptr )
        {
            auto user_filenames = std::make_unique<std::vector<std::wstring>>();

            for( const CString& filename : get_pff().GetUserFiles() )
            {
                if( PortableFunctions::FileIsRegular(filename) )
                    user_filenames->emplace_back(filename);
            }

            m_userFilenames = std::move(user_filenames);
        }

        VectorHelpers::Append(filenames, *m_userFilenames);
    }

    return filenames;    
}


// --------------------------------------------------------------------------
//
// ApplicationPackEntry
//
// --------------------------------------------------------------------------

ApplicationPackEntry::ApplicationPackEntry(std::wstring path)
    :   PackEntry(std::move(path), true)
{
    auto get_dictionary_extras = [&]()
    {
        if( m_dictionaryExtras == nullptr )
            m_dictionaryExtras = std::make_shared<DictionaryPackEntryExtras>();

        return m_dictionaryExtras;
    };

    // load the application and add the application files
    Application application;
    application.Open(m_path.c_str(), true, false);

    // application properties
    AddPotentiallyBlankFilename(m_applicationFilenames, application.GetApplicationPropertiesFilename());

    // form files
    for( const CString& form_filename : application.GetFormFilenames() )
        m_formPackEntries.emplace_back(CS2WS(form_filename), get_dictionary_extras());

    // tab specs
    for( const CString& tab_spec_filename : application.GetTabSpecFilenames() )
        m_tabSpecPackEntries.emplace_back(CS2WS(tab_spec_filename), get_dictionary_extras());

    // external dictionaries
    for( const CString& dictionary_filename : application.GetExternalDictionaryFilenames() )
        m_externalDictionaryPackEntries.emplace_back(CS2WS(dictionary_filename), get_dictionary_extras());

    // code files
    for( const CodeFile& code_file : application.GetCodeFiles() )
        m_applicationFilenames.emplace_back(code_file.GetFilename());

    // message files
    for( const std::shared_ptr<TextSource>& message_text_source : application.GetMessageTextSources() )
        m_applicationFilenames.emplace_back(message_text_source->GetFilename());

    // reports
    for( const std::shared_ptr<NamedTextSource>& report_named_text_sources : application.GetReportNamedTextSources() )
        m_applicationFilenames.emplace_back(report_named_text_sources->text_source->GetFilename());

    // question text
    AddPotentiallyBlankFilename(m_applicationFilenames, application.GetQuestionTextFilename());

    // resource folders
    for( const CString& resource_folder : application.GetResourceFolders() )
        m_resourceFolderFilenames.try_emplace(CS2WS(resource_folder), nullptr);

    // PFF
    std::wstring expected_pff_filename = ( !m_tabSpecPackEntries.empty() ? m_path : PortableFunctions::PathRemoveFileExtension(m_path) ) +
                                         FileExtensions::WithDot::Pff;

    if( PortableFunctions::FileIsRegular(expected_pff_filename.c_str()) )
        m_pffPackEntry = std::make_unique<PffPackEntry>(std::move(expected_pff_filename), nullptr);
}


DictionaryPackEntryExtras* ApplicationPackEntry::GetDictionaryExtras()
{
    return m_dictionaryExtras.get();
}


PffPackEntryExtras* ApplicationPackEntry::GetPffExtras()
{
    return ( m_applicationExtras.pff && m_pffPackEntry != nullptr ) ? m_pffPackEntry->GetPffExtras() :
                                                                      nullptr;
}


ApplicationPackEntryExtras* ApplicationPackEntry::GetApplicationExtras()
{
    return &m_applicationExtras;
}


std::vector<std::wstring> ApplicationPackEntry::GetAssociatedFilenames() const
{
    std::vector<std::wstring> filenames = PackEntry::GetAssociatedFilenames();

    // form files
    for( const FormPackEntry& form_pack_entry : m_formPackEntries )
        VectorHelpers::Append(filenames, form_pack_entry.GetAssociatedFilenames());

    // tab specs
    for( const TabSpecPackEntry& tab_spec_pack_entry : m_tabSpecPackEntries )
        VectorHelpers::Append(filenames, tab_spec_pack_entry.GetAssociatedFilenames());
    
    // external dictionaries
    for( const DictionaryPackEntry& dictionary_pack_entry : m_externalDictionaryPackEntries )
        VectorHelpers::Append(filenames, dictionary_pack_entry.GetAssociatedFilenames());

    // application files
    VectorHelpers::Append(filenames, m_applicationFilenames);

    // resource folders
    if( m_applicationExtras.resource_folders )
    {
        for( auto& [resource_folder, resource_filenames] : m_resourceFolderFilenames )
        {
            if( resource_filenames == nullptr )
                resource_filenames = std::make_unique<std::vector<std::wstring>>(DirectoryLister(true, true, false).GetPaths(resource_folder));

            VectorHelpers::Append(filenames, *resource_filenames);
        }
    }

    // PFF
    if( m_applicationExtras.pff && m_pffPackEntry != nullptr )
        VectorHelpers::Append(filenames, m_pffPackEntry->GetAssociatedFilenames());

    return filenames;
}
