#pragma once

#include <zToolsO/PortableFunctions.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/PathHelpers.h>


namespace ConcatenatorHelpers
{
    std::tuple<std::vector<std::optional<uint64_t>>, uint64_t> CalculateFileSizes(const std::vector<ConnectionString>& input_connection_strings,
                                                                                  std::optional<uint64_t> file_size_for_url_based_data_sources);

    std::string GetReporterSourceText(const char* concatenation_target,
                                      const std::vector<ConnectionString>& input_connection_strings, size_t index);
}


inline std::tuple<std::vector<std::optional<uint64_t>>, uint64_t> ConcatenatorHelpers::CalculateFileSizes(const std::vector<ConnectionString>& input_connection_strings,
                                                                                                          const std::optional<uint64_t> file_size_for_url_based_data_sources)
{
    std::vector<std::optional<uint64_t>> file_sizes;
    uint64_t total_file_size = 0;

    for( const ConnectionString& input_connection_string : input_connection_strings )
    {
        std::optional<uint64_t>& file_size = file_sizes.emplace_back();

        if( input_connection_string.HasFilePath() )
        {
            file_size = PortableFunctions::FileSize<std::optional<uint64_t>>(input_connection_string.GetFilePath());
        }

        else if( file_size_for_url_based_data_sources.has_value() && input_connection_string.HasUrl() )
        {
            file_size = *file_size_for_url_based_data_sources;
        }

        if( file_size.has_value() )
            total_file_size += *file_size;
    }

    return std::make_tuple(std::move(file_sizes), total_file_size);
}


inline std::string ConcatenatorHelpers::GetReporterSourceText(const char* const concatenation_target,
                                                              const std::vector<ConnectionString>& input_connection_strings, const size_t index)
{
    return FormatText("%s %d of %d: %s", concatenation_target,
                                         static_cast<int>(index) + 1, static_cast<int>(input_connection_strings.size()),
                                         input_connection_strings[index].ToDisplayString().c_str());
}
