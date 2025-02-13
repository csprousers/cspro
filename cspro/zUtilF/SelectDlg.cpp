#include "StdAfx.h"
#include "SelectDlg.h"

CREATE_JSON_KEY(columns)
CREATE_JSON_KEY(header)
CREATE_JSON_KEY(rowIndices)


SelectDlg::SelectDlg(bool single_selection, size_t number_columns)
    :   m_singleSelection(single_selection),
        m_numberColumns(number_columns)
{
    ASSERT(m_numberColumns != 0);
}


const TCHAR* SelectDlg::GetDialogName()
{
    return _T("select");
}


std::wstring SelectDlg::GetJsonArgumentsText()
{
    ASSERT(!m_rows.empty());
    ASSERT(m_header.empty() || m_header.size() == m_rows.front().column_texts.size());

    auto json_writer = Json::CreateStringWriter();

    // title + multiple
    json_writer->BeginObject()
                .Write(JK::title, m_title)
                .Write(JK::multiple, !m_singleSelection);

    // header
    json_writer->BeginArray(JK::header);

    for( size_t i = 0; i < m_numberColumns; ++i )
    {
        json_writer->BeginObject()
                    .Write(JK::caption, !m_header.empty() ? m_header[i] : std::wstring())
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
                [&](const std::wstring& column_text)
                {
                    json_writer->Write(JK::text, column_text);
                });
        });

    json_writer->EndObject();

    return json_writer->GetString();
}


void SelectDlg::ProcessJsonResults(const JsonNode<wchar_t>& json_results)
{
    m_selectedRows.emplace();

    for( const auto& row_index_element : json_results.GetArray(JK::rowIndices) )
    {
        size_t row_index = row_index_element.Get<size_t>();

        if( row_index >= m_rows.size() )
            throw CSProException(_T("Invalid row index: %d"), static_cast<int>(row_index));

        m_selectedRows->insert(row_index);
    }

    if( m_singleSelection && m_selectedRows->size() != 1 )
        throw CSProException("One row must be be selected.");
}
