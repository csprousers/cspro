#include "stdafx.h"
#include "FrequencyPrinterEntry.h"
#include "FrequencyPrinter.h"
#include <zToolsO/FloatingPointMath.h>
#include <zDictO/ValueProcessor.h>


template<typename ValueType, typename CountType>
void GenerateStatistics(FrequencyTable& frequency_table, std::vector<std::tuple<ValueType, CountType>>& non_zero_counts);



template<typename ValueType, typename CountType>
FrequencyPrinterEntry<ValueType, CountType>::FrequencyPrinterEntry(std::shared_ptr<const FrequencyCounter<ValueType, CountType>> frequency_counter,
                                                                   std::wstring symbol_name)
    :   m_frequencyCounter(std::move(frequency_counter)),
        m_symbolName(std::move(symbol_name)),
        m_dictItem(nullptr),
        m_currentDictValueSet(nullptr)        
{
}


template<typename ValueType, typename CountType>
FrequencyPrinterEntry<ValueType, CountType>::FrequencyPrinterEntry(std::shared_ptr<const FrequencyCounter<ValueType, CountType>> frequency_counter,
                                                                   const CDictItem& dict_item, const DictValueSet* current_dict_value_set,
                                                                   std::optional<size_t> record_occurrence, std::optional<size_t> item_subitem_occurrence)
    :   m_frequencyCounter(std::move(frequency_counter)),
        m_symbolName(CS2WS(dict_item.GetName())),
        m_dictItem(&dict_item),
        m_currentDictValueSet(current_dict_value_set),        
        m_recordOccurrence(std::move(record_occurrence)),
        m_itemSubitemOccurrence(std::move(item_subitem_occurrence))
{
}


template<typename ValueType, typename CountType>
bool FrequencyPrinterEntry<ValueType, CountType>::Compare(const FrequencyPrinterEntry& rhs) const
{
    // this method should only be used for sorting occurrences
    ASSERT(m_symbolName == rhs.m_symbolName && m_dictItem == rhs.m_dictItem && m_currentDictValueSet == rhs.m_currentDictValueSet);

    return ( m_recordOccurrence == rhs.m_recordOccurrence ) ? ( m_itemSubitemOccurrence < rhs.m_itemSubitemOccurrence ) :
                                                              ( m_recordOccurrence < rhs.m_recordOccurrence );
}


template<typename ValueType, typename CountType>
void FrequencyPrinterEntry<ValueType, CountType>::Print(FrequencyPrinter& frequency_printer, std::wstring frequency_name,
                                                        const FrequencyPrinterOptions& frequency_printer_options)
{
    auto create_and_print_frequency_table = [&](const DictValueSet* dict_value_set, bool distinct)
    {
        std::unique_ptr<FrequencyTable> frequency_table = CreateFrequencyTable(frequency_name, frequency_printer_options, dict_value_set, distinct);
        frequency_printer.Print(*frequency_table);
    };


    // if any specific value sets have been specified for printing, we will only print those
    if( m_dictItem != nullptr )
    {
        bool found_specific_value_set = false;

        for( const int symbol_index : frequency_printer_options.GetValueSetSymbolIndices() )
        {
            for( const DictValueSet& dict_value_set : m_dictItem->GetValueSets() )
            {
                if( dict_value_set.GetSymbolIndex() == symbol_index )
                {
                    create_and_print_frequency_table(&dict_value_set, frequency_printer_options.GetDistinct());
                    found_specific_value_set = true;
                }
            }
        }

        if( found_specific_value_set )
            return;
    }


    // if not, determine what the primary printing value set should be
    const DictValueSet* primary_dict_value_set = m_currentDictValueSet;
    bool primary_value_set_distinct_override = frequency_printer_options.GetDistinct();

    if( !frequency_printer_options.GetPrioritizeCurrentValueSet() )
    {
        if( m_dictItem != nullptr )
            primary_dict_value_set = m_dictItem->GetFirstValueSetOrNull();

        primary_value_set_distinct_override = true;
    }


    // if not printing out all value sets, print the table and quit out
    if( !frequency_printer_options.GetUseAllValueSets() || primary_dict_value_set == nullptr )
    {
        create_and_print_frequency_table(primary_dict_value_set, primary_value_set_distinct_override);
        return;
    }


    // otherwise print out a distinct table with the primary value set and then a table
    // for each value set using the specified distinct setting (potentially using the 
    // same value set twice, once distinct and later not distinct)
    create_and_print_frequency_table(primary_dict_value_set, true);

    for( const DictValueSet& dict_value_set : m_dictItem->GetValueSets() )
    {
        if( primary_dict_value_set != &dict_value_set || !frequency_printer_options.GetDistinct() )
            create_and_print_frequency_table(&dict_value_set, frequency_printer_options.GetDistinct());
    }
}


