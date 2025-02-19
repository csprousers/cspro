#include "stdafx.h"
#include "TextFrequencyPrinter.h"


namespace
{
    constexpr int DefaultPageLength    = 60;
    constexpr int MinimumLineLength    = 89;

    constexpr int SpacingBetweenTables = 2;

    constexpr const char* TitleSeparatorFormatter    = "____________%s_____________________________________________________________________________";
                                                         
    constexpr const char* RowAboveColumns1Formatter  = "            %s                    _____________________________ _____________";
    constexpr const char* RowColumns1Formatter       = "  Categories%s                         Frequency        CumFreq      %%  Cum %%";
    constexpr const char* RowBelowColumns1Formatter  = "____________%s___________________ _____________________________ _____________";
                                                         
    constexpr const char* RowAboveColumns2           = " _____________";
    constexpr const char* RowColumns2                = "  Net % cNet %";
    constexpr const char* RowBelowColumns2           = " _____________";
                                                         
    constexpr int RowFormatter1BaseWidth             = 31;
    constexpr const char* RowFormatter1              = "%s %14s %14s  %5s  %5s";
    constexpr const char* RowFormatter2              = "  %5s  %5s";

    constexpr const char* OutOfValueSetRowText       = "@";
    constexpr const char* MultipleLabelsPerValueText = u8"†";
}


// --------------------------------------------------------------------------
// TextFrequencyPrinterWorker
// --------------------------------------------------------------------------

class TextFrequencyPrinterWorker
{
public:
    TextFrequencyPrinterWorker(int line_length, std::vector<std::string>& title_lines, std::vector<std::string>& lines);

    void Print(const FrequencyTable& frequency_table);

private:
    void AddLine(std::string line = std::string())
    {
        ASSERT(!SO::ContainsNewlineCharacter(line));
        SO::MakeTrimRight(line);
        m_lines.emplace_back(std::move(line));
    }

    void PrintRowsAndTotal(const FrequencyTable& frequency_table);
    void PrintStatistics(const FrequencyTable& frequency_table);

private:
    std::vector<std::string>& m_titleLines;
    std::vector<std::string>& m_lines;

    const int m_lineLength;
    std::string m_titleSeparator;
    std::string m_rowAboveColumns1;
    std::string m_rowColumns1;
    std::string m_rowBelowColumns1;
    int m_rowFormatter1String1Width;
};


TextFrequencyPrinterWorker::TextFrequencyPrinterWorker(const int line_length, std::vector<std::string>& title_lines, std::vector<std::string>& lines)
    :   m_lineLength(line_length),
        m_titleLines(title_lines),
        m_lines(lines)
{
    // adjust the formats for the listing width
    const int line_length_increase = m_lineLength - MinimumLineLength;

    const char* const underscores = SO::GetRepeatingCharacterString('_', line_length_increase);
    const char* const spaces = SO::GetRepeatingCharacterString(' ', line_length_increase);

    m_titleSeparator = FormatText(TitleSeparatorFormatter, underscores);
    m_rowAboveColumns1 = FormatText(RowAboveColumns1Formatter, spaces);
    m_rowColumns1 = FormatText(RowColumns1Formatter, spaces);
    m_rowBelowColumns1 = FormatText(RowBelowColumns1Formatter, underscores);

    m_rowFormatter1String1Width = RowFormatter1BaseWidth + line_length_increase;
}


void TextFrequencyPrinterWorker::Print(const FrequencyTable& frequency_table)
{
    // print the titles
    m_titleLines.emplace_back(m_titleSeparator);

    for( std::string title : frequency_table.titles )
    {
        ASSERT(!SO::ContainsNewlineCharacter(title));

        if( frequency_table.special_formatting == FrequencyTable::SpecialFormatting::CenterTitles )
        {
            SO::MakeTrim(title);
            SO::WideCenterExactLength(title, m_lineLength);
        }

        m_titleLines.emplace_back(std::move(title));
    }

    // print the frequency data as long as NOFREQ wasn't requested
    // (or if NOFREQ was requested but STAT wasn't, which is a contradictory setting pair)
    if( !frequency_table.frequency_printer_options.GetShowNoFrequencies() || !frequency_table.table_statistics.has_value() )
        PrintRowsAndTotal(frequency_table);

    // print the statistics
    if( frequency_table.table_statistics.has_value() )
        PrintStatistics(frequency_table);
}


