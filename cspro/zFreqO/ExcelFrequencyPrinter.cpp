#include "stdafx.h"
#include "ExcelFrequencyPrinter.h"
#include <zExcelO/ExcelWriter.h>


// --------------------------------------------------------------------------
// ExcelFrequencyPrinterWorker
// --------------------------------------------------------------------------

class ExcelFrequencyPrinterWorker
{
public:
    ExcelFrequencyPrinterWorker(const std::wstring& filename);
    ~ExcelFrequencyPrinterWorker();

    void Print(const FrequencyTable& frequency_table);

private:
    lxw_format* CreateFormat(ExcelWriter::Format format, const std::optional<std::wstring>& numeric_format = std::nullopt);

    static std::wstring CreateNumericFormatString(int decimals, bool add_percent_sign);

    void AddWorksheet();

    void SetupTitle();
    void SetupColumns();

    void AddTotalAndRows();
    void AddStatistics();
    void AddPercentiles();

    void LogColumnTextForAdjustingColumnWidths(uint16_t column, wstring_view text);
    void AdjustColumnWidths();

private:
    ExcelWriter m_excelWriter;
    std::map<std::wstring, int> m_worksheetNameCounter;

    using FormatOptions = std::tuple<ExcelWriter::Format, std::optional<std::wstring>>;
    std::map<FormatOptions, lxw_format*> m_formats;
    lxw_format* m_numericItemFormatter;

    const FrequencyTable* m_frequencyTable;
    bool m_showNetPercents;
    bool m_valueLabelColumnsCombined;
    uint16_t m_totalColumns;
    uint32_t m_row;
    std::vector<size_t> m_columnStringLengths;
};


ExcelFrequencyPrinterWorker::ExcelFrequencyPrinterWorker(const std::wstring& filename)
    :   m_frequencyTable(nullptr),
        m_numericItemFormatter(nullptr),
        m_showNetPercents(false),
        m_valueLabelColumnsCombined(false),
        m_totalColumns(0),
        m_row(0)
{
    m_excelWriter.CreateWorkbook(filename, false);
}


ExcelFrequencyPrinterWorker::~ExcelFrequencyPrinterWorker()
{
    m_excelWriter.Close();
}


void ExcelFrequencyPrinterWorker::Print(const FrequencyTable& frequency_table)
{
    m_frequencyTable = &frequency_table;

    // create a formatter for numeric items
    if( m_frequencyTable->dict_item != nullptr && m_frequencyTable->dict_item->GetContentType() == ContentType::Numeric )
    {
        m_numericItemFormatter = CreateFormat(ExcelWriter::Format::None, CreateNumericFormatString(m_frequencyTable->dict_item->GetDecimal(), false));
    }

    else
    {
        m_numericItemFormatter = nullptr;
    }

    m_showNetPercents = FPH::ShowFrequencyTableNetPercents(*m_frequencyTable);
    m_valueLabelColumnsCombined = ( m_frequencyTable->dict_item == nullptr );
    m_totalColumns = ( ( !m_valueLabelColumnsCombined && m_frequencyTable->distinct ) ?  2 : 1 ) +
                        4 +
                        ( m_showNetPercents ? 2 : 0 );
    m_row = 0;
    m_columnStringLengths.resize(m_valueLabelColumnsCombined ? 1 : 2, 0);

    AddWorksheet();

    SetupTitle();
        
    SetupColumns();

    AddTotalAndRows();

    if( m_frequencyTable->frequency_printer_options.GetShowStatistics() )
        AddStatistics();

    if( m_frequencyTable->frequency_printer_options.GetUsingPercentiles() )
        AddPercentiles();

    AdjustColumnWidths();
}


lxw_format* ExcelFrequencyPrinterWorker::CreateFormat(const ExcelWriter::Format format, const std::optional<std::wstring>& numeric_format/* = std::nullopt*/)
{
    FormatOptions options(format, numeric_format);
    const auto& format_lookup = m_formats.find(options);

    if( format_lookup != m_formats.cend() )
    {
        return format_lookup->second;
    }

    else
    {
        return m_formats.try_emplace(std::move(options), m_excelWriter.GetFormat(format, numeric_format)).first->second;
    }
}