template<typename ValueType, typename CountType>
std::unique_ptr<FrequencyTable> FrequencyPrinterEntry<ValueType, CountType>::CreateFrequencyTable(std::wstring frequency_name,
    const FrequencyPrinterOptions& frequency_printer_options, const DictValueSet* dict_value_set, bool distinct)
{
    auto frequency_table = std::make_unique<FrequencyTable>(
        FrequencyTable
        {
            std::move(frequency_name),
            frequency_printer_options,
            m_symbolName,
            m_dictItem,
            dict_value_set
        });

    frequency_table->distinct = distinct;

    // create a value processor to help with operations
    std::shared_ptr<const ValueProcessor> value_processor;

    if( dict_value_set != nullptr )
        value_processor = ValueProcessor::CreateValueProcessor(*m_dictItem, dict_value_set);

    // process each count
    std::optional<std::vector<std::tuple<ValueType, CountType>>> non_zero_counts_for_statistics;

    if( frequency_table->frequency_printer_options.GetShowStatistics() ||
        frequency_table->frequency_printer_options.GetUsingPercentiles() )
    {
        non_zero_counts_for_statistics.emplace();
    }

    for( const auto& [value, count] : m_frequencyCounter->GetCounts() )
    {
        if( count == 0 )
            continue;

        if( non_zero_counts_for_statistics.has_value() )
            non_zero_counts_for_statistics->emplace_back(value, count);

        // format the value for display
        std::wstring formatted_value;
        bool value_is_blank;

        if constexpr(std::is_same_v<ValueType, double>)
        {
            if( m_dictItem == nullptr )
            {
                formatted_value = DoubleToString(value);
            }

            else
            {
                double value_for_formatting = value;

                if( value_processor != nullptr )
                    value_for_formatting = assert_cast<const NumericValueProcessor&>(*value_processor).ConvertNumberFromEngineFormat(value);

                if( value == NOTAPPL )
                    value_for_formatting = MASKBLK;

                formatted_value = dvaltochar(value_for_formatting, m_dictItem->GetCompleteLen(), m_dictItem->GetDecimal(), false, true);
            }

            value_is_blank = ( value == NOTAPPL );
        }

        else
        {
            formatted_value = SO::TrimRightSpace(value);

            value_is_blank = formatted_value.empty();
        }


        // tally the totals
        frequency_table->total_count += count;

        if( !value_is_blank )
            frequency_table->total_non_blank_count += count;


        // add the frequency rows
        auto add_frequency_row = [&, value_ = std::ref(value), count_ = std::ref(count)](const DictValue* dict_value) -> FrequencyRow&
        {
            return frequency_table->frequency_rows.emplace_back(FrequencyRow { { value_ }, { formatted_value }, dict_value, static_cast<double>(count_), value_is_blank });
        };

        // if there is no value processor, this will be a unique entry because there are value set labels to match against
        if( value_processor == nullptr )
        {
            add_frequency_row(nullptr);
        }

        // if using a value processor, find the value set labels
        else
        {
            std::vector<const DictValue*> matching_dict_values = value_processor->GetMatchingDictValues(value);

            // if there are no matches, add a row without a label
            if( matching_dict_values.empty() )
            {
                add_frequency_row(nullptr);
            }

            // if creating a row for every value, use the first entry
            else if( distinct )
            {
                add_frequency_row(matching_dict_values.front());
            }

            // otherwise create a new row or add the count to a previously added row
            else
            {
                for( const DictValue* dict_value : matching_dict_values )
                {
                    FrequencyRow* frequency_row;

                    auto frequency_rows_lookup = std::find_if(frequency_table->frequency_rows.begin(), frequency_table->frequency_rows.end(),
                                                              [&](const FrequencyRow& fr) { return ( fr.dict_value == dict_value ); });

                    if( frequency_rows_lookup == frequency_table->frequency_rows.end() )
                    {
                        frequency_row = &add_frequency_row(dict_value);
                    }

                    else
                    {
                        frequency_rows_lookup->values.emplace_back(value);
                        frequency_rows_lookup->formatted_values.emplace_back(formatted_value);
                        frequency_rows_lookup->count += count;
                        frequency_rows_lookup->value_is_blank |= value_is_blank;
                        frequency_row = &(*frequency_rows_lookup);
                    }

                    if( matching_dict_values.size() > 1 )
                    {
                        frequency_table->has_multiple_labels_per_value = true;
                        frequency_row->mark_as_value_appearing_in_multiple_rows = true;
                    }
                }
            }
        }
    }


    // add additional information to the frequency table and then print it
    SortFrequencyRows(*frequency_table);

    AddTitle(*frequency_table);

    AddAdditionalDisplayInformation(*frequency_table);

    AddFrequencyRowStatistics(*frequency_table);

    if( non_zero_counts_for_statistics.has_value() )
        GenerateStatistics(*frequency_table, *non_zero_counts_for_statistics);

    return frequency_table;    
}