void TextFrequencyPrinterWorker::PrintRowsAndTotal(const FrequencyTable& frequency_table)
{
    // print column information
    const bool show_net_percents = FPH::ShowFrequencyTableNetPercents(frequency_table);

    auto join_columns = [&](std::string set1, const char* const set2)
    {
        if( show_net_percents )
            set1.append(set2);

        return set1;
    };

    AddLine(join_columns(m_rowAboveColumns1, RowAboveColumns2));
    AddLine(join_columns(m_rowColumns1, RowColumns2));

    const std::string below_columns_text = join_columns(m_rowBelowColumns1, RowBelowColumns2);
    const std::string row_formatter = join_columns(RowFormatter1, RowFormatter2);

    if( !frequency_table.frequency_rows.empty() )
        AddLine(below_columns_text);


    // print each of the frequency rows
    const int number_frequency_decimals = frequency_table.frequency_printer_options.GetUsingDecimals() ?
                                          frequency_table.frequency_printer_options.GetDecimals() : 0;

    auto print_row = [&](std::string category, const double count, const FrequencyRowStatistics& frequency_row_statistics,
                         const size_t left_indentation_if_wrapping_lines = SIZE_MAX)
    {
        auto get_formatted_frequency = [&](const std::optional<double>& frequency)
        {
            return frequency.has_value() ? FormatText("%0.*f", number_frequency_decimals, *frequency) :
                                           std::string();
        };

        auto get_formatted_percent = [&](const std::optional<double>& percent)
        {
            return percent.has_value() ? FormatText("%3.1f", *percent) :
                                         std::string();
        };

        auto add_first_row = [&](std::string this_category)
        {
            SO::WideMakeExactLength(this_category, m_rowFormatter1String1Width);

            AddLine(FormatText(row_formatter.c_str(),
                               this_category.c_str(),
                               get_formatted_frequency(count).c_str(),
                               get_formatted_frequency(frequency_row_statistics.cumulative_count).c_str(),
                               get_formatted_percent(frequency_row_statistics.percent_against_total).c_str(),
                               get_formatted_percent(frequency_row_statistics.cumulative_percent_against_total).c_str(),
                               get_formatted_percent(frequency_row_statistics.percent_against_non_blank_total).c_str(),
                               get_formatted_percent(frequency_row_statistics.cumulative_percent_against_non_blank_total).c_str()));
        };

        if( left_indentation_if_wrapping_lines == SIZE_MAX || !SO::ContainsNewlineCharacter(category) )
        {
            add_first_row(std::move(category));
        }

        else
        {
            const char* indentation_text = nullptr;

            SO::ForeachLine<std::string>(category, true,
                [&](std::string per_line_category)
                {
                    if( indentation_text == nullptr )
                    {
                        add_first_row(std::move(per_line_category));
                        indentation_text = SO::GetRepeatingCharacterString(' ', left_indentation_if_wrapping_lines);
                    }

                    else
                    {
                        SO::WideMakeExactLength(per_line_category, m_rowFormatter1String1Width - left_indentation_if_wrapping_lines);
                        AddLine(indentation_text + per_line_category);
                    }
                });
        }
    };

    bool has_out_of_value_set_rows = false;
    bool has_value_appearing_in_multiple_rows = false;

    for( size_t i = 0; i < frequency_table.frequency_rows.size(); ++i )
    {
        const FrequencyRow& frequency_row = frequency_table.frequency_rows[i];
        std::string formatted_value;
        std::string display_label = frequency_row.display_label;

        // add the values if printing values
        if( frequency_table.distinct )
        {
            formatted_value = frequency_row.formatted_values.front();
        }

        // if not printing values but there is no label, then use the value as the label
        else if( display_label.empty() )
        {
            display_label = SO::CreateSingleString<false>(frequency_row.formatted_values);
        }

        // use some indicator characters to show details about each row
        std::string indicator_characters;

        auto update_indicator_characters = [&](const bool condition, bool& global_value, const char* const character_text)
        {
            if( condition )
            {
                global_value = true;
                indicator_characters.append(character_text);
            }
        };

        update_indicator_characters(frequency_row.mark_as_out_of_value_set, has_out_of_value_set_rows, OutOfValueSetRowText);
        update_indicator_characters(frequency_row.mark_as_value_appearing_in_multiple_rows, has_value_appearing_in_multiple_rows, MultipleLabelsPerValueText);
        SO::WideMakeExactLength(indicator_characters, 2);

        constexpr size_t LeftIndentationForCategoryIfWrappingLines = 2;

        std::string category = SO::Concatenate(indicator_characters,
                                               formatted_value,
                                               formatted_value.empty() ? "" : " ",
                                               display_label);

        // add a separator before the blank entry
        if( frequency_row.value_is_blank )
            AddLine(below_columns_text);

        print_row(std::move(category), frequency_row.count, frequency_table.frequency_row_statistics[i], LeftIndentationForCategoryIfWrappingLines);
    }


    // print the total line
    AddLine(m_rowBelowColumns1);
    print_row("  Total", frequency_table.total_count, FPH::CreateTotalFrequencyRowStatistics(frequency_table, true));


    // print information on the indicators
    bool indicator_space_added = false;

    auto print_indicator = [&](const bool global_value, const char* const character_text, const char* const text)
    {
        if( !global_value )
            return;

        if( !indicator_space_added )
        {
            AddLine();
            indicator_space_added = true;
        }

        AddLine(FormatText("%s %s", character_text, text));
    };

    print_indicator(has_out_of_value_set_rows, OutOfValueSetRowText,
                    "This value is out of range (not in the value set).");

    print_indicator(has_value_appearing_in_multiple_rows, MultipleLabelsPerValueText,
                    "This value appears in multiple rows and therefore cumulative frequencies are not shown.");
}


