#pragma once

#include <zFreqO/FrequencyTable.h>

class CDictItem;


namespace FPH // FPH = frequency printer helpers
{
    constexpr const TCHAR* ValueLabel                 = _T("Value");
    constexpr const TCHAR* LabelLabel                 = _T("Label");
    constexpr const TCHAR* TotalLabel                 = _T("Total");
    constexpr const TCHAR* PercentLabel               = _T("Percent");
    constexpr const TCHAR* NetPercentLabel            = _T("Net Percent");
    constexpr const TCHAR* FrequencyLabel             = _T("Frequency");
    constexpr const TCHAR* CumulativeLabel            = _T("Cumulative");

    constexpr const TCHAR* StatisticsLabel            = _T("Statistics");
    constexpr const TCHAR* CategoriesLabel            = _T("Categories");
    constexpr const TCHAR* SpecialValuesLabel         = _T("Special Values");
    constexpr const TCHAR* MinLabel                   = _T("Min");
    constexpr const TCHAR* MaxLabel                   = _T("Max");

    constexpr const TCHAR* MeanLabel                   = _T("Mean");
    constexpr const TCHAR* MedianLabel                 = _T("Median");
    constexpr const TCHAR* MedianInterpolatedLabel     = _T("Median (Interpolated)");
    constexpr const TCHAR* ModeLabel                   = _T("Mode");
    constexpr const TCHAR* VarianceLabel               = _T("Variance");
    constexpr const TCHAR* StandardDeviationLabel      = _T("Standard Deviation");

    constexpr const TCHAR* PercentilesLabel           = _T("Percentiles");
    constexpr const TCHAR* DiscontinuousLabel         = _T("Discontinuous");
    constexpr const TCHAR* ContinuousTextLabel        = _T("Continuous");

    constexpr const TCHAR* NoCumulativeColumnsWarning = _T("No cumulative frequencies are shown because ")
                                                        _T("the same value appears in multiple rows.");


    size_t GetNumberDecimalsUsed(wstring_view text);

    size_t GetNumberDecimalsUsed(const std::vector<std::wstring>& texts);

    void EnsureValueHasMinimumDecimals(std::wstring& text, size_t decimals);

    // a function to format a value using the dictionary item's settings
    std::wstring GetFormattedValue(double value, const CDictItem* dict_item);

    // a function to format a value using DoubleToString but then to ensure that there are 
    // at least the number of decimals as specified in the dictionary item's settings
    std::wstring GetValueWithMinimumDecimals(double value, const CDictItem* dict_item);

    std::wstring GetFrequencyTableName(const FrequencyTable& frequency_table);

    bool ShowFrequencyTableNetPercents(const FrequencyTable& frequency_table);

    bool FrequencyTableValuesAreNumeric(const FrequencyTable& frequency_table);

    FrequencyRowStatistics CreateTotalFrequencyRowStatistics(const FrequencyTable& frequency_table, bool include_cumulative_columns);

    std::vector<std::wstring> GetFormattedPercentilePercents(const FrequencyNumericStatistics& table_statistics);
}
