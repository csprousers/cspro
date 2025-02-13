#pragma once

#include <zUtilO/ConnectionString.h>


struct DataFileFilter
{
    DataRepositoryType type;
    const TCHAR* extension;
    bool force_extension;
};


class DataFileFilterManager
{
public:
    enum class UseType { FileChooserDlg, FileAssociationsDlg };

    enum class CombinedType { CSProData, AllFiles };

private:
    DataFileFilterManager(UseType use_type, bool add_only_readable_types);

public:
    static const DataFileFilterManager& Get(UseType use_type, bool add_only_readable_types);

    const std::wstring& GetFilterText() const
    {
        ASSERT(m_useType == UseType::FileChooserDlg);
        return m_filterText;
    }

    const std::vector<const TCHAR*>& GetTypeNames() const
    {
        ASSERT(m_useType == UseType::FileAssociationsDlg);
        return m_typeNames;
    }

    size_t GetFilterIndex(const ConnectionString& connection_string) const;
    std::optional<size_t> GetFilterIndex(DataRepositoryType type) const;
    size_t GetFilterIndex(CombinedType combined_type) const;

    const DataFileFilter* GetDataFileFilterFromTypeName(const std::wstring& type_name) const;

    void AdjustConnectionStringFromDataFileFilter(ConnectionString& connection_string, const DataFileFilter& data_file_filter) const;
    bool AdjustConnectionStringFromFilterIndex(ConnectionString& connection_string, size_t filter_index) const;

private:
    const UseType m_useType;
    std::vector<std::variant<CombinedType, std::shared_ptr<const DataFileFilter>>> m_filters;

    std::wstring m_filterText;

    std::map<std::wstring, std::shared_ptr<const DataFileFilter>> m_filterNameMap;
    std::vector<const TCHAR*> m_typeNames;
};
