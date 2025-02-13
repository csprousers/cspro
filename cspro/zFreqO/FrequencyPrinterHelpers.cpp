#include "stdafx.h"
#include "FrequencyPrinterHelpers.h"


size_t FPH::GetNumberDecimalsUsed(wstring_view text)
{
    size_t period_pos = text.find('.');
    return ( period_pos == wstring_view::npos ) ? 0 : ( text.length() - period_pos - 1 );
};


size_t FPH::GetNumberDecimalsUsed(const std::vector<std::wstring>& texts)
{
    size_t decimals = 0;

    for( const std::wstring& text : texts )
        decimals = std::max(decimals, GetNumberDecimalsUsed(text));

    return decimals;
}


void FPH::EnsureValueHasMinimumDecimals(std::wstring& text, size_t decimals)
{
    size_t period_pos = text.find('.');

    if( period_pos == std::wstring::npos )
    {
        if( decimals == 0 )
            return;

        period_pos = text.length();
        text.push_back('.');
    }

    size_t number_decimals_specified = text.length() - period_pos - 1;

    for( ; number_decimals_specified < decimals; ++number_decimals_specified )
        text.push_back('0');
}


std::wstring FPH::GetFormattedValue(double value, const CDictItem* dict_item)
{
    if( dict_item == nullptr || IsSpecial(value) )
    {
        return DoubleToString(value);
    }

    else
    {
        std::wstring formatted_value = dvaltochar(value, dict_item->GetCompleteLen(), dict_item->GetDecimal(), false, true);
        return SO::MakeTrimLeft(formatted_value);
    }
}


std::wstring FPH::GetValueWithMinimumDecimals(double value, const CDictItem* dict_item)
{
    std::wstring formatted_value = DoubleToString(value);

    if( dict_item != nullptr && dict_item->GetDecimal() > 0 && !IsSpecial(value) )
        EnsureValueHasMinimumDecimals(formatted_value, dict_item->GetDecimal());

    return formatted_value;
}


std::wstring FPH::GetFrequencyTableName(const FrequencyTable& frequency_table)
{
    return ( frequency_table.dict_value_set != nullptr ) ? CS2WS(frequency_table.dict_value_set->GetName()) :
           ( frequency_table.dict_item != nullptr )      ? CS2WS(frequency_table.dict_item->GetName()) :
                                                           frequency_table.symbol_name;
}


bool FPH::ShowFrequencyTableNetPercents(const FrequencyTable& frequency_table)
{
    return ( frequency_table.frequency_printer_options.GetShowNetPercents() &&
             frequency_table.total_count != frequency_table.total_non_blank_count );
}


bool FPH::FrequencyTableValuesAreNumeric(const FrequencyTable& frequency_table)
{
    return ( !frequency_table.frequency_rows.empty() &&
             std::holds_alternative<double>(frequency_table.frequency_rows.front().values.front()) );
}


FrequencyRowStatistics FPH::CreateTotalFrequencyRowStatistics(const FrequencyTable& frequency_table, bool include_cumulative_columns)
{
    FrequencyRowStatistics total_row_statistics;

    total_row_statistics.percent_against_total = ( frequency_table.total_count > 0 ) ? 100 : 0;

    if( include_cumulative_columns && !frequency_table.has_multiple_labels_per_value )
    {
        total_row_statistics.cumulative_count = frequency_table.total_count;
        total_row_statistics.cumulative_percent_against_total = total_row_statistics.percent_against_total;
    }

    return total_row_statistics;
}


std::vector<std::wstring> FPH::GetFormattedPercentilePercents(const FrequencyNumericStatistics& table_statistics)
{
    constexpr size_t MaxDecimalsToDisplay = 2;
    ASSERT(table_statistics.percentiles.has_value());
    std::vector<std::wstring> percentile_strings;
    size_t decimals_necessary = 0;

    for( const auto& percentile : *table_statistics.percentiles )
    {
        const std::wstring& text = percentile_strings.emplace_back(DoubleToString(percentile.percentile * 100, std::nullopt, MaxDecimalsToDisplay));
        decimals_necessary = std::max(decimals_necessary, GetNumberDecimalsUsed(text));
    }

    if( decimals_necessary != 0 )
    {
        for( std::wstring& percentile_string : percentile_strings )
            EnsureValueHasMinimumDecimals(percentile_string, decimals_necessary);
    }

    return percentile_strings;
}
