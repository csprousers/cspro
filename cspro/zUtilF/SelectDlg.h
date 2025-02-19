#pragma once

#include <zUtilF/zUtilF.h>
#include <zUtilO/PortableColor.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF SelectDlg : public CSHtmlDlgRunner
{
public:
    struct Row
    {
        std::vector<SharableString> column_texts;
        std::optional<PortableColor> text_color;
    };

    SelectDlg(bool single_selection, size_t number_columns);

    void SetTitle(SharableString title) { m_title = std::move(title); }

    void SetHeader(std::vector<SharableString> header)
    {
        ASSERT(header.size() == m_numberColumns);
        m_header = std::move(header);
    }

    void AddRow(std::vector<SharableString> column_texts, std::optional<PortableColor> text_color = std::nullopt)
    {
        ASSERT(column_texts.size() == m_numberColumns);
        m_rows.emplace_back(Row { std::move(column_texts), std::move(text_color) });
    }

    template<typename T>
    void AddRow(T&& single_column_text, std::optional<PortableColor> text_color = std::nullopt)
    {
        AddRow({ std::forward<T>(single_column_text) }, std::move(text_color));
    }

    const std::vector<Row>& GetRows() const { return m_rows; }

    const std::optional<std::set<size_t>>& GetSelectedRows() const { return m_selectedRows; } // zero-based row numbers

protected:
    std::string GetDialogName() override;
    SharableString GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode& json_results) override;

private:
    bool m_singleSelection;
    size_t m_numberColumns;
    SharableString m_title;
    std::vector<SharableString> m_header;
    std::vector<Row> m_rows;
    std::optional<std::set<size_t>> m_selectedRows;
};