std::wstring ExcelFrequencyPrinterWorker::CreateNumericFormatString(const int decimals, const bool add_percent_sign)
{
    std::wstring numeric_format = _T("0");

    if( decimals > 0 )
        SO::AppendFormat(numeric_format, _T(".%0*d"), decimals, 0);

    if( add_percent_sign )
        numeric_format.push_back('%');

    return numeric_format;
}


void ExcelFrequencyPrinterWorker::AddWorksheet()
{
    // because worksheets must have unique names, we may have to add a suffix to the worksheet name
    std::wstring worksheet_name = FPH::GetFrequencyTableName(*m_frequencyTable);
    auto worksheet_name_lookup = m_worksheetNameCounter.find(worksheet_name);

    if( worksheet_name_lookup == m_worksheetNameCounter.end() )
    {
        m_worksheetNameCounter.try_emplace(worksheet_name, 1);
    }

    else
    {
        worksheet_name_lookup->second = worksheet_name_lookup->second + 1;
        SO::AppendFormat(worksheet_name, _T("_%d"), worksheet_name_lookup->second);
    }

    m_excelWriter.AddAndSetCurrentWorksheet(worksheet_name);        
}


void ExcelFrequencyPrinterWorker::SetupTitle()
{
    const std::wstring title_text = SO::CreateSingleString<false>(m_frequencyTable->titles, _T("\n"));

    lxw_format* format = CreateFormat(ExcelWriter::Format::TitleFont | ExcelWriter::Format::TextWrap | ExcelWriter::Format::Top);

    m_excelWriter.WriteMerged(m_row, 0, m_row, m_totalColumns - 1, title_text, format);

    const double row_height = m_excelWriter.GetHeightForText(m_frequencyTable->titles.size(), format);
    m_excelWriter.SetRowHeight(m_row, row_height);

    ++m_row;
}


void ExcelFrequencyPrinterWorker::SetupColumns()
{
    uint16_t column = 0;

    lxw_format* centered_line_on_left_bold_format = CreateFormat(ExcelWriter::Format::Center |
                                                                 ExcelWriter::Format::LineOnLeft |
                                                                 ExcelWriter::Format::Bold);
    lxw_format* right_justified_line_on_left_bold_format = m_excelWriter.GetFormat(ExcelWriter::Format::Right |
                                                                                   ExcelWriter::Format::LineOnLeft |
                                                                                   ExcelWriter::Format::Bold);
    lxw_format* right_justified_bold_format = m_excelWriter.GetFormat(ExcelWriter::Format::Right | ExcelWriter::Format::Bold);
    lxw_format* bold_format = m_excelWriter.GetFormat(ExcelWriter::Format::Bold);

    if( m_valueLabelColumnsCombined || m_frequencyTable->distinct )
    {
        lxw_format* format = FPH::FrequencyTableValuesAreNumeric(*m_frequencyTable) ? right_justified_bold_format : bold_format;
        m_excelWriter.WriteMerged(m_row, column, m_row + 1, column, FPH::ValueLabel, format);
        ++column;
    }

    if( !m_valueLabelColumnsCombined )
    {
        m_excelWriter.WriteMerged(m_row, column, m_row + 1, column, FPH::LabelLabel, bold_format);
        ++column;
    }

    auto write_column_set = [&](const TCHAR* type)
    {
        m_excelWriter.WriteMerged(m_row, column, m_row, column + ( m_showNetPercents ? 2 : 1 ), type, centered_line_on_left_bold_format);

        m_excelWriter.Write(m_row + 1, column++, FPH::TotalLabel, right_justified_line_on_left_bold_format);
        m_excelWriter.Write(m_row + 1, column++, FPH::PercentLabel, right_justified_bold_format);

        if( m_showNetPercents )
            m_excelWriter.Write(m_row + 1, column++, FPH::NetPercentLabel, right_justified_bold_format);
    };

    write_column_set(FPH::FrequencyLabel);
    write_column_set(FPH::CumulativeLabel);

    m_row += 2;

    m_excelWriter.FreezeTopRow(m_row);
}