template<typename ValueType, typename CountType>
void FrequencyPrinterEntry<ValueType, CountType>::SortFrequencyRows(FrequencyTable& frequency_table)
{
    std::sort(frequency_table.frequency_rows.begin(), frequency_table.frequency_rows.end(),
        [&](const FrequencyRow& fr1, const FrequencyRow& fr2)
        {
            // blank values will always be last
            if( fr1.value_is_blank != fr2.value_is_blank )
                return fr2.value_is_blank;

            // otherwise use the sort type
            FrequencyPrinterOptions::SortType sort_type = frequency_table.frequency_printer_options.GetSortType();

            // by count
            if( sort_type == FrequencyPrinterOptions::SortType::ByCount )
            {
                // if equal, sort by value set order
                if( fr1.count == fr2.count )
                {
                    sort_type = FrequencyPrinterOptions::SortType::ByValueSetOrder;
                }

                else
                {
                    return ( fr1.count < fr2.count );
                }
            }


            // by label (case insensitive)
            if( sort_type == FrequencyPrinterOptions::SortType::ByLabel )
            {
                auto get_label = [](const FrequencyRow& fr)
                {
                    // if no dictionary label exists, use the formatted value
                    return ( fr.dict_value != nullptr ) ? CS2WS(fr.dict_value->GetLabel()) :
                                                          fr.formatted_values.front();
                };

                const int difference = SO::CompareNoCase(get_label(fr1), get_label(fr2));

                // if equal, sort by value set order
                if( difference == 0 )
                {
                    sort_type = FrequencyPrinterOptions::SortType::ByValueSetOrder;
                }

                else
                {
                    return ( difference < 0 );
                }
            }


            // by value set order
            if( sort_type == FrequencyPrinterOptions::SortType::ByValueSetOrder )
            {
                auto get_order = [&](const FrequencyRow& fr)
                {
                    // sort value set entries before values without such an entry
                    if( fr.dict_value != nullptr )
                    {
                        const std::vector<DictValue>& dict_values = frequency_table.dict_value_set->GetValues();
                        const auto& lookup = std::find_if(dict_values.cbegin(), dict_values.cend(),
                                                          [&](const DictValue& dict_value) { return ( fr.dict_value == &dict_value ); });

                        if( lookup != dict_values.cend() )
                            return static_cast<int>(std::distance(dict_values.cbegin(), lookup));
                    }

                    return INT_MAX;
                };

                const int difference = get_order(fr1) - get_order(fr2);

                // if equal, sort by code
                if( difference == 0 )
                {
                    sort_type = FrequencyPrinterOptions::SortType::ByCode;
                }

                else
                {
                    return ( difference < 0 );
                }
            }


            // by code (or a fallback from the previous sorts)
            ASSERT(sort_type == FrequencyPrinterOptions::SortType::ByCode);

            // make sure that special values are sorted in display order
            if constexpr(std::is_same_v<ValueType, double>)
            {
                return SpecialValues::CompareForDisplayOrder(std::get<double>(fr1.values.front()), std::get<double>(fr2.values.front()));
            }

            else
            {
                return ( fr1.values.front() < fr2.values.front() );
            }
        });


    // if necessary reverse the sort for descending order
    if( !frequency_table.frequency_printer_options.IsAscendingSort() )
        std::reverse(frequency_table.frequency_rows.begin(), frequency_table.frequency_rows.end());
}


