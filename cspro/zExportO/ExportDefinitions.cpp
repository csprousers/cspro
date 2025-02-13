#include "stdafx.h"
#include "ExportDefinitions.h"
#include "CSProExportWriter.h"
#include "SasExportWriter.h"


const TCHAR* ExportTypeDefaultExtension(const DataRepositoryType type)
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


std::vector<std::wstring> GetExportFilenames(const ConnectionString& connection_string)
{
    ASSERT(DataRepositoryHelpers::IsTypeExportWriter(connection_string.GetType()));

    std::vector<std::wstring> filenames;

    switch( connection_string.GetType() )
    {
        case DataRepositoryType::CSProExport:
            filenames.emplace_back(CSProExportWriter::GetDataConnectionString(connection_string).GetFilename());
            filenames.emplace_back(CSProExportWriter::GetDictionaryPath(connection_string));
            break;

        case DataRepositoryType::SAS:
            filenames.emplace_back(connection_string.GetFilename());
            filenames.emplace_back(SasExportWriter::GetSyntaxPath(connection_string));
            break;

        default:
            filenames.emplace_back(connection_string.GetFilename());
            break;

    }

    return filenames;
}
