#pragma once

#include <zFreqO/zFreqO.h>
#include <zFreqO/FrequencyPrinterOptions.h>

class DictValueSet;


struct FrequencyRow
{
    std::vector<std::variant<double, std::wstring>> values;
    std::vector<std::wstring> formatted_values;
    const DictValue* dict_value = nullptr;
    double count = 0;
    bool value_is_blank = false;

    // some additional values that can be used for printing
    std::wstring display_label;
    bool mark_as_out_of_value_set = false;
    bool mark_as_value_appearing_in_multiple_rows = false;
};


struct FrequencyRowStatistics
{
    double percent_against_total = 0;
    std::optional<double> cumulative_count;
    std::optional<double> cumulative_percent_against_total;
    std::optional<double> percent_against_non_blank_total;
    std::optional<double> cumulative_percent_against_non_blank_total;
};


struct FrequencyNumericStatistics
{
    std::vector<double> non_blank_special_values_used;
    size_t number_defined_categories = 0;

    std::optional<double> min_value;
    std::optional<double> max_value;

    std::optional<double> mode_value;
    std::optional<double> mode_count;
    std::optional<double> median;
    std::optional<double> median_interpolated;

    struct Percentile
    {
        double percentile;
        double value_type2;
        double value_type6;
    };

    std::optional<std::vector<Percentile>> percentiles;

    std::optional<double> sum_count;
    std::optional<double> product_value_count;
    std::optional<double> product_value_value_count;

    std::optional<double> mean;
    std::optional<double> variance;
    std::optional<double> standard_deviation;
};


struct FrequencyAlphanumericStatistics
{
    size_t number_defined_categories = 0;
};


struct FrequencyTable
{
    std::wstring frequency_name;

    FrequencyPrinterOptions frequency_printer_options;

    std::wstring symbol_name;
    const CDictItem* dict_item = nullptr;
    const DictValueSet* dict_value_set = nullptr;

    std::vector<std::wstring> titles;
    std::map<size_t, std::tuple<std::wstring, std::wstring>> logic_based_titles; // index into titles -> heading + logic

    double total_count = 0;
    double total_non_blank_count = 0;

    bool distinct = false;
    bool has_multiple_labels_per_value = false;

    std::vector<FrequencyRow> frequency_rows;

    std::vector<FrequencyRowStatistics> frequency_row_statistics;

    std::optional<std::variant<FrequencyNumericStatistics, FrequencyAlphanumericStatistics>> table_statistics;

    enum class SpecialFormatting { CenterTitles };
    std::optional<SpecialFormatting> special_formatting;

    // serialization (defined in JsonFrequencyPrinter.cpp)
    ZFREQO_API void WriteJson(JsonWriter& json_writer) const;
};