void ExcelFrequencyPrinterWorker::AddTotalAndRows()
{
    // set the formats
    const std::optional<std::wstring> percent_numeric_format = CreateNumericFormatString(1, true);
    std::optional<std::wstring> count_numeric_format;

    if( m_frequencyTable->frequency_printer_options.GetUsingDecimals() )
        count_numeric_format = CreateNumericFormatString(m_frequencyTable->frequency_printer_options.GetDecimals(), false);

    // write the rows
    const uint32_t starting_row = m_row;

    auto fill_row = [&](const std::variant<double, std::wstring>& value, wstring_view label,//
                        double count, const FrequencyRowStatistics& frequency_row_statistics)
    {
        uint16_t column = 0;

        // write the value and label
        if( m_valueLabelColumnsCombined || m_frequencyTable->distinct )
        {
            bool value_written;

            if( std::holds_alternative<double>(value) )
            {
                if( value_written = !IsSpecial(std::get<double>(value)); value_written )
                    m_excelWriter.Write(m_row, column, std::get<double>(value), m_numericItemFormatter);
            }

            else
            {
                const std::wstring& text = std::get<std::wstring>(value);

                if( value_written = !text.empty(); value_written )
                {
                    LogColumnTextForAdjustingColumnWidths(column, text);
                    m_excelWriter.Write(m_row, column, text);
                }
            }

            // when combining values and labels, if no value is written, write the label
            if( !value_written && m_valueLabelColumnsCombined )
            {
                LogColumnTextForAdjustingColumnWidths(column, label);
                m_excelWriter.Write(m_row, column, label);
            }

            ++column;
        }

        if( !m_valueLabelColumnsCombined )
        {
            LogColumnTextForAdjustingColumnWidths(column, label);
            m_excelWriter.Write(m_row, column++, label);
        }

        // write the counts and percents
        auto write_number = [&](double number, bool count_cell)
        {
            lxw_format* format = CreateFormat(count_cell ? ExcelWriter::Format::LineOnLeft : ExcelWriter::Format::None,
                                              count_cell ? count_numeric_format : percent_numeric_format);

            m_excelWriter.Write(m_row, column++, count_cell ? number : ( number / 100 ), format);
        };

        auto write_optional_number = [&](const std::optional<double>& optional_number, bool count_cell)
        {
            if( optional_number.has_value() )
            {
                write_number(*optional_number, count_cell);
            }

            else
            {
                // if there is no number, make sure that the line is still drawn if necessary
                if( count_cell )
                    m_excelWriter.Write(m_row, column, _T(""), CreateFormat(ExcelWriter::Format::LineOnLeft));

                ++column;
            }
        };

        write_number(count, true);
        write_number(frequency_row_statistics.percent_against_total, false);

        if( m_showNetPercents )
            write_optional_number(frequency_row_statistics.percent_against_non_blank_total, false);

        write_optional_number(frequency_row_statistics.cumulative_count, true);
        write_optional_number(frequency_row_statistics.cumulative_percent_against_total, false);

        if( m_showNetPercents )
            write_optional_number(frequency_row_statistics.cumulative_percent_against_non_blank_total, false);

        ++m_row;
    };

    // add the total row
    fill_row(std::wstring(), FPH::TotalLabel, m_frequencyTable->total_count, FPH::CreateTotalFrequencyRowStatistics(*m_frequencyTable, false));

    // add each frequency row
    for( size_t i = 0; i < m_frequencyTable->frequency_rows.size(); ++i )
    {
        const FrequencyRow& frequency_row = m_frequencyTable->frequency_rows[i];

        std::wstring display_label = frequency_row.display_label;

        // modify the default label for notappl
        if( frequency_row.value_is_blank && display_label == SpecialValues::ValueToString(NOTAPPL, false) )
        {
            display_label = _T("Not Applicable");
        }

        // if not printing values but there is no label, then use the value as the label
        else if( !m_frequencyTable->distinct && display_label.empty() )
        {
            display_label = SO::CreateSingleString<false>(frequency_row.formatted_values);
        }

        fill_row(frequency_row.values.front(), display_label, frequency_row.count, m_frequencyTable->frequency_row_statistics[i]);
    }

    // if necessary, indicate why the cumulative columns are not filled
    if( m_frequencyTable->has_multiple_labels_per_value )
    {
        lxw_format* warning_format = CreateFormat(ExcelWriter::Format::Italics | ExcelWriter::Format::LineOnLeft |
                                                  ExcelWriter::Format::Center | ExcelWriter::Format::Middle | ExcelWriter::Format::TextWrap);
        const uint16_t cumulative_start_column = m_totalColumns - 1 - ( m_showNetPercents ? 2 : 1 );
        m_excelWriter.WriteMerged(starting_row, cumulative_start_column, m_row - 1, m_totalColumns - 1, FPH::NoCumulativeColumnsWarning, warning_format);
    }
}


