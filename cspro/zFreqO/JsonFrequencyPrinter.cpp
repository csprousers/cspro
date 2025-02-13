#include "stdafx.h"
#include "JsonFrequencyPrinter.h"


JsonFrequencyPrinter::JsonFrequencyPrinter()
    :   m_jsonWriter(nullptr)
{
}


JsonFrequencyPrinter::JsonFrequencyPrinter(JsonWriter& json_writer)
    :   m_jsonWriter(&json_writer)
{
}


void JsonFrequencyPrinter::Print(const FrequencyTable& frequency_table)
{
    ASSERT(m_jsonWriter != nullptr);

    m_jsonWriter->BeginObject();

    // print the symbol name (for named frequencies)
    m_jsonWriter->WriteIfNotBlank(JK::name, frequency_table.frequency_name);

    // print the heading and titles
    m_jsonWriter->WriteIfNotEmpty(JK::heading, frequency_table.frequency_printer_options.GetHeadings());
    m_jsonWriter->WriteIfNotEmpty(JK::titles, frequency_table.titles);

    // print information about the variable and value set
    m_jsonWriter->BeginObject(JK::variable);

    if( frequency_table.dict_item != nullptr )
    {
        m_jsonWriter->Write(JK::name, frequency_table.dict_item->GetName())
                     .Write(JK::label, frequency_table.dict_item->GetLabel());
    }

    else
    {
        m_jsonWriter->Write(JK::name, frequency_table.symbol_name);
    }

    m_jsonWriter->EndObject();

    if( frequency_table.dict_value_set != nullptr )
    {
        m_jsonWriter->BeginObject(JK::valueSet)
                     .Write(JK::name, frequency_table.dict_value_set->GetName())
                     .Write(JK::label, frequency_table.dict_value_set->GetLabel())
                     .EndObject();
    }

    // print the frequency data as long as NOFREQ wasn't requested
    // (or if NOFREQ was requested but STAT wasn't, which is a contradictory setting pair)
    if( !frequency_table.frequency_printer_options.GetShowNoFrequencies() || !frequency_table.table_statistics.has_value() )
        PrintRowsAndTotal(frequency_table);

    // print the statistics
    if( frequency_table.table_statistics.has_value() )
        PrintStatistics(frequency_table);

    m_jsonWriter->EndObject();
}


void JsonFrequencyPrinter::PrintRowsAndTotal(const FrequencyTable& frequency_table)
{
    ASSERT(m_jsonWriter != nullptr);

    // frequency rows
    m_jsonWriter->BeginArray(JK::rows);

    for( size_t i = 0; i < frequency_table.frequency_rows.size(); ++i )
    {
        const FrequencyRow& frequency_row = frequency_table.frequency_rows[i];
        const FrequencyRowStatistics& frequency_row_statistics = frequency_table.frequency_row_statistics[i];

        m_jsonWriter->BeginObject();

        m_jsonWriter->WriteIfNotBlank(JK::label, frequency_row.display_label);

        m_jsonWriter->WriteArray(JK::values, frequency_row.values,
            [&](const std::variant<double, std::wstring>& value)
            {
                m_jsonWriter->WriteEngineValue(value);
            });

        if( frequency_table.dict_value_set != nullptr )
        {
            m_jsonWriter->Write(JK::outOfRange, frequency_row.mark_as_out_of_value_set)
                         .Write(JK::overlappingRange, frequency_row.mark_as_value_appearing_in_multiple_rows);
        }

        PrintCountAndPercents(frequency_row.count, frequency_row_statistics);

        m_jsonWriter->EndObject();
    }

    m_jsonWriter->EndArray();

    // total row
    m_jsonWriter->BeginObject(JK::total);
    PrintCountAndPercents(frequency_table.total_count, FPH::CreateTotalFrequencyRowStatistics(frequency_table, true));
    m_jsonWriter->EndObject();
}


