#include "stdafx.h"
#include "HtmlFrequencyPrinter.h"
#include <zToolsO/File.h>
#include <zUtilO/StdioFileUnicode.h>
#include <zHtml/HtmlWriter.h>
#include <zEngineF/EngineUI.h>


HtmlFrequencyPrinter::HtmlFrequencyPrinter(std::string file_path)
    :   m_textFile(std::make_unique<FileIO::TextFile>()),
        m_writeHeaderAndFooter(true),
        m_expectingFrequencyGroupFirstTable(true)
{
    SetupEnvironmentToCreateFile(file_path);

    m_textFile->OpenForTextWritingCreate(std::move(file_path)); // TEXT_ENCODING_TODO refactor to use connection strings with properties

    m_htmlWriter = std::make_unique<HtmlWriter>(m_textFile->GetOutputStream());

    PrintHeader();
}


HtmlFrequencyPrinter::HtmlFrequencyPrinter(HtmlWriter& html_writer, const bool writer_header_and_footer)
    :   m_htmlWriter(&html_writer),
        m_writeHeaderAndFooter(writer_header_and_footer),
        m_expectingFrequencyGroupFirstTable(true)
{
    if( m_writeHeaderAndFooter )
        PrintHeader();
}


HtmlFrequencyPrinter::~HtmlFrequencyPrinter()
{
    if( m_writeHeaderAndFooter )
        *m_htmlWriter << "</body>\n</html>\n";
}


void HtmlFrequencyPrinter::PrintHeader()
{
    ASSERT(m_writeHeaderAndFooter);

    m_htmlWriter->WriteDefaultHeader("Frequency", Html::CSS::Common);
    *m_htmlWriter << "\n<body class='container-page'>\n";
}


void HtmlFrequencyPrinter::StartFrequencyGroup()
{
    m_expectingFrequencyGroupFirstTable = true;
}


void HtmlFrequencyPrinter::Print(const FrequencyTable& frequency_table)
{
    if( m_expectingFrequencyGroupFirstTable )
    {
        const std::vector<std::string>& header_titles = frequency_table.frequency_printer_options.GetHeadings();

        if( !header_titles.empty() )
        {
            *m_htmlWriter << "<hgroup>\n";

            for( const std::string& header_title : header_titles )
                *m_htmlWriter << "<h1 class='center'>" << header_title << "</h1>\n";

            *m_htmlWriter << "</hgroup>\n";
        }

        m_expectingFrequencyGroupFirstTable = false;
    }


    const char* const table_title_alignment = ( frequency_table.special_formatting == FrequencyTable::SpecialFormatting::CenterTitles ) ? " class='center'" : "";

    if( !frequency_table.titles.empty() )
    {
        *m_htmlWriter << "<hgroup>\n";

        for( size_t i = 0; i < frequency_table.titles.size(); ++i )
        {
            // format logic based titles using HTML when possible
            const auto& logic_based_title_lookup = frequency_table.logic_based_titles.find(i);

            if( logic_based_title_lookup != frequency_table.logic_based_titles.cend() )
            {
                const std::tuple<std::string, std::string>& heading_and_logic = logic_based_title_lookup->second;
                EngineUI::ColorizeLogicNode colorize_logic_node { std::get<1>(heading_and_logic), std::string() };

                if( SendEngineUIMessage(EngineUI::Type::ColorizeLogic, colorize_logic_node) == 1 )
                {
                    *m_htmlWriter << "<h2" << table_title_alignment << ">" << std::get<0>(heading_and_logic) <<
                                     "<span style=\"font-family: Consolas\">" << colorize_logic_node.html.c_str() << "</span></h2>\n";
                    continue;
                }
            }

            // otherwise write out the title as text
            *m_htmlWriter << "<h2" << table_title_alignment << ">" << frequency_table.titles[i] << "</h2>\n";
        }

        *m_htmlWriter << "</hgroup>\n";
    }

    // print the frequency data as long as NOFREQ wasn't requested
    // (or if NOFREQ was requested but STAT wasn't, which is a contradictory setting pair)
    if (!frequency_table.frequency_printer_options.GetShowNoFrequencies() || !frequency_table.table_statistics.has_value())
        PrintRowsAndTotal(frequency_table);

    // print the statistics
    if (frequency_table.table_statistics.has_value())
        PrintStatistics(frequency_table);
}


