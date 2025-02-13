#pragma once

#include <zExportO/zExportO.h>
#include <zDataO/ConnectionStringProperties.h>
#include <zDataO/DataRepositoryHelpers.h>


constexpr size_t ExportExpansiveListVectorSize = 10000;


// some helper functions
constexpr bool ExportTypeSupportsMultipleRecords(const DataRepositoryType data_repository_type)
{
    return ( data_repository_type == DataRepositoryType::CSProExport ||
             data_repository_type == DataRepositoryType::Excel ||
             data_repository_type == DataRepositoryType::R ||
             data_repository_type == DataRepositoryType::SAS );
}


ZEXPORTO_API const TCHAR* ExportTypeDefaultExtension(DataRepositoryType type);

ZEXPORTO_API std::vector<std::wstring> GetExportFilenames(const ConnectionString& connection_string);
