#include "StdAfx.h"
#include "SelectFileDlg.h"


SelectFileDlg::SelectFileDlg()
    :   m_showDirectories(true),
        m_startDirectory(std::string())
{
}


std::string SelectFileDlg::GetDialogName()
{
    return "Path-selectFile";
}


SharableString SelectFileDlg::GetJsonArgumentsText()
{
    ASSERT(!std::holds_alternative<std::string>(m_startDirectory) ||
           !std::get<std::string>(m_startDirectory).empty());

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->WriteIfHasValue(JK::title, m_title)
                .Write(JK::showDirectories, m_showDirectories)
                .WriteIfHasValue(JK::filter, m_filter)
                .Write(JK::startDirectory, SpecialDirectoryLister::GetSpecialDirectoryPath(m_startDirectory));

    if( m_rootDirectory.has_value() )
        json_writer->Write(JK::rootDirectory, SpecialDirectoryLister::GetSpecialDirectoryPath(*m_rootDirectory));

    json_writer->EndObject();

    return json_writer->ReleaseSharableString();
}


void SelectFileDlg::ProcessJsonResults(const JsonNode& json_results)
{
    m_selectedPath = json_results.Get<std::string>();
}
