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

    void CreateWorkbook(InterfaceString file_path, bool use_constant_memory_mode = true);
    void Close();

    size_t AddWorksheet(cs::string_sz worksheet_name);
    void SetCurrentWorksheet(size_t index);
    size_t AddAndSetCurrentWorksheet(cs::string_sz worksheet_name);

    bool ValidateWorksheetName(cs::string_sz worksheet_name);
    std::string CreateValidWorksheetName(std::string worksheet_name);

    enum class Format
    {
        None = 0x0,
        TextWrap = 0x1,
        Bold = 0x2, Italic = 0x4, Underline = 0x8,
        Center = 0x10, Right = 0x20,
        Top = 0x100, Middle = 0x200,
        LineOnLeft = 0x1000,
        TitleFont = 0x2000
    };

    lxw_format* GetFormat(Format format, const char* numeric_format = nullptr);

    void Write(uint32_t row, uint16_t column, cs::string_sz text, lxw_format* format = nullptr);
    void Write(uint32_t row, uint16_t column, double value, lxw_format* format = nullptr);

    void WriteMerged(uint32_t first_row, uint16_t first_column, uint32_t last_row, uint16_t last_column,
                     cs::string_sz text, lxw_format* format = nullptr);

    void WriteUrl(uint32_t row, uint16_t column, cs::string_sz url, lxw_format* format = nullptr);

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



inline ExcelWriter::Format operator|(const ExcelWriter::Format format1, const ExcelWriter::Format format2)
{
    return static_cast<ExcelWriter::Format>(static_cast<int>(format1) | static_cast<int>(format2) );
}
