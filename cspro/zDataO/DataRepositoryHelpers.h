#pragma once

#include <zDataO/zDataO.h>
#include <zDataO/DataRepositoryDefines.h>

class CDataDict;
class ConnectionString;
class DataRepository;
struct sqlite3;


namespace DataRepositoryHelpers
{
    constexpr bool IsTypeSQLiteOrDerived(DataRepositoryType data_repository_type);

    constexpr bool DoesTypeContainEmbeddedDictionary(DataRepositoryType data_repository_type);

    constexpr bool DoesTypeUseIndexableText(DataRepositoryType data_repository_type);

    constexpr bool IsTypeExportWriter(DataRepositoryType data_repository_type);

    constexpr bool TypeSupportsUndeletes(DataRepositoryType data_repository_type);

    constexpr bool TypeSupportsDuplicates(DataRepositoryType data_repository_type);

    constexpr bool TypeSupportsSync(DataRepositoryType data_repository_type);

    constexpr bool TypeSupportsRecordSort(DataRepositoryType data_repository_type);

    constexpr bool TypeSupportsIndexedQueriesInBatchOutput(DataRepositoryType data_repository_type);

    constexpr bool TypeWritesToText(DataRepositoryType data_repository_type);

    constexpr bool TypeDoesNotUseFilename(DataRepositoryType data_repository_type);

    // Gets a repository's embedded dictionary, if available, returning null if not.
    ZDATAO_API std::unique_ptr<CDataDict> GetEmbeddedDictionary(const ConnectionString& connection_string);

    // Renames the repository, overwriting any existing repository with the new name.
    ZDATAO_API void RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string);

    // Returns any files associated with the repository, including the data file. Only files that
    // cannot be recreated are returned (e.g., a text data file's index will not be returned).
    ZDATAO_API std::vector<std::wstring> GetAssociatedFileList(const ConnectionString& connection_string, bool include_only_files_that_exist);

    // Returns the SQLite database associated with a repository, or null if none exists.
    ZDATAO_API sqlite3* GetSqliteDatabase(DataRepository& data_repository);
}



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

constexpr bool DataRepositoryHelpers::IsTypeSQLiteOrDerived(const DataRepositoryType data_repository_type)
{
    return ( data_repository_type == DataRepositoryType::SQLite ||
             data_repository_type == DataRepositoryType::EncryptedSQLite );
}
    

constexpr bool DataRepositoryHelpers::DoesTypeContainEmbeddedDictionary(const DataRepositoryType data_repository_type)
{
    return IsTypeSQLiteOrDerived(data_repository_type);
}

    
constexpr bool DataRepositoryHelpers::DoesTypeUseIndexableText(const DataRepositoryType data_repository_type)
{
    return ( data_repository_type == DataRepositoryType::Text ||
             data_repository_type == DataRepositoryType::Json );
}

    
constexpr bool DataRepositoryHelpers::IsTypeExportWriter(const DataRepositoryType data_repository_type)
{
    return ( data_repository_type == DataRepositoryType::CommaDelimited ||
             data_repository_type == DataRepositoryType::SemicolonDelimited ||
             data_repository_type == DataRepositoryType::TabDelimited ||
             data_repository_type == DataRepositoryType::Excel ||
             data_repository_type == DataRepositoryType::CSProExport ||
             data_repository_type == DataRepositoryType::R ||
             data_repository_type == DataRepositoryType::SAS ||
             data_repository_type == DataRepositoryType::SPSS ||
             data_repository_type == DataRepositoryType::Stata );
}
    

constexpr bool DataRepositoryHelpers::TypeSupportsUndeletes(const DataRepositoryType data_repository_type)
{
    return ( DataRepositoryHelpers::IsTypeSQLiteOrDerived(data_repository_type) ||
             data_repository_type == DataRepositoryType::Memory ||
             data_repository_type == DataRepositoryType::Json );
}


constexpr bool DataRepositoryHelpers::TypeSupportsDuplicates(const DataRepositoryType data_repository_type)
{
    return ( DataRepositoryHelpers::IsTypeSQLiteOrDerived(data_repository_type) ||
             data_repository_type == DataRepositoryType::Memory ||
             data_repository_type == DataRepositoryType::Json  );
}


constexpr bool DataRepositoryHelpers::TypeSupportsSync(const DataRepositoryType data_repository_type)
{
    return IsTypeSQLiteOrDerived(data_repository_type);
}


constexpr bool DataRepositoryHelpers::TypeSupportsRecordSort(const DataRepositoryType data_repository_type)
{
    return ( data_repository_type == DataRepositoryType::Text ||
             data_repository_type == DataRepositoryType::Null );
}


constexpr bool DataRepositoryHelpers::TypeSupportsIndexedQueriesInBatchOutput(const DataRepositoryType data_repository_type)
{
    return ( IsTypeSQLiteOrDerived(data_repository_type) ||
             data_repository_type == DataRepositoryType::Null ||
             data_repository_type == DataRepositoryType::Memory );
}


constexpr bool DataRepositoryHelpers::TypeWritesToText(const DataRepositoryType data_repository_type)
{
    return ( data_repository_type == DataRepositoryType::Text ||
             data_repository_type == DataRepositoryType::Json ||
             data_repository_type == DataRepositoryType::CommaDelimited ||
             data_repository_type == DataRepositoryType::SemicolonDelimited ||
             data_repository_type == DataRepositoryType::TabDelimited );
}


constexpr bool DataRepositoryHelpers::TypeDoesNotUseFilename(const DataRepositoryType data_repository_type)
{
    return ( data_repository_type == DataRepositoryType::Null ||
             data_repository_type == DataRepositoryType::Memory );
}
