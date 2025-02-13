#pragma once

#include <zListingO/zListingO.h>

class CDataDict;
class ConnectionString;


namespace Listing
{
    std::unique_ptr<CStdioFileUnicode> OpenListingFile(const std::wstring& filename, bool append, const TCHAR* file_type = _T("listing"));

    /// <summary>Gets the current "long" date.</summary>
    ZLISTINGO_API std::wstring GetSystemDate();

    /// <summary>Gets the current time in the format hh:mm:ss.</summary>
    ZLISTINGO_API std::wstring GetSystemTime();

    std::wstring Base64EncodeIfNecessary(const std::wstring& text);

    std::wstring CreateTextViewerUri(const std::wstring& filename);

    std::optional<std::wstring> CreateDataUri(const ConnectionString& connection_string, const CDataDict& dictionary);

    std::optional<std::wstring> CreateCaseUri(const std::optional<std::wstring>& input_data_uri,
                                              const std::optional<std::tuple<std::wstring, std::wstring>>& case_key_uuid);
}
