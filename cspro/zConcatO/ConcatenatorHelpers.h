#pragma once

#include <zToolsO/PortableFunctions.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/PathHelpers.h>


namespace ConcatenatorHelpers
{
    std::tuple<std::vector<std::optional<uint64_t>>, uint64_t> CalculateFileSizes(const std::vector<ConnectionString>& input_connection_strings);
    ConnectionString GetTemporaryOutputConnectionString(const ConnectionString& output_connection_string);
    std::wstring GetReporterSourceText(const std::vector<ConnectionString>& input_connection_strings, size_t index);
}


inline std::tuple<std::vector<std::optional<uint64_t>>, uint64_t> ConcatenatorHelpers::CalculateFileSizes(const std::vector<ConnectionString>& input_connection_strings)
{
    std::vector<std::optional<uint64_t>> file_sizes;
    uint64_t total_file_size = 0;

    for( const ConnectionString& input_connection_string : input_connection_strings )
    {
        const std::optional<uint64_t>& file_size = file_sizes.emplace_back(
            PortableFunctions::FileSize<std::optional<uint64_t>>(input_connection_string.GetFilename()));

        if( file_size.has_value() )
            total_file_size += *file_size;
    }

    return std::make_tuple(std::move(file_sizes), total_file_size);
}


inline ConnectionString ConcatenatorHelpers::GetTemporaryOutputConnectionString(const ConnectionString& output_connection_string)
{
    return PathHelpers::AppendToConnectionStringFilename(output_connection_string, _T("~temp~"));
}


inline std::wstring ConcatenatorHelpers::GetReporterSourceText(const std::vector<ConnectionString>& input_connection_strings, const size_t index)
{
    return FormatTextCS2WS(_T("File %d of %d: %s"),
                           static_cast<int>(index) + 1,
                           static_cast<int>(input_connection_strings.size()),
                           input_connection_strings[index].GetFilename().c_str());
}
