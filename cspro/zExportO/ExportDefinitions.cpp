#include "stdafx.h"
#include "ExportDefinitions.h"
#include "CSProExportWriter.h"
#include "SasExportWriter.h"


const char* ExportTypeDefaultExtension(const DataRepositoryType type)
{
    switch( type )
    {
        case DataRepositoryType::CommaDelimited:
            return FileExtensions::CSV;

        case DataRepositoryType::SemicolonDelimited:
            return FileExtensions::SemicolonDelimited;

        case DataRepositoryType::TabDelimited:
            return FileExtensions::TabDelimited;

        case DataRepositoryType::Excel:
            return FileExtensions::Excel;

        case DataRepositoryType::CSProExport:
            return FileExtensions::Data::CSProDB;

        case DataRepositoryType::R:
            return FileExtensions::RData;

        case DataRepositoryType::SAS:
            return FileExtensions::SasData;

        case DataRepositoryType::SPSS:
            return FileExtensions::SpssData;

        case DataRepositoryType::Stata:
            return FileExtensions::StataData;

        default:
            throw ProgrammingErrorException();
    }
}


std::vector<std::string> GetExportFilePaths(const ConnectionString& connection_string)
{
    ASSERT(DataRepositoryHelpers::IsTypeExportWriter(connection_string.GetType()));

    std::vector<std::string> file_paths;

    switch( connection_string.GetType() )
    {
        case DataRepositoryType::CSProExport:
            file_paths.emplace_back(CSProExportWriter::GetDataConnectionString(connection_string).GetFilePath());
            file_paths.emplace_back(CSProExportWriter::GetDictionaryFilePath(connection_string));
            break;

        case DataRepositoryType::SAS:
            file_paths.emplace_back(connection_string.GetFilePath());
            file_paths.emplace_back(SasExportWriter::GetSyntaxPath(connection_string));
            break;

        default:
            file_paths.emplace_back(connection_string.GetFilePath());
            break;
    }

    return file_paths;
}