void TextFrequencyPrinterWorker::PrintStatistics(const FrequencyTable& frequency_table)
{
    ASSERT(frequency_table.table_statistics.has_value());

    AddLine();

    auto get_category_text = [](const auto& table_statistics)
    {
        return FormatText(u8"• Statistics:  %d categor%s",
                          static_cast<int>(table_statistics.number_defined_categories),
                          PluralizeWord(table_statistics.number_defined_categories, "y", "ies"));
    };

    // spacing to match the statistics
    constexpr const char* StatisticsSpacing = "               ";

    // numeric statistics
    if( std::holds_alternative<FrequencyNumericStatistics>(*frequency_table.table_statistics) )
    {
        const FrequencyNumericStatistics& table_statistics = std::get<FrequencyNumericStatistics>(*frequency_table.table_statistics);

        if( frequency_table.frequency_printer_options.GetShowStatistics() )
        {
            // categories
            std::string category_text = get_category_text(table_statistics);

            for( size_t i = 0; i < table_statistics.non_blank_special_values_used.size(); ++i )
            {
                if( i == 0 )
                {
                    category_text.append(" as well as ");
                }

                else
                {
                    category_text.append(", ");

                    if( ( i + 1 ) == table_statistics.non_blank_special_values_used.size() )
                        category_text.append("and ");
                }

                category_text.append(SpecialValues::ValueToString(table_statistics.non_blank_special_values_used[i], false));
            }

            AddLine(std::move(category_text));


            // min / max
            if( table_statistics.min_value.has_value() )
            {
                AddLine(FormatText("%sMin: %s,  Max: %s",
                                   StatisticsSpacing,
                                   FPH::GetFormattedValue(*table_statistics.min_value, frequency_table.dict_item).c_str(),
                                   FPH::GetFormattedValue(*table_statistics.max_value, frequency_table.dict_item).c_str()));
            }


            // mean / standard deviation / variance
            std::string mean_stddev_variance_text;

            if( table_statistics.mean.has_value() )
            {
                mean_stddev_variance_text = FormatText("%sMean: %s",
                                                       StatisticsSpacing,
                                                       FPH::GetValueWithMinimumDecimals(*table_statistics.mean, frequency_table.dict_item).c_str());
            }

            if( table_statistics.variance.has_value() )
            {
                mean_stddev_variance_text.append(FormatText("%sStd.Dev: %s,  Variance: %s",
                                                            mean_stddev_variance_text.empty() ? StatisticsSpacing : ",  ",
                                                            FPH::GetValueWithMinimumDecimals(*table_statistics.standard_deviation, frequency_table.dict_item).c_str(),
                                                            FPH::GetValueWithMinimumDecimals(*table_statistics.variance, frequency_table.dict_item).c_str()));
            }

            if( !mean_stddev_variance_text.empty() )
                AddLine(std::move(mean_stddev_variance_text));


            // mode / median
            std::string mode_median_text;

            if( table_statistics.mode_value.has_value() )
            {
                mode_median_text = FormatText("%sMode: %s",
                                              StatisticsSpacing,
                                              FPH::GetFormattedValue(*table_statistics.mode_value, frequency_table.dict_item).c_str());
            }

            if( table_statistics.median.has_value() )
            {
                mode_median_text.append(FormatText("%sMedian: %s,  Interpolated Median: %s",
                                                   mode_median_text.empty() ? StatisticsSpacing : ",  ",
                                                   FPH::GetValueWithMinimumDecimals(*table_statistics.median, frequency_table.dict_item).c_str(),
                                                   FPH::GetValueWithMinimumDecimals(*table_statistics.median_interpolated, frequency_table.dict_item).c_str()));
            }

            if( !mode_median_text.empty() )
                AddLine(std::move(mode_median_text));
        }


        // percentiles
        if( table_statistics.percentiles.has_value() )
        {
            // format the values first so that they can be aligned properly
            const std::vector<std::string> formatted_percentile_percents = FPH::GetFormattedPercentilePercents(table_statistics);
            ASSERT(SO::WideLength(formatted_percentile_percents.back()) == formatted_percentile_percents.back().length());
            const size_t max_percentile_length = formatted_percentile_percents.back().length();

            std::vector<std::string> formatted_value_type2s;
            size_t max_value_type2_length = std::string_view(FPH::DiscontinuousLabel).length();

            std::vector<std::string> formatted_value_type6s;
            size_t max_value_type6_length  = std::string_view(FPH::ContinuousTextLabel).length();

            for( const FrequencyNumericStatistics::Percentile& percentile : *table_statistics.percentiles )
            {
                formatted_value_type2s.emplace_back(FPH::GetFormattedValue(percentile.value_type2, frequency_table.dict_item));
                max_value_type2_length = std::max(max_value_type2_length, SO::WideLength(formatted_value_type2s.back()));

                formatted_value_type6s.emplace_back(FPH::GetValueWithMinimumDecimals(percentile.value_type6, frequency_table.dict_item));
            }

            const size_t max_decimals_after_value_type6 = FPH::GetNumberDecimalsUsed(formatted_value_type6s);

            for( std::string& formatted_value_type6 : formatted_value_type6s )
            {
                FPH::EnsureValueHasMinimumDecimals(formatted_value_type6, max_decimals_after_value_type6);
                max_value_type6_length = std::max(max_value_type6_length, SO::WideLength(formatted_value_type6));
            }

            AddLine(FormatText(u8"• Percentiles:   %*s  %*s  %*s",
                               static_cast<int>(max_percentile_length), "",
                               static_cast<int>(max_value_type2_length), "Discontinuous",
                               static_cast<int>(max_value_type6_length), "Continuous"));

            for( size_t i = 0; i < formatted_percentile_percents.size(); ++i )
            {
                AddLine(FormatText("%s%*s%%:  %*s  %*s",
                                   StatisticsSpacing,
                                   static_cast<int>(max_percentile_length), formatted_percentile_percents[i].c_str(),
                                   static_cast<int>(max_value_type2_length), formatted_value_type2s[i].c_str(),
                                   static_cast<int>(max_value_type6_length), formatted_value_type6s[i].c_str()));
            }
        }


        if( frequency_table.frequency_printer_options.GetShowStatistics() )
        {
            // variance calculations
            if( table_statistics.sum_count.has_value() )
            {
                AddLine(FormatText(u8"• SumsNumCats: (freq) %s, (cat*freq) %s, (cat*cat*freq) %s",
                                   FPH::GetValueWithMinimumDecimals(*table_statistics.sum_count, frequency_table.dict_item).c_str(),
                                   FPH::GetValueWithMinimumDecimals(*table_statistics.product_value_count, frequency_table.dict_item).c_str(),
                                   FPH::GetValueWithMinimumDecimals(*table_statistics.product_value_value_count, frequency_table.dict_item).c_str()));
            }
        }
    }


    // alphanumeric statistics
    else
    {
        ASSERT(std::holds_alternative<FrequencyAlphanumericStatistics>(*frequency_table.table_statistics));
        ASSERT(frequency_table.frequency_printer_options.GetShowStatistics());
        const FrequencyAlphanumericStatistics& table_statistics = std::get<FrequencyAlphanumericStatistics>(*frequency_table.table_statistics);
        AddLine(get_category_text(table_statistics));
    }
}



