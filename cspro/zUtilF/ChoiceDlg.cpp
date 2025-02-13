#include "StdAfx.h"
#include "ChoiceDlg.h"


CREATE_JSON_KEY(choices)
CREATE_JSON_KEY(defaultIndex)


ChoiceDlg::ChoiceDlg(int starting_choice_index)
    :   m_startingChoiceIndex(starting_choice_index),
        m_selectedChoiceIndex(-1)
{
}


const std::wstring& ChoiceDlg::GetSelectedChoiceText() const
{
    size_t index = static_cast<size_t>(m_selectedChoiceIndex - m_startingChoiceIndex);
    ASSERT(index < m_choices.size());
    return m_choices[index];
}


const TCHAR* ChoiceDlg::GetDialogName()
{
    return _T("choice");
}


std::wstring ChoiceDlg::GetJsonArgumentsText()
{
    ASSERT(!m_choices.empty());

    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    // title
    json_writer->Write(JK::title, m_title);

    // default index (when defined)
    if( m_defaultChoiceIndex.has_value() )
    {
        ASSERT(( *m_defaultChoiceIndex - m_startingChoiceIndex ) < static_cast<int>(m_choices.size()));
        json_writer->Write(JK::defaultIndex, *m_defaultChoiceIndex);
    }

    // choices
    int choice_index = m_startingChoiceIndex;

    json_writer->WriteObjects(JK::choices, m_choices,
        [&](const std::wstring& choice)
        {
            json_writer->Write(JK::caption, choice)
                        .Write(JK::index, choice_index++);
        });

    json_writer->EndObject();

    return json_writer->GetString();
}


void ChoiceDlg::ProcessJsonResults(const JsonNode<wchar_t>& json_results)
{
    m_selectedChoiceIndex = json_results.Get<int>(JK::index);

    size_t index = static_cast<size_t>(m_selectedChoiceIndex - m_startingChoiceIndex);

    if( index >= m_choices.size() )
        throw CSProException(_T("Invalid index: %d"), m_selectedChoiceIndex);
}
