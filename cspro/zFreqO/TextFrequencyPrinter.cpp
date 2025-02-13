#include "stdafx.h"
#include "TextFrequencyPrinter.h"


namespace
{
    constexpr int DefaultPageLength    = 60;
    constexpr int MinimumLineLength    = 89;

    constexpr int SpacingBetweenTables = 2;

    constexpr const TCHAR* TitleSeparatorFormatter   = _T("____________%s_____________________________________________________________________________");

    constexpr const TCHAR* RowAboveColumns1Formatter = _T("            %s                    _____________________________ _____________");
    constexpr const TCHAR* RowColumns1Formatter      = _T("  Categories%s                         Frequency        CumFreq      %%  Cum %%");
    constexpr const TCHAR* RowBelowColumns1Formatter = _T("____________%s___________________ _____________________________ _____________");

    constexpr const TCHAR* RowAboveColumns2          = _T(" _____________");
    constexpr const TCHAR* RowColumns2               = _T("  Net % cNet %");
    constexpr const TCHAR* RowBelowColumns2          = _T(" _____________");

    constexpr int RowFormatter1BaseWidth             = 31;
    constexpr const TCHAR* RowFormatter1Formatter    = _T("%%-%d.%ds %%14s %%14s  %%5s  %%5s");
    constexpr const TCHAR* RowFormatter2             = _T("  %5s  %5s");

    constexpr TCHAR OutOfValueSetRowCharacter        = _T('@');
    constexpr TCHAR MultipleLabelsPerValueCharacter  = _T('†');
}


// --------------------------------------------------------------------------
// TextFrequencyPrinterWorker
// --------------------------------------------------------------------------

class TextFrequencyPrinterWorker
{
public:
    TextFrequencyPrinterWorker(int line_length, std::vector<std::wstring>& title_lines, std::vector<std::wstring>& lines);

    void Print(const FrequencyTable& frequency_table);

private:
    void AddLine(std::wstring line = std::wstring())
    {
        ASSERT(!SO::ContainsNewlineCharacter(line));
        SO::MakeTrimRight(line);
        m_lines.emplace_back(std::move(line));
    }

    void PrintRowsAndTotal(const FrequencyTable& frequency_table);
    void PrintStatistics(const FrequencyTable& frequency_table);

private:
    std::vector<std::wstring>& m_titleLines;
    std::vector<std::wstring>& m_lines;

    const int m_lineLength;
    std::wstring m_titleSeparator;
    std::wstring m_rowAboveColumns1;
    std::wstring m_rowColumns1;
    std::wstring m_rowBelowColumns1;
    std::wstring m_rowFormatter1;
};


TextFrequencyPrinterWorker::TextFrequencyPrinterWorker(int line_length, std::vector<std::wstring>& title_lines, std::vector<std::wstring>& lines)
    :   m_lineLength(line_length),
        m_titleLines(title_lines),
        m_lines(lines)
{
    // adjust the formats for the listing width
    const int line_length_increase = m_lineLength - MinimumLineLength;

    const std::wstring underscores(line_length_increase, '_');
    const std::wstring spaces(line_length_increase, ' ');

    m_titleSeparator = FormatTextCS2WS(TitleSeparatorFormatter, underscores.c_str());
    m_rowAboveColumns1 = FormatTextCS2WS(RowAboveColumns1Formatter, spaces.c_str());
    m_rowColumns1 = FormatTextCS2WS(RowColumns1Formatter, spaces.c_str());
    m_rowBelowColumns1 = FormatTextCS2WS(RowBelowColumns1Formatter, underscores.c_str());

    const int width = RowFormatter1BaseWidth + line_length_increase;
    m_rowFormatter1 = FormatTextCS2WS(RowFormatter1Formatter, width, width);
}


