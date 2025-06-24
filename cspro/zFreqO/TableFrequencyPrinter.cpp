#include "stdafx.h"
#include "TableFrequencyPrinter.h"
#include <zTableO/Table.h>


namespace FTD // frequency table definitions
{
    constexpr const char* FileLabel = "CSPro Frequencies";
    constexpr const char* FileName  = "CSPRO_FREQUENCIES";

    constexpr int TitlePaddingBottom = 10;

    constexpr COLORREF NoCumulativeColumnsColor = RGB(50, 50, 50);
    constexpr bool NoCumulativeColumnsItalic    = true;

    constexpr int PercentDecimals = 1;

    constexpr COLORREF OutOfRangeColor = RGB(155, 0, 0);
}


// --------------------------------------------------------------------------
// TableFrequencyPrinterWorker
// --------------------------------------------------------------------------

class TableFrequencyPrinterWorker
{
public:
    TableFrequencyPrinterWorker();

    void Save(const std::string& file_path);

    void Print(const FrequencyTable& frequency_table);

private:
    template<typename T, typename TO, typename UFCB>
    T* CreateFormat(TO& table_object, FMT_ID format_id, const UFCB& update_format_callback_function);

    static constexpr NUM_DECIMALS DecimalsIntToFormat(int decimals);
    CDataCellFmt* GetColumnHeadDecimalsFormat(int decimals);

    void Initialize();
    void SetupTitle();
    void SetupPageAndEndNote();
    void SetupColumns();

    CDataCellFmt* GetStubOrCellFormat(FMT_ID format_id, bool out_of_range_row, bool line_on_bottom_row, bool percent_cell, bool blank_cell);

    void AddTotalAndRows();
    void AddStatistics();
    void AddPercentiles();

private:
    CTabSet m_tabSet;

    // the current objects being generated
    const FrequencyTable* m_frequencyTable;
    bool m_showNetPercents;
    CTable* m_table;

    // formats (memory controlled by CTabSet)
    CFmt* m_pageNoteFormat;
    std::map<int, CDataCellFmt*> m_columnFormatsMap;
    using StubOrCellDataCellFormatOptions = std::tuple<FMT_ID, bool, bool, bool, bool>;
    std::map<StubOrCellDataCellFormatOptions, CDataCellFmt*> m_stubOrCellDataCellFormats;
};


TableFrequencyPrinterWorker::TableFrequencyPrinterWorker()
    :   m_frequencyTable(nullptr),
        m_showNetPercents(false),
        m_table(nullptr),
        m_pageNoteFormat(nullptr)
{
    Initialize();
}


void TableFrequencyPrinterWorker::Save(const std::string& file_path)
{
    m_tabSet.Save(UTF8_TODO::GetCString(file_path), CString());
}


void TableFrequencyPrinterWorker::Print(const FrequencyTable& frequency_table)
{
    m_frequencyTable = &frequency_table;

    m_showNetPercents = FPH::ShowFrequencyTableNetPercents(frequency_table);

    m_table = new CTable;
    m_tabSet.AddTable(m_table);

    m_table->SetName(UTF8_TODO::GetCString(FPH::GetFrequencyTableName(*m_frequencyTable)));

    SetupTitle();

    // the statistics end note will only be shown (in Table Viewer) if there are rows of non-blank data
    m_table->m_bHasFreqStats = ( m_frequencyTable->frequency_printer_options.GetShowStatistics() &&
                                 frequency_table.total_non_blank_count > 0 );

    SetupPageAndEndNote();

    SetupColumns();

    AddTotalAndRows();

    if( m_table->m_bHasFreqStats )
        AddStatistics();

    if( m_frequencyTable->frequency_printer_options.GetUsingPercentiles() )
        AddPercentiles();
}


template<typename T, typename TO, typename UFCB>
T* TableFrequencyPrinterWorker::CreateFormat(TO& table_object, const FMT_ID format_id, const UFCB& update_format_callback_function)
{
    CFmtReg* const format_registry = table_object.GetFmtRegPtr();
    ASSERT(format_registry != nullptr);

    const T* source_format = assert_cast<const T*>(format_registry->GetFmt(format_id));
    T* new_format = new T(*source_format);

    update_format_callback_function(new_format);

    new_format->SetIndex(format_registry->GetNextCustomFmtIndex(*new_format));

    format_registry->AddFmt(new_format);

    return new_format;
}