void JsonFrequencyPrinter::PrintCountAndPercents(double count, const FrequencyRowStatistics& frequency_row_statistics)
{
    ASSERT(m_jsonWriter != nullptr);

    auto print = [&](const double count, const double percent, const std::optional<double>& net_percent)
    {
        m_jsonWriter->Write(JK::count, count)
                     .Write(JK::percent, percent)
                     .WriteIfHasValue(JK::netPercent, net_percent);
    };

    print(count, frequency_row_statistics.percent_against_total, frequency_row_statistics.percent_against_non_blank_total);

    if( frequency_row_statistics.cumulative_count.has_value() )
    {
        m_jsonWriter->BeginObject(JK::cumulative);
        print(*frequency_row_statistics.cumulative_count, *frequency_row_statistics.cumulative_percent_against_total, frequency_row_statistics.cumulative_percent_against_non_blank_total);
        m_jsonWriter->EndObject();
    }
}


void JsonFrequencyPrinter::PrintStatistics(const FrequencyTable& frequency_table)
{
    ASSERT(m_jsonWriter != nullptr);

    ASSERT(frequency_table.table_statistics.has_value());

    m_jsonWriter->BeginObject(JK::statistics);

    // numeric statistics
    if( std::holds_alternative<FrequencyNumericStatistics>(*frequency_table.table_statistics) )
    {
        const FrequencyNumericStatistics& table_statistics = std::get<FrequencyNumericStatistics>(*frequency_table.table_statistics);

        if( frequency_table.frequency_printer_options.GetShowStatistics() )
        {
            // categories
            PrintStatisticsCategories(table_statistics.number_defined_categories,
                [&]()
                {
                    if( !table_statistics.non_blank_special_values_used.empty() )
                    {
                        m_jsonWriter->BeginArray(JK::specialValues);

                        for( const double value : table_statistics.non_blank_special_values_used )
                            m_jsonWriter->WriteEngineValue(value);

                        m_jsonWriter->EndArray();
                    }
                });

            // min / max
            m_jsonWriter->WriteIfHasValue(JK::minimum, table_statistics.min_value)
                         .WriteIfHasValue(JK::maximum, table_statistics.max_value);

            // mean / median / mode
            m_jsonWriter->WriteIfHasValue(JK::mean, table_statistics.mean)
                         .WriteIfHasValue(JK::median, table_statistics.median)
                         .WriteIfHasValue(JK::medianInterpolated, table_statistics.median_interpolated)
                         .WriteIfHasValue(JK::mode, table_statistics.mode_value);

            // variance / standard deviation
            m_jsonWriter->WriteIfHasValue(JK::variance, table_statistics.variance)
                         .WriteIfHasValue(JK::standardDeviation, table_statistics.standard_deviation);
        }

        // percentiles
        if( table_statistics.percentiles.has_value() )
        {
            m_jsonWriter->WriteObjects(JK::percentiles, *table_statistics.percentiles,
                [&](const FrequencyNumericStatistics::Percentile& percentile)
                {
                    m_jsonWriter->Write(JK::percentile, percentile.percentile * 100)
                                 .Write(JK::continuous, percentile.value_type2)
                                 .Write(JK::discontinuous, percentile.value_type6);
                });
        }
    }


    // alphanumeric statistics
    else
    {
        ASSERT(std::holds_alternative<FrequencyAlphanumericStatistics>(*frequency_table.table_statistics));

        if( frequency_table.frequency_printer_options.GetShowStatistics() )
        {
            const FrequencyAlphanumericStatistics& table_statistics = std::get<FrequencyAlphanumericStatistics>(*frequency_table.table_statistics);

            PrintStatisticsCategories(table_statistics.number_defined_categories, []() { });
        }
    }    

    m_jsonWriter->EndObject();
}


template<typename CF>
void JsonFrequencyPrinter::PrintStatisticsCategories(const size_t number_defined_categories, const CF callback_function)
{
    ASSERT(m_jsonWriter != nullptr);

    m_jsonWriter->BeginObject(JK::categories);
    m_jsonWriter->Write(JK::count, number_defined_categories);
    callback_function();
    m_jsonWriter->EndObject();
}



// a routine to write the JSON for a single FrequencyTable
void FrequencyTable::WriteJson(JsonWriter& json_writer) const
{
    JsonFrequencyPrinter json_frequency_printer(json_writer);
    json_frequency_printer.Print(*this);
}