template<typename ValueType, typename CountType>
void FrequencyPrinterEntry<ValueType, CountType>::AddTitle(FrequencyTable& frequency_table)
{
    // create a meaningful title
    if( m_dictItem == nullptr )
    {
        frequency_table.titles.emplace_back(m_symbolName);
    }

    else
    {
        // add the item details
        frequency_table.titles.emplace_back(FormatTextCS2WS(_T("Item %s: %s"), m_dictItem->GetName().GetString(), m_dictItem->GetLabel().GetString()));

        // add the value set details
        if( frequency_table.dict_value_set != nullptr )
        {
            std::wstring& value_set_title = frequency_table.titles.emplace_back(SO::Concatenate(_T("Value Set "), frequency_table.dict_value_set->GetName()));

            if( !frequency_table.dict_value_set->GetLabel().IsEmpty() )
                SO::Append(value_set_title, _T(": "), frequency_table.dict_value_set->GetLabel());
        }

        // add details on what occurrences were generated
        auto add_occurrence_label = [&](std::wstring occurrence_type, auto dict_object, auto max_occurrences, auto specific_occurrence)
        {
            std::wstring& occurrence_text = frequency_table.titles.emplace_back(std::move(occurrence_type));

            if( specific_occurrence.has_value() )
            {
                SO::AppendFormat(occurrence_text, _T(" occurrence: %d"), static_cast<int>(*specific_occurrence) + 1);

                const CString& occurrence_label = dict_object->GetOccurrenceLabels().GetLabel(*specific_occurrence);

                if( !occurrence_label.IsEmpty() )
                    SO::AppendFormat(occurrence_text, _T(" (%s)"), occurrence_label.GetString());
            }

            else
            {
                SO::AppendFormat(occurrence_text, _T(" occurrences: all (1 - %d)"), static_cast<int>(max_occurrences));
            }
        };

        if( m_dictItem->GetRecord()->GetMaxRecs() > 1 )
            add_occurrence_label(_T("Record"), m_dictItem->GetRecord(), m_dictItem->GetRecord()->GetMaxRecs(), m_recordOccurrence);

        if( m_dictItem->GetItemSubitemOccurs() > 1 )
        {
            const CDictItem* repeating_item = ( m_dictItem->GetOccurs() > 1 ) ? m_dictItem : m_dictItem->GetParentItem();
            add_occurrence_label(( repeating_item == m_dictItem ) ? _T("Item") : _T("Subitem"), repeating_item,
                                 m_dictItem->GetItemSubitemOccurs(), m_itemSubitemOccurrence);
        }
    }

    // if run from CSFreq, add the universe and weight to the title
    std::tuple<std::wstring, std::wstring> universe_and_weight;

    if( WindowsDesktopMessage::Send(UWM::Freq::GetUniverseAndWeight, &universe_and_weight) == 1 )
    {
        for( auto& [text_description, text] : std::initializer_list<std::tuple<std::wstring, std::wstring>>
                                              { { _T("Universe: "), std::move(std::get<0>(universe_and_weight)) },
                                                { _T("Weight: "),   std::move(std::get<1>(universe_and_weight)) } } )
        {
            if( !text.empty() )
            {
                frequency_table.titles.emplace_back(SO::Concatenate(text_description, text));
                frequency_table.logic_based_titles.try_emplace(frequency_table.titles.size() - 1,
                                                               std::make_tuple(std::move(text_description), std::move(text)));
            }
        };
    }
}