constexpr NUM_DECIMALS TableFrequencyPrinterWorker::DecimalsIntToFormat(const int decimals)
{
    ASSERT(decimals >= 1 && decimals <= 5);

    return ( decimals == 1 ) ? NUM_DECIMALS::NUM_DECIMALS_ONE :
           ( decimals == 2 ) ? NUM_DECIMALS::NUM_DECIMALS_TWO :
           ( decimals == 3 ) ? NUM_DECIMALS::NUM_DECIMALS_THREE :
           ( decimals == 4 ) ? NUM_DECIMALS::NUM_DECIMALS_FOUR :
                               NUM_DECIMALS::NUM_DECIMALS_FIVE;
}


CDataCellFmt* TableFrequencyPrinterWorker::GetColumnHeadDecimalsFormat(const int decimals)
{
    const auto& format_lookup = m_columnFormatsMap.find(decimals);

    if( format_lookup != m_columnFormatsMap.cend() )
        return format_lookup->second;

    CDataCellFmt* column_head_format = CreateFormat<CDataCellFmt>(m_tabSet, FMT_ID_COLHEAD,
        [&](CDataCellFmt* format)
        {
            format->SetNumDecimals(DecimalsIntToFormat(decimals));
        });

    m_columnFormatsMap.try_emplace(decimals, column_head_format);

    return column_head_format;
}


void TableFrequencyPrinterWorker::Initialize()
{
    m_tabSet.SetName(FTD::FileName);
    m_tabSet.SetLabel(FTD::FileLabel);
    m_tabSet.SetSpecType(SPECTYPE::FREQ_SPEC);
    m_tabSet.SetNumLevels(1);

    m_pageNoteFormat = CreateFormat<CFmt>(m_tabSet, FMT_ID_PAGENOTE,
        [&](CFmt* const format)
        {
            LOGFONT lf;
            format->GetFont()->GetLogFont(&lf);
            lf.lfItalic = FTD::NoCumulativeColumnsItalic;
            format->SetFont(&lf);
            format->SetTextColor(FMT_COLOR{ FTD::NoCumulativeColumnsColor, false });
        });
}


void TableFrequencyPrinterWorker::SetupTitle()
{
    CTblOb* const title = m_table->GetTitle();
    ASSERT(title->GetDerFmt() == nullptr);

    CFmt* const title_format = CreateFormat<CFmt>(m_tabSet, FMT_ID_TITLE, [](CFmt* /*format*/) { });
    title->SetFmt(title_format);

    const std::string title_text = SO::CreateSingleString<false>(m_frequencyTable->titles, "\n");
    title_format->SetCustomText(UTF8_TODO::GetCString(title_text));

    // set the title's height
    CClientDC dc(nullptr);
    dc.SelectObject(title_format->GetFont());

    TEXTMETRIC metrics;
    dc.GetTextMetrics(&metrics);

    CGrdViewInfo grid_view_info;
    grid_view_info.SetCurrSize(CSize(0, m_frequencyTable->titles.size() * metrics.tmHeight + FTD::TitlePaddingBottom));
    title->AddGrdViewInfo(grid_view_info);
}


void TableFrequencyPrinterWorker::SetupPageAndEndNote()
{
    // the page note will indicate why cumulative frequencies are not filled;
    // the end note will contain the statistics
    if( m_frequencyTable->has_multiple_labels_per_value || m_table->m_bHasFreqStats )
    {
        ASSERT(m_table->GetDerFmt() == nullptr);
        m_table->SetFmtRegPtr(m_tabSet.GetFmtRegPtr());

        CTblFmt* const table_format = CreateFormat<CTblFmt>(*m_table, FMT_ID_TABLE,
            [&](CTblFmt* const format)
            {
                format->Init();

                if( m_frequencyTable->has_multiple_labels_per_value )
                {
                    format->SetIncludePageNote(true);
                    m_table->GetPageNote()->SetFmt(m_pageNoteFormat);
                    m_table->GetPageNote()->SetText(FPH::NoCumulativeColumnsWarning);
                }

                if( m_table->m_bHasFreqStats )
                    format->SetIncludeEndNote(true);
            });

        m_table->SetFmt(table_format);
    }
}


