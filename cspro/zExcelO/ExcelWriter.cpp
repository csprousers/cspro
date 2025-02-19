#include "stdafx.h"
#include "ExcelWriter.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/PortableFunctions.h>
#include <zToolsO/NumberToString.h>
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


void ExcelWriter::CreateWorkbook(const InterfaceString file_path, const bool use_constant_memory_mode/* = true*/)
{
    ASSERT(m_workbook == nullptr);

    if( PortableFunctions::FileExists(file_path) && !PortableFunctions::FileDelete(file_path) )
    {
        throw CSProException("The Excel file '%s' could not be created. Make sure that it is not open in another application.",
                             PortableFunctions::PathGetFilename(file_path.GetString<std::string>()).c_str());
    }

    lxw_workbook_options options
    {
        static_cast<uint8_t>(use_constant_memory_mode ? LXW_TRUE : LXW_FALSE),
        nullptr,
        LXW_FALSE
    };

#ifdef ANDROID
    // on Android the temp directory must be specified
    std::string temp_directory = PlatformInterface::GetInstance()->GetTempDirectory();
    options.tmpdir = temp_directory.data();
#endif

    m_workbook = workbook_new_opt(file_path.c_str_utf8(), &options);

    if( m_workbook == nullptr )
        throw CSProException("Could not create an Excel workbook.");
}


void ExcelWriter::Close()
{
    if( m_workbook == nullptr )
        return;

    const lxw_error close_result = workbook_close(m_workbook);
    ASSERT(close_result == LXW_NO_ERROR);

    m_workbook = nullptr;
}


size_t ExcelWriter::AddWorksheet(const cs::string_sz worksheet_name)
{
    ASSERT(m_workbook != nullptr && ValidateWorksheetName(worksheet_name));

    lxw_worksheet* const worksheet = worksheet_name.empty() ? workbook_add_worksheet(m_workbook, nullptr) :
                                                              workbook_add_worksheet(m_workbook, worksheet_name.c_str());

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


size_t ExcelWriter::AddAndSetCurrentWorksheet(const cs::string_sz worksheet_name)
{
    const size_t index = AddWorksheet(worksheet_name);
    SetCurrentWorksheet(index);
    return index;
}


bool ExcelWriter::ValidateWorksheetName(const cs::string_sz worksheet_name)
{
    ASSERT(m_workbook != nullptr);
    const lxw_error error_code = workbook_validate_sheet_name(m_workbook, worksheet_name.c_str());
    return ( error_code == LXW_NO_ERROR );
}


std::string ExcelWriter::CreateValidWorksheetName(std::string worksheet_name)
{
    ASSERT(m_workbook != nullptr);

    lxw_error error_code;

    while( true )
    {
        error_code = workbook_validate_sheet_name(m_workbook, worksheet_name.c_str());

        if( error_code == LXW_NO_ERROR )
            return worksheet_name;

        // if the worksheet name is blank, use the default SheetN name
        if( error_code == LXW_ERROR_PARAMETER_IS_EMPTY )
        {
            worksheet_name = FormatText("Sheet%d", m_workbook->num_sheets + 1);
        }

        // if the worksheet name is too long, truncate the name
        else if( error_code == LXW_ERROR_SHEETNAME_LENGTH_EXCEEDED )
        {
            ASSERT(SO::WideLength(worksheet_name) > LXW_SHEETNAME_MAX);
            SO::WideMakeExactLength(worksheet_name, LXW_SHEETNAME_MAX);
        }

        // if the worksheet contains invalid characters, remove them
        else if( error_code == LXW_ERROR_INVALID_SHEETNAME_CHARACTER )
        {
            for( size_t ch_pos; ( ch_pos = worksheet_name.find_first_of("[]:*?/\\") ) != std::string::npos; )
                worksheet_name.erase(ch_pos, 1);
        }

        // if the worksheet starts or ends with an apostrophe, remove them
        else if( error_code == LXW_ERROR_SHEETNAME_START_END_APOSTROPHE )
        {
            SO::MakeTrim(worksheet_name, '\'');
        }

        // break if the worksheet name is already in use
        else if( error_code == LXW_ERROR_SHEETNAME_ALREADY_USED )
        {
            break;
        }
    }

    ASSERT(error_code == LXW_ERROR_SHEETNAME_ALREADY_USED);

    // when the worksheet name is already in use, append numbers to the end until the name is unique, trying up to 99999
    constexpr int MaxNumber = 99999;
    constexpr size_t MaxNumberLength = IntToStringLength(MaxNumber) + 1; // + 1 is for the _

    if( ( SO::WideLength(worksheet_name) + MaxNumberLength ) > LXW_SHEETNAME_MAX )
        SO::WideMakeExactLength(worksheet_name, LXW_SHEETNAME_MAX - MaxNumberLength);

    for( int i = 1; i <= MaxNumber; ++i )
    {
        std::string test_worksheet_name = FormatText("%s_%d", worksheet_name.c_str(), i);
        ASSERT(SO::WideLength(test_worksheet_name) <= LXW_SHEETNAME_MAX);

        error_code = workbook_validate_sheet_name(m_workbook, test_worksheet_name.c_str());

        if( error_code == LXW_NO_ERROR )
            return test_worksheet_name;

        if( error_code != LXW_ERROR_SHEETNAME_ALREADY_USED )
            break;
    }

    // unless there were 99999 versions of the worksheet name, we should not be here;
    // in that case, return the default sheet name
    return ReturnProgrammingError(CreateValidWorksheetName(""));
}


lxw_format* ExcelWriter::GetFormat(const Format format, const char* const numeric_format/* = nullptr*/)
{
    ASSERT(m_workbook != nullptr);

    lxw_format* const excel_format = workbook_add_format(m_workbook);

    if( numeric_format != nullptr )
        format_set_num_format(excel_format, numeric_format);

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


void ExcelWriter::Write(const uint32_t row, const uint16_t column, const cs::string_sz text, lxw_format* const format/* = nullptr*/)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_write_string(m_currentWorksheet, row, column, text.c_str(), format);
}


void ExcelWriter::Write(const uint32_t row, const uint16_t column, const double value, lxw_format* const format/* = nullptr*/)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_write_number(m_currentWorksheet, row, column, value, format);
}


void ExcelWriter::WriteMerged(const uint32_t first_row, const uint16_t first_column, const uint32_t last_row, const uint16_t last_column,
                              const cs::string_sz text, lxw_format* const format/* = nullptr*/)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_merge_range(m_currentWorksheet, first_row, first_column, last_row, last_column, text.c_str(), format);
}


void ExcelWriter::WriteUrl(const uint32_t row, const uint16_t column, const cs::string_sz url, lxw_format* const format/* = nullptr*/)
{
    ASSERT(m_currentWorksheet != nullptr);
    worksheet_write_url(m_currentWorksheet, row, column, url.c_str(), format);
}


double ExcelWriter::GetWidthForText(const TextType text_type, const unsigned number_characters, lxw_format* const format/* = nullptr*/)
{
    // these numbers come from tests in Excel using the default font size 11
    constexpr double DefaultFontSize = 11;
    constexpr double Width0 = 1.029;
    constexpr double AverageWidthMultipleCharacters = 1.211;

    const double font_size_multiplier = ( format != nullptr ) ? ( format->font_size / DefaultFontSize ) : 1;

    return number_characters * font_size_multiplier * ( ( text_type == TextType::Numbers ) ? Width0 : AverageWidthMultipleCharacters );
}


double ExcelWriter::GetHeightForText(const unsigned lines, lxw_format* const format/* = nullptr*/)
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