template<typename ValueType, typename CountType>
void FrequencyPrinterEntry<ValueType, CountType>::AddAdditionalDisplayInformation(FrequencyTable& frequency_table)
{
    for( FrequencyRow& frequency_row : frequency_table.frequency_rows )
    {
        frequency_row.mark_as_out_of_value_set = ( frequency_table.dict_value_set != nullptr && frequency_row.dict_value == nullptr );

        // use the value label if possible
        if( frequency_row.dict_value != nullptr )
        {
            frequency_row.display_label = frequency_row.dict_value->GetLabel();
        }

        // add friendly names for special values
        else
        {
            if( std::holds_alternative<double>(frequency_row.values.front()) )
            {
                const double value = std::get<double>(frequency_row.values.front());

                if( IsSpecial(value) )
                {
                    frequency_row.display_label = SpecialValues::ValueToString(value, false);
                    frequency_row.mark_as_out_of_value_set = false;
                }
            }

            else if( frequency_row.value_is_blank )
            {
                frequency_row.display_label = _T("Blank");
                frequency_row.mark_as_out_of_value_set = false;
            }
        }
    }
}


template<typename ValueType, typename CountType>
void FrequencyPrinterEntry<ValueType, CountType>::AddFrequencyRowStatistics(FrequencyTable& frequency_table)
{
    double cumulative_count = 0;
    double cumulative_non_blank_count = 0;

    for( const FrequencyRow& frequency_row : frequency_table.frequency_rows )
    {
        FrequencyRowStatistics& statistics = frequency_table.frequency_row_statistics.emplace_back();

        cumulative_count += frequency_row.count;

        statistics.percent_against_total = CreatePercent<double>(frequency_row.count, frequency_table.total_count);

        if( !frequency_table.has_multiple_labels_per_value )
        {
            statistics.cumulative_count = cumulative_count;
            statistics.cumulative_percent_against_total = CreatePercent<double>(cumulative_count, frequency_table.total_count);
        }

        if( !frequency_row.value_is_blank )
        {
            cumulative_non_blank_count += frequency_row.count;

            statistics.percent_against_non_blank_total = CreatePercent<double>(frequency_row.count, frequency_table.total_non_blank_count);

            if( !frequency_table.has_multiple_labels_per_value )
                statistics.cumulative_percent_against_non_blank_total = CreatePercent<double>(cumulative_non_blank_count, frequency_table.total_non_blank_count);
        }
    }
}


namespace
{
    template<typename CountType>
    std::tuple<double, double> CalculateMedian(const std::vector<std::tuple<double, CountType>>& non_zero_counts, double sum_count)
    {
        ASSERT(!non_zero_counts.empty());

        double median_cumulative_count = 0;
        double median_cumulative_percent = 0;

        for( size_t i = 0; i < non_zero_counts.size(); ++i )
        {
            const CountType count = std::get<1>(non_zero_counts[i]);

            median_cumulative_count += count;
            median_cumulative_percent += count / sum_count;

            if( FloatingPointMath::GreaterThanEquals(median_cumulative_percent, 0.5) )
            {
                const double value = std::get<0>(non_zero_counts[i]);
                ASSERT(!IsSpecial(value));

                // if arriving at the exact midpoint, use the average of this value and the next value
                // as both the median and interpolated median
                if( FloatingPointMath::Equals(median_cumulative_percent, 0.5) )
                {
                    ASSERT(( i + 1 ) < non_zero_counts.size());
                    const double next_value = std::get<0>(non_zero_counts[i + 1]);
                    const double median = ( value + next_value ) / 2;
                    return std::make_tuple(median, median);
                }

                // otherwise use the value as the median and calculate the interpolated median
                else
                {
                    const double median = value;

                    // this matches the values given by http://personality-project.org/r/html/interp.median.html
                    // assuming an interval of 1 between values
                    const double cumulative_count_before_median = median_cumulative_count - count;

                    const double median_interpolated = median - 0.5 +  ( ( 0.5 * sum_count - cumulative_count_before_median ) / count );

                    return std::make_tuple(median, median_interpolated);
                }
            }
        }

        throw ProgrammingErrorException();
    }