// --------------------------------------------------------------------------
// TextFrequencyPrinter
// --------------------------------------------------------------------------

TextFrequencyPrinter::TextFrequencyPrinter(const FormatType format_type, const int listing_width)
    :   m_formatType(format_type),
        m_lineLength(std::max(listing_width, MinimumLineLength)),
        m_pageNumber(0),
        m_lineNumber(0),
        m_expectingPrinterFirstTable(true),
        m_expectingFrequencyGroupFirstTable(true)
{
}


void TextFrequencyPrinter::StartFrequencyGroup()
{
    m_expectingFrequencyGroupFirstTable = true;
}


void TextFrequencyPrinter::Print(const FrequencyTable& frequency_table)
{
    // generate the table
    std::vector<std::string> title_lines;
    std::vector<std::string> lines;

    TextFrequencyPrinterWorker worker(m_lineLength, title_lines, lines);
    worker.Print(frequency_table);


    // now do the actual printing, adding line breaks as needed
    int page_length;

    if( m_formatType == FormatType::IgnorePageLength )
    {
        page_length = std::numeric_limits<int>::max();
    }

    else
    {
        page_length = frequency_table.frequency_printer_options.GetUsingPageLength() ? static_cast<size_t>(frequency_table.frequency_printer_options.GetPageLength()) :
                                                                                       DefaultPageLength;
    }

    // the minimum page length must have space for the heading (or the page number),
    // a space between the heading and the title, the title, and then at least one line of data
    page_length = std::max(page_length, std::max(1, static_cast<int>(frequency_table.frequency_printer_options.GetHeadings().size())) +
                                        1 + 
                                        static_cast<int>(title_lines.size()) +
                                        1);


    // routines for printing lines and form feeds
    auto print_line = [&](std::string_view line_sv = std::string_view())
    {
        ASSERT(line_sv == SO::TrimRightSpace(line_sv));

        WriteLine(line_sv);

        ++m_lineNumber;
        ASSERT(m_lineNumber <= page_length);
    };

    auto print_form_feed = [&]()
    {
        if( m_formatType != FormatType::IgnorePageLength )
            WriteLine("\f");

        m_lineNumber = 0;
    };


    // add a form feed if that is the format type or if separating frequency groups
    if( m_formatType == FormatType::UsePageLengthAddFormFeedBeforeFirstFrequency )
    {
        print_form_feed();
        m_formatType = FormatType::UsePageLength;
    }

    else if( !m_expectingPrinterFirstTable && m_expectingFrequencyGroupFirstTable )
    {
        print_form_feed();
    }

    const bool format_for_pages = ( m_formatType == FormatType::UsePageLength );


    // a routine for printing the header
    auto print_heading = [&]()
    {
        ASSERT(m_lineNumber == 0);

        std::vector<std::string> heading_lines = frequency_table.frequency_printer_options.GetHeadings();

        // if formatting for pages, add a blank heading line for the page number if no heading exists
        if( format_for_pages && heading_lines.empty() )
            heading_lines.emplace_back();

        // center the headings
        for( std::string& heading_line : heading_lines )
            SO::WideCenterExactLength(heading_line, m_lineLength);

        // if formatting for pages, add the page number (right-justified) to the first heading line
        if( format_for_pages )
        {
            std::string& first_heading_line = heading_lines.front();
            ASSERT(SO::WideLength(first_heading_line) == static_cast<size_t>(m_lineLength));

            const std::string page_text = "Page " + IntToString(++m_pageNumber);
            ASSERT(SO::WideLength(page_text) == page_text.length());
            first_heading_line.resize(m_lineLength - page_text.length());
            first_heading_line.append(page_text);
        }

        // print the headings
        for( const std::string& heading_line : heading_lines )
            print_line(SO::TrimRightSpace(heading_line));

        // add a blank line to separate the heading from the title
        print_line();
    };


    // print the heading if on the first table of a frequency group
    bool print_heading_before_title = m_expectingFrequencyGroupFirstTable;

    // print multiple tables on a page if the entire table will fit in the remaining space
    if( format_for_pages && !print_heading_before_title )
    {
        const int lines_needed_for_entire_table = title_lines.size() + lines.size() + SpacingBetweenTables;

        if( ( m_lineNumber + lines_needed_for_entire_table ) > page_length )
        {
            print_form_feed();
            print_heading_before_title = true;
        }
    }

    if( print_heading_before_title )
    {
        print_heading();
    }

    // if not printing out the heading, add some spacing between tables
    else
    {
        for( int i = 0; i < SpacingBetweenTables; ++i )
            print_line();
    }


    // print the title
    for( const std::string& title_line : title_lines )
        print_line(SO::TrimRightSpace(title_line));


    // print the table, adding the heading anytime a new page is reached
    for( const std::string& line : lines )
    {
        if( format_for_pages && m_lineNumber == page_length )
        {
            print_form_feed();
            print_heading();
        }

        print_line(line);
    }


    m_expectingPrinterFirstTable = false;
    m_expectingFrequencyGroupFirstTable = false;
}
