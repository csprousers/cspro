#pragma once

#include <zExcelO/zExcelO.h>

struct lxw_format;
struct lxw_workbook;
struct lxw_worksheet;


class ZEXCELO_API ExcelWriter
{
public:
    ExcelWriter();
    ~ExcelWriter();

    void CreateWorkbook(const std::wstring& filename, bool use_constant_memory_mode = true);
    void Close();

    size_t AddWorksheet(wstring_view worksheet_name_sv);
    void SetCurrentWorksheet(size_t index);
    size_t AddAndSetCurrentWorksheet(wstring_view worksheet_name_sv);
    bool ValidateWorksheetName(wstring_view worksheet_name_sv);

    enum class Format
    {
        None = 0x0,
        TextWrap = 0x1,
        Bold = 0x2, Italics = 0x4, Underline = 0x8,
        Center = 0x10, Right = 0x20,
        Top = 0x100, Middle = 0x200,
        LineOnLeft = 0x1000,
        TitleFont = 0x2000
    };

    lxw_format* GetFormat(Format format, const std::optional<wstring_view>& numeric_format_sv = std::nullopt);

    void Write(uint32_t row, uint16_t column, wstring_view text_sv, lxw_format* format = nullptr);
    void Write(uint32_t row, uint16_t column, double value, lxw_format* format = nullptr);

    void WriteMerged(uint32_t first_row, uint16_t first_column, uint32_t last_row, uint16_t last_column,
                     wstring_view text_sv, lxw_format* format = nullptr);

    void WriteUrl(uint32_t row, uint16_t column, wstring_view url_sv, lxw_format* format = nullptr);

    enum class TextType { Numbers, Characters };
    static double GetWidthForText(TextType text_type, unsigned number_characters, lxw_format* format = nullptr);
    static double GetHeightForText(unsigned lines, lxw_format* format = nullptr);

    void SetColumnWidth(uint16_t column, double width);
    void SetRowHeight(uint32_t row, double height);

    void FreezeTopRow(uint32_t row = 1);

private:
    lxw_workbook* m_workbook;
    std::vector<lxw_worksheet*> m_worksheets;
    lxw_worksheet* m_currentWorksheet;
};


inline ExcelWriter::Format operator|(ExcelWriter::Format format1, ExcelWriter::Format format2)
{
    return (ExcelWriter::Format)( (int)format1 | (int)format2 );
}
