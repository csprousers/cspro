#include "StdAfx.h"
#include "ChoiceDlg.h"


CREATE_JSON_KEY(choices)
CREATE_JSON_KEY(defaultIndex)


ChoiceDlg::ChoiceDlg(const int starting_choice_index)
    :   m_startingChoiceIndex(starting_choice_index),
        m_selectedChoiceIndex(-1)
{
}


const SharableString& ChoiceDlg::GetSelectedChoiceText() const
{
    const size_t index = static_cast<size_t>(m_selectedChoiceIndex - m_startingChoiceIndex);
    ASSERT(index < m_choices.size());
    return m_choices[index];
}


std::string ChoiceDlg::GetDialogName()
{
    return "choice";
}


SharableString ChoiceDlg::GetJsonArgumentsText()
{
    ASSERT(!m_choices.empty());

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

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
        [&](const SharableString& choice)
        {
            json_writer->Write(JK::caption, choice)
                        .Write(JK::index, choice_index++);
        });

    json_writer->EndObject();

    return json_writer->ReleaseSharableString();
}


void ChoiceDlg::ProcessJsonResults(const JsonNode& json_results)
{
    m_selectedChoiceIndex = json_results.Get<int>(JK::index);

    const size_t index = static_cast<size_t>(m_selectedChoiceIndex - m_startingChoiceIndex);

    if( index >= m_choices.size() )
        throw CSProException("Invalid index: %d", m_selectedChoiceIndex);
}
