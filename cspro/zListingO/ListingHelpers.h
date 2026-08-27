#pragma once

#include <zListingO/zListingO.h>
#include <zListingO/Lister.h>
#include <zToolsO/TextFile.h>

class CDataDict;
class ConnectionString;


namespace Listing
{
    std::unique_ptr<FileIO::TextFile> OpenListingFile(const std::string& file_path, bool append, const char* file_type = "listing");

    std::optional<std::string> CreateDataUri(const ConnectionString& connection_string, const CDataDict& dictionary);

    std::optional<std::string> CreateCaseUri(const std::optional<std::string>& input_data_uri,
                                             const std::optional<std::tuple<std::string, std::string>>& case_key_uuid);
}
