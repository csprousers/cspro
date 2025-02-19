#include "stdafx.h"
#include "ListingHelpers.h"
#include <zToolsO/base64.h>
#include <zUtilO/CustomUri.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <ctime>


std::unique_ptr<FileIO::TextFile> Listing::OpenListingFile(const std::string& file_path, const bool append, const char* const file_type/* = "listing"*/)
{
    auto file = std::make_unique<FileIO::TextFile>(); // TEXT_ENCODING_TODO refactor to use connection strings with properties

    try
    {
        if( append && PortableFunctions::FileIsRegular(file_path) )
        {
            // make sure that the file is UTF-8
            Encoding encoding;

            if( GetFileBOM(UTF8_TODO::GetWide(file_path), encoding) && encoding == Encoding::Ansi )
            {
                if( !CStdioFileUnicode::ConvertAnsiToUTF8(file_path) )
                    throw CSProException("Could not convert the listing file from ANSI to UTF-8.");
            }

            file->OpenForTextWritingAppend(file_path);
        }

        else
        {
            file->OpenForTextWritingCreate(file_path);
        }
    }

    catch( const CSProException& exception )
    {
        throw CSProException("Could not create the %s file '%s': %s", file_type, file_path.c_str(), exception.what());
    }

    return file;
}


std::optional<std::string> Listing::CreateDataUri(const ConnectionString& connection_string, const CDataDict& dictionary)
{
    if( !connection_string.HasResource() )
        return std::nullopt;

    return CustomUri::CreateDataUri(connection_string, &dictionary.GetFilePath());
}


std::optional<std::string> Listing::CreateCaseUri(const std::optional<std::string>& input_data_uri,
                                                  const std::optional<std::tuple<std::string, std::string>>& case_key_uuid)
{
    if( !input_data_uri.has_value() || !case_key_uuid.has_value() )
        return std::nullopt;

    // construct a full link that will open the case in Data Manager
    ASSERT(!std::get<0>(*case_key_uuid).empty());

    const std::string* const case_uuid = std::get<1>(*case_key_uuid).empty() ? nullptr :
                                                                               &std::get<1>(*case_key_uuid);

    return CustomUri::AddCaseToDataUri(*input_data_uri, std::get<0>(*case_key_uuid), case_uuid);
}