    template<typename CountType>
    std::vector<FrequencyNumericStatistics::Percentile> CalculatePercentiles(const std::vector<std::tuple<double, CountType>>& non_zero_counts,
                                                                             double sum_count, int ntiles)
    {
        ASSERT(!non_zero_counts.empty() && ntiles > 1);

        std::vector<FrequencyNumericStatistics::Percentile> percentiles;
        const double percentile_step = 1.0 / ntiles;
        double next_percentile = 0;

        // percentiles will be calculated to match the Type 2 and Type 6 percentiles outlined here:
        // https://www.rdocumentation.org/packages/stats/versions/3.6.2/topics/quantile

        // calculate the Type 2 percentiles
        auto non_zero_counts_itr = non_zero_counts.cbegin();
        double cumulative_percent = 0;

        auto get_next_value = [&](double value)
        {
            // make sure that a special value isn't returned
            if( ( non_zero_counts_itr + 1 ) != non_zero_counts.cend() )
            {
                const double next_value = std::get<0>(*(non_zero_counts_itr + 1));

                if( !IsSpecial(next_value) )
                    return next_value;
            }

            return value;
        };

        for( ; non_zero_counts_itr != non_zero_counts.cend(); ++non_zero_counts_itr )
        {
            cumulative_percent += std::get<1>(*non_zero_counts_itr) / sum_count;

            while( FloatingPointMath::GreaterThanEquals(cumulative_percent, next_percentile) )
            {
                double percentile_value = std::get<0>(*non_zero_counts_itr);
                ASSERT(!IsSpecial(percentile_value));

                if( FloatingPointMath::Equals(cumulative_percent, next_percentile) )
                    percentile_value = ( percentile_value + get_next_value(percentile_value) ) / 2;

                percentiles.emplace_back(FrequencyNumericStatistics::Percentile { next_percentile, percentile_value, DEFAULT });

                next_percentile += percentile_step;
            }

            if( FloatingPointMath::GreaterThan(next_percentile, 1) )
                break;
        }


        // calculate the Type 6 percentiles:
        // https://support.minitab.com/en-us/minitab/18/help-and-how-to/calculations-data-generation-and-matrices/calculator/calculator-functions/statistics-calculator-functions/percentile-function/ )
        non_zero_counts_itr = non_zero_counts.cbegin();
        bool process_next_non_zero_count = true;
        double cumulative_count = 0;

        for( FrequencyNumericStatistics::Percentile& percentile : percentiles )
        {
            const double w = std::min(sum_count, percentile.percentile * ( sum_count + 1 ));
            const double y = std::trunc(w);
            const double z = w - y;

            while( true )
            {
                if( process_next_non_zero_count )
                {
                    ASSERT(non_zero_counts_itr != non_zero_counts.cend());
                    cumulative_count += std::get<1>(*non_zero_counts_itr);
                    process_next_non_zero_count = false;
                }

                if( FloatingPointMath::LessThanEquals(y, cumulative_count) )
                {
                    double percentile_value = std::get<0>(*non_zero_counts_itr);
                    ASSERT(!IsSpecial(percentile_value));

                    if( FloatingPointMath::Equals(y, cumulative_count) )
                        percentile_value += z * ( get_next_value(percentile_value) - percentile_value );

                    percentile.value_type6 = percentile_value;

                    break;
                }

                else
                {
                    ++non_zero_counts_itr;
                    process_next_non_zero_count = true;
                }
            }
        }

        return percentiles;
    }
}


