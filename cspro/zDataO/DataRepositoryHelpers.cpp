#include "stdafx.h"
#include "DataRepositoryHelpers.h"
#include "ExportWriterRepository.h"
#include "JsonRepository.h"
#include "SQLiteRepository.h"
#include "TextRepository.h"


void DataRepositoryHelpers::RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string)
{
    ASSERT(old_connection_string.GetType() == new_connection_string.GetType());
    ASSERT(old_connection_string.HasFilePath() == new_connection_string.HasFilePath());

    if( !old_connection_string.HasFilePath() )
        return;

    ASSERT(old_connection_string.GetFilePath() != new_connection_string.GetFilePath());

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

    else if( ( PortableFunctions::FileIsRegular(new_connection_string.GetFilePath()) && !PortableFunctions::FileDelete(new_connection_string.GetFilePath()) ) ||
             ( PortableFunctions::FileIsRegular(old_connection_string.GetFilePath()) && !PortableFunctions::FileRename(old_connection_string.GetFilePath(), new_connection_string.GetFilePath()) ) )
    {
        throw DataRepositoryException::RenameRepositoryError();
    }
}


std::vector<std::string> DataRepositoryHelpers::GetAssociatedFileList(const ConnectionString& connection_string, const bool include_only_files_that_exist)
{
    std::vector<std::string> associated_files;

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

    else if( connection_string.HasFilePath() )
    {
        associated_files.emplace_back(connection_string.GetFilePath());
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

    else if( TypeUsesIndexableText(real_data_repository.GetRepositoryType()) )
    {
        return assert_cast<IndexableTextRepository&>(real_data_repository).GetIndexSqlite();
    }

    else
    {
        return nullptr;
    }
}
