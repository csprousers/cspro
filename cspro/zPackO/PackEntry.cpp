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
    template<typename T>
    void AddPotentiallyBlankFilePath(std::vector<std::string>& file_paths, T&& file_path)
    {
        if( !file_path.empty() )
            file_paths.emplace_back(std::forward<T>(file_path));
    }

    template<typename T>
    std::shared_ptr<T> GetNonNullExtras(std::shared_ptr<T> extras)
    {
        if( extras == nullptr )
            extras = std::make_shared<T>();

        return extras;
    }
}


// --------------------------------------------------------------------------
//
// PackEntry
//
// --------------------------------------------------------------------------

PackEntry::PackEntry(std::string path, const bool entry_is_file)
    :   m_path(std::move(path))
{
    if( entry_is_file )
    {
        if( !PortableFunctions::FileIsRegular(m_path) )
            throw FileIO::Exception::FileNotFound(m_path);
    }

    else if( !PortableFunctions::FileIsDirectory(m_path) )
    {
        throw CSProException("The directory '%s' does not exist.", PortableFunctions::PathGetFilename(m_path).c_str());
    }
}


std::unique_ptr<PackEntry> PackEntry::Create(std::string path)
{
    if( PortableFunctions::FileIsDirectory(path) )
        return std::make_unique<DirectoryPackEntry>(std::move(path));

    auto matches = [extension = PortableFunctions::PathGetFileExtension(path)](const auto& test_extension)
    {
        return SO::EqualsNoCase(extension, UTF8_TODO::EnsureUtf8(test_extension));
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


std::vector<std::tuple<std::string, std::string>> PackEntry::GetFilenamesForDisplay() const
{
    std::vector<std::string> file_paths = GetAssociatedFilePaths();
    VectorHelpers::RemoveDuplicates(file_paths);

    // if the pack entry only consists of this file path, then there are no additional filenames to display
    if( file_paths.empty() || ( file_paths.size() == 1 && SO::EqualsNoCase(file_paths.front(), m_path) ) )
        return { };

    // GetRelativeFNameForDisplay requires a filename, so create a fake one for directories
    std::string relative_to_file_path = m_path;

    if( dynamic_cast<const DirectoryPackEntry*>(this) != nullptr )
        relative_to_file_path = Path::Combine(relative_to_file_path, "a");

    //  turn the paths into relative paths and then sort by filename
    std::vector<std::tuple<std::string, std::string>> filenames_for_display;

    for( const std::string& file_path : file_paths )
        filenames_for_display.emplace_back(file_path, GetRelativePathForDisplay(relative_to_file_path, file_path));

    std::sort(filenames_for_display.begin(), filenames_for_display.end(),
        [&](const auto& ffd1, const auto& ffd2)
        {
            // sort order: files in the directory (1), files in subdirectories (2), files in other directories or drives (3)
            auto get_sort_order = [](const std::string& filename) -> int
            {
                return ( !filename.empty() && filename.front() == '.' )                     ? 3 :
                       ( filename.find_first_of(":") != std::string::npos )                 ? 3 :
                       ( filename.find_first_of(Path::SlashChars_sv) != std::string::npos ) ? 2 :
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

DirectoryPackEntry::DirectoryPackEntry(std::string path)
    :   PackEntry(std::move(path), false)
{
}


DirectoryPackEntryExtras* DirectoryPackEntry::GetDirectoryExtras()
{
    return &m_directoryExtras;
}


std::vector<std::string> DirectoryPackEntry::GetAssociatedFilePaths() const
{
    const auto& file_paths_lookup = m_directoryFilePaths.find(m_directoryExtras.recursive);

    if( file_paths_lookup != m_directoryFilePaths.cend() )
        return file_paths_lookup->second;

    std::vector<std::string>& directory_file_paths = m_directoryFilePaths[m_directoryExtras.recursive];

    directory_file_paths = DirectoryLister(m_directoryExtras.recursive, true, false).GetPaths(m_path);

    return directory_file_paths;
}



// --------------------------------------------------------------------------
//
// DictionaryPackEntry
//
// --------------------------------------------------------------------------

DictionaryPackEntry::DictionaryPackEntry(std::string path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras)
    :   PackEntry(std::move(path), true),
        m_dictionaryExtras(GetNonNullExtras(std::move(dictionary_extras)))
{
}


DictionaryPackEntryExtras* DictionaryPackEntry::GetDictionaryExtras()
{
    return m_dictionaryExtras.get();
}


std::vector<std::string> DictionaryPackEntry::GetAssociatedFilePaths() const
{
    std::vector<std::string> file_paths = PackEntry::GetAssociatedFilePaths();

    if( m_dictionaryExtras->value_set_images )
    {
        if( m_valueSetImageFilePaths == nullptr )
        {
            const std::unique_ptr<const CDataDict> dictionary = CDataDict::InstantiateAndOpen(m_path, true);

            m_valueSetImageFilePaths = std::make_unique<std::vector<std::string>>();

            DictionaryIterator::Foreach<DictValue>(*dictionary,
                [&](const DictValue& dict_value)
                {
                    AddPotentiallyBlankFilePath(*m_valueSetImageFilePaths, dict_value.GetImageFilePath());
                });
        }

        VectorHelpers::Append(file_paths, *m_valueSetImageFilePaths);
    }

    return file_paths;
}



// --------------------------------------------------------------------------
//
// FormPackEntry
//
// --------------------------------------------------------------------------

FormPackEntry::FormPackEntry(std::string path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras)
    :   PackEntry(std::move(path), true)
{
    CDEFormFile form_file;

    if( !form_file.Open(UTF8_TODO::GetCString(m_path), true) )
        throw ApplicationFileLoadException(m_path);

    m_dictionaryPackEntry = std::make_unique<DictionaryPackEntry>(UTF8_TODO::GetUtf8(form_file.GetDictionaryFilename()), std::move(dictionary_extras));
}


DictionaryPackEntryExtras* FormPackEntry::GetDictionaryExtras()
{
    return m_dictionaryPackEntry->GetDictionaryExtras();
}


std::vector<std::string> FormPackEntry::GetAssociatedFilePaths() const
{
    return VectorHelpers::Concatenate(PackEntry::GetAssociatedFilePaths(), m_dictionaryPackEntry->GetAssociatedFilePaths());
}



// --------------------------------------------------------------------------
//
// TabSpecPackEntry
//
// --------------------------------------------------------------------------

TabSpecPackEntry::TabSpecPackEntry(std::string path, std::shared_ptr<DictionaryPackEntryExtras> dictionary_extras)
    :   PackEntry(std::move(path), true)
{
    CSpecFile specfile;

    if( specfile.Open(UTF8_TODO::GetCString(m_path), CFile::modeRead) )
    {
        std::vector<std::string> dictionary_file_paths = GetFileNameArrayFromSpecFile(specfile, CSPRO_DICTS);
        specfile.Close();

        if( dictionary_file_paths.size() == 1 )
        {
            m_dictionaryPackEntry = std::make_unique<DictionaryPackEntry>(std::move(dictionary_file_paths.front()), std::move(dictionary_extras));
            return;
        }
    }

    throw ApplicationFileLoadException(m_path);
}


DictionaryPackEntryExtras* TabSpecPackEntry::GetDictionaryExtras()
{
    return m_dictionaryPackEntry->GetDictionaryExtras();
}


std::vector<std::string> TabSpecPackEntry::GetAssociatedFilePaths() const
{
    return VectorHelpers::Concatenate(PackEntry::GetAssociatedFilePaths(), m_dictionaryPackEntry->GetAssociatedFilePaths());
}



// --------------------------------------------------------------------------
//
// PffPackEntry
//
// --------------------------------------------------------------------------

PffPackEntry::PffPackEntry(std::string path, std::shared_ptr<PffPackEntryExtras> pff_extras)
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


std::vector<std::string> PffPackEntry::GetAssociatedFilePaths() const
{
    std::vector<std::string> file_paths = PackEntry::GetAssociatedFilePaths();

    auto get_pff = [&]() -> PFF&
    {
        if( m_pff == nullptr )
        {
            auto pff = std::make_unique<PFF>(UTF8_TODO::GetCString(m_path));

            if( !pff->LoadPifFile(true) )
                throw ApplicationFileLoadException(m_path);

            m_pff = std::move(pff);
        }

        return *m_pff;
    };

    auto add_connection_string_data = [&](std::vector<std::string>& destination_file_paths, const ConnectionString& connection_string)
    {
        VectorHelpers::Append(destination_file_paths, DataRepositoryHelpers::GetAssociatedFileList(connection_string, true));
    };


    if( m_pffExtras->input_data )
    {
        if( m_inputDataFilePaths == nullptr )
        {
            auto input_data_file_paths = std::make_unique<std::vector<std::string>>();

            for( const ConnectionString& input_connection_string : get_pff().GetInputDataConnectionStrings() )
                add_connection_string_data(*input_data_file_paths, input_connection_string);

            m_inputDataFilePaths = std::move(input_data_file_paths);
        }

        VectorHelpers::Append(file_paths, *m_inputDataFilePaths);
    }


    if( m_pffExtras->external_dictionary_data )
    {
        if( m_externalDictionaryDataFilePaths == nullptr )
        {
            auto external_dictionary_data_file_paths = std::make_unique<std::vector<std::string>>();

            for( const auto& [dictionary_name, connection_string] : get_pff().GetExternalDataConnectionStrings() )
                add_connection_string_data(*external_dictionary_data_file_paths, connection_string);

            m_externalDictionaryDataFilePaths = std::move(external_dictionary_data_file_paths);
        }

        VectorHelpers::Append(file_paths, *m_externalDictionaryDataFilePaths);
    }


    if( m_pffExtras->user_files )
    {
        if( m_userFilePaths == nullptr )
        {
            auto user_file_paths = std::make_unique<std::vector<std::string>>();

            for( const CString& file_path : get_pff().GetUserFiles() )
            {
                if( PortableFunctions::FileIsRegular(file_path) )
                    user_file_paths->emplace_back(UTF8_TODO::GetUtf8(file_path));
            }

            m_userFilePaths = std::move(user_file_paths);
        }

        VectorHelpers::Append(file_paths, *m_userFilePaths);
    }

    return file_paths;
}


// --------------------------------------------------------------------------
//
// ApplicationPackEntry
//
// --------------------------------------------------------------------------

ApplicationPackEntry::ApplicationPackEntry(std::string path)
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
    application.Open(UTF8_TODO::GetWide(m_path), true, false);

    // application properties
    AddPotentiallyBlankFilePath(m_applicationFilePaths, application.GetApplicationPropertiesFilePath());

    // form files
    for( const std::string& form_file_path : application.GetFormFilePaths() )
        m_formPackEntries.emplace_back(form_file_path, get_dictionary_extras());

    // tab specs
    for( const std::string& table_spec_file_path : application.GetTableSpecFilePaths() )
        m_tabSpecPackEntries.emplace_back(table_spec_file_path, get_dictionary_extras());

    // external dictionaries
    for( const std::string& dictionary_file_path : application.GetExternalDictionaryFilePaths() )
        m_externalDictionaryPackEntries.emplace_back(dictionary_file_path, get_dictionary_extras());

    // code files
    for( const CodeFile& code_file : application.GetCodeFiles() )
        m_applicationFilePaths.emplace_back(code_file.GetFilePath());

    // message files
    for( const AppMessageFile& app_message_file : application.GetMessageFiles() )
        m_applicationFilePaths.emplace_back(app_message_file.GetFilePath());

    // reports
    for( const ReportFile& report_file : application.GetReportFiles() )
        m_applicationFilePaths.emplace_back(report_file.GetFilePath());

    // question text
    AddPotentiallyBlankFilePath(m_applicationFilePaths, application.GetQuestionTextFilePath());

    // resources
    for( const AppResource& resource : application.GetResources() )
        m_resourcesAndEvaluatedFilePaths.emplace_back(resource, nullptr);

    // PFF
    std::string expected_pff_file_path = PortableFunctions::PathAppendFileExtension(!m_tabSpecPackEntries.empty() ? m_path : PortableFunctions::PathRemoveFileExtension(m_path),
                                                                                    FileExtensions::Pff);

    if( PortableFunctions::FileIsRegular(expected_pff_file_path) )
        m_pffPackEntry = std::make_unique<PffPackEntry>(std::move(expected_pff_file_path), nullptr);
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


std::vector<std::string> ApplicationPackEntry::GetAssociatedFilePaths() const
{
    std::vector<std::string> file_paths = PackEntry::GetAssociatedFilePaths();

    // form files
    for( const FormPackEntry& form_pack_entry : m_formPackEntries )
        VectorHelpers::Append(file_paths, form_pack_entry.GetAssociatedFilePaths());

    // tab specs
    for( const TabSpecPackEntry& tab_spec_pack_entry : m_tabSpecPackEntries )
        VectorHelpers::Append(file_paths, tab_spec_pack_entry.GetAssociatedFilePaths());

    // external dictionaries
    for( const DictionaryPackEntry& dictionary_pack_entry : m_externalDictionaryPackEntries )
        VectorHelpers::Append(file_paths, dictionary_pack_entry.GetAssociatedFilePaths());

    // application files
    VectorHelpers::Append(file_paths, m_applicationFilePaths);

    // resources
    if( m_applicationExtras.resources )
    {
        for( auto& [resource, evaluated_file_paths] : m_resourcesAndEvaluatedFilePaths )
        {
            if( evaluated_file_paths == nullptr )
            {
                try
                {
                    evaluated_file_paths = std::make_unique<std::vector<std::string>>(resource.GetEvaluatedPaths(false));
                    VectorHelpers::Append(file_paths, *evaluated_file_paths);
                }
                catch(...) { ASSERT(false); } // an exception would be thrown if the resource's filename filter was invalid
            }
        }
    }

    // PFF
    if( m_applicationExtras.pff && m_pffPackEntry != nullptr )
        VectorHelpers::Append(file_paths, m_pffPackEntry->GetAssociatedFilePaths());

    return file_paths;
}