void ExcelFrequencyPrinterWorker::AddStatistics()
{
    ASSERT(m_frequencyTable->table_statistics.has_value());

    lxw_format* bold_format = m_excelWriter.GetFormat(ExcelWriter::Format::Bold);

    m_row += 2;
    m_excelWriter.Write(m_row++, 0, FPH::StatisticsLabel, bold_format);

    auto write_statistic = [&](const TCHAR* label, const auto& number_or_text, lxw_format* format = nullptr)
    {
        LogColumnTextForAdjustingColumnWidths(0, label);
        m_excelWriter.Write(m_row, 0, label);

        m_excelWriter.Write(m_row++, 1, number_or_text, format);
    };

    auto write_optional_statistic = [&](const TCHAR* label, const auto& optional_number, lxw_format* format = nullptr)
    {
        if( optional_number.has_value() )
            write_statistic(label, *optional_number, format);
    };

    // alphanumeric statistics
    if( std::holds_alternative<FrequencyAlphanumericStatistics>(*m_frequencyTable->table_statistics) )
    {
        const FrequencyAlphanumericStatistics& table_statistics = std::get<FrequencyAlphanumericStatistics>(*m_frequencyTable->table_statistics);
        write_statistic(FPH::CategoriesLabel, table_statistics.number_defined_categories);
    }

    // numeric statistics
    else
    {
        const FrequencyNumericStatistics& table_statistics = std::get<FrequencyNumericStatistics>(*m_frequencyTable->table_statistics);

        // categories
        write_statistic(FPH::CategoriesLabel, table_statistics.number_defined_categories);

        std::wstring special_value_categories;

        for( const double value : table_statistics.non_blank_special_values_used )
            SO::AppendWithSeparator(special_value_categories, SpecialValues::ValueToString(value, false), _T(", "));

        if( !special_value_categories.empty() )
            write_statistic(FPH::SpecialValuesLabel, special_value_categories, CreateFormat(ExcelWriter::Format::Right));

        // min / max
        write_optional_statistic(FPH::MinLabel, table_statistics.min_value, m_numericItemFormatter);
        write_optional_statistic(FPH::MaxLabel, table_statistics.max_value, m_numericItemFormatter);

        // mean / median / mode
        write_optional_statistic(FPH::MeanLabel, table_statistics.mean);
        write_optional_statistic(FPH::MedianLabel, table_statistics.median);
        write_optional_statistic(FPH::MedianInterpolatedLabel, table_statistics.median_interpolated);
        write_optional_statistic(FPH::ModeLabel, table_statistics.mode_value, m_numericItemFormatter);

        // variance / standard deviation
        write_optional_statistic(FPH::VarianceLabel, table_statistics.variance);
        write_optional_statistic(FPH::StandardDeviationLabel, table_statistics.standard_deviation);
    }
}