void TableFrequencyPrinterWorker::SetupColumns()
{
    // add the frequency name and label
    CTallyFmt* const table_tally_format = CreateFormat<CTallyFmt>(m_tabSet, FMT_ID_TALLY, [](CTallyFmt* /*format*/) { });

    CTabVar* const label_table_var = new CTabVar();

    label_table_var->SetName(UTF8_TODO::GetCString(m_frequencyTable->symbol_name));
    label_table_var->SetText(( m_frequencyTable->dict_item != nullptr ) ? m_frequencyTable->dict_item->GetLabel() :
                                                                          UTF8_TODO::GetCString(m_frequencyTable->symbol_name));
    label_table_var->SetFmt(table_tally_format);

    // add the total row
    CTabValue* const first_column_total_cell = new CTabValue();
    first_column_total_cell->SetText(FPH::TotalLabel);
    first_column_total_cell->SetParentVar(label_table_var);
    label_table_var->GetArrTabVals().Add(first_column_total_cell);

    CTabVar* const row_root = m_table->GetRowRoot();
    row_root->AddChildVar(label_table_var);

    // add the frequency and cumulative columns
    CTabVar* const column_root = m_table->GetColRoot();

    auto add_columns = [&](const char* const type)
    {
        CTabVar* const column_table_var = new CTabVar();
        column_root->AddChildVar(column_table_var);

        column_table_var->SetName(UTF8_TODO::GetCString(type));
        column_table_var->SetText(UTF8_TODO::GetCString(type));

        auto add_cell = [&](const char* const text, const bool use_percent_format)
        {
            CTabValue* cell = new CTabValue();
            cell->SetText(UTF8_TODO::GetCString(text));

            if( use_percent_format )
            {
                cell->SetFmt(GetColumnHeadDecimalsFormat(FTD::PercentDecimals));
            }

            // set the number of decimals for count columns if decimals have been specified
            else if( m_frequencyTable->frequency_printer_options.GetUsingDecimals() )
            {
                cell->SetFmt(GetColumnHeadDecimalsFormat(m_frequencyTable->frequency_printer_options.GetDecimals()));
            }

            cell->SetParentVar(column_table_var);
            column_table_var->GetArrTabVals().Add(cell);
        };

        add_cell(FPH::TotalLabel, false);
        add_cell(FPH::PercentLabel, true);

        if( m_showNetPercents )
            add_cell(FPH::NetPercentLabel, true);
    };

    add_columns(FPH::FrequencyLabel);
    add_columns(FPH::CumulativeLabel);
}


CDataCellFmt* TableFrequencyPrinterWorker::GetStubOrCellFormat(const FMT_ID format_id, const bool out_of_range_row, bool line_on_bottom_row,
                                                               const bool percent_cell, const bool blank_cell)
{
    // if only the percent cell value is set (or not set), then a special format isn't needed
    // (because the columns specify the number of decimals), so this only checks the other properties
    if( !out_of_range_row && !line_on_bottom_row && !blank_cell )
        return nullptr;

    // lines can't be set for data cells
    if( format_id == FMT_ID_DATACELL )
        line_on_bottom_row = false;

    StubOrCellDataCellFormatOptions options(format_id, out_of_range_row, line_on_bottom_row, percent_cell, blank_cell);

    // see if the format already exists
    const auto& format_lookup = m_stubOrCellDataCellFormats.find(options);

    if( format_lookup != m_stubOrCellDataCellFormats.cend() )
        return format_lookup->second;

    // create the format if it doesn't exist
    CDataCellFmt* const cell_format = CreateFormat<CDataCellFmt>(m_tabSet, format_id,
        [&](CDataCellFmt* const format)
        {
            if( out_of_range_row )
            {
                format->SetTextColor(FMT_COLOR{ FTD::OutOfRangeColor, false });
                format->SetTextColorExtends();
            }

            if( line_on_bottom_row )
            {
                format->SetLineBottom(LINE::LINE_THIN);
                format->SetLinesExtend();
            }

            if( percent_cell )
            {
                format->SetNumDecimals(DecimalsIntToFormat(FTD::PercentDecimals));
            }

            else if( m_frequencyTable->frequency_printer_options.GetUsingDecimals() )
            {
                format->SetNumDecimals(DecimalsIntToFormat(m_frequencyTable->frequency_printer_options.GetDecimals()));
            }

            if( blank_cell )
                format->SetCustomText(SO::Empty_CString);
        });

    m_stubOrCellDataCellFormats.try_emplace(std::move(options), cell_format);

    return cell_format;
}


