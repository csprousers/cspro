#include "StdAfx.h"
#include "SelectFileDlg.h"


SelectFileDlg::SelectFileDlg()
    :   m_showDirectories(true),
        m_startDirectory(std::wstring())
{
}


const TCHAR* SelectFileDlg::GetDialogName()
{
    return _T("Path-selectFile");
}


std::wstring SelectFileDlg::GetJsonArgumentsText()
{
    ASSERT(!std::holds_alternative<std::wstring>(m_startDirectory) ||
           !std::get<std::wstring>(m_startDirectory).empty());

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->WriteIfHasValue(JK::title, m_title)
                .Write(JK::showDirectories, m_showDirectories)
                .WriteIfHasValue(JK::filter, m_filter)
                .Write(JK::startDirectory, SpecialDirectoryLister::GetSpecialDirectoryPath(m_startDirectory));

    if( m_rootDirectory.has_value() )
        json_writer->Write(JK::rootDirectory, SpecialDirectoryLister::GetSpecialDirectoryPath(*m_rootDirectory));

    json_writer->EndObject();

    return json_writer->GetString();
}


void SelectFileDlg::ProcessJsonResults(const JsonNode<wchar_t>& json_results)
{
    m_selectedPath = json_results.Get<std::wstring>();
}
