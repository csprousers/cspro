#include "StdAfx.h"
#include "ErrmsgDlg.h"


CREATE_JSON_KEY(buttons)
CREATE_JSON_KEY(defaultButtonIndex)


ErrmsgDlg::ErrmsgDlg()
    :   m_defaultButtonIndex(0),
        m_selectedButtonIndex(0)
{
}


const TCHAR* ErrmsgDlg::GetDialogName()
{
    return ErrmsgDialogName;
}


std::wstring ErrmsgDlg::GetJsonArgumentsText()
{
    ASSERT(!m_buttons.empty());

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::title, m_title)
                .Write(JK::message, m_message)
                .Write(JK::defaultButtonIndex, m_defaultButtonIndex);

    int button_index = 1;

    json_writer->WriteObjects(JK::buttons, m_buttons,
        [&](const std::wstring& button)
        {
            json_writer->Write(JK::caption, button)
                        .Write(JK::index, button_index++);
        });

    json_writer->EndObject();

    return json_writer->GetString();
}


void ErrmsgDlg::ProcessJsonResults(const JsonNode<wchar_t>& json_results)
{
    m_selectedButtonIndex = json_results.Get<int>(JK::index);

    if( m_selectedButtonIndex < 1 || m_selectedButtonIndex > static_cast<int>(m_buttons.size()) )
        throw CSProException(_T("Invalid index: %d"), m_selectedButtonIndex);
}
