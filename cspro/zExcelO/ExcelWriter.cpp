#include "stdafx.h"
#include "ExcelWriter.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/Utf8Convert.h>
#include <xlsxwriter.h>


ExcelWriter::ExcelWriter()
    :   m_workbook(nullptr),
        m_currentWorksheet(nullptr)
{
}


ExcelWriter::~ExcelWriter()
{
    Close();
}


void ExcelWriter::CreateWorkbook(const std::wstring& filename, const bool use_constant_memory_mode/* = true*/)
{
    ASSERT(m_workbook == nullptr);

    if( PortableFunctions::FileExists(filename) && !PortableFunctions::FileDelete(filename) )
    {
        throw CSProException(_T("The Excel file '%s' could not be created. Make sure that it is not open in another application."),
                             PortableFunctions::PathGetFilename(filename));
    }

    lxw_workbook_options options
    {
        static_cast<uint8_t>(use_constant_memory_mode ? LXW_TRUE : LXW_FALSE),
        nullptr,
        LXW_FALSE
    };

#ifdef ANDROID
    // on Android the temp directory must be specified
    std::string temp_directory = UTF8Convert::WideToUTF8(PlatformInterface::GetInstance()->GetTempDirectory());
    options.tmpdir = temp_directory.data();
#endif

    m_workbook = workbook_new_opt(UTF8Convert::WideToUTF8(filename).c_str(), &options);

    if( m_workbook == nullptr )
        throw CSProException("Could not create an Excel workbook.");
}


void ExcelWriter::Close()
{
    if( m_workbook != nullptr )
    {
        const lxw_error close_result = workbook_close(m_workbook);
        ASSERT(close_result == LXW_NO_ERROR);

        m_workbook = nullptr;
    }
}


size_t ExcelWriter::AddWorksheet(const wstring_view worksheet_name_sv)
{
    ASSERT(m_workbook != nullptr);
    ASSERT(workbook_validate_sheet_name(m_workbook, UTF8Convert::WideToUTF8(worksheet_name_sv).c_str()) == LXW_NO_ERROR);

    lxw_worksheet* worksheet = worksheet_name_sv.empty() ? workbook_add_worksheet(m_workbook, nullptr) :
                                                           workbook_add_worksheet(m_workbook, UTF8Convert::WideToUTF8(worksheet_name_sv).c_str());

    if( worksheet == nullptr )
        throw CSProException("Could not create an Excel worksheet.");

    m_worksheets.emplace_back(worksheet);

    return m_worksheets.size() - 1;
}


void ExcelWriter::SetCurrentWorksheet(const size_t index)
{
    ASSERT(index < m_worksheets.size());
    m_currentWorksheet = m_worksheets[index];
}


size_t ExcelWriter::AddAndSetCurrentWorksheet(const wstring_view worksheet_name_sv)
{
    const size_t index = AddWorksheet(worksheet_name_sv);
    SetCurrentWorksheet(index);
    return index;
}


bool ExcelWriter::ValidateWorksheetName(const wstring_view worksheet_name_sv)
{
    ASSERT(m_workbook != nullptr);
    const lxw_error error_code = workbook_validate_sheet_name(m_workbook, UTF8Convert::WideToUTF8(worksheet_name_sv).c_str());
    return ( error_code == LXW_NO_ERROR );
}


lxw_format* ExcelWriter::GetFormat(Format format, const std::optional<wstring_view>& numeric_format/* = std::nullopt*/)
{
    ASSERT(m_workbook != nullptr);

    lxw_format* excel_format = workbook_add_format(m_workbook);

    if( numeric_format.has_value() )
        format_set_num_format(excel_format, UTF8Convert::WideToUTF8(*numeric_format).c_str());

    auto selected = [&](const Format check_format)
    {
        return ( ( static_cast<int>(format) & static_cast<int>(check_format) ) != 0 );
    };

    if( selected(Format::TextWrap) )
        format_set_text_wrap(excel_format);

    if( selected(Format::Bold) )
        format_set_bold(excel_format);

    if( selected(Format::Italics) )
        format_set_italic(excel_format);

    if( selected(Format::Underline) )
        format_set_underline(excel_format, LXW_UNDERLINE_SINGLE);

    if( selected(Format::Center) )
        format_set_align(excel_format, LXW_ALIGN_CENTER);

    if( selected(Format::Right) )
        format_set_align(excel_format, LXW_ALIGN_RIGHT);

    if( selected(Format::Top) )
        format_set_align(excel_format, LXW_ALIGN_VERTICAL_TOP);

    if( selected(Format::Middle) )
        format_set_align(excel_format, LXW_ALIGN_VERTICAL_CENTER);

    if( selected(Format::LineOnLeft) )
        format_set_left(excel_format, LXW_BORDER_THIN);

    if( selected(Format::TitleFont) )
    {
        format_set_bold(excel_format);
        format_set_font_size(excel_format, 16);
    }

    return excel_format;
}


void ExcelWriter::Write(const uint32_t row, const uint16_t column, const wstring_view text_sv, lxw_format* format/* = nullptr*/)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_write_string(m_currentWorksheet, row, column, UTF8Convert::WideToUTF8(text_sv).c_str(), format);
}


void ExcelWriter::Write(const uint32_t row, const uint16_t column, const double value, lxw_format* format/* = nullptr*/)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_write_number(m_currentWorksheet, row, column, value, format);
}


void ExcelWriter::WriteMerged(const uint32_t first_row, const uint16_t first_column, const uint32_t last_row, const uint16_t last_column,
                              const wstring_view text_sv, lxw_format* format/* = nullptr*/)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_merge_range(m_currentWorksheet, first_row, first_column, last_row, last_column, UTF8Convert::WideToUTF8(text_sv).c_str(), format);
}


void ExcelWriter::WriteUrl(const uint32_t row, const uint16_t column, const wstring_view url_sv, lxw_format* format/* = nullptr*/)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_write_url(m_currentWorksheet, row, column, UTF8Convert::WideToUTF8(url_sv).c_str(), format);
}


double ExcelWriter::GetWidthForText(const TextType text_type, const unsigned number_characters, lxw_format* format/* = nullptr*/)
{
    // these numbers come from tests in Excel using the default font size 11
    constexpr double DefaultFontSize = 11;
    constexpr double Width0 = 1.029;
    constexpr double AverageWidthMultipleCharacters = 1.211;

    const double font_size_multiplier = ( format != nullptr ) ? ( format->font_size / DefaultFontSize ) : 1;

    return number_characters * font_size_multiplier * ( ( text_type == TextType::Numbers ) ? Width0 : AverageWidthMultipleCharacters );
}


double ExcelWriter::GetHeightForText(const unsigned lines, lxw_format* format/* = nullptr*/)
{
    // this number matches with the row height 15 according to font size 11
    constexpr double DefaultFontSize = 11;
    constexpr double Height = 15;

    const double font_size_multiplier = ( format != nullptr ) ? ( format->font_size / DefaultFontSize ) : 1;

    return lines * font_size_multiplier * Height;
}


void ExcelWriter::SetColumnWidth(const uint16_t column, const double width)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_set_column(m_currentWorksheet, column, column, width, nullptr);
}


void ExcelWriter::SetRowHeight(const uint32_t row, const double height)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_set_row(m_currentWorksheet, row, height, nullptr);
}


void ExcelWriter::FreezeTopRow(const uint32_t row/* = 1*/)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_freeze_panes(m_currentWorksheet, row, 0);
}
