#include "stdafx.h"
#include "DataRepositoryHelpers.h"
#include "EncryptedSQLiteRepository.h"
#include "ExportWriterRepository.h"
#include "JsonRepository.h"
#include "SQLiteRepository.h"
#include "TextRepository.h"


std::unique_ptr<CDataDict> DataRepositoryHelpers::GetEmbeddedDictionary(const ConnectionString& connection_string)
{
    if( connection_string.IsFilenamePresent() && PortableFunctions::FileIsRegular(connection_string.GetFilename()) )
    {
        try
        {
            if( connection_string.GetType() == DataRepositoryType::SQLite )
            {
                return SQLiteRepository::GetEmbeddedDictionary(connection_string);
            }

            else if( connection_string.GetType() == DataRepositoryType::EncryptedSQLite )
            {
                return EncryptedSQLiteRepository::GetEmbeddedDictionary(connection_string);
            }
        }

        catch( const CSProException& )
        {
            // ignore errors
        }
    }

    return nullptr;
}


void DataRepositoryHelpers::RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string)
{
    ASSERT(old_connection_string.GetType() == new_connection_string.GetType());
    ASSERT(old_connection_string.IsFilenamePresent() == new_connection_string.IsFilenamePresent());

    if( !old_connection_string.IsFilenamePresent() )
        return;

    ASSERT(old_connection_string.GetFilename() != new_connection_string.GetFilename());

    if( old_connection_string.GetType() == DataRepositoryType::Json )
    {
        JsonRepository::RenameRepository(old_connection_string, new_connection_string);
    }

    else if( old_connection_string.GetType() == DataRepositoryType::Text )
    {
        TextRepository::RenameRepository(old_connection_string, new_connection_string);
    }

    else if( IsTypeExportWriter(old_connection_string.GetType()) )
    {
        ExportWriterRepository::RenameRepository(old_connection_string, new_connection_string);
    }

    else if( ( PortableFunctions::FileIsRegular(new_connection_string.GetFilename()) && !PortableFunctions::FileDelete(new_connection_string.GetFilename()) ) ||
             ( PortableFunctions::FileIsRegular(old_connection_string.GetFilename()) && !PortableFunctions::FileRename(old_connection_string.GetFilename(), new_connection_string.GetFilename()) ) )
    {
        throw DataRepositoryException::RenameRepositoryError();
    }
}


std::vector<std::wstring> DataRepositoryHelpers::GetAssociatedFileList(const ConnectionString& connection_string, bool include_only_files_that_exist)
{
    std::vector<std::wstring> associated_files;

    if( connection_string.GetType() == DataRepositoryType::Json )
    {
        associated_files = JsonRepository::GetAssociatedFileList(connection_string);
    }

    else if( connection_string.GetType() == DataRepositoryType::Text )
    {
        associated_files = TextRepository::GetAssociatedFileList(connection_string);
    }

    else if( IsTypeExportWriter(connection_string.GetType()) )
    {
        associated_files = ExportWriterRepository::GetAssociatedFileList(connection_string);
    }

    else if( connection_string.IsFilenamePresent() )
    {
        associated_files.emplace_back(connection_string.GetFilename());
    }

    // filter out files that don't exist
    if( include_only_files_that_exist )
    {
        for( auto itr = associated_files.begin(); itr != associated_files.end(); )
        {
            if( PortableFunctions::FileIsRegular(*itr) )
            {
                ++itr;
            }

            else
            {
                itr = associated_files.erase(itr);
            }
        }
    }

    return associated_files;
}


sqlite3* DataRepositoryHelpers::GetSqliteDatabase(DataRepository& data_repository)
{
    DataRepository& real_data_repository = data_repository.GetRealRepository();

    if( IsTypeSQLiteOrDerived(real_data_repository.GetRepositoryType()) )
    {
        return assert_cast<SQLiteRepository&>(real_data_repository).GetSqlite();
    }

    else if( DoesTypeUseIndexableText(real_data_repository.GetRepositoryType()) )
    {
        return assert_cast<IndexableTextRepository&>(real_data_repository).GetIndexSqlite();
    }

    else
    {
        return nullptr;
    }
}