void TableFrequencyPrinterWorker::AddTotalAndRows()
{
    ASSERT(m_table->GetTabDataArray().GetCount() == 0 &&
           m_table->GetNumRows() == 1 &&
           m_table->GetNumCols() == ( m_showNetPercents ? 6 : 4 ));

    CTabData* const table_data = new CTabData();
    m_table->GetTabDataArray().Add(table_data);

    CArray<double, double&>& cells = table_data->GetCellArray();
    ASSERT(cells.GetSize() == 0);

    auto fill_row = [&](CTabValue* const table_value, const bool out_of_range_row, const bool line_on_bottom_row,
                        const double count, const FrequencyRowStatistics& frequency_row_statistics)
    {
        int column_number = 0;

        auto add_cell = [&](const double value, const bool percent_cell, const bool blank_cell)
        {
            ++column_number;
            cells.Add(static_cast<double>(value));

            CDataCellFmt* cell_format = GetStubOrCellFormat(FMT_ID_DATACELL, out_of_range_row, line_on_bottom_row, percent_cell, blank_cell);

            if( cell_format != nullptr )
            {
                CSpecialCell special_cell(1, column_number);
                special_cell.SetFmt(cell_format);
                table_value->GetSpecialCellArr().Add(special_cell);
            }
        };

        auto add_optional_cell = [&](const std::optional<double>& optional_value, const bool percent_cell)
        {
            add_cell(optional_value.value_or(NOTAPPL), percent_cell, !optional_value.has_value());
        };

        add_cell(count, false, false);
        add_cell(frequency_row_statistics.percent_against_total, true, false);

        if( m_showNetPercents )
            add_optional_cell(frequency_row_statistics.percent_against_non_blank_total, true);

        add_optional_cell(frequency_row_statistics.cumulative_count, false);
        add_optional_cell(frequency_row_statistics.cumulative_percent_against_total, true);

        if( m_showNetPercents )
            add_optional_cell(frequency_row_statistics.cumulative_percent_against_non_blank_total, true);
    };


    // update the total row
    CTabVar* const table_var = m_table->GetRowRoot()->GetChild(0);
    CTabValue* const total_cell = table_var->GetValue(0);
    fill_row(total_cell, false, false, m_frequencyTable->total_count, FPH::CreateTotalFrequencyRowStatistics(*m_frequencyTable, false));


    // add each frequency row
    for( size_t i = 0; i < m_frequencyTable->frequency_rows.size(); ++i )
    {
        const FrequencyRow& frequency_row = m_frequencyTable->frequency_rows[i];

        const bool out_of_range_row = frequency_row.mark_as_out_of_value_set;
        const bool line_on_bottom_row = ( m_table->m_bHasFreqStats && ( ( i + 1 ) == m_frequencyTable->frequency_rows.size() ) );

        std::string formatted_values = SO::CreateSingleString<false>(frequency_row.formatted_values);

        std::string display_label = frequency_row.display_label;

        // modify the default label for notappl and the formatted value for default
        if( std::holds_alternative<double>(frequency_row.values.front()) )
        {
            if( frequency_row.value_is_blank )
            {
                if( display_label == SpecialValues::ValueToString(NOTAPPL, false) )
                    display_label = "Not Applicable";
            }

            else if( std::get<double>(frequency_row.values.front()) == DEFAULT )
            {
                ASSERT(formatted_values[0] == '*');
                formatted_values.clear();
            }
        }

        // add the values if printing values
        if( m_frequencyTable->distinct && !display_label.empty() )
        {
            display_label = SO::Concatenate(formatted_values, formatted_values.empty() ? "" : " ", display_label);
        }

        // if not printing values but there is no label, then use the value as the label
        else if( display_label.empty() )
        {
            display_label = formatted_values;
        }

        // add the row
        CTabValue* const label_cell = new CTabValue();

        CDataCellFmt* const row_format = GetStubOrCellFormat(FMT_ID_STUB, out_of_range_row, line_on_bottom_row, false, false);

        if( row_format != nullptr )
            label_cell->SetFmt(row_format);

        label_cell->SetText(UTF8_TODO::GetCString(display_label));

        table_var->GetArrTabVals().Add(label_cell);

        fill_row(label_cell, out_of_range_row, line_on_bottom_row, frequency_row.count, m_frequencyTable->frequency_row_statistics[i]);
    }
}