void ExcelFrequencyPrinterWorker::AddPercentiles()
{
    // percentiles only exist for numeric frequencies
    if( !m_frequencyTable->table_statistics.has_value() || !std::holds_alternative<FrequencyNumericStatistics>(*m_frequencyTable->table_statistics) )
        return;

    const FrequencyNumericStatistics& table_statistics = std::get<FrequencyNumericStatistics>(*m_frequencyTable->table_statistics);

    // make sure that percentiles exist (which they won't if no values were tallied)
    if( !table_statistics.percentiles.has_value() )
        return;

    lxw_format* right_justified_bold_format = m_excelWriter.GetFormat(ExcelWriter::Format::Right | ExcelWriter::Format::Bold);

    m_row += 2;
    LogColumnTextForAdjustingColumnWidths(0, FPH::PercentilesLabel);
    m_excelWriter.Write(m_row, 0, FPH::PercentilesLabel, right_justified_bold_format);
    m_excelWriter.Write(m_row, 1, FPH::DiscontinuousLabel, right_justified_bold_format);
    m_excelWriter.Write(m_row++, 2, FPH::ContinuousTextLabel, right_justified_bold_format);

    lxw_format* percent_format = CreateFormat(ExcelWriter::Format::None, CreateNumericFormatString(0, true));

    for( const FrequencyNumericStatistics::Percentile& percentile : *table_statistics.percentiles )
    {
        m_excelWriter.Write(m_row, 0, percentile.percentile, percent_format);
        m_excelWriter.Write(m_row, 1, percentile.value_type2, m_numericItemFormatter);
        m_excelWriter.Write(m_row++, 2, percentile.value_type6);
    }
}


void ExcelFrequencyPrinterWorker::LogColumnTextForAdjustingColumnWidths(uint16_t column, wstring_view text)
{
    ASSERT(column < m_columnStringLengths.size());
    m_columnStringLengths[column] = std::max(text.length(), m_columnStringLengths[column]);
}


void ExcelFrequencyPrinterWorker::AdjustColumnWidths()
{
    uint16_t column = 0;

    // the value and the label (if applicable) columns will be sized based on the
    // longest text added, with a minimum width to fit a 15-digit number
    for( ; column < m_columnStringLengths.size(); ++column )
    {
        const double value_label_width = std::max(15.71,
                                                  m_excelWriter.GetWidthForText(ExcelWriter::TextType::Characters, m_columnStringLengths[column]));
        m_excelWriter.SetColumnWidth(column, value_label_width);
    }

    // the default column/percent widths will be a width that fits a number less than a billion,
    // or the column heading if net percents are being shown
    double column_percent_width = m_showNetPercents ? 10.86 : 9.29;

    // increase the width if incredibly large numbers are being used
    if( m_frequencyTable->total_count > 1000000000 )
        column_percent_width = 11.29;

    // if the value and labels columns are combined, the first total column needs to fit the
    // percentile text, so use a larger width in that case
    if( m_valueLabelColumnsCombined && m_frequencyTable->table_statistics.has_value() &&            
        std::holds_alternative<FrequencyNumericStatistics>(*m_frequencyTable->table_statistics) &&
        std::get<FrequencyNumericStatistics>(*m_frequencyTable->table_statistics).percentiles.has_value() )
    {
        column_percent_width = 13.00;
    }

    for( ; column < m_totalColumns; ++column )
        m_excelWriter.SetColumnWidth(column, column_percent_width);
}




// --------------------------------------------------------------------------
// ExcelFrequencyPrinter
// --------------------------------------------------------------------------

ExcelFrequencyPrinter::ExcelFrequencyPrinter(NullTerminatedString filename)
    :   m_worker(std::make_unique<ExcelFrequencyPrinterWorker>(filename))
{    
}


ExcelFrequencyPrinter::~ExcelFrequencyPrinter()
{        
}


void ExcelFrequencyPrinter::Print(const FrequencyTable& frequency_table)
{
    m_worker->Print(frequency_table);
}
