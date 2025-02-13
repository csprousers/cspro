#include "stdafx.h"
#include "DataFileFilterManager.h"


const DataFileFilterManager& DataFileFilterManager::Get(const UseType use_type, const bool add_only_readable_types)
{
    static std::unique_ptr<DataFileFilterManager> data_file_filter_managers[4];

    const size_t index = ( ( use_type == UseType::FileChooserDlg ) ? 2 : 0 ) +  ( add_only_readable_types ? 1 : 0 );
    auto& data_file_filter_manager = data_file_filter_managers[index];

    if( data_file_filter_manager == nullptr )
        data_file_filter_manager.reset(new DataFileFilterManager(use_type, add_only_readable_types));

    return *data_file_filter_manager;
}


DataFileFilterManager::DataFileFilterManager(const UseType use_type, const bool add_only_readable_types)
    :   m_useType(use_type)
{
    constexpr const TCHAR* FilterFormatter = _T("%s Files (*.%s)|*.%s|");
    std::wstring all_cspro_extensions;

    auto add_type = [&](const DataRepositoryType type, const bool cspro_type = true, const bool force_extension = true)
    {
        const TCHAR* const extension = DataRepositoryTypeDefaultExtensions[static_cast<size_t>(type)];

        auto filter = std::make_shared<DataFileFilter>(DataFileFilter { type, extension, force_extension });
        const TCHAR* const type_name = ToString(type);

        m_filters.emplace_back(filter);

        if( m_useType == UseType::FileChooserDlg )
        {
            m_filterText.append(FormatTextCS2WS(FilterFormatter, type_name, extension, extension));

            if( cspro_type )
                SO::AppendWithSeparator(all_cspro_extensions, extension, _T(";*."));
        }

        else
        {
            m_filterNameMap.try_emplace(type_name, filter);
            m_typeNames.emplace_back(type_name);
        }
    };

    // CSPro Data will be added to the filter text later
    if( m_useType == UseType::FileChooserDlg )
    {
        m_filters.emplace_back(CombinedType::CSProData);
    }

    // add the CSPro data files
    add_type(DataRepositoryType::SQLite);
    add_type(DataRepositoryType::EncryptedSQLite);
    add_type(DataRepositoryType::Text, true, false);
    add_type(DataRepositoryType::Json);

    if( m_useType == UseType::FileAssociationsDlg )
    {
        add_type(DataRepositoryType::Null, true, false);
        add_type(DataRepositoryType::Memory, true, false);
    }

    // only add the export types if possibly writing to the file
    if( !add_only_readable_types )
    {
        add_type(DataRepositoryType::Excel, false);
        add_type(DataRepositoryType::CommaDelimited, false);
        add_type(DataRepositoryType::SemicolonDelimited, false);
        add_type(DataRepositoryType::TabDelimited, false);
        add_type(DataRepositoryType::R, false);
        add_type(DataRepositoryType::SAS, false);
        add_type(DataRepositoryType::SPSS, false);
        add_type(DataRepositoryType::Stata, false);
    }

    // add CSPro Data to the filter text
    if( m_useType == UseType::FileChooserDlg )
    {
        m_filterText.insert(0, FormatTextCS2WS(FilterFormatter, _T("CSPro Data"), all_cspro_extensions.c_str(), all_cspro_extensions.c_str()));

        // add the All Files filter
        m_filters.emplace_back(CombinedType::AllFiles);

        m_filterText.append(FormatTextCS2WS(FilterFormatter, _T("All"), _T("*"), _T("*")));
        m_filterText.push_back('|');
    }
}


size_t DataFileFilterManager::GetFilterIndex(const ConnectionString& connection_string) const
{
    ASSERT(m_useType == UseType::FileChooserDlg);

    const std::wstring extension = PortableFunctions::PathGetFileExtension(connection_string.GetFilename());

    const auto& filter_search = std::find_if(m_filters.cbegin(), m_filters.cend(),
        [&](const auto& filter)
        {
            if( std::holds_alternative<std::shared_ptr<const DataFileFilter>>(filter) )
            {
                return SO::EqualsNoCase(extension, std::get<std::shared_ptr<const DataFileFilter>>(filter)->extension);
            }            

            // select All Files if the file was not found
            else
            {
                return ( std::get<CombinedType>(filter) == CombinedType::AllFiles );
            }
        });

    ASSERT(filter_search != m_filters.cend());
    return std::distance(m_filters.cbegin(), filter_search);
}


std::optional<size_t> DataFileFilterManager::GetFilterIndex(DataRepositoryType type) const
{
    ASSERT(m_useType == UseType::FileChooserDlg);

    const auto& filter_search = std::find_if(m_filters.cbegin(), m_filters.cend(),
        [&](const auto& filter)
        {
            return ( std::holds_alternative<std::shared_ptr<const DataFileFilter>>(filter) &&
                     std::get<std::shared_ptr<const DataFileFilter>>(filter)->type == type );
        });

    if( filter_search != m_filters.cend() )
    {
        return std::distance(m_filters.cbegin(), filter_search);
    }

    else
    {
        return std::nullopt;
    }
}


size_t DataFileFilterManager::GetFilterIndex(const CombinedType combined_type) const
{
    ASSERT(m_useType == UseType::FileChooserDlg);

    const auto& filter_search = std::find_if(m_filters.cbegin(), m_filters.cend(),
        [&](const auto& filter)
        {
            return ( std::holds_alternative<CombinedType>(filter) &&
                     std::get<CombinedType>(filter) == combined_type );
        });

    ASSERT(filter_search != m_filters.cend());
    return std::distance(m_filters.cbegin(), filter_search);
}


const DataFileFilter* DataFileFilterManager::GetDataFileFilterFromTypeName(const std::wstring& type_name) const
{
    ASSERT(m_useType == UseType::FileAssociationsDlg);

    const auto& filter_search = m_filterNameMap.find(type_name);

    if( filter_search != m_filterNameMap.cend() )
    {
        return filter_search->second.get();
    }

    else
    {
        return nullptr;
    }
}


void DataFileFilterManager::AdjustConnectionStringFromDataFileFilter(ConnectionString& connection_string, const DataFileFilter& data_file_filter) const
{
    if( !connection_string.IsFilenamePresent() )
        return;
    
    connection_string.m_filename = PortableFunctions::PathReplaceFileExtension(connection_string.m_filename, data_file_filter.extension);
    connection_string.m_dataRepositoryType = data_file_filter.type;
}


bool DataFileFilterManager::AdjustConnectionStringFromFilterIndex(ConnectionString& connection_string, const size_t filter_index) const
{
    ASSERT(m_useType == UseType::FileChooserDlg);

    ASSERT(filter_index < m_filters.size());
    const auto& filter = m_filters[filter_index];

    // there is nothing to adjust if one of the combined entries is chosen or if there is no filename
    if( std::holds_alternative<CombinedType>(filter) || !connection_string.IsFilenamePresent() )
        return false;

    AdjustConnectionStringFromDataFileFilter(connection_string, *std::get<std::shared_ptr<const DataFileFilter>>(filter));

    return true;
}
