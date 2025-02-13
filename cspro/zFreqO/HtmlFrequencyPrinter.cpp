#include "stdafx.h"
#include "HtmlFrequencyPrinter.h"
#include <zUtilO/StdioFileUnicode.h>
#include <zHtml/HtmlWriter.h>
#include <zEngineF/EngineUI.h>


HtmlFrequencyPrinter::HtmlFrequencyPrinter(CStdioFileUnicode& file)
    :   m_file(&file),
        m_ownedHtmlWriter(std::make_unique<HtmlStringWriter>()),
        m_htmlWriter(*m_ownedHtmlWriter),
        m_writeHeaderAndFooter(false)
{
}


HtmlFrequencyPrinter::HtmlFrequencyPrinter(NullTerminatedString filename)
    :   m_ownedFile(std::make_unique<CStdioFileUnicode>()),
        m_file(m_ownedFile.get()),
        m_ownedHtmlWriter(std::make_unique<HtmlStringWriter>()),
        m_htmlWriter(*m_ownedHtmlWriter),
        m_writeHeaderAndFooter(true)
{
    SetupEnvironmentToCreateFile(filename);

    if( !m_ownedFile->Open(filename.c_str(), CFile::modeWrite | CFile::modeCreate) )
        throw CSProException(L"Could not create the frequency file: %s", filename.c_str());

    PrintHeader();
}


HtmlFrequencyPrinter::HtmlFrequencyPrinter(HtmlWriter& html_writer, bool writer_header_and_footer)
    :   m_file(nullptr),
        m_htmlWriter(html_writer),
        m_writeHeaderAndFooter(writer_header_and_footer)
{
    if( m_writeHeaderAndFooter )
        PrintHeader();
}


HtmlFrequencyPrinter::~HtmlFrequencyPrinter()
{
    if( m_writeHeaderAndFooter )
        m_htmlWriter << L"</body>\n</html>\n";

    if( m_file != nullptr )
    {
        m_file->WriteString(assert_cast<HtmlStringWriter&>(m_htmlWriter).str());

        if( m_ownedFile != nullptr )
            m_ownedFile->Close();
    }
}


void HtmlFrequencyPrinter::PrintHeader()
{
    ASSERT(m_writeHeaderAndFooter);

    m_htmlWriter.WriteDefaultHeader(L"Frequency", Html::CSS::Common);
    m_htmlWriter << L"\n<body class='container-page'>\n";
}


void HtmlFrequencyPrinter::StartFrequencyGroup()
{
    m_expectingFrequencyGroupFirstTable = true;
}