void HtmlFrequencyPrinter::PrintRowsAndTotal(const FrequencyTable& frequency_table)
{
    *m_htmlWriter << "<div class='container-freq'>\n";
    *m_htmlWriter << "<table>\n";

    // print column information
    const bool show_net_percents = FPH::ShowFrequencyTableNetPercents(frequency_table);
    const char* const value_column_justification = FPH::FrequencyTableValuesAreNumeric(frequency_table) ? "right" : "left";

    const bool display_value_column = frequency_table.distinct;
    std::string value_label_columns = display_value_column ? ( std::string("<th rowspan='2' class='") + value_column_justification + " bottom'>Value</th>" ) :
                                                             std::string();

    value_label_columns.append("<th rowspan='2' class='left bottom'>Label</th>");

    const char* const colspan = show_net_percents ? "3" : "2";
    const std::string headerRow1 = "<tr>" + value_label_columns + "<th colspan='" +
                                   colspan + "' class='center'>Frequency</th><th colspan='" +
                                   colspan + "' class='center'>Cumulative</th></tr>\n";

    constexpr const char* totalPercentColumns = "<th>Total</th><th>Percent</th>";
    constexpr const char* netPercentColumn = "<th>Net Percent</th>";
    const std::string headerRow2 = "<tr>" + std::string(totalPercentColumns) + ( show_net_percents ? netPercentColumn : "" ) +
                                   totalPercentColumns + ( show_net_percents ? netPercentColumn : "" ) + "</tr>\n";

    *m_htmlWriter << "<thead>\n"
                  << headerRow1.c_str() << headerRow2.c_str()
                  << "</thead>\n";

    // print each of the frequency rows
    const int number_frequency_decimals = frequency_table.frequency_printer_options.GetUsingDecimals() ? frequency_table.frequency_printer_options.GetDecimals() :
                                                                                                         0;

    auto print_row = [&](const std::string& category_value, const std::string& category_label, const double count,
                         const FrequencyRowStatistics& frequency_row_statistics, const bool display_value_column,
                         const cs::string_sz out_of_value_set_row_style)
    {
        auto get_formatted_frequency = [&](const std::optional<double>& frequency)
        {
            return frequency.has_value() ? FormatText("%0.*f", number_frequency_decimals, *frequency) :
                                           std::string();
        };

        auto get_formatted_percent = [&](const std::optional<double>& percent)
        {
            return percent.has_value() ? FormatText("%3.1f%%", *percent) :
                                         std::string();
        };

        *m_htmlWriter << "<tr>";

        if( display_value_column )
        {
            *m_htmlWriter << "<td class='"
                          << out_of_value_set_row_style.c_str() << ( out_of_value_set_row_style.empty() ? "" : " " )
                          << value_column_justification << "'>" << category_value << "</td>";
        }

        *m_htmlWriter << "<td class='" << out_of_value_set_row_style.c_str() << ( out_of_value_set_row_style.empty() ? "" : " ") << "left'>" << category_label << "</td>";
        *m_htmlWriter << "<td class='" << out_of_value_set_row_style.c_str() << "'>" << get_formatted_frequency(count) << "</td>";

        const std::string frequency_percent = get_formatted_percent(frequency_row_statistics.percent_against_total);
        *m_htmlWriter << "<td class='" << out_of_value_set_row_style.c_str() << "'>" << frequency_percent << "</td>";

        if( show_net_percents )
        {
            const std::string frequency_net_percent = get_formatted_percent(frequency_row_statistics.percent_against_non_blank_total);
            *m_htmlWriter << "<td class='" << out_of_value_set_row_style.c_str() << "'>" << frequency_net_percent << "</td>";
        }

        *m_htmlWriter << "<td class='" << out_of_value_set_row_style.c_str() << "'>" << get_formatted_frequency(frequency_row_statistics.cumulative_count) << "</td>";

        const std::string cumulative_percent = get_formatted_percent(frequency_row_statistics.cumulative_percent_against_total);
        *m_htmlWriter << "<td class='" << out_of_value_set_row_style.c_str() << "'>" << cumulative_percent << "</td>";

        if( show_net_percents )
        {
            const std::string cumulative_net_percent = get_formatted_percent(frequency_row_statistics.cumulative_percent_against_non_blank_total);
            *m_htmlWriter << "<td class='" << out_of_value_set_row_style.c_str() << "'>" << cumulative_net_percent << "</td>";
        }

        *m_htmlWriter << "</tr>\n";
    };

    print_row("", "Total", frequency_table.total_count, FPH::CreateTotalFrequencyRowStatistics(frequency_table, true), display_value_column, "");

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

        // use indicator css to show details about each row
        const char* const out_of_value_set_row_style = frequency_row.mark_as_out_of_value_set ? "outOfValueSetRow" : "";

        print_row(formatted_value, display_label, frequency_row.count, frequency_table.frequency_row_statistics[i], display_value_column, out_of_value_set_row_style);
    }

    *m_htmlWriter << "</table>\n"
                     "</div>\n";
}


