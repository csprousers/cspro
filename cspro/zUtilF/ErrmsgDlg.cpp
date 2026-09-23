#include "StdAfx.h"
#include "ErrmsgDlg.h"


CREATE_JSON_KEY(buttons)
CREATE_JSON_KEY(defaultButtonIndex)


ErrmsgDlg::ErrmsgDlg()
    :   m_defaultButtonIndex(0),
        m_selectedButtonIndex(0)
{
}


std::string ErrmsgDlg::GetDialogName()
{
    return DialogName;
}


SharableString ErrmsgDlg::GetJsonArgumentsText()
{
    ASSERT(!m_buttons.empty());

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::title, m_title)
                .Write(JK::message, m_message)
                .Write(JK::defaultButtonIndex, m_defaultButtonIndex);

    int button_index = 1;

    json_writer->WriteObjects(JK::buttons, m_buttons,
        [&](const SharableString& button)
        {
            json_writer->Write(JK::caption, button)
                        .Write(JK::index, button_index++);
        });

    json_writer->EndObject();

    return json_writer->ReleaseSharableString();
}


void ErrmsgDlg::ProcessJsonResults(const JsonNode& json_results)
{
    m_selectedButtonIndex = json_results.Get<int>(JK::index);

    if( m_selectedButtonIndex < 1 || m_selectedButtonIndex > static_cast<int>(m_buttons.size()) )
        throw CSProException("Invalid index: %d", m_selectedButtonIndex);
}