void HtmlFrequencyPrinter::Print(const FrequencyTable& frequency_table)
{
    if (m_expectingFrequencyGroupFirstTable)
    {
        const std::vector<std::wstring>& header_titles = frequency_table.frequency_printer_options.GetHeadings();
        const size_t title_header_count = header_titles.size();

        for (size_t i = 0; i < title_header_count; ++i)
        {
            if (i == 0)
            {
                m_htmlWriter << L"<hgroup>\n";
            }

            m_htmlWriter << L"<h1 class='center'>" << header_titles[i] << L"</h1>\n";

            if (i == title_header_count - 1)
            {
                m_htmlWriter << L"</hgroup>\n";
            }
        }

        m_expectingFrequencyGroupFirstTable = false;
    }


    std::wstring table_title_alignment;
    if (frequency_table.special_formatting == FrequencyTable::SpecialFormatting::CenterTitles)
    {
        table_title_alignment = L" class='center'";
    }

    if (!frequency_table.titles.empty())
    {
        m_htmlWriter << L"<hgroup>\n";

        for( size_t i = 0; i < frequency_table.titles.size(); ++i )
        {
            // format logic based titles using HTML when possible
            const auto& logic_based_title_lookup = frequency_table.logic_based_titles.find(i);

            if( logic_based_title_lookup != frequency_table.logic_based_titles.cend() )
            {
                const std::tuple<std::wstring, std::wstring>& heading_and_logic = logic_based_title_lookup->second;
                EngineUI::ColorizeLogicNode colorize_logic_node { std::get<1>(heading_and_logic), std::wstring() };

                if( SendEngineUIMessage(EngineUI::Type::ColorizeLogic, colorize_logic_node) == 1 )
                {
                    m_htmlWriter << L"<h2" << table_title_alignment << L">" << std::get<0>(heading_and_logic) <<
                                    L"<span style=\"font-family: Consolas\">" << colorize_logic_node.html.c_str() << L"</span></h2>\n";
                    continue;
                }
            }

            // otherwise write out the title as text
            m_htmlWriter << L"<h2" << table_title_alignment << L">" << frequency_table.titles[i] << L"</h2>\n";
        }

        m_htmlWriter << L"</hgroup>\n";
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
    m_htmlWriter << L"<div class='container-freq'>\n";
    m_htmlWriter << L"<table>\n";

    // print column information
    const bool show_net_percents = FPH::ShowFrequencyTableNetPercents(frequency_table);
    const std::wstring value_column_justification = FPH::FrequencyTableValuesAreNumeric(frequency_table) ? L"right" : L"left";

    const bool display_value_column = frequency_table.distinct;
    std::wstring value_label_columns;

    if (display_value_column)
    {
        value_label_columns += L"<th rowspan='2' class='" + value_column_justification + L" bottom'>Value</th>";
    }

    value_label_columns += L"<th rowspan='2' class='left bottom'>Label</th>";

    const std::wstring colspan = show_net_percents ? L"3" : L"2";
    const std::wstring headerRow1 = L"<tr>" + value_label_columns + L"<th colspan='"
        + colspan + L"' class='center'>Frequency</th><th colspan='"
        + colspan + L"' class='center'>Cumulative</th></tr>\n";

    const std::wstring totalPercentColumns = L"<th>Total</th><th>Percent</th>";
    const std::wstring netPercentColumn = L"<th>Net Percent</th>";
    const std::wstring headerRow2 = L"<tr>" + totalPercentColumns + (show_net_percents ? netPercentColumn : L"")
        + totalPercentColumns + (show_net_percents ? netPercentColumn : L"") + L"</tr>\n";

    m_htmlWriter << L"<thead>\n";
    m_htmlWriter << headerRow1.c_str() << headerRow2.c_str();
    m_htmlWriter << L"</thead>\n";

    // print each of the frequency rows
    const int number_frequency_decimals = frequency_table.frequency_printer_options.GetUsingDecimals() ?
        frequency_table.frequency_printer_options.GetDecimals() : 0;

    std::wstring OutOfValueSetRowStyle;

    auto print_row = [&](const std::wstring& category_value, const std::wstring& category_label, double count,
                         const FrequencyRowStatistics& frequency_row_statistics, bool display_value_column)
    {
        auto get_formatted_frequency = [&](const std::optional<double>& frequency)
        {
            return frequency.has_value() ? FormatTextCS2WS(L"%0.*f", number_frequency_decimals, frequency) : std::wstring();
        };

        auto get_formatted_percent = [&](const std::optional<double>& percent)
        {
            return percent.has_value() ? FormatTextCS2WS(L"%3.1f", *percent) : std::wstring();
        };

        m_htmlWriter << L"<tr>";

        if (display_value_column)
        {
            m_htmlWriter << L"<td class='" << OutOfValueSetRowStyle.c_str() << (OutOfValueSetRowStyle.empty() ? L"" : L" ") << value_column_justification.c_str() << L"'>" << category_value << L"</td>";
        }

        m_htmlWriter << L"<td class='" << OutOfValueSetRowStyle.c_str() << (OutOfValueSetRowStyle.empty() ? L"" : L" ") << L"left'>" << category_label << L"</td>";
        m_htmlWriter << L"<td class='" << OutOfValueSetRowStyle.c_str() << L"'>" << get_formatted_frequency(count) << L"</td>";

        std::wstring frequency_percent = get_formatted_percent(frequency_row_statistics.percent_against_total);
        frequency_percent += frequency_percent.empty() ? L"" : L"%";
        m_htmlWriter << L"<td class='" << OutOfValueSetRowStyle.c_str() << L"'>" << frequency_percent << L"</td>";

        if (show_net_percents)
        {
            std::wstring frequency_net_percent = get_formatted_percent(frequency_row_statistics.percent_against_non_blank_total);
            frequency_net_percent += frequency_net_percent.empty() ? L"" : L"%";
            m_htmlWriter << L"<td class='" << OutOfValueSetRowStyle.c_str() << L"'>" << frequency_net_percent << L"</td>";
        }

        m_htmlWriter << L"<td class='" << OutOfValueSetRowStyle.c_str() << L"'>" << get_formatted_frequency(frequency_row_statistics.cumulative_count) << L"</td>";

        std::wstring cumulative_percent = get_formatted_percent(frequency_row_statistics.cumulative_percent_against_total);
        cumulative_percent += cumulative_percent.empty() ? L"" : L"%";
        m_htmlWriter << L"<td class='" << OutOfValueSetRowStyle.c_str() << L"'>" << cumulative_percent << L"</td>";

        if (show_net_percents)
        {
            std::wstring cumulative_net_percent = get_formatted_percent(frequency_row_statistics.cumulative_percent_against_non_blank_total);
            cumulative_net_percent += cumulative_net_percent.empty() ? L"" : L"%";
            m_htmlWriter << L"<td class='" << OutOfValueSetRowStyle.c_str() << L"'>" << cumulative_net_percent << L"</td>";
        }

        m_htmlWriter << L"</tr>\n";
    };

    print_row(L"", L"Total", frequency_table.total_count, FPH::CreateTotalFrequencyRowStatistics(frequency_table, true), display_value_column);

    for (size_t i = 0; i < frequency_table.frequency_rows.size(); ++i)
    {
        const FrequencyRow& frequency_row = frequency_table.frequency_rows[i];
        std::wstring formatted_value;
        std::wstring display_label = frequency_row.display_label;

        // add the values if printing values
        if (frequency_table.distinct)
        {
            formatted_value = frequency_row.formatted_values.front();
        }

        // if not printing values but there is no label, then use the value as the label
        else if (display_label.empty())
        {
            display_label = SO::CreateSingleString<false>(frequency_row.formatted_values);
        }

        // use indicator css to show details about each row
        if (frequency_row.mark_as_out_of_value_set)
        {
            OutOfValueSetRowStyle = L"outOfValueSetRow";
        }

        const std::wstring& category_value = formatted_value;
        const std::wstring& category_label = display_label;

        print_row(category_value, category_label, frequency_row.count, frequency_table.frequency_row_statistics[i], display_value_column);
        OutOfValueSetRowStyle = L"";
    }

    m_htmlWriter << L"</table>\n";
    m_htmlWriter << L"</div>\n";
}


void HtmlFrequencyPrinter::PrintStatistics(const FrequencyTable& frequency_table)
{
    ASSERT(frequency_table.table_statistics.has_value());

    auto get_category_text = [](const auto& table_statistics)
    {
        return FormatTextCS2WS(L"%d categor%s", (int)table_statistics.number_defined_categories,
                               PluralizeWord(table_statistics.number_defined_categories, L"y", L"ies"));
    };

    m_htmlWriter << L"<div class='container-stat'>\n";

    // numeric statistics
    if (std::holds_alternative<FrequencyNumericStatistics>(*frequency_table.table_statistics))
    {
        const FrequencyNumericStatistics& table_statistics = std::get<FrequencyNumericStatistics>(*frequency_table.table_statistics);

        if (frequency_table.frequency_printer_options.GetShowStatistics())
        {
            // categories
            std::wstring category_text = get_category_text(table_statistics);

            for (size_t i = 0; i < table_statistics.non_blank_special_values_used.size(); ++i)
            {
                if (i == 0)
                {
                    category_text.append(L" as well as ");
                }

                else
                {
                    category_text.append(L", ");

                    if ((i + 1) == table_statistics.non_blank_special_values_used.size())
                        category_text.append(L"and ");
                }

                category_text.append(SpecialValues::ValueToString(table_statistics.non_blank_special_values_used[i], false));
            }

            m_htmlWriter << L"<table id='statistics'>\n";
            m_htmlWriter << L"<tr><td class='width-10 left bold'>• Statistics" << L"</td><td class='left'>" << category_text << L"</td></tr>\n";

            // min / max
            if (table_statistics.min_value.has_value())
            {
                m_htmlWriter << L"<tr><td></td><td class='left'>Min: " << FPH::GetFormattedValue(*table_statistics.min_value, frequency_table.dict_item)
                    << L", Max: " << FPH::GetFormattedValue(*table_statistics.max_value, frequency_table.dict_item)
                    << L"</td></tr>\n";
            }

            // mean / standard deviation / variance
            std::wstring mean_stddev_variance_text;

            if (table_statistics.mean.has_value())
            {
                mean_stddev_variance_text = L"Mean: " + FPH::GetValueWithMinimumDecimals(*table_statistics.mean, frequency_table.dict_item);
            }

            if (table_statistics.variance.has_value())
            {
                SO::AppendFormat(mean_stddev_variance_text, L"%sStd.Dev: %s, Variance: %s",
                    mean_stddev_variance_text.empty() ? L"" : L", ",
                    FPH::GetValueWithMinimumDecimals(*table_statistics.standard_deviation, frequency_table.dict_item).c_str(),
                    FPH::GetValueWithMinimumDecimals(*table_statistics.variance, frequency_table.dict_item).c_str());
            }

            if (!mean_stddev_variance_text.empty())
                m_htmlWriter << L"<tr><td></td><td class='left'>" << mean_stddev_variance_text << L"</td></tr>\n";

            // mode / median
            std::wstring mode_median_text;

            if (table_statistics.mode_value.has_value())
            {
                mode_median_text = FormatTextCS2WS(L"Mode: %s", FPH::GetFormattedValue(*table_statistics.mode_value, frequency_table.dict_item).c_str());
            }

            if (table_statistics.median.has_value())
            {
                SO::AppendFormat(mode_median_text, L"%sMedian: %s, Interpolated Median: %s",
                    mode_median_text.empty() ? L"" : L", ",
                    FPH::GetValueWithMinimumDecimals(*table_statistics.median, frequency_table.dict_item).c_str(),
                    FPH::GetValueWithMinimumDecimals(*table_statistics.median_interpolated, frequency_table.dict_item).c_str());
            }

            if (!mode_median_text.empty())
                    m_htmlWriter << L"<tr><td></td><td class='left'>" << mode_median_text << L"</td></tr>\n";

            m_htmlWriter << L"</table>\n";
        }

        // percentiles
        if (table_statistics.percentiles.has_value())
        {
            // format the values first
            const std::vector<std::wstring> formatted_percentile_percents = FPH::GetFormattedPercentilePercents(table_statistics);
            std::vector<std::wstring> formatted_value_type2s;
            std::vector<std::wstring> formatted_value_type6s;

            for (const FrequencyNumericStatistics::Percentile& percentile : *table_statistics.percentiles)
            {
                formatted_value_type2s.emplace_back(FPH::GetFormattedValue(percentile.value_type2, frequency_table.dict_item));
                formatted_value_type6s.emplace_back(FPH::GetValueWithMinimumDecimals(percentile.value_type6, frequency_table.dict_item));
            }

            size_t max_decimals_after_value_type6 = FPH::GetNumberDecimalsUsed(formatted_value_type6s);

            for (std::wstring& formatted_value_type6 : formatted_value_type6s)
            {
                FPH::EnsureValueHasMinimumDecimals(formatted_value_type6, max_decimals_after_value_type6);
            }

            m_htmlWriter << L"<table id='percentiles'>\n";
            m_htmlWriter << L"<thead>\n";
            m_htmlWriter << L"<tr><th class='width-34'>Percentiles</th><th class='width-33'>Discontinuous</th><th class='width-33'>Continuous</th></tr>\n";
            m_htmlWriter << L"</thead>\n";

            for (size_t i = 0; i < formatted_percentile_percents.size(); ++i)
            {
                std::wstring percentile_percents = formatted_percentile_percents[i];
                percentile_percents += percentile_percents.empty() ? L"" : L"%";
                m_htmlWriter << L"<tr><td>" << percentile_percents << L"</td><td>"
                    << formatted_value_type2s[i] << L"</td><td>"
                    << formatted_value_type6s[i] << L"</td></tr>";
            }

            m_htmlWriter << L"</table>\n";
        }
    }

    else
    {
        // alphanumeric statistics
        ASSERT(std::holds_alternative<FrequencyAlphanumericStatistics>(*frequency_table.table_statistics));
        ASSERT(frequency_table.frequency_printer_options.GetShowStatistics());
        const FrequencyAlphanumericStatistics& table_statistics = std::get<FrequencyAlphanumericStatistics>(*frequency_table.table_statistics);
        const std::wstring category_text = get_category_text(table_statistics);

        m_htmlWriter << L"<table id='statistics'>\n";
        m_htmlWriter << L"<tr><td class='width-10 left bold'>• Statistics" << L"</td><td class='left'>" << category_text << L"</td></tr>\n";
        m_htmlWriter << L"</table>\n";
    }

    m_htmlWriter << L"</div>\n";
}
