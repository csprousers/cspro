#include "StdAfx.h"
#include "SelectDlg.h"


CREATE_JSON_KEY(columns)
CREATE_JSON_KEY(header)
CREATE_JSON_KEY(rowIndices)


SelectDlg::SelectDlg(const bool single_selection, const size_t number_columns)
    :   m_singleSelection(single_selection),
        m_numberColumns(number_columns)
{
    ASSERT(m_numberColumns != 0);
}


std::string SelectDlg::GetDialogName()
{
    return "select";
}


SharableString SelectDlg::GetJsonArgumentsText()
{
    ASSERT(!m_rows.empty());
    ASSERT(m_header.empty() || m_header.size() == m_rows.front().column_texts.size());

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    // title + multiple
    json_writer->BeginObject()
                .Write(JK::title, m_title)
                .Write(JK::multiple, !m_singleSelection);

    // header
    json_writer->BeginArray(JK::header);

    for( size_t i = 0; i < m_numberColumns; ++i )
    {
        json_writer->BeginObject()
                    .Write(JK::caption, !m_header.empty() ? *m_header[i] : SO::Empty_string)
                    .EndObject();
    }

    json_writer->EndArray();

    // rows
    size_t row_number = 0;

    json_writer->WriteObjects(JK::rows, m_rows,
        [&](const Row& row)
        {
            json_writer->Write(JK::index, row_number++);
        
            json_writer->Write(JK::textColor, row.text_color.value_or(PortableColor::Black));

            // columns
            json_writer->WriteObjects(JK::columns, row.column_texts,
                [&](const SharableString& column_text)
                {
                    json_writer->Write(JK::text, column_text);
                });
        });

    json_writer->EndObject();

    return json_writer->ReleaseSharableString();
}


void SelectDlg::ProcessJsonResults(const JsonNode& json_results)
{
    m_selectedRows.emplace();

    for( const JsonNode& row_index_element : json_results.GetArray(JK::rowIndices) )
    {
        const size_t row_index = row_index_element.Get<size_t>();

        if( row_index >= m_rows.size() )
            throw CSProException("Invalid row index: %d", static_cast<int>(row_index));

        m_selectedRows->insert(row_index);
    }

    if( m_singleSelection && m_selectedRows->size() != 1 )
        throw CSProException("One row must be be selected.");
}
