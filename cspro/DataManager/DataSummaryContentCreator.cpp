#include "StdAfx.h"
#include "DataSummaryContentCreator.h"
#include <zHtml/HtmlWriter.h>
#include <zDataO/SyncHistoryEntry.h>


DataSummaryContentCreator::DataSummaryContentCreator(DataSourceDoc& data_source_doc)
    :   m_dataSourceDoc(data_source_doc)
{
}


const wchar_t* DataSummaryContentCreator::GetSaveTitle() const
{
    return L"Save Data Summary";
}


std::string DataSummaryContentCreator::GetSaveSuggestedFilename() const
{
    return SO::CreateParentheticalExpression("Data Summary", m_dataSourceDoc.GetDictionary().GetName());
}


SharableString DataSummaryContentCreator::GetHtmlContent(bool /*embed_resources = false*/)
{
    DataRepository& data_repository = m_dataSourceDoc.GetDataRepository();

    HtmlStringWriter html_writer;
    html_writer.WriteDefaultHeader(data_repository.GetName(DataRepositoryNameType::Concise), Html::CSS::CaseView);

    html_writer << "<body><div class=\"dv_summary_alignment\">";

    html_writer << "<table class=\"dv_summary_table\">";

    // display the data source information and type
    html_writer << "<tr><td class=\"dv_summary_table_header\">Data Source</td><td>"
                << data_repository.GetConnectionString().GetName(DataRepositoryNameType::Full)
                << "</td></tr>";

    html_writer << "<tr><td class=\"dv_summary_table_header\">Type</td><td>"
                << std::string_view(ToString(data_repository.GetRepositoryType()))
                << "</td></tr>";

    constexpr std::string_view BlankRow_sv = "<tr><td colspan=\"2\"></td></tr>";
    html_writer.WriteRaw(BlankRow_sv);

    // display information on the number of cases
    const size_t cases = data_repository.GetNumberCases();
    const size_t deleted_cases = data_repository.GetNumberCases(CaseIterationCaseStatus::All) - cases;
    const size_t partial_cases = data_repository.GetNumberCases(CaseIterationCaseStatus::PartialsOnly);

    html_writer << "<tr><td class=\"dv_summary_table_header\">Cases</td><td>"
                << IntToString(cases)
                << "</td></tr>";

    if( deleted_cases != 0 )
    {
        html_writer << "<tr><td class=\"dv_summary_table_header\">Deleted Cases</td><td>"
                    << IntToString(deleted_cases)
                    << "</td></tr>";
    }

    if( partial_cases != 0 )
    {
        html_writer << "<tr><td class=\"dv_summary_table_header\">Partial Cases</td><td>"
                    << FormatText("%d (%0.1f%%)", static_cast<int>(partial_cases), 100.0 * partial_cases / cases)
                    << "</td></tr>";
    }

    // display information on the last time the data was synced
    ISyncableDataRepository* const syncable_repository = data_repository.GetSyncableDataRepository();

    if( syncable_repository != nullptr )
    {
        constexpr bool ShowOnlyLastSync = true;

        const std::vector<SyncHistoryEntry>& sync_history = syncable_repository->GetSyncHistory(
            DeviceId(),
            std::nullopt,
            std::nullopt,
            ShowOnlyLastSync ? 1 : std::numeric_limits<size_t>::max()
        );

        if( !sync_history.empty() )
        {
            html_writer.WriteRaw(BlankRow_sv);

            for( const SyncHistoryEntry& sync_history_entry : sync_history )
            {
                const double sync_time = static_cast<double>(sync_history_entry.GetDateTime());

                const std::string time_ago_with_time = FormatText("%s (%s)", GetTimeAgo(sync_time).c_str(),
                                                                             FormatTimestamp(sync_time).c_str());

                html_writer << "<tr><td class=\"dv_summary_table_header\">Last Sync</td><td>" << time_ago_with_time
                            << "<br>" << sync_history_entry.GetDeviceName() << "</td></tr>";
            }
        }
    }

    html_writer << "</table>";

    html_writer << "</div></body>"
                << "</html>";

    return html_writer.str();
}
