#include "StdAfx.h"
#include "ChartManager.h"
#include "FrequencyTableCreatorFromTableGridExporter.h"
#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zAction/AccessToken.h>


ChartManager::ChartManager()
{
}


ChartManager::~ChartManager()
{
}


const std::string& ChartManager::GetFrequencyViewUrl()
{
    ASSERT(m_fileServer == nullptr || !m_frequencyViewUrl.empty());

    if( m_fileServer == nullptr )
    {
        m_fileServer = std::make_unique<SharedHtmlLocalFileServer>();

        const std::string frequency_view_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Charting),
                                                                   "frequency-view.html");

        ASSERT(ActionInvoker::AccessToken::Charting_FrequencyView_sv == ActionInvoker::AccessToken::CreateAccessTokenForHtmlDirectoryFile(frequency_view_file_path));

        m_frequencyViewUrl = m_fileServer->CreateFileUrl(frequency_view_file_path);
    }

    return m_frequencyViewUrl;
}


bool ChartManager::TableSupportsCharting(CTblGrid* const table_grid)
{
    if( table_grid->GetSafeHwnd() == nullptr )
        return false;

    CTable* table = table_grid->GetTable();
    ASSERT(table != nullptr);

    // see if the frequency JSON has already been created
    const auto& lookup = m_frequencyJson.find(table);

    if( lookup != m_frequencyJson.cend() )
        return lookup->second.IsSet();

    // if not, try to create frequency JSON for the table
    SharableString& frequency_json = m_frequencyJson.try_emplace(table, SharableString()).first->second;

    try
    {
        frequency_json = FrequencyTableCreatorFromTableGridExporter().CreateFrequencyJson(*table_grid);
    }

    catch( const FrequencyTableCreatorFromTableGridExporter::CreationException& )
    {
    }

    catch(...)
    {
        ASSERT(false);
    }

    return frequency_json.IsSet();
}


SharableString ChartManager::GetTableFrequencyJson(CTblGrid* const table_grid) const
{
    if( table_grid != nullptr )
    {
        const auto& lookup = m_frequencyJson.find(table_grid->GetTable());

        if( lookup != m_frequencyJson.cend() )
            return lookup->second;
    }

    return ReturnProgrammingError(SO::Empty_string);
}