void TextFrequencyPrinterWorker::Print(const FrequencyTable& frequency_table)
{
    // print the titles
    m_titleLines.emplace_back(m_titleSeparator);

    for( std::wstring title : frequency_table.titles )
    {
        ASSERT(!SO::ContainsNewlineCharacter(title));

        if( frequency_table.special_formatting == FrequencyTable::SpecialFormatting::CenterTitles )
        {
            SO::MakeTrim(title);
            SO::CenterExactLength(title, m_lineLength);
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

    auto join_columns = [&](std::wstring set1, const TCHAR* set2)
    {
        if( show_net_percents )
            set1.append(set2);

        return set1;
    };

    AddLine(join_columns(m_rowAboveColumns1, RowAboveColumns2));
    AddLine(join_columns(m_rowColumns1, RowColumns2));

    const std::wstring below_columns_text = join_columns(m_rowBelowColumns1, RowBelowColumns2);
    const std::wstring row_formatter = join_columns(m_rowFormatter1, RowFormatter2);

    if( !frequency_table.frequency_rows.empty() )
        AddLine(below_columns_text);


    // print each of the frequency rows
    const int number_frequency_decimals = frequency_table.frequency_printer_options.GetUsingDecimals() ?
                                          frequency_table.frequency_printer_options.GetDecimals() : 0;

    auto print_row = [&](NullTerminatedString category, double count, const FrequencyRowStatistics& frequency_row_statistics, size_t left_indentation_if_wrapping_lines = SIZE_MAX)
    {
        auto get_formatted_frequency = [&](const std::optional<double>& frequency)
        {
            return frequency.has_value() ? FormatTextCS2WS(_T("%0.*f"), number_frequency_decimals, frequency) : std::wstring();
        };

        auto get_formatted_percent = [&](const std::optional<double>& percent)
        {
            return percent.has_value() ? FormatTextCS2WS(_T("%3.1f"), *percent) : std::wstring();
        };

        auto add_first_row = [&](NullTerminatedString this_category)
        {
            AddLine(FormatTextCS2WS(row_formatter.c_str(),
                                    this_category.c_str(),
                                    get_formatted_frequency(count).c_str(),
                                    get_formatted_frequency(frequency_row_statistics.cumulative_count).c_str(),
                                    get_formatted_percent(frequency_row_statistics.percent_against_total).c_str(),
                                    get_formatted_percent(frequency_row_statistics.cumulative_percent_against_total).c_str(),
                                    get_formatted_percent(frequency_row_statistics.percent_against_non_blank_total).c_str(),
                                    get_formatted_percent(frequency_row_statistics.cumulative_percent_against_non_blank_total).c_str()));
        };

        if( left_indentation_if_wrapping_lines == SIZE_MAX || !SO::ContainsNewlineCharacter(wstring_view(category)) )
        {
            add_first_row(category);
        }

        else
        {
            const TCHAR* indentation_text = nullptr;

            SO::ForeachLine(category, true,
                [&](const std::wstring& per_line_category)
                {
                    if( indentation_text == nullptr )
                    {
                        add_first_row(per_line_category);
                        indentation_text = SO::GetRepeatingCharacterString(' ', left_indentation_if_wrapping_lines);
                    }

                    else
                    {
                        AddLine(FormatTextCS2WS(row_formatter.c_str(),
                                                ( indentation_text + per_line_category ).c_str(),
                                                _T(""), _T(""), _T(""), _T(""), _T(""), _T("")));
                    }
                    
                    return true;
                });
        }
    };

    bool has_out_of_value_set_rows = false;
    bool has_value_appearing_in_multiple_rows = false;

    for( size_t i = 0; i < frequency_table.frequency_rows.size(); ++i )
    {
        const FrequencyRow& frequency_row = frequency_table.frequency_rows[i];
        std::wstring formatted_value;
        std::wstring display_label = frequency_row.display_label;

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
        std::wstring indicator_characters;

        auto update_indicator_characters = [&](bool condition, bool& global_value, TCHAR character)
        {
            if( condition )
            {
                global_value = true;
                indicator_characters.push_back(character);
            }
        };

        update_indicator_characters(frequency_row.mark_as_out_of_value_set, has_out_of_value_set_rows, OutOfValueSetRowCharacter);
        update_indicator_characters(frequency_row.mark_as_value_appearing_in_multiple_rows, has_value_appearing_in_multiple_rows, MultipleLabelsPerValueCharacter);

        constexpr size_t LeftIndentationForCategoryIfWrappingLines = 2;

        const std::wstring category = FormatTextCS2WS(_T("%-2s%s%s%s"),
                                                      indicator_characters.c_str(),
                                                      formatted_value.c_str(),
                                                      formatted_value.empty() ? _T("") : _T(" "),
                                                      display_label.c_str());

        // add a separator before the blank entry
        if( frequency_row.value_is_blank )
            AddLine(below_columns_text);

        print_row(category, frequency_row.count, frequency_table.frequency_row_statistics[i], LeftIndentationForCategoryIfWrappingLines);
    }


    // print the total line
    AddLine(m_rowBelowColumns1);
    print_row(_T("  Total"), frequency_table.total_count, FPH::CreateTotalFrequencyRowStatistics(frequency_table, true));


    // print information on the indicators
    bool indicator_space_added = false;

    auto print_indicator = [&](bool global_value, TCHAR character, const TCHAR* text)
    {
        if( !global_value )
            return;

        if( !indicator_space_added )
        {
            AddLine();
            indicator_space_added = true;
        }

        AddLine(FormatTextCS2WS(_T("%c %s"), character, text));
    };

    print_indicator(has_out_of_value_set_rows, OutOfValueSetRowCharacter,
                    _T("This value is out of range (not in the value set)."));

    print_indicator(has_value_appearing_in_multiple_rows, MultipleLabelsPerValueCharacter,
                    _T("This value appears in multiple rows and therefore cumulative frequencies are not shown."));
}


void TextFrequencyPrinterWorker::PrintStatistics(const FrequencyTable& frequency_table)
{
    ASSERT(frequency_table.table_statistics.has_value());

    AddLine();

    auto get_category_text = [](const auto& table_statistics)
    {
        return FormatTextCS2WS(_T("• Statistics:  %d categor%s"),
                               static_cast<int>(table_statistics.number_defined_categories),
                               ( table_statistics.number_defined_categories == 1 ) ? _T("y") : _T("ies"));
    };

    // spacing to match the statistics
    constexpr const TCHAR* StatisticsSpacing = _T("               ");

    // numeric statistics
    if( std::holds_alternative<FrequencyNumericStatistics>(*frequency_table.table_statistics) )
    {
        const FrequencyNumericStatistics& table_statistics = std::get<FrequencyNumericStatistics>(*frequency_table.table_statistics);

        if( frequency_table.frequency_printer_options.GetShowStatistics() )
        {
            // categories
            std::wstring category_text = get_category_text(table_statistics);

            for( size_t i = 0; i < table_statistics.non_blank_special_values_used.size(); ++i )
            {
                if( i == 0 )
                {
                    category_text.append(_T(" as well as "));
                }

                else
                {
                    category_text.append(_T(", "));

                    if( ( i + 1 ) == table_statistics.non_blank_special_values_used.size() )
                        category_text.append(_T("and "));
                }

                category_text.append(SpecialValues::ValueToString(table_statistics.non_blank_special_values_used[i], false));
            }

            AddLine(std::move(category_text));


            // min / max
            if( table_statistics.min_value.has_value() )
            {
                AddLine(FormatTextCS2WS(_T("%sMin: %s,  Max: %s"),
                                        StatisticsSpacing,
                                        FPH::GetFormattedValue(*table_statistics.min_value, frequency_table.dict_item).c_str(),
                                        FPH::GetFormattedValue(*table_statistics.max_value, frequency_table.dict_item).c_str()));
            }


            // mean / standard deviation / variance
            std::wstring mean_stddev_variance_text;

            if( table_statistics.mean.has_value() )
            {
                mean_stddev_variance_text = FormatTextCS2WS(_T("%sMean: %s"),
                                                            StatisticsSpacing,
                                                            FPH::GetValueWithMinimumDecimals(*table_statistics.mean, frequency_table.dict_item).c_str());
            }

            if( table_statistics.variance.has_value() )
            {
                SO::AppendFormat(mean_stddev_variance_text, _T("%sStd.Dev: %s,  Variance: %s"),
                                                            mean_stddev_variance_text.empty() ? StatisticsSpacing : _T(",  "),
                                                            FPH::GetValueWithMinimumDecimals(*table_statistics.standard_deviation, frequency_table.dict_item).c_str(),
                                                            FPH::GetValueWithMinimumDecimals(*table_statistics.variance, frequency_table.dict_item).c_str());
            }

            if( !mean_stddev_variance_text.empty() )
                AddLine(std::move(mean_stddev_variance_text));


            // mode / median
            std::wstring mode_median_text;

            if( table_statistics.mode_value.has_value() )
            {
                mode_median_text = FormatTextCS2WS(_T("%sMode: %s"),
                                                   StatisticsSpacing,
                                                   FPH::GetFormattedValue(*table_statistics.mode_value, frequency_table.dict_item).c_str());
            }

            if( table_statistics.median.has_value() )
            {
                SO::AppendFormat(mode_median_text, _T("%sMedian: %s,  Interpolated Median: %s"),
                                                   mode_median_text.empty() ? StatisticsSpacing : _T(",  "),
                                                   FPH::GetValueWithMinimumDecimals(*table_statistics.median, frequency_table.dict_item).c_str(),
                                                   FPH::GetValueWithMinimumDecimals(*table_statistics.median_interpolated, frequency_table.dict_item).c_str());
            }

            if( !mode_median_text.empty() )
                AddLine(std::move(mode_median_text));
        }


        // percentiles
        if( table_statistics.percentiles.has_value() )
        {
            // format the values first so that they can be aligned properly
            const std::vector<std::wstring> formatted_percentile_percents = FPH::GetFormattedPercentilePercents(table_statistics);
            const size_t max_percentile_length = formatted_percentile_percents.back().length();

            std::vector<std::wstring> formatted_value_type2s;
            size_t max_value_type2_length = wstring_view(FPH::DiscontinuousLabel).length();

            std::vector<std::wstring> formatted_value_type6s;
            size_t max_value_type6_length  = wstring_view(FPH::ContinuousTextLabel).length();

            for( const FrequencyNumericStatistics::Percentile& percentile : *table_statistics.percentiles )
            {
                formatted_value_type2s.emplace_back(FPH::GetFormattedValue(percentile.value_type2, frequency_table.dict_item));
                max_value_type2_length = std::max(max_value_type2_length, formatted_value_type2s.back().length());

                formatted_value_type6s.emplace_back(FPH::GetValueWithMinimumDecimals(percentile.value_type6, frequency_table.dict_item));
            }

            const size_t max_decimals_after_value_type6 = FPH::GetNumberDecimalsUsed(formatted_value_type6s);

            for( std::wstring& formatted_value_type6 : formatted_value_type6s )
            {
                FPH::EnsureValueHasMinimumDecimals(formatted_value_type6, max_decimals_after_value_type6);
                max_value_type6_length = std::max(max_value_type6_length, formatted_value_type6.length());
            }

            AddLine(FormatTextCS2WS(_T("• Percentiles:   %*s  %*s  %*s"),
                                    max_percentile_length, _T(""),
                                    max_value_type2_length, _T("Discontinuous"),
                                    max_value_type6_length, _T("Continuous")));

            for( size_t i = 0; i < formatted_percentile_percents.size(); ++i )
            {
                AddLine(FormatTextCS2WS(_T("%s%*s%%:  %*s  %*s"),
                                        StatisticsSpacing,
                                        max_percentile_length, formatted_percentile_percents[i].c_str(),
                                        max_value_type2_length, formatted_value_type2s[i].c_str(),
                                        max_value_type6_length, formatted_value_type6s[i].c_str()));
            }
        }


        if( frequency_table.frequency_printer_options.GetShowStatistics() )
        {
            // variance calculations
            if( table_statistics.sum_count.has_value() )
            {
                AddLine(FormatTextCS2WS(_T("• SumsNumCats: (freq) %s, (cat*freq) %s, (cat*cat*freq) %s"),
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

TextFrequencyPrinter::TextFrequencyPrinter(FormatType format_type, int listing_width)
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
    std::vector<std::wstring> title_lines;
    std::vector<std::wstring> lines;
    TextFrequencyPrinterWorker(m_lineLength, title_lines, lines).Print(frequency_table);


    // now do the actual printing, adding line breaks as needed
    int page_length;

    if( m_formatType == FormatType::IgnorePageLength )
    {
        page_length = INT_MAX;
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
    auto print_line = [&](NullTerminatedString line = SO::EmptyString)
    {
        ASSERT(SO::Equals(SO::TrimRightSpace(line), line));

        WriteLine(line);

        ++m_lineNumber;
        ASSERT(m_lineNumber <= page_length);
    };

    auto print_form_feed = [&]()
    {
        if( m_formatType != FormatType::IgnorePageLength )
            WriteLine(_T("\f"));

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

        std::vector<std::wstring> heading_lines = frequency_table.frequency_printer_options.GetHeadings();

        // if formatting for pages, add a blank heading line for the page number if no heading exists
        if( format_for_pages && heading_lines.empty() )
            heading_lines.emplace_back();

        // center the headings
        for( std::wstring& heading_line : heading_lines )
            SO::CenterExactLength(heading_line, m_lineLength);

        // if formatting for pages, add the page number (right-justified) to the first heading line
        if( format_for_pages )
        {
            std::wstring& first_heading_line = heading_lines.front();
            ASSERT(first_heading_line.length() == static_cast<size_t>(m_lineLength));

            const std::wstring page_text = SO::Concatenate(_T("Page "), IntToString(++m_pageNumber));
            first_heading_line.resize(m_lineLength - page_text.length());
            first_heading_line.append(page_text);
        }

        // print the headings
        for( std::wstring& heading_line : heading_lines )
        {
            SO::MakeTrimRightSpace(heading_line);
            print_line(heading_line);
        }

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
    for( std::wstring& title_line : title_lines )
    {
        SO::MakeTrimRightSpace(title_line);
        print_line(title_line);
    }


    // print the table, adding the heading anytime a new page is reached
    for( const std::wstring& line : lines )
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
