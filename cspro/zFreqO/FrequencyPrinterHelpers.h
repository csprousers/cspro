#pragma once

#include <zFreqO/FrequencyTable.h>

class CDictItem;


namespace FPH // FPH = frequency printer helpers
{
    constexpr const char* ValueLabel                 = "Value";
    constexpr const char* LabelLabel                 = "Label";
    constexpr const char* TotalLabel                 = "Total";
    constexpr const char* PercentLabel               = "Percent";
    constexpr const char* NetPercentLabel            = "Net Percent";
    constexpr const char* FrequencyLabel             = "Frequency";
    constexpr const char* CumulativeLabel            = "Cumulative";

    constexpr const char* StatisticsLabel            = "Statistics";
    constexpr const char* CategoriesLabel            = "Categories";
    constexpr const char* SpecialValuesLabel         = "Special Values";
    constexpr const char* MinLabel                   = "Min";
    constexpr const char* MaxLabel                   = "Max";

    constexpr const char* MeanLabel                  = "Mean";
    constexpr const char* MedianLabel                = "Median";
    constexpr const char* MedianInterpolatedLabel    = "Median (Interpolated)";
    constexpr const char* ModeLabel                  = "Mode";
    constexpr const char* VarianceLabel              = "Variance";
    constexpr const char* StandardDeviationLabel     = "Standard Deviation";

    constexpr const char* PercentilesLabel           = "Percentiles";
    constexpr const char* DiscontinuousLabel         = "Discontinuous";
    constexpr const char* ContinuousTextLabel        = "Continuous";

    constexpr const char* NoCumulativeColumnsWarning = "No cumulative frequencies are shown because "
                                                       "the same value appears in multiple rows.";


    size_t GetNumberDecimalsUsed(std::string_view text_sv);

    size_t GetNumberDecimalsUsed(const std::vector<std::string>& texts);

    void EnsureValueHasMinimumDecimals(std::string& text, size_t decimals);

    // a function to format a value using the dictionary item's settings
    std::string GetFormattedValue(double value, const CDictItem* dict_item);

    // a function to format a value using DoubleToString but then to ensure that there are
    // at least the number of decimals as specified in the dictionary item's settings
    std::string GetValueWithMinimumDecimals(double value, const CDictItem* dict_item);

    std::string GetFrequencyTableName(const FrequencyTable& frequency_table);

    bool ShowFrequencyTableNetPercents(const FrequencyTable& frequency_table);

    bool FrequencyTableValuesAreNumeric(const FrequencyTable& frequency_table);

    FrequencyRowStatistics CreateTotalFrequencyRowStatistics(const FrequencyTable& frequency_table, bool include_cumulative_columns);

    std::vector<std::string> GetFormattedPercentilePercents(const FrequencyNumericStatistics& table_statistics);
}