void TableFrequencyPrinterWorker::AddStatistics()
{
    ASSERT(m_frequencyTable->table_statistics.has_value());

    m_table->m_bAlphaFreqStats = std::holds_alternative<FrequencyAlphanumericStatistics>(*m_frequencyTable->table_statistics);

    // alphanumeric statistics
    if( m_table->m_bAlphaFreqStats )
    {
        const FrequencyAlphanumericStatistics& table_statistics = std::get<FrequencyAlphanumericStatistics>(*m_frequencyTable->table_statistics);
        m_table->m_iTotalCategories = table_statistics.number_defined_categories;
    }

    // numeric statistics
    else
    {
        const FrequencyNumericStatistics& table_statistics = std::get<FrequencyNumericStatistics>(*m_frequencyTable->table_statistics);
        m_table->m_iTotalCategories = table_statistics.number_defined_categories;

        auto set_value_if_defined = [](auto& destination, const auto& source)
        {
            if( source.has_value() )
                destination = *source;
        };

        set_value_if_defined(m_table->m_dFrqMinCode, table_statistics.min_value);
        set_value_if_defined(m_table->m_dFrqMaxCode, table_statistics.max_value);
        set_value_if_defined(m_table->m_dFrqModeCount, table_statistics.mode_count);
        set_value_if_defined(m_table->m_dFrqModeCode, table_statistics.mode_value);
        set_value_if_defined(m_table->m_dFrqMedianCode, table_statistics.median);
        set_value_if_defined(m_table->m_dFrqMedianInt, table_statistics.median_interpolated);
        set_value_if_defined(m_table->m_dFrqMean, table_statistics.mean);
        set_value_if_defined(m_table->m_dFrqVariance, table_statistics.variance);
        set_value_if_defined(m_table->m_dFrqStdDev, table_statistics.standard_deviation);
    }
}


void TableFrequencyPrinterWorker::AddPercentiles()
{
    // percentiles only exist for numeric frequencies
    if( !m_frequencyTable->table_statistics.has_value() || !std::holds_alternative<FrequencyNumericStatistics>(*m_frequencyTable->table_statistics) )
        return;

    const FrequencyNumericStatistics& table_statistics = std::get<FrequencyNumericStatistics>(*m_frequencyTable->table_statistics);

    // make sure that percentiles exist (which they won't if no values were tallied)
    if( !table_statistics.percentiles.has_value() )
        return;

    m_table->m_bHasFreqNTiles = true;

    std::vector<std::string> formatted_percentile_percents = FPH::GetFormattedPercentilePercents(table_statistics);
    std::vector<std::string> formatted_value_type2s;
    std::vector<std::string> formatted_value_type6s;

    for( const FrequencyNumericStatistics::Percentile& percentile : *table_statistics.percentiles )
    {
        formatted_value_type2s.emplace_back(FPH::GetFormattedValue(percentile.value_type2, m_frequencyTable->dict_item));
        formatted_value_type6s.emplace_back(FPH::GetValueWithMinimumDecimals(percentile.value_type6, m_frequencyTable->dict_item));
    }

    const size_t max_decimals_after_value_type6 = FPH::GetNumberDecimalsUsed(formatted_value_type6s);

    for( std::string& formatted_value_type6 : formatted_value_type6s )
        FPH::EnsureValueHasMinimumDecimals(formatted_value_type6, max_decimals_after_value_type6);

    for( size_t i = 0; i < formatted_percentile_percents.size(); ++i )
    {
        m_table->m_arrFrqNTiles.Add(UTF8_TODO::GetCString(formatted_percentile_percents[i] + "%"));
        m_table->m_arrFrqNTiles.Add(UTF8_TODO::GetCString(formatted_value_type2s[i]));
        m_table->m_arrFrqNTiles.Add(UTF8_TODO::GetCString(formatted_value_type6s[i]));
    }
}



// --------------------------------------------------------------------------
// TableFrequencyPrinter
// --------------------------------------------------------------------------

TableFrequencyPrinter::TableFrequencyPrinter(std::string file_path)
    :   m_filePath(std::move(file_path)),
        m_worker(std::make_unique<TableFrequencyPrinterWorker>())
{
}


TableFrequencyPrinter::~TableFrequencyPrinter()
{
    // save the tables
    m_worker->Save(m_filePath);
}


void TableFrequencyPrinter::Print(const FrequencyTable& frequency_table)
{
    m_worker->Print(frequency_table);
}