void HtmlFrequencyPrinter::PrintStatistics(const FrequencyTable& frequency_table)
{
    ASSERT(frequency_table.table_statistics.has_value());

    auto get_category_text = [](const auto& table_statistics)
    {
        return FormatText("%d categor%s",
                          static_cast<int>(table_statistics.number_defined_categories),
                          PluralizeWord(table_statistics.number_defined_categories, "y", "ies"));
    };

    *m_htmlWriter << "<div class='container-stat'>\n";

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

                    if( (i + 1) == table_statistics.non_blank_special_values_used.size() )
                        category_text.append("and ");
                }

                category_text.append(SpecialValues::ValueToString(table_statistics.non_blank_special_values_used[i], false));
            }

            *m_htmlWriter << "<table id='statistics'>\n"
                          << u8"<tr><td class='width-10 left bold'>• Statistics" << "</td><td class='left'>" << category_text << "</td></tr>\n";

            // min / max
            if( table_statistics.min_value.has_value() )
            {
                *m_htmlWriter << "<tr><td></td><td class='left'>Min: " << FPH::GetFormattedValue(*table_statistics.min_value, frequency_table.dict_item)
                              << ", Max: " << FPH::GetFormattedValue(*table_statistics.max_value, frequency_table.dict_item)
                              << "</td></tr>\n";
            }

            // mean / standard deviation / variance
            std::string mean_stddev_variance_text;

            if( table_statistics.mean.has_value() )
            {
                mean_stddev_variance_text = "Mean: " + FPH::GetValueWithMinimumDecimals(*table_statistics.mean, frequency_table.dict_item);
            }

            if( table_statistics.variance.has_value() )
            {
                mean_stddev_variance_text.append(FormatText("%sStd.Dev: %s, Variance: %s",
                                                            mean_stddev_variance_text.empty() ? "" : ", ",
                                                            FPH::GetValueWithMinimumDecimals(*table_statistics.standard_deviation, frequency_table.dict_item).c_str(),
                                                            FPH::GetValueWithMinimumDecimals(*table_statistics.variance, frequency_table.dict_item).c_str()));
            }

            if( !mean_stddev_variance_text.empty() )
            {
                *m_htmlWriter << "<tr><td></td><td class='left'>" << mean_stddev_variance_text << "</td></tr>\n";
            }

            // mode / median
            std::string mode_median_text;

            if( table_statistics.mode_value.has_value() )
            {
                mode_median_text = FormatText("Mode: %s", FPH::GetFormattedValue(*table_statistics.mode_value, frequency_table.dict_item).c_str());
            }

            if( table_statistics.median.has_value() )
            {
                mode_median_text.append(FormatText("%sMedian: %s, Interpolated Median: %s",
                                                   mode_median_text.empty() ? "" : ", ",
                                                   FPH::GetValueWithMinimumDecimals(*table_statistics.median, frequency_table.dict_item).c_str(),
                                                   FPH::GetValueWithMinimumDecimals(*table_statistics.median_interpolated, frequency_table.dict_item).c_str()));
            }

            if( !mode_median_text.empty() )
            {
                *m_htmlWriter << "<tr><td></td><td class='left'>" << mode_median_text << "</td></tr>\n";
            }

            *m_htmlWriter << "</table>\n";
        }

        // percentiles
        if( table_statistics.percentiles.has_value() )
        {
            // format the values first
            const std::vector<std::string> formatted_percentile_percents = FPH::GetFormattedPercentilePercents(table_statistics);
            std::vector<std::string> formatted_value_type2s;
            std::vector<std::string> formatted_value_type6s;

            for( const FrequencyNumericStatistics::Percentile& percentile : *table_statistics.percentiles )
            {
                formatted_value_type2s.emplace_back(FPH::GetFormattedValue(percentile.value_type2, frequency_table.dict_item));
                formatted_value_type6s.emplace_back(FPH::GetValueWithMinimumDecimals(percentile.value_type6, frequency_table.dict_item));
            }

            const size_t max_decimals_after_value_type6 = FPH::GetNumberDecimalsUsed(formatted_value_type6s);

            for( std::string& formatted_value_type6 : formatted_value_type6s )
                FPH::EnsureValueHasMinimumDecimals(formatted_value_type6, max_decimals_after_value_type6);

            *m_htmlWriter << "<table id='percentiles'>\n"
                             "<thead>\n"
                             "<tr><th class='width-34'>Percentiles</th><th class='width-33'>Discontinuous</th><th class='width-33'>Continuous</th></tr>\n"
                             "</thead>\n";

            for( size_t i = 0; i < formatted_percentile_percents.size(); ++i )
            {
                *m_htmlWriter << "<tr><td>" << formatted_percentile_percents[i] << ( formatted_percentile_percents[i].empty() ? "" : "%" )
                              << "</td><td>" << formatted_value_type2s[i]
                              << "</td><td>" << formatted_value_type6s[i]
                              << "</td></tr>";
                    
            }

            *m_htmlWriter << "</table>\n";
        }
    }

    else
    {
        // alphanumeric statistics
        ASSERT(std::holds_alternative<FrequencyAlphanumericStatistics>(*frequency_table.table_statistics));
        ASSERT(frequency_table.frequency_printer_options.GetShowStatistics());
        const FrequencyAlphanumericStatistics& table_statistics = std::get<FrequencyAlphanumericStatistics>(*frequency_table.table_statistics);
        const std::string category_text = get_category_text(table_statistics);

        *m_htmlWriter << "<table id='statistics'>\n"
                      << u8"<tr><td class='width-10 left bold'>• Statistics" << "</td><td class='left'>" << category_text << "</td></tr>\n"
                         "</table>\n";
    }

    *m_htmlWriter << "</div>\n";
}