template<typename CountType>
void GenerateStatistics(FrequencyTable& frequency_table, std::vector<std::tuple<double, CountType>>& non_zero_counts)
{
    frequency_table.table_statistics = FrequencyNumericStatistics();
    FrequencyNumericStatistics& table_statistics = std::get<FrequencyNumericStatistics>(*frequency_table.table_statistics);
    double min_value = std::numeric_limits<double>::max();
    double max_value = std::numeric_limits<double>::lowest();
    std::tuple<double, double> mode_value_and_count = std::make_tuple(std::numeric_limits<double>::lowest(), 0);
    double sum_count = 0;
    double product_value_count = 0;
    double product_value_value_count = 0;

    // sort by values to facilitate median and percentile calculations (and to get the special values
    // in the correct order for adding to table_statistics.non_blank_special_values_used)
    std::sort(non_zero_counts.begin(), non_zero_counts.end(),
              [](const auto& nzc1, const auto& nzc2) { return SpecialValues::CompareForDisplayOrder(std::get<0>(nzc1), std::get<0>(nzc2)); });

    // generate some helper values
    for( const auto& [value, count] : non_zero_counts )
    {       
        // exclude special values from calculations
        if( IsSpecial(value) )
        {
            if( value != NOTAPPL )
                table_statistics.non_blank_special_values_used.emplace_back(value);

            continue;
        }

        ++table_statistics.number_defined_categories;

        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);

        if( count > std::get<1>(mode_value_and_count) )
            mode_value_and_count = std::make_tuple(value, count);

        sum_count += count;
        product_value_count += value * count;
        product_value_value_count += value * value * count;
    }

    // fill out the other statistics
    if( table_statistics.number_defined_categories != 0 )
    {
        table_statistics.min_value = min_value;
        table_statistics.max_value = max_value;

        table_statistics.mode_value = std::get<0>(mode_value_and_count);
        table_statistics.mode_count = std::get<1>(mode_value_and_count);

        table_statistics.sum_count = sum_count;
        table_statistics.product_value_count = product_value_count;
        table_statistics.product_value_value_count = product_value_value_count;        

        if( sum_count > 0 )
        {
            table_statistics.mean = product_value_count / sum_count;

            if( sum_count > 1 )
            {
                table_statistics.variance = ( product_value_value_count - product_value_count * *table_statistics.mean ) / ( sum_count - 1 );
                table_statistics.standard_deviation = std::sqrt(*table_statistics.variance);
            }

            std::tie(table_statistics.median, table_statistics.median_interpolated) = CalculateMedian(non_zero_counts, sum_count);

            if( frequency_table.frequency_printer_options.GetUsingPercentiles() )
                table_statistics.percentiles = CalculatePercentiles(non_zero_counts, sum_count, frequency_table.frequency_printer_options.GetPercentiles());
        }
    }
}


template<typename CountType>
void GenerateStatistics(FrequencyTable& frequency_table, std::vector<std::tuple<std::wstring, CountType>>& non_zero_counts)
{
    frequency_table.table_statistics = FrequencyAlphanumericStatistics();
    FrequencyAlphanumericStatistics& table_statistics = std::get<FrequencyAlphanumericStatistics>(*frequency_table.table_statistics);

    table_statistics.number_defined_categories = non_zero_counts.size();

    if( frequency_table.total_count != frequency_table.total_non_blank_count )
        --table_statistics.number_defined_categories;
}



template class FrequencyPrinterEntry<double, double>;
template class FrequencyPrinterEntry<double, size_t>;
template class FrequencyPrinterEntry<std::wstring, double>;
template class FrequencyPrinterEntry<std::wstring, size_t>;
